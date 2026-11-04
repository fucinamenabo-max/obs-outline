#include <obs-module.h>
#include <graphics/vec2.h>
#include <graphics/vec4.h>
#include <util/platform.h>
#include <math.h>
#include <plugin-support.h>

#define S_THICKNESS "thickness"
#define S_COLOR "color"
#define S_OPACITY "opacity"
#define S_SOFTNESS "softness"
#define S_POSITION "position"
#define S_ANIM "anim"
#define S_ANIM_SPEED "anim_speed"
#define S_OUTLINE2 "outline2"
#define S_THICKNESS2 "thickness2"
#define S_COLOR2 "color2"
#define S_GLOW "glow"
#define S_GLOW_SIZE "glow_size"
#define S_GLOW_COLOR "glow_color"
#define S_GLOW_OPACITY "glow_opacity"
#define S_SHADOW "shadow"
#define S_SHADOW_X "shadow_x"
#define S_SHADOW_Y "shadow_y"
#define S_SHADOW_BLUR "shadow_blur"
#define S_SHADOW_COLOR "shadow_color"
#define S_SHADOW_OPACITY "shadow_opacity"

#define MAX_THICKNESS 128
#define MAX_GLOW 128
#define MAX_SHADOW 128
#define MAX_TOTAL_DIM 8192

enum position { POS_OUTSIDE = 0, POS_INSIDE = 1, POS_CENTER = 2 };
enum anim { ANIM_NONE = 0, ANIM_RAINBOW = 1, ANIM_PULSE = 2, ANIM_FLOW = 3 };

struct outline_filter {
	obs_source_t *source;
	gs_effect_t *effect;

	gs_texrender_t *tr_src;
	gs_texrender_t *tr_out[2];
	gs_texrender_t *tr_in[2];

	/* impostazioni */
	int thickness;
	uint32_t color;
	float opacity;
	float softness;
	int position;
	int anim;
	float anim_speed;
	bool outline2;
	int thickness2;
	uint32_t color2;
	bool glow;
	int glow_size;
	uint32_t glow_color;
	float glow_opacity;
	bool shadow;
	int shadow_x;
	int shadow_y;
	int shadow_blur;
	uint32_t shadow_color;
	float shadow_opacity;

	/* derivati da update() */
	int out_t;
	int in_t;
	int t2;
	int glow_r;
	int pad;
	int range_out;
	int range_in;
	bool active;

	float time;
};

static const char *outline_name(void *unused)
{
	UNUSED_PARAMETER(unused);
	return obs_module_text("OutlineFilter");
}

static void outline_load_effect(struct outline_filter *f)
{
	char *path = obs_module_file("effects/outline.effect");
	char *errors = NULL;

	obs_enter_graphics();
	gs_effect_destroy(f->effect);
	f->effect = NULL;
	if (path)
		f->effect = gs_effect_create_from_file(path, &errors);
	obs_leave_graphics();

	if (!f->effect)
		obs_log(LOG_ERROR, "impossibile caricare outline.effect: %s", errors ? errors : "(nessun dettaglio)");
	else {
		static const char *names[] = {"image",      "field",       "field_in",   "base_size",   "total_size",
					      "pad",        "step_px",     "seed_inside", "out_t",       "in_t",
					      "feather",    "color1",      "t2",         "color2",      "glow_r",
					      "glow_color", "shadow_off",  "shadow_blur", "shadow_color", "flow", "flow_phase"};
		obs_enter_graphics();
		for (size_t i = 0; i < sizeof(names) / sizeof(names[0]); i++) {
			if (!gs_effect_get_param_by_name(f->effect, names[i]))
				obs_log(LOG_ERROR, "parametro shader mancante: %s", names[i]);
		}
		static const char *techs[] = {"Seed", "Step", "Composite", "Draw"};
		for (size_t i = 0; i < 4; i++) {
			if (!gs_effect_get_technique(f->effect, techs[i]))
				obs_log(LOG_ERROR, "tecnica shader mancante: %s", techs[i]);
		}
		obs_leave_graphics();
	}

	bfree(errors);
	bfree(path);
}

static int max_int(int a, int b)
{
	return a > b ? a : b;
}

