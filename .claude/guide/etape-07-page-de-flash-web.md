# Étape 07 — Page de flash web « 1 clic »

## Objectif

Permettre à une personne **non technique** d'installer Gluco-Family sur sa carte sans
PlatformIO, sans ligne de commande, sans rien télécharger : elle ouvre une page web,
branche la carte en USB, clique sur **« Flasher ma carte »**, puis suit un petit tutoriel
illustré pour la configuration.

## Comment ça marche

La page utilise [**ESP Web Tools**](https://esphome.github.io/esp-web-tools/), un composant
web qui flashe l'ESP32 **depuis le navigateur** via l'API **Web Serial**. Le firmware envoyé
est le binaire fusionné que le build produit déjà (`Gluco-Family_<version>_merged.bin`,
ESP32-S3, offset 0) — aucune modification du firmware n'a été nécessaire.

### Contrainte importante (Web Serial)

Le flash par navigateur fonctionne **uniquement** sur :

- un **ordinateur** (Windows / Mac / Linux), jamais sur téléphone ou tablette ;
- un navigateur **basé sur Chromium** : Chrome, Edge, Brave, Opera, Vivaldi.

Il ne fonctionne **pas** sur Firefox ni Safari. La page le dit clairement et affiche un
message d'aide si le navigateur n'est pas compatible. Sur Brave, si le bouton reste inactif,
il faut baisser le bouclier (Shields) pour la page.

## Architecture des fichiers

Tout vit dans le dossier `web/` du dépôt `gluco-family` (la page et le `.bin` sont donc au
**même endroit** → aucun problème de CORS).

```
web/
  index.html
  vite.config.js            # base: './'  -> chemins relatifs (marche en sous-domaine OU sous-chemin)
  package.json
  public/
    manifest.json           # décrit la puce ESP32-S3 + le fichier firmware.bin
    favicon.svg
    photos/                 # vos captures d'écran (voir public/photos/README.md)
  src/
    App.jsx                 # toute la page (sections + tutoriel)
    components/
      InstallButton.jsx     # enveloppe <esp-web-install-button> avec les textes français
      PhotoSlot.jsx         # cadre "photo à ajouter" tant qu'aucune image n'est fournie
      icons.jsx             # icônes SVG inline
```

Le firmware `firmware.bin` n'est **pas** versionné : il est injecté au déploiement par la CI.

## Déploiement automatique

Le workflow `.github/workflows/deploy-flasher.yml` :

1. compile le firmware avec PlatformIO (`pio run -e esp32-s3-devkitc-1`) ;
2. build la page (`npm run build` dans `web/`) ;
3. copie le `..._merged.bin` en `web/dist/firmware.bin` et reporte la version dans le manifest ;
4. publie le tout sur **GitHub Pages**.

Déclencheurs : push sur `main`, tags `v*`, ou lancement manuel. La page reflète donc
toujours le dernier firmware.

## À faire une seule fois côté GitHub (manuel)

1. Pousser ces fichiers sur `main`.
2. Sur GitHub : **Settings → Pages → Build and deployment → Source = « GitHub Actions »**.
3. Au prochain push, le workflow déploie la page sur `https://slcode777.github.io/gluco-family/`.

## Domaine personnalisé (optionnel) — `flash.tondomaine.com`

Pour que la page ait l'aspect d'un site indépendant sous votre propre domaine :

1. Chez votre hébergeur DNS, créer un enregistrement **CNAME** :
   `flash` → `slcode777.github.io`.
2. Ajouter un fichier `web/public/CNAME` contenant une seule ligne : `flash.tondomaine.com`
   (il sera copié dans le build et lu par GitHub Pages).
3. Sur GitHub : **Settings → Pages → Custom domain** → saisir `flash.tondomaine.com`,
   puis cocher **Enforce HTTPS** une fois le certificat émis (quelques minutes).

Comme `vite.config.js` utilise `base: './'`, **aucune reconstruction** n'est nécessaire pour
passer du sous-chemin GitHub au domaine racine.

## Ajouter les photos du tutoriel

Déposer les images dans `web/public/photos/` puis renseigner le `src` du `<PhotoSlot>`
correspondant dans `src/App.jsx`. Tant qu'aucune photo n'est fournie, un cadre
« Photo à ajouter » s'affiche — la page reste présentable. Voir `web/public/photos/README.md`.

## Limites assumées

- Cette page sert à la **première installation** : le flash remet la carte à neuf (la config
  LittleFS existante est effacée). Pour une **mise à jour**, on garde la page `/OTA` du firmware.
- Pas de provisioning WiFi par navigateur (Improv) : la configuration WiFi/Dexcom se fait sur
  l'écran tactile de la carte, puis sur sa page **Réglages** via son adresse réseau.

## Développement local

```bash
cd web
npm install
npm run dev      # http://localhost:5173 (le flash marche sur localhost)
```
