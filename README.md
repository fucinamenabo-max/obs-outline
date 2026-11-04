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
Nessun installer: è uno zip da estrarre.
1. Scarica `obs-outline-<versione>.zip` dalle **Release** e chiudi OBS.
2. Premi `Win+R`, scrivi `%APPDATA%\obs-studio\plugins` e premi Invio (se la cartella non esiste, creala).
3. Copia dentro la cartella `obs-outline` dello zip, in modo che resti `plugins\obs-outline\bin\64bit\obs-outline.dll`.
4. Riapri OBS.

Per disinstallare basta cancellare la cartella `obs-outline` da lì. Non servono permessi di amministratore.

Licenza: GPL-2.0-or-later (come il template ufficiale).

## Lingue
Interfaccia del filtro: italiano, inglese, spagnolo, francese, tedesco, cinese semplificato (segue la lingua di OBS, altrimenti inglese). Le traduzioni sono automatiche e non riviste da madrelingua. Per aggiungerne una basta un nuovo file in `data/locale/` con le stesse chiavi.