static void outline_update(void *data, obs_data_t *settings)
{
	struct outline_filter *f = data;

	f->thickness = (int)obs_data_get_int(settings, S_THICKNESS);
	f->color = (uint32_t)obs_data_get_int(settings, S_COLOR);
	f->opacity = (float)obs_data_get_double(settings, S_OPACITY) / 100.0f;
	f->softness = (float)obs_data_get_double(settings, S_SOFTNESS) / 100.0f;
	f->position = (int)obs_data_get_int(settings, S_POSITION);
	f->anim = (int)obs_data_get_int(settings, S_ANIM);
	f->anim_speed = (float)obs_data_get_double(settings, S_ANIM_SPEED);

	f->outline2 = obs_data_get_bool(settings, S_OUTLINE2);
	f->thickness2 = (int)obs_data_get_int(settings, S_THICKNESS2);
	f->color2 = (uint32_t)obs_data_get_int(settings, S_COLOR2);

	f->glow = obs_data_get_bool(settings, S_GLOW);
	f->glow_size = (int)obs_data_get_int(settings, S_GLOW_SIZE);
	f->glow_color = (uint32_t)obs_data_get_int(settings, S_GLOW_COLOR);
	f->glow_opacity = (float)obs_data_get_double(settings, S_GLOW_OPACITY) / 100.0f;

	f->shadow = obs_data_get_bool(settings, S_SHADOW);
	f->shadow_x = (int)obs_data_get_int(settings, S_SHADOW_X);
	f->shadow_y = (int)obs_data_get_int(settings, S_SHADOW_Y);
	f->shadow_blur = (int)obs_data_get_int(settings, S_SHADOW_BLUR);
	f->shadow_color = (uint32_t)obs_data_get_int(settings, S_SHADOW_COLOR);
	f->shadow_opacity = (float)obs_data_get_double(settings, S_SHADOW_OPACITY) / 100.0f;

	/* spessori esterno/interno in base alla posizione */
	switch (f->position) {
	case POS_INSIDE:
		f->out_t = 0;
		f->in_t = f->thickness;
		break;
	case POS_CENTER:
		f->out_t = (f->thickness + 1) / 2;
		f->in_t = f->thickness / 2;
		break;
	default:
		f->out_t = f->thickness;
		f->in_t = 0;
		break;
	}

	f->t2 = f->outline2 ? f->thickness2 : 0;
	f->glow_r = f->glow ? f->glow_size : 0;

	int reach = f->out_t + f->t2;
	int extent = reach;
	int range = reach + 1;

	if (f->glow_r > 0) {
		extent = max_int(extent, reach + f->glow_r);
		range = max_int(range, reach + f->glow_r);
	}
	if (f->shadow) {
		int off = max_int(abs(f->shadow_x), abs(f->shadow_y));
		extent = max_int(extent, reach + f->shadow_blur + off);
		range = max_int(range, reach + f->shadow_blur + 1);
	}

	f->pad = extent + 2;
	f->range_out = range;
	f->range_in = f->in_t + 1;
	f->active = (reach > 0 || f->in_t > 0 || f->glow_r > 0 || f->shadow);
}

static void outline_tick(void *data, float seconds)
{
	struct outline_filter *f = data;
	f->time += seconds;
}

static void *outline_create(obs_data_t *settings, obs_source_t *source)
{
	struct outline_filter *f = bzalloc(sizeof(*f));
	f->source = source;

	outline_load_effect(f);
	if (!f->effect) {
		bfree(f);
		return NULL;
	}

	obs_enter_graphics();
	f->tr_src = gs_texrender_create(GS_RGBA, GS_ZS_NONE);
	for (int i = 0; i < 2; i++) {
		f->tr_out[i] = gs_texrender_create(GS_RGBA32F, GS_ZS_NONE);
		f->tr_in[i] = gs_texrender_create(GS_RGBA32F, GS_ZS_NONE);
	}
	obs_leave_graphics();

	outline_update(f, settings);
	return f;
}

static void outline_destroy(void *data)
{
	struct outline_filter *f = data;

	obs_enter_graphics();
	gs_effect_destroy(f->effect);
	gs_texrender_destroy(f->tr_src);
	for (int i = 0; i < 2; i++) {
		gs_texrender_destroy(f->tr_out[i]);
		gs_texrender_destroy(f->tr_in[i]);
	}
	obs_leave_graphics();

	bfree(f);
}

