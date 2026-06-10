# Gluco-Family — page de flash web

Page web « friendly » qui permet à une personne non technique d'installer le firmware
Gluco-Family sur sa carte ESP32-S3 **directement depuis le navigateur** (un seul clic),
puis la guide pour la configuration.

Construite avec **Vite + React + Tailwind**, déployée sur **GitHub Pages**.
Le flash s'appuie sur [ESP Web Tools](https://esphome.github.io/esp-web-tools/)
(API Web Serial — Chrome / Edge / Brave sur ordinateur uniquement).

## Développement local

```bash
cd web
npm install
npm run dev      # http://localhost:5173
```

> ⚠️ En local, le flash ne fonctionne que sur `http://localhost` (autorisé par les
> navigateurs) ou en https. Le fichier `public/firmware.bin` n'est pas versionné :
> en local le bouton « Flasher » renverra une erreur 404 tant qu'aucun firmware n'est
> présent. Pour tester un vrai flash en local, copiez-y un binaire fusionné :
>
> ```bash
> cp ../.pio/build/esp32-s3-devkitc-1/Gluco-Family_3.2_merged.bin public/firmware.bin
> ```

## Déploiement

Automatique via `.github/workflows/deploy-flasher.yml` (à la racine du repo) :
compile le firmware, l'injecte dans `firmware.bin`, build la page, publie sur GitHub Pages.

## Ajouter les photos du tutoriel

Voir `public/photos/README.md`.

## Domaine personnalisé (optionnel)

Pour servir la page sur `flash.tondomaine.com` au lieu de `slcode777.github.io/gluco-family/` :
voir `.claude/guide/etape-07-page-de-flash-web.md`.
