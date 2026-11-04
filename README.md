# Outline Filter per OBS

Filtro video per OBS Studio (Windows x64) che aggiunge un contorno a qualsiasi sorgente con trasparenza: PNG, APNG, GIF e WebP animati, immagini, sorgenti media.

## Funzioni
- **Contorno** con spessore 0–128 px, colore, opacità, morbidezza.
- **Posizione**: esterno, interno o centrato sul bordo.
- **Animazione colore**: arcobaleno, arcobaleno scorrevole (tinta che gira attorno alla forma) o pulsazione, con velocità regolabile.
- **Secondo contorno** (anello esterno di colore diverso).
- **Bagliore** (glow) con dimensione, colore e intensità.
- **Ombra** con offset X/Y, sfocatura, colore e opacità.

Il calcolo usa un distance field su GPU (jump flooding): nessun buco su dettagli sottili, anche con spessori alti.

## Uso
1. Tasto destro sulla sorgente → **Filtri**.
2. In *Filtri effetti* premi **+** → **Contorno (Outline)**.

## Installazione
Esegui `obs-outline-<versione>-setup.exe`: rileva da solo la cartella di OBS (modificabile) e chiede di chiudere OBS se è aperto.
Installazione manuale: copia `obs-outline.dll` in `obs-plugins\64bit\` e la cartella `data\` in `data\obs-plugins\obs-outline\`.

Licenza: GPL-2.0-or-later (come il template ufficiale).

## Lingue
Interfaccia del filtro: italiano, inglese, spagnolo, francese, tedesco, cinese semplificato (segue la lingua di OBS, altrimenti inglese). Installer: italiano, inglese, spagnolo, francese, tedesco. Le traduzioni sono automatiche e non riviste da madrelingua. Per aggiungerne una basta un nuovo file in `data/locale/` con le stesse chiavi.