static void add_color_opacity(obs_properties_t *props, const char *color_id, const char *color_text,
			      const char *opacity_id)
{
	obs_properties_add_color(props, color_id, color_text);
	obs_property_t *p = obs_properties_add_float_slider(props, opacity_id, obs_module_text("Opacity"), 0.0,
							    100.0, 1.0);
	obs_property_float_set_suffix(p, "%");
}

static obs_properties_t *outline_properties(void *data)
{
	UNUSED_PARAMETER(data);
	obs_properties_t *props = obs_properties_create();
	obs_property_t *p;

	/* contorno principale */
	p = obs_properties_add_int_slider(props, S_THICKNESS, obs_module_text("Thickness"), 0, MAX_THICKNESS, 1);
	obs_property_int_set_suffix(p, " px");

	p = obs_properties_add_list(props, S_POSITION, obs_module_text("Position"), OBS_COMBO_TYPE_LIST,
				    OBS_COMBO_FORMAT_INT);
	obs_property_list_add_int(p, obs_module_text("PositionOutside"), POS_OUTSIDE);
	obs_property_list_add_int(p, obs_module_text("PositionInside"), POS_INSIDE);
	obs_property_list_add_int(p, obs_module_text("PositionCenter"), POS_CENTER);

	add_color_opacity(props, S_COLOR, obs_module_text("Color"), S_OPACITY);

	p = obs_properties_add_float_slider(props, S_SOFTNESS, obs_module_text("Softness"), 0.0, 100.0, 1.0);
	obs_property_float_set_suffix(p, "%");

	p = obs_properties_add_list(props, S_ANIM, obs_module_text("Animation"), OBS_COMBO_TYPE_LIST,
				    OBS_COMBO_FORMAT_INT);
	obs_property_list_add_int(p, obs_module_text("AnimNone"), ANIM_NONE);
	obs_property_list_add_int(p, obs_module_text("AnimRainbow"), ANIM_RAINBOW);
	obs_property_list_add_int(p, obs_module_text("AnimPulse"), ANIM_PULSE);
	obs_property_list_add_int(p, obs_module_text("AnimFlow"), ANIM_FLOW);
	obs_properties_add_float_slider(props, S_ANIM_SPEED, obs_module_text("AnimSpeed"), 0.1, 5.0, 0.1);

	/* secondo contorno */
	obs_properties_t *g2 = obs_properties_create();
	p = obs_properties_add_int_slider(g2, S_THICKNESS2, obs_module_text("Thickness"), 1, MAX_THICKNESS, 1);
	obs_property_int_set_suffix(p, " px");
	obs_properties_add_color(g2, S_COLOR2, obs_module_text("Color"));
	obs_properties_add_group(props, S_OUTLINE2, obs_module_text("SecondOutline"), OBS_GROUP_CHECKABLE, g2);

	/* bagliore */
	obs_properties_t *gg = obs_properties_create();
	p = obs_properties_add_int_slider(gg, S_GLOW_SIZE, obs_module_text("Size"), 1, MAX_GLOW, 1);
	obs_property_int_set_suffix(p, " px");
	add_color_opacity(gg, S_GLOW_COLOR, obs_module_text("Color"), S_GLOW_OPACITY);
	obs_properties_add_group(props, S_GLOW, obs_module_text("Glow"), OBS_GROUP_CHECKABLE, gg);

	/* ombra */
	obs_properties_t *gs = obs_properties_create();
	p = obs_properties_add_int_slider(gs, S_SHADOW_X, obs_module_text("OffsetX"), -MAX_SHADOW, MAX_SHADOW, 1);
	obs_property_int_set_suffix(p, " px");
	p = obs_properties_add_int_slider(gs, S_SHADOW_Y, obs_module_text("OffsetY"), -MAX_SHADOW, MAX_SHADOW, 1);
	obs_property_int_set_suffix(p, " px");
	p = obs_properties_add_int_slider(gs, S_SHADOW_BLUR, obs_module_text("Blur"), 0, MAX_SHADOW, 1);
	obs_property_int_set_suffix(p, " px");
	add_color_opacity(gs, S_SHADOW_COLOR, obs_module_text("Color"), S_SHADOW_OPACITY);
	obs_properties_add_group(props, S_SHADOW, obs_module_text("Shadow"), OBS_GROUP_CHECKABLE, gs);

	return props;
}

