# PRD — Gluco-Family

**Statut :** Brouillon initial
**Date :** 2026-05-14
**Auteur :** Stella
**Projet :** Fork de [Gluco-Monitor](https://f1atb.fr/gluco-monitor-diy/) (F1ATB) adapté pour un usage familial multi-personnes.

---

## 1. Contexte & motivation

Gluco-Monitor est un projet open-source de F1ATB qui affiche en continu la glycémie d'**une personne** sur un petit écran ESP32 (~25 €). Il consomme les données d'un capteur CGM (Continuous Glucose Monitor) via les API publiques de **LibreLinkUp** (Abbott FreeStyle Libre) ou **Dexcom Share**.

Le foyer compte **3 personnes diabétiques de type 1** sous capteur Dexcom. Aujourd'hui, surveiller leurs glycémies nécessite de jongler entre 3 téléphones ou applications. Un **affichage unique, toujours visible dans la maison**, simplifierait drastiquement le quotidien.

Gluco-Family est l'adaptation de Gluco-Monitor pour ce cas d'usage : **3 glycémies sur un même écran**, en temps réel.

---

## 2. Personas & cas d'usage

### Persona principal
**Stella**, mère de famille, gère les glycémies de :
- 2 enfants (capteurs Dexcom)
- Son mari (capteur Dexcom)

### Cas d'usage
- **Coup d'œil à distance** : depuis la cuisine ou le salon, voir les 3 valeurs sans ouvrir un téléphone.
- **Identification rapide d'une alerte** : repérer immédiatement qui est en hypo/hyper grâce à une couleur ou une flèche.
- **Suivi de tendance court terme** : la flèche permet d'anticiper "ça monte / ça descend".
- **Vérification de fraîcheur** : l'âge de la dernière mesure indique si les données sont fiables ou bloquées (capteur déconnecté, téléphone éteint…).

### Non-objectifs
- Pas un dispositif médical. Les décisions thérapeutiques restent prises avec les apps officielles.
- Pas une console d'analyse statistique (HbA1c, time-in-range, etc.).
- Pas une alarme sonore d'urgence (les apps officielles le font déjà mieux).

---

## 3. Périmètre

### Dans le périmètre — MVP

| # | Exigence |
|---|----------|
| F1 | Affichage simultané de 3 personnes sur un écran portrait 320×480 |
| F2 | Pour chaque personne : prénom/label, valeur de glycémie actuelle, flèche de tendance, âge depuis dernière mesure |
| F3 | Récupération des données via l'API Dexcom Share (3 jeux d'identifiants) |
| F4 | Configuration WiFi à la première utilisation |
| F5 | Configuration par personne (prénom + identifiants Dexcom) via les pages tactiles |
| F6 | Indicateur visuel si une mesure devient trop vieille (>20 min) |
| F7 | Persistance des paramètres (WiFi, 3 comptes Dexcom) en mémoire LittleFS |
| F8 | Mise à jour OTA conservée (héritée de l'amont) |
| F9 | Choix unité mg/dL ou mmol/L (global, identique aux 3 personnes) |
| F10 | Langues conservées (FR, EN, DE, ES, IT) |

### Hors périmètre MVP — différé v2+

| # | Évolution future |
|---|------------------|
| V1 | Support **mixte** Libre+Dexcom (certaines personnes sur Libre, d'autres Dexcom) |
| V2 | Plus de 3 personnes (4, 5…) |
| V3 | Courbe d'historique 24h |
| V4 | Jauge graphique en arc (présente dans l'original) |
| V5 | Alarmes sonores |
| V6 | Page web embarquée riche pour visualiser l'historique |
| V7 | Seuils cible (`targetLow`, `targetHigh`) personnalisables par personne |

### Explicitement retiré par rapport à l'amont
- La jauge graphique en arc de cercle (`Trace_Gauge`)
- La courbe d'historique 24h (`glucoseValues[300]`)
- Les pages associées à ces éléments

---

## 4. Hardware

### Plateforme cible
Module **ESP32-S3 DevKitC-1 + écran tactile AXS15231B 2.8" 320×480**, identique à celui supporté par l'amont. Disponible ~25 € (AliExpress, Waveshare, revendeurs EU).

### Justification
- Portage tactile vers d'autres contrôleurs (FT6236, CST816S, XPT2046) = ~10 h de travail spécialisé, sans valeur fonctionnelle ajoutée.
- Stratégie retenue : acheter le même hardware, concentrer l'effort sur le logiciel.

### Caractéristiques exploitées
- 16 MB flash + PSRAM (utilisée pour les canvas de rendu)
- WiFi intégré
- USB pour flashage + alimentation
- Écran tactile capacitif

### Alimentation
Adaptateur USB 5V standard. Pas de batterie embarquée.

### Environnement de build
- **Plateforme PlatformIO** : `pioarduino/platform-espressif32` (release 53.03.13 ou ultérieure). La plateforme officielle `espressif32` est abandonnée (bloquée sur Arduino-ESP32 core 2.x) et **ne compile pas** ce projet, qui utilise des macros du core 3.x (ex. `EXT_RAM_BSS_ATTR` dans `Config.h`).
- **Lib graphique** : `moononournation/GFX Library for Arduino` — dernière version compatible pioarduino, pas de pin nécessaire.
- **Permissions Linux** : utilisateur doit appartenir au groupe `uucp` (Arch) ou `dialout` (Debian/Ubuntu) pour accéder à `/dev/ttyACM0`.
- **Validation hardware réussie** : 2026-05-24 (premier flash + lecture Dexcom OK sur compte test).

---

## 5. Exigences fonctionnelles détaillées

### 5.1 Acquisition des données

- **Fréquence de poll** : 5 min 15 s par personne (Dexcom mesure toutes les 5 min, marge de 15 s).
- **Décalage entre personnes** : 20 s de décalage entre les 3 requêtes pour étaler la charge WiFi.
- **Retry rapide** : si la dernière mesure d'une personne dépasse 5 min 15 s, re-poll toutes les 30 s. À 8 min, espacer à 90 s pour ne pas saturer un serveur en panne.
- **Authentification** : 2-step Dexcom (account ID puis session ID), session ID mis en cache par personne, ré-authentification uniquement si l'API renvoie un 401.
- **Région Dexcom** : choisissable globalement (US / Non-US / JP), identique aux 3 personnes (cas d'usage familial).

### 5.2 Affichage — page d'accueil

Layout cible (portrait 320×480), **sans bandeau d'heure** :

```
┌─────────────────────────────────┐
│  Léa                ↗         ┃ │
│   142  mg/dL                  ┃ │
├─────────────────────────────────┤
│  Tom                →         ┃ │
│    98  mg/dL                  ┃ │
├─────────────────────────────────┤
│  Marc               ↘         ┃ │
│   185  mg/dL                  ┃ │
└─────────────────────────────────┘
```

**Règles visuelles :**
- **Prénom** : police moyenne, alignée à gauche.
- **Valeur** : police très grande (équivalent `u8g2_font_inb46_mn` ou similaire), centrée dans la ligne. **Seul élément potentiellement coloré** selon zone hypo/normal/hyper si `glucoseColor` activé.
- **Unité** : petite, à côté de la valeur, en blanc.
- **Flèche** : grande, alignée à droite haut. Réutilise la logique de dessin triangulaire existante (`Gestion.cpp`).
- **Âge — jauge verticale** : fine ligne (~4-6 px de large) collée au bord droit de chaque zone, sur toute la hauteur de la zone. Remplissage **de bas en haut**.
  - **Cycle normal (0 → 5 min)** : la jauge se remplit linéairement, en **blanc**. À l'arrivée d'une nouvelle mesure (typiquement à 5 min), elle se vide d'un coup et le cycle recommence.
  - **Refresh en retard (5 → 15 min)** : la jauge **reste pleine** et passe en **orange**.
  - **Critique (> 15 min ou valeur manquante)** : la jauge **reste pleine** et passe en **rouge**.
  - Intuition : tant que la jauge "respire" (se remplit puis se vide toutes les 5 min), la chaîne d'acquisition fonctionne. Une jauge figée pleine = alerte visuelle immédiate.
- **Séparateurs** : lignes horizontales fines en gris foncé entre les 3 zones.
- **Pas de couleur de fond** : fond noir uniforme sur les 3 zones (seule la valeur se colore éventuellement).

**Hauteur de chaque zone** : `EcranH / 3 ≈ 160 px`.

### 5.3 Pages tactiles — navigation

Navigation par swipe horizontal entre 3 pages tournantes :

1. **Accueil** (`pageAccueil`) — l'affichage 3 lignes décrit ci-dessus.
2. **Messages** (`pageMessages`) — log d'événements (héritage, gardé pour le debug).
3. **Configuration** (`pageConfiguration`) — menu vers les pages fixes.

Pages fixes accessibles depuis Configuration :
- **WiFi** : sélection SSID + mot de passe (hérité).
- **Personne 1 / Personne 2 / Personne 3** : prénom + login Dexcom + mot de passe. *Nouvelle structure.*
- **Région Dexcom** : US / Non-US / JP (global).
- **Affichage** : unité, couleur, luminosité nuit (hérité, simplifié).
- **Fuseau horaire** (hérité).
- **Langue** (hérité).
- **Infos / OTA** (hérité).

### 5.4 Configuration au premier démarrage

Au boot, si aucun WiFi connu :
1. Demande WiFi → page liste WiFi + clavier mot de passe.
2. Pour chaque personne i ∈ {1,2,3} si non configurée → demande prénom + identifiants Dexcom.
3. Lance l'acquisition.

Si une personne reste non configurée (ex. : utilisateur veut démarrer avec 2 personnes), sa zone affiche `— non configuré —`.

---

## 6. Exigences non-fonctionnelles

| # | Exigence |
|---|----------|
| NF1 | Lecture confortable à 2-3 m de distance (taille de police adaptée) |
| NF2 | Démarrage à froid → premier affichage < 60 s |
| NF3 | Aucune perte de configuration après coupure d'alimentation |
| NF4 | Résilience : si une personne sur trois est en échec (capteur HS, identifiants invalides), les 2 autres continuent de s'afficher sans interruption |
| NF5 | Pas de redémarrage automatique sauf si **les 3 personnes** sont en échec depuis >30 min (assouplissement par rapport à l'amont qui reboot si 1 personne échoue) |
| NF6 | Mémoire : tenir dans la PSRAM disponible (8 MB sur S3) |
| NF7 | Lisibilité tactile : zones cliquables ≥ 40×40 px |
| NF8 | Watchdog réinitialisé tant que le WiFi est connecté |
| NF9 | Aucune donnée médicale n'est exposée à un service externe au-delà de Dexcom Share |

---

## 7. Architecture technique

### 7.1 Refactor central : du global au tableau

Le firmware actuel manipule des **variables globales scalaires** dans `src/Config.cpp`/`Config.h` :

```cpp
int16_t GlycemieVal;
int8_t TrendArrow;
unsigned long lastGlyUnixTime;
String Glycemie;
// + tableaux historique sur 300 points
```

Et un état de session statique unique dans `Dexcom.cpp` :

```cpp
static String dexcomSessionId = "";
static String dexcomAccountId = "";
```

**Refactor cible :** une struct `Person`, et un tableau de 3 personnes en variable globale.

```cpp
#define MAX_PERSONS 3

struct Person {
  // Identité
  String name;                  // affichage : "Léa"
  bool configured;              // false = zone vide à l'écran

  // Source de données (figé Dexcom en MVP, prêt pour mixte)
  SensorType sensorType;        // SENSOR_DEXCOM

  // Credentials
  String dexcomUsername;
  String dexcomPassword;
  // libreEmail, librePass, libreZone pour v2

  // État de session (par personne)
  String dexcomSessionId;
  String dexcomAccountId;

  // Dernière mesure
  int16_t glucoseMgDl;          // 0 = pas de mesure
  int8_t trendArrow;            // -1..6, 0 = inconnu
  unsigned long lastGlyUnixTime;
  long ageSeconds;              // recalculé par loop

  // Timers de poll par personne
  unsigned long lastDemandeMillis;
  unsigned long lastReceptionMillis;
  unsigned long lastOkMillis;
  unsigned long recurMillis;

  // Cibles (héritage, global pour MVP mais préparé per-person)
  int16_t targetLow;
  int16_t targetHigh;
};

extern Person persons[MAX_PERSONS];
extern int activePersonsCount;  // 0..3
```

### 7.2 Modules concernés

| Module | Modification |
|--------|--------------|
| `Config.h/.cpp` | Remplacer les globales scalaires par `Person persons[3]`. Garder les globales transverses (`HEURE`, `glucoseUnit`, etc.). |
| `Dexcom.h/.cpp` | Chaque fonction prend `Person&` en paramètre. La fonction `LectureDexcom()` devient une boucle sur `persons[]`. |
| `Libreview.h/.cpp` | Inchangé en MVP (non appelé), mais signature future cohérente avec Dexcom. |
| `Stock.h/.cpp` | Sérialisation JSON : remplacer les champs uniques par un tableau `persons` dans `parametres.json`. |
| `Ecran/pageAccueil.cpp` | Réécriture complète : 3 zones, plus de jauge, plus de courbe. |
| `Ecran/pageCompte.cpp` | Ajouter un sélecteur "Personne 1/2/3" avant d'éditer. |
| `Ecran/pageConfiguration.cpp` | Ajouter 3 entrées (Personne 1, Personne 2, Personne 3). |
| `main.cpp` | `setup()` : boucler init sur les 3 personnes. `loop()` : déléguer à un `lectureGlycemies()` qui itère. |
| `Langues/*.h` | Ajouter clés `Person1`, `Person2`, `Person3`, `NotConfigured`, `NoSensor`. |
| `HTML/*` | Pages web mises à jour pour refléter le multi-personnes (consultation et debug). |

### 7.3 Format de stockage (parametres.json)

**Avant :**
```json
{
  "ssid": "...",
  "password": "...",
  "dexcomUsername": "...",
  "dexcomPassword": "...",
  "dexcomRegion": "Non-US",
  "sensorType": 1,
  "...": "..."
}
```

**Après :**
```json
{
  "ssid": "...",
  "password": "...",
  "dexcomRegion": "Non-US",
  "persons": [
    {
      "name": "Léa",
      "sensorType": 1,
      "dexcomUsername": "lea@example.com",
      "dexcomPassword": "***"
    },
    {
      "name": "Tom",
      "sensorType": 1,
      "dexcomUsername": "tom@example.com",
      "dexcomPassword": "***"
    },
    {
      "name": "Marc",
      "sensorType": 1,
      "dexcomUsername": "marc@example.com",
      "dexcomPassword": "***"
    }
  ],
  "glucoseUnit": 0,
  "glucoseColor": 0,
  "idxFuseau": 2,
  "LaLangue": 1,
  "LuminositeNuit": 255,
  "rotation": 1
}
```

### 7.4 Boucle de polling multi-personnes

Pseudo-code de la boucle principale :

```
loop():
  for each person i in 0..2:
    if person[i].configured:
      if time_since(person[i].lastDemandeMillis) > person[i].recurMillis:
        # décalage 20 s entre personnes pour étaler
        if (millis() % decalage) matches person index:
          LectureDexcom(person[i])

  AffichageAccueil()  # rendu 3 lignes
```

---

## 8. API Dexcom — détails d'intégration

### 8.1 Endpoints utilisés (héritage)

| Étape | Méthode | Endpoint |
|-------|---------|----------|
| Auth | POST | `/ShareWebServices/Services/General/AuthenticatePublisherAccount` |
| Login | POST | `/ShareWebServices/Services/General/LoginPublisherAccountById` |
| Glucose | POST | `/ShareWebServices/Services/Publisher/ReadPublisherLatestGlucoseValues` |

### 8.2 Multi-comptes

Chaque appel se fait avec les credentials d'**une personne**. Pas d'API "follower" exposée publiquement par Dexcom (les apps tierces utilisent toutes la voie "publisher"). Donc :
- 3 cycles d'auth (espacés)
- 3 session IDs maintenus en parallèle dans `persons[].dexcomSessionId`

### 8.3 Région

Trois bases URL possibles, **identiques pour toute la famille** (cas d'usage : foyer dans un seul pays) :
- US : `https://share2.dexcom.com`
- Non-US : `https://shareous1.dexcom.com` (défaut)
- JP : `https://share.dexcom.jp` (avec App ID spécifique)

### 8.4 Gestion d'erreur par personne

| Code HTTP | Comportement |
|-----------|--------------|
| 200 | Mise à jour de la mesure |
| 401 | Invalider session ID, retry avec re-auth |
| Autre | Log erreur sur page Messages, conserver la dernière mesure connue, retry plus tard |
| Pas de réponse | Idem |

Une erreur sur une personne **n'affecte pas** les autres.

---

## 9. Stratégie de test

### 9.1 Sans hardware (avant arrivée du module)

| Type de test | Outil |
|--------------|-------|
| Compilation | `pio run -e esp32-s3-devkitc-1` — valide types, includes, signatures |
| Validation API Dexcom | Script Python ad hoc, avec les vrais credentials des 3 personnes, pour confirmer région + format des réponses + capture de fixtures JSON |
| Tests natifs | Env PlatformIO `native` : isoler parsing JSON, machine d'état du polling multi-personnes, formatage de l'âge, calcul des couleurs. Utiliser les fixtures Python comme entrées. |
| Mockup UI | Page HTML/CSS reproduisant le rendu 320×480 cible, ajustement des proportions/polices avant code Arduino_GFX |

### 9.2 Avec hardware (après réception du module)

| Test | Validation |
|------|------------|
| Flashage initial | Carte démarre, écran allumé, page d'accueil affiche "ConfNul" |
| Config WiFi | Sélection SSID, saisie mot de passe, connexion réussie |
| Config 3 personnes | Saisie via clavier tactile des 3 credentials, sauvegarde en LittleFS, persistance après reboot |
| Premier polling | Les 3 valeurs apparaissent en moins de 5 min |
| Tendances | Comparer flèche affichée vs app Dexcom officielle des 3 personnes |
| Vieillissement | Couper le WiFi, vérifier passage orange à 10 min, rouge à 15 min |
| Résilience | Mettre 1 mauvais mot de passe sur 3 → les 2 autres continuent |
| Reboot | Couper l'alim, rebrancher → reprise immédiate avec la config persistée |
| OTA | Flashage à distance via interface web |

---

## 10. Roadmap d'implémentation

### Étape 1 — Refactor du modèle de données
- Introduire `struct Person` et `Person persons[MAX_PERSONS]` dans `Config.h/.cpp`.
- Remplacer chaque usage des globales scalaires (`GlycemieVal`, `TrendArrow`, etc.) par `persons[0].glucoseMgDl`, etc.
- **À cette étape, seule `persons[0]` est utilisée** — comportement identique à l'amont, mais base posée.
- **Critère de sortie :** `pio run` OK, comportement inchangé en théorie.

### Étape 2 — Persistance multi-personnes
- Adapter `Stock.cpp` (`SerializeConfiguration`, `DeserializeConfiguration`) au nouveau JSON avec tableau `persons`.
- Écrire la migration : si l'ancien format est détecté, mapper sur `persons[0]`.
- **Critère de sortie :** `parametres.json` produit/relu au nouveau format.

### Étape 3 — Acquisition par personne (Dexcom)
- Refactor `Dexcom.cpp` : fonctions prennent `Person&`.
- État de session (`dexcomSessionId`, `dexcomAccountId`) déplacé dans la struct.
- `LectureDexcom()` boucle sur les 3 personnes avec décalage temporel.
- **Critère de sortie :** validation native avec fixtures JSON, parsing correct pour 3 réponses simulées.

### Étape 4 — Nouvelle page d'accueil
- Réécrire `pageAccueil.cpp` : 3 zones empilées.
- Réutiliser le code de dessin des flèches existantes.
- **Critère de sortie :** mockup HTML validé visuellement, puis rendu testé à la réception du hardware.

### Étape 5 — Pages de configuration multi-personnes
- `pageConfiguration.cpp` : ajouter 3 entrées "Personne 1/2/3".
- `pageCompte.cpp` : prendre un index de personne en paramètre.
- Pages clavier : adapter pour saisir prénom + credentials par personne.
- **Critère de sortie :** flux complet de configuration testable sur hardware.

### Étape 6 — Comportements globaux
- Adapter `AlertePasdeGlycemie()` : ne reboot que si les 3 sont silencieuses.
- Indicateurs visuels par zone (orange/rouge selon `ageSeconds`).
- Retirer définitivement le code de jauge et de courbe.
- Nettoyer les pages devenues obsolètes.
- **Critère de sortie :** comportement complet validé sur hardware.

### Étape 7 — Polish
- Mise à jour des chaînes de langue (FR/EN au minimum).
- Pages web (`HTML/`) mises à jour : `pageBrute.h` affiche 3 blocs JSON (un par personne), `pageMain.h` affiche les 3 glycémies.
- mDNS configuré sur `gluco-family.local`.
- README adapté au nouveau projet, **mentionner explicitement le stockage en clair** des identifiants et les implications.
- **Critère de sortie :** projet prêt à partager.

---

## 11. Décisions arrêtées

| # | Sujet | Décision |
|---|-------|----------|
| D1 | **Comptes Dexcom Share** | ✅ Activés pour les 3 personnes. Aucun blocage côté API. |
| D2 | **Bandeau heure** dans le layout | ❌ Pas de bandeau. Espace gagné réparti sur les 3 zones. |
| D3 | **Couleurs de fond** par zone | ❌ Pas de fond coloré. Seule la valeur de glycémie peut être colorée (option `glucoseColor` héritée). Le reste reste sur fond noir uniforme. |
| D4 | **Stockage des identifiants** | En clair dans `/parametres.json` sur LittleFS, comme dans l'amont. Justification : la flash n'est pas accessible via le réseau ; chiffrer impliquerait une clé à stocker quelque part (problème de l'œuf et la poule), disproportionné pour un objet de domicile. Pratique standard des firmwares ESP32 hobby (Tasmota, ESPHome…). À documenter clairement dans le README. |
| D5 | **Nom mDNS** du device | `gluco-family.local` |
| D6 | **Page brute** (`/Brute` web) | ✅ Conservée et adaptée. Affichera les 3 JSON Dexcom côte à côte (un bloc par personne). Le garde-fou "consentement tactile" de 3 min est conservé tel quel (sécurité contre exposition accidentelle des données médicales sur le réseau local). |

---

## 12. Risques identifiés

| Risque | Impact | Mitigation |
|--------|--------|------------|
| API Dexcom Share change ou est rate-limitée pour des comptes "follower" | Le firmware ne récupère plus les données | Polling espacé (20 s entre personnes, 5 min entre cycles), surveiller la communauté xDrip/Nightscout pour les évolutions |
| RAM/PSRAM insuffisante pour 3 sessions simultanées | Crash/reboot | Sessions stockées en `String` héritées, taille raisonnable (~100 chars × 3) ; surveiller `ESP.getFreeHeap()` à chaque cycle |
| Police trop grosse pour la valeur dans une zone de 155 px | Texte tronqué | Mockup HTML d'abord, choix de police testé à plusieurs tailles |
| Hardware AXS15231B en rupture | Délai projet | Plan B : pivot vers Waveshare 2.8" capacitif ILI9341 + portage tactile (chiffré à ~10 h) |
| 1 enfant éloigné (école) → données non rafraîchies par son téléphone | Affichage figé pendant des heures | Indicateur orange/rouge sur l'âge ; comportement attendu, pas un bug |

---

## 13. Définition de "fini" pour le MVP

Le MVP est considéré comme livré quand :
- [ ] Les 3 glycémies des membres de la famille s'affichent en continu avec leur flèche et leur âge.
- [ ] La configuration des 3 comptes est possible 100% via les pages tactiles.
- [ ] Une erreur sur 1 capteur n'interrompt pas les 2 autres.
- [ ] La configuration survit à un reboot.
- [ ] Le `README.md` documente la nouvelle utilisation.

---

## 14. Annexes

### A. Liens

- Projet amont : https://github.com/F1ATB/Gluco-Monitor
- Doc amont : https://f1atb.fr/gluco-monitor-diy/
- API Dexcom Share (non officielle) : https://github.com/gagebenne/pydexcom
- API LibreLinkUp (non officielle) : https://github.com/timoschlueter/nightscout-libre-link-up

### B. Glossaire

- **CGM** : Continuous Glucose Monitor — capteur de glycémie en continu
- **LibreLinkUp** : app de partage Abbott pour les FreeStyle Libre
- **Dexcom Share / Follow** : système de partage Dexcom
- **Publisher** : compte de la personne portant le capteur (côté API Dexcom)
- **Follower** : compte d'un proche qui consulte les données du publisher
- **OTA** : Over The Air — mise à jour firmware par WiFi
- **PSRAM** : RAM externe additionnelle, accessible sur ESP32-S3
- **LittleFS** : système de fichiers minimal pour microcontrôleur