static void outline_defaults(obs_data_t *settings)
{
	obs_data_set_default_int(settings, S_THICKNESS, 8);
	obs_data_set_default_int(settings, S_POSITION, POS_OUTSIDE);
	obs_data_set_default_int(settings, S_COLOR, 0xFFFFFFFF);
	obs_data_set_default_double(settings, S_OPACITY, 100.0);
	obs_data_set_default_double(settings, S_SOFTNESS, 20.0);
	obs_data_set_default_int(settings, S_ANIM, ANIM_NONE);
	obs_data_set_default_double(settings, S_ANIM_SPEED, 1.0);

	obs_data_set_default_bool(settings, S_OUTLINE2, false);
	obs_data_set_default_int(settings, S_THICKNESS2, 6);
	obs_data_set_default_int(settings, S_COLOR2, 0xFF000000);

	obs_data_set_default_bool(settings, S_GLOW, false);
	obs_data_set_default_int(settings, S_GLOW_SIZE, 24);
	obs_data_set_default_int(settings, S_GLOW_COLOR, 0xFFFFFFFF);
	obs_data_set_default_double(settings, S_GLOW_OPACITY, 80.0);

	obs_data_set_default_bool(settings, S_SHADOW, false);
	obs_data_set_default_int(settings, S_SHADOW_X, 6);
	obs_data_set_default_int(settings, S_SHADOW_Y, 6);
	obs_data_set_default_int(settings, S_SHADOW_BLUR, 8);
	obs_data_set_default_int(settings, S_SHADOW_COLOR, 0xFF000000);
	obs_data_set_default_double(settings, S_SHADOW_OPACITY, 60.0);
}

static uint32_t outline_width(void *data)
{
	struct outline_filter *f = data;
	obs_source_t *target = obs_filter_get_target(f->source);
	uint32_t w = target ? obs_source_get_base_width(target) : 0;
	if (!w)
		return 0;
	return f->active ? w + 2 * (uint32_t)f->pad : w;
}

static uint32_t outline_height(void *data)
{
	struct outline_filter *f = data;
	obs_source_t *target = obs_filter_get_target(f->source);
	uint32_t h = target ? obs_source_get_base_height(target) : 0;
	if (!h)
		return 0;
	return f->active ? h + 2 * (uint32_t)f->pad : h;
}

static gs_eparam_t *P(gs_effect_t *e, const char *name)
{
	gs_eparam_t *p = gs_effect_get_param_by_name(e, name);
	if (!p)
		obs_log(LOG_ERROR, "parametro shader NULL in render: %s", name);
	return p;
}

static void set_vec2(gs_effect_t *e, const char *name, float x, float y)
{
	struct vec2 v;
	vec2_set(&v, x, y);
	gs_effect_set_vec2(P(e, name), &v);
}

static void set_float(gs_effect_t *e, const char *name, float v)
{
	gs_effect_set_float(P(e, name), v);
}

static void set_tex(gs_effect_t *e, const char *name, gs_texture_t *tex)
{
	gs_effect_set_texture(P(e, name), tex);
}

static void set_color(gs_effect_t *e, const char *name, uint32_t abgr, float alpha_mul)
{
	struct vec4 c;
	vec4_from_rgba(&c, abgr);
	c.w = alpha_mul;
	gs_effect_set_vec4(P(e, name), &c);
}

/* Esegue una passata a schermo intero della tecnica `tech` dentro `tr`. */
static bool render_pass(gs_effect_t *effect, const char *tech, gs_texrender_t *tr, uint32_t w, uint32_t h)
{
	gs_texrender_reset(tr);
	if (!gs_texrender_begin(tr, w, h))
		return false;

	struct vec4 clear;
	vec4_zero(&clear);
	gs_clear(GS_CLEAR_COLOR, &clear, 0.0f, 0);
	gs_ortho(0.0f, (float)w, 0.0f, (float)h, -100.0f, 100.0f);

	gs_blend_state_push();
	gs_enable_blending(false);
	while (gs_effect_loop(effect, tech))
		gs_draw_sprite(NULL, 0, w, h);
	gs_blend_state_pop();

	gs_texrender_end(tr);
	return true;
}

/* Jump flooding: ritorna la texture con il seme piu' vicino per ogni pixel. */
static gs_texture_t *run_jfa(struct outline_filter *f, gs_texture_t *src_tex, uint32_t bw, uint32_t bh, uint32_t tw,
			     uint32_t th, bool inside, int range, gs_texrender_t *tr[2])
{
	gs_effect_t *e = f->effect;

	set_tex(e, "image", src_tex);
	set_vec2(e, "base_size", (float)bw, (float)bh);
	set_vec2(e, "total_size", (float)tw, (float)th);
	set_vec2(e, "pad", (float)f->pad, (float)f->pad);
	set_float(e, "seed_inside", inside ? 1.0f : 0.0f);

	if (!render_pass(e, "Seed", tr[0], tw, th))
		return NULL;

	int cur = 0;
	int n = 1;
	while (n < range + 1)
		n <<= 1;

	for (int step = n / 2; step >= 1; step /= 2) {
		set_tex(e, "field", gs_texrender_get_texture(tr[cur]));
		set_float(e, "step_px", (float)step);
		set_vec2(e, "total_size", (float)tw, (float)th);
		if (!render_pass(e, "Step", tr[1 - cur], tw, th))
			return NULL;
		cur = 1 - cur;
	}

	return gs_texrender_get_texture(tr[cur]);
}

static void hsv_to_rgb(float h, float *r, float *g, float *b)
{
	float x = h * 6.0f;
	int i = (int)floorf(x);
	float fr = x - (float)i;
	float q = 1.0f - fr;
	switch (((i % 6) + 6) % 6) {
	case 0: *r = 1.0f; *g = fr; *b = 0.0f; break;
	case 1: *r = q; *g = 1.0f; *b = 0.0f; break;
	case 2: *r = 0.0f; *g = 1.0f; *b = fr; break;
	case 3: *r = 0.0f; *g = q; *b = 1.0f; break;
	case 4: *r = fr; *g = 0.0f; *b = 1.0f; break;
	default: *r = 1.0f; *g = 0.0f; *b = q; break;
	}
}


static void outline_render(void *data, gs_effect_t *unused)
{
	UNUSED_PARAMETER(unused);
	struct outline_filter *f = data;

	obs_source_t *target = obs_filter_get_target(f->source);
	uint32_t bw = target ? obs_source_get_base_width(target) : 0;
	uint32_t bh = target ? obs_source_get_base_height(target) : 0;

	if (!f->effect || !bw || !bh || !f->active) {
		obs_source_skip_video_filter(f->source);
		return;
	}

	uint32_t tw = bw + 2 * (uint32_t)f->pad;
	uint32_t th = bh + 2 * (uint32_t)f->pad;
	if (tw > MAX_TOTAL_DIM || th > MAX_TOTAL_DIM) {
		obs_source_skip_video_filter(f->source);
		return;
	}

	/* 1. cattura la sorgente (con i filtri a monte) tramite il meccanismo
	 * ufficiale di OBS, poi la copia in un buffer di dimensione nota. Non si
	 * puo' chiamare obs_source_video_render() sul target: le sorgenti
	 * (es. GIF) si aspettano un effetto attivo e fallirebbero. */
	if (!obs_source_process_filter_begin(f->source, GS_RGBA, OBS_NO_DIRECT_RENDERING))
		return;

	gs_texrender_reset(f->tr_src);
	if (!gs_texrender_begin(f->tr_src, bw, bh)) {
		/* chiude comunque la cattura per non lasciare lo stato a meta' */
		obs_source_process_filter_end(f->source, f->effect, bw, bh);
		return;
	}
	struct vec4 clear;
	vec4_zero(&clear);
	gs_clear(GS_CLEAR_COLOR, &clear, 0.0f, 0);
	gs_ortho(0.0f, (float)bw, 0.0f, (float)bh, -100.0f, 100.0f);
	gs_blend_state_push();
	gs_blend_function(GS_BLEND_ONE, GS_BLEND_ZERO);
	obs_source_process_filter_end(f->source, f->effect, bw, bh);
	gs_blend_state_pop();
	gs_texrender_end(f->tr_src);

	gs_texture_t *src_tex = gs_texrender_get_texture(f->tr_src);
	if (!src_tex) {
		obs_source_skip_video_filter(f->source);
		return;
	}

	/* 2. distance field esterno (e interno se serve) */
	gs_texture_t *field_out = run_jfa(f, src_tex, bw, bh, tw, th, false, f->range_out, f->tr_out);
	gs_texture_t *field_in = NULL;
	if (f->in_t > 0)
		field_in = run_jfa(f, src_tex, bw, bh, tw, th, true, f->range_in, f->tr_in);

	if (!field_out || (f->in_t > 0 && !field_in)) {
		obs_source_skip_video_filter(f->source);
		return;
	}

	/* 3. colore principale, con animazione */
	struct vec4 c1;
	vec4_from_rgba(&c1, f->color);
	float a1 = f->opacity;
	if (f->anim == ANIM_RAINBOW) {
		float h = fmodf(f->time * f->anim_speed * 0.2f, 1.0f);
		hsv_to_rgb(h, &c1.x, &c1.y, &c1.z);
	} else if (f->anim == ANIM_PULSE) {
		a1 *= 0.5f + 0.5f * sinf(f->time * f->anim_speed * 3.14159265f);
	}


	float flow = (f->anim == ANIM_FLOW) ? 1.0f : 0.0f;
	float flow_phase = fmodf(f->time * f->anim_speed * 0.2f, 1.0f);
	/* 4. composizione finale */
	gs_effect_t *e = f->effect;
	set_tex(e, "image", src_tex);
	set_tex(e, "field", field_out);
	set_tex(e, "field_in", field_in ? field_in : field_out);
	set_vec2(e, "base_size", (float)bw, (float)bh);
	set_vec2(e, "total_size", (float)tw, (float)th);
	set_vec2(e, "pad", (float)f->pad, (float)f->pad);

	set_float(e, "out_t", (float)f->out_t);
	set_float(e, "in_t", (float)f->in_t);
	{
		float feather = f->softness * (float)(f->out_t > f->in_t ? f->out_t : f->in_t);
		set_float(e, "feather", feather < 1.0f ? 1.0f : feather);
	}
	{
		struct vec4 v;
		vec4_set(&v, c1.x, c1.y, c1.z, a1);
		gs_effect_set_vec4(P(e, "color1"), &v);
	}

	set_float(e, "flow", flow);
	set_float(e, "flow_phase", flow_phase);

	set_float(e, "t2", (float)f->t2);
	set_color(e, "color2", f->color2, f->t2 > 0 ? 1.0f : 0.0f);

	set_float(e, "glow_r", (float)f->glow_r);
	set_color(e, "glow_color", f->glow_color, f->glow_r > 0 ? f->glow_opacity : 0.0f);

	set_vec2(e, "shadow_off", (float)f->shadow_x, (float)f->shadow_y);
	set_float(e, "shadow_blur", (float)f->shadow_blur);
	set_color(e, "shadow_color", f->shadow_color, f->shadow ? f->shadow_opacity : 0.0f);

	gs_blend_state_push();
	gs_blend_function(GS_BLEND_ONE, GS_BLEND_INVSRCALPHA);
	while (gs_effect_loop(e, "Composite"))
		gs_draw_sprite(NULL, 0, tw, th);
	gs_blend_state_pop();
}

struct obs_source_info outline_filter_info = {
	.id = "fm_outline_filter",
	.type = OBS_SOURCE_TYPE_FILTER,
	.output_flags = OBS_SOURCE_VIDEO,
	.get_name = outline_name,
	.create = outline_create,
	.destroy = outline_destroy,
	.update = outline_update,
	.get_defaults = outline_defaults,
	.get_properties = outline_properties,
	.video_tick = outline_tick,
	.get_width = outline_width,
	.get_height = outline_height,
	.video_render = outline_render,
};
