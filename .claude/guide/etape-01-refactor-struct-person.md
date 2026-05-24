# Guide — Étape 1 : Refactor du modèle de données (`struct Person`)

> **Référence PRD** : §7.1 (struct) + §10 étape 1 (scope)
> **Objectif** : poser la fondation multi-personnes en introduisant `struct Person` et `Person persons[MAX_PERSONS]`, **sans changer le comportement** de l'app. À la fin de cette étape, l'app fonctionne exactement comme avant (1 seul compte affiché), mais toute la donnée per-person vit dans `persons[0]` au lieu de globales scalaires.

---

## 📚 Ce qu'on construit

Aujourd'hui, le firmware manipule les données d'**une seule personne** via une vingtaine de variables globales scalaires dans `Config.h`/`Config.cpp` :

```cpp
int16_t GlycemieVal;
int8_t TrendArrow;
unsigned long lastGlyUnixTime;
String dexcomUsername;
String dexcomPassword;
// ... etc, 18 globales per-person
```

Pour afficher 3 personnes, il faudra **3 jeux complets de toutes ces données**. La voie évidente — préfixer (`GlycemieVal1`, `GlycemieVal2`, `GlycemieVal3`) — produirait un code épouvantable. La voie propre, c'est une **struct** qui regroupe tout ce qui concerne une personne, et un **tableau** de 3 structs :

```cpp
struct Person {
  String dexcomUsername;
  int16_t glucoseMgDl;
  // ... tous les champs per-person
};

Person persons[3];   // 3 personnes
```

Et au lieu de `GlycemieVal`, on écrira `persons[0].glucoseMgDl`. Pour cette étape, **seule `persons[0]` est utilisée** — le reste du tableau existe mais n'est jamais lu/écrit. Les étapes suivantes activeront `persons[1]` et `persons[2]`.

### Pourquoi cette étape isolée

On aurait pu faire le refactor "modèle + boucle de polling + page d'accueil" en une seule grosse passe. Mais ce serait risqué : un bug dans le code de polling deviendrait invisible derrière un bug d'affichage. En isolant l'étape 1 à un refactor **iso-comportemental** (l'app marche pareil avant/après), on a un critère de validation simple :

> Après refactor : flash → l'écran affiche la glycémie de ma fille exactement comme avant.

Si oui : la base est saine, on peut empiler les étapes suivantes.

### Pourquoi pas de classe avec constructeurs

C'est du C++ embarqué, pas du C++ moderne plein de RAII. Le projet utilise déjà `String` Arduino (qui est un peu lourd mais OK), et l'écosystème ESP32 préfère les structs POD avec accès direct aux champs. Pas de constructeurs, pas de héritage, pas de virtual. Une init explicite dans `setup()` ou dans la lecture du `parametres.json`, c'est tout.

---

## 🎯 Scope précis

### ✅ Dans cette étape

- Définir `struct Person` dans `Config.h`
- Définir `Person persons[MAX_PERSONS]` + `int activePersonsCount` dans `Config.cpp`
- Migrer **tous les callsites** des globales per-person vers `persons[0].xxx`
- Supprimer les anciennes globales de `Config.h`/`Config.cpp`
- Build + flash + vérifier comportement identique

### ❌ Hors scope (étapes suivantes)

| Quoi                                                                  | Pourquoi pas maintenant                                                      | Étape qui le traite |
| --------------------------------------------------------------------- | ---------------------------------------------------------------------------- | ------------------- |
| Format JSON multi-personnes dans `parametres.json`                    | On garde le format mono-compte ; on lit/écrit juste depuis `persons[0]`      | Étape 2             |
| Boucle de polling qui itère sur les 3 personnes                       | Reste mono-compte ; appelle juste `persons[0]` au lieu de la globale         | Étape 3             |
| State statique de `Dexcom.cpp` (`dexcomSessionId`, `dexcomAccountId`) | Reste `static` au fichier ; déplacé dans `Person` quand le polling itèrera   | Étape 3             |
| State statique de `Libreview.cpp` (`AuthToken`, `userID`, etc.)       | Idem — pas appelé en MVP Dexcom, refactor plus tard                          | Étape 3             |
| Tableau `glucoseValues[300]` (historique 24h)                         | Encore utilisé par `pageAccueil.cpp` ; supprimé quand l'accueil sera réécrit | Étape 4             |
| `LoginJSON`, `GraphJSON`, `ConnectionJSON` (buffers debug PSRAM)      | Restent globaux ; deviendront per-person dans la page `/Brute`               | Étape 7             |

---

## ⚠️ Pré-requis

- ✅ Étape 0 validée (build + flash + Dexcom OK sur ta fille)
- ✅ Commit du fix `pioarduino` déjà poussé
- ⚠️ **Branche dédiée recommandée** : `git checkout -b refactor/struct-person` — c'est un gros refactor, ça aide à isoler.
- ⚠️ **Ne fais pas de "Replace All" sauvage en aveugle** sur le projet entier. La section "Migration" ci-dessous te guide fichier par fichier — c'est plus sûr.

---

## Section A — Définir `struct Person` dans `Config.h`

📄 **Édite** : `src/Config.h`

### A.1 — Ajouter `MAX_PERSONS` et la struct

Repère l'enum `SensorType` (lignes 7-10) et **juste en dessous**, insère :

```cpp
#define MAX_PERSONS 3

struct Person {
  // Identité
  String name;                       // "Léa", "Tom", "Marc" — affiché sur la zone
  bool configured;                   // false = zone vide à l'écran (Étape 4)

  // Source de données (per-person, prêt pour mixte Libre+Dexcom en v2)
  SensorType sensorType;             // SENSOR_DEXCOM en MVP

  // Credentials Dexcom
  String dexcomUsername;
  String dexcomPassword;

  // Dernière mesure
  int16_t glucoseMgDl;               // 0 = pas de mesure
  int8_t trendArrow;                 // -1..6, 0 = inconnu
  unsigned long lastGlyUnixTime;     // timestamp Unix de la mesure
  long ageSeconds;                   // recalculé par main loop

  // Timers de poll (millis)
  unsigned long lastDemandeMillis;
  unsigned long lastReceptionMillis;
  unsigned long lastOkMillis;
  unsigned long recurMillis;         // intervalle de poll adaptatif

  // Cibles glycémiques (per-person dans la struct, valeur identique MVP)
  int16_t targetLow;
  int16_t targetHigh;
};
```

### A.2 — Déclarer les globales

Juste après la struct, ajoute les `extern` :

```cpp
extern Person persons[MAX_PERSONS];
extern int activePersonsCount;       // 0..3, nombre de personnes effectivement configurées
```

### A.3 — Supprimer les anciennes globales per-person

Repère et **supprime** les `extern` suivants (toujours dans `Config.h`) :

```diff
-extern String dexcomUsername;
-extern String dexcomPassword;
-extern String Glycemie;
-extern int16_t GlycemieVal;
-extern int8_t TrendArrow;
-extern unsigned long lastGlyUnixTime;
-extern long AgeGlycemie;
-extern unsigned long lastDemandeGlycMillis;
-extern unsigned long recurGlycMillis;
-extern unsigned long lastReceptionGlycMillis;
-extern unsigned long lastGlycOkMillis;
-extern int16_t targetLow;
-extern int16_t targetHigh;
-extern SensorType sensorType;   // ⚠️ devient persons[*].sensorType
```

### A.4 — Conserver ces globales (ne PAS les toucher)

Ces variables ne sont **pas** per-person et doivent rester globales :

| Variable                                                                  | Pourquoi global                           |
| ------------------------------------------------------------------------- | ----------------------------------------- |
| `dexcomRegion`                                                            | PRD : région unique pour les 3 personnes  |
| `glucoseUnit`, `glucoseColor`                                             | Préférence d'affichage commune            |
| `HEURE`, timezone                                                         | Horloge système                           |
| `MessageEcran`                                                            | Buffer d'affichage UI partagé             |
| `LoginJSON`, `GraphJSON`, `ConnectionJSON`                                | Cache debug (rotation à chaque appel API) |
| `glucoseValues[]`, `glucoseHeure[]`, `pointCountGly`                      | Historique 24h — sera supprimé en étape 4 |
| Tout ce qui concerne LibreLinkUp (`libreEmail`, `librePass`, `libreZone`) | Inchangé en MVP, dans v2                  |

---

## Section B — Définir `persons[]` et `activePersonsCount` dans `Config.cpp`

📄 **Édite** : `src/Config.cpp`

### B.1 — Définir les nouvelles globales

En haut du fichier, après les autres définitions de variables, ajoute :

```cpp
Person persons[MAX_PERSONS];
int activePersonsCount = 0;
```

### B.2 — Initialiser `persons[]` dans `setup()` ou ailleurs

Tu ne peux pas initialiser un tableau de structs C++ avec des `String` directement à la déclaration globale (les `String` ne sont pas constexpr). L'initialisation se fait **au démarrage** dans une fonction. Crée une fonction d'init que tu appelleras depuis `setup()`. Ajoute en haut de `Config.cpp` (avant `clearData`) :

```cpp
void InitPersons() {
  for (int i = 0; i < MAX_PERSONS; i++) {
    persons[i].name = "";
    persons[i].configured = false;
    persons[i].sensorType = SENSOR_DEXCOM;  // défaut MVP
    persons[i].dexcomUsername = "";
    persons[i].dexcomPassword = "";
    persons[i].glucoseMgDl = 0;
    persons[i].trendArrow = 0;
    persons[i].lastGlyUnixTime = 0;
    persons[i].ageSeconds = 0;
    persons[i].lastDemandeMillis = 0;
    persons[i].lastReceptionMillis = 0;
    persons[i].lastOkMillis = 0;
    persons[i].recurMillis = 120000;        // 2 min initial (héritage)
    persons[i].targetLow = 70;              // valeurs sentinel, écrasées par config
    persons[i].targetHigh = 180;
  }
  activePersonsCount = 0;
}
```

Et déclare-la dans `Config.h` à côté des autres prototypes :

```cpp
void InitPersons();
```

### B.3 — Supprimer les anciennes définitions

Dans `Config.cpp`, **supprime** toutes les définitions des globales que tu as enlevées de `Config.h` à l'étape A.3 (`String dexcomUsername = "";`, `int16_t GlycemieVal = 0;`, etc.).

### B.4 — Adapter `clearData()`

La fonction `clearData()` actuelle remet à zéro les globales per-person. Adapte-la pour qu'elle remette à zéro **les données de glycémie de toutes les personnes** (mais pas les credentials — ils restent) :

```cpp
void clearData() {
  for (int i = 0; i < MAX_PERSONS; i++) {
    persons[i].glucoseMgDl = 0;
    persons[i].trendArrow = 0;
    persons[i].lastGlyUnixTime = 0;
    persons[i].ageSeconds = 0;
    persons[i].lastReceptionMillis = 0;
    persons[i].lastOkMillis = 0;
    // credentials et name conservés
  }

  // Historique 24h (sera supprimé en étape 4, mais on garde pour l'instant)
  for (int i = 0; i < MAX_POINTS; i++) {
    glucoseValues[i] = 0;
    glucoseHeure[i] = 0;
  }
  pointCountGly = 0;

  // Buffers debug
  LoginJSON = "";
  GraphJSON = "";
  ConnectionJSON = "";
}
```

---

## Section C — Tableau de mapping (référence pour la migration)

C'est ta **table de correspondance** pour les sections D-J. Garde-la sous les yeux pendant les renames.

| Ancien global (à remplacer) | Nouveau (utiliser à la place)                            |
| --------------------------- | -------------------------------------------------------- |
| `GlycemieVal`               | `persons[0].glucoseMgDl`                                 |
| `TrendArrow`                | `persons[0].trendArrow`                                  |
| `lastGlyUnixTime`           | `persons[0].lastGlyUnixTime`                             |
| `AgeGlycemie`               | `persons[0].ageSeconds`                                  |
| `lastDemandeGlycMillis`     | `persons[0].lastDemandeMillis`                           |
| `lastReceptionGlycMillis`   | `persons[0].lastReceptionMillis`                         |
| `lastGlycOkMillis`          | `persons[0].lastOkMillis`                                |
| `recurGlycMillis`           | `persons[0].recurMillis`                                 |
| `dexcomUsername`            | `persons[0].dexcomUsername`                              |
| `dexcomPassword`            | `persons[0].dexcomPassword`                              |
| `targetLow`                 | `persons[0].targetLow`                                   |
| `targetHigh`                | `persons[0].targetHigh`                                  |
| `sensorType`                | `persons[0].sensorType`                                  |
| `Glycemie` (String mirror)  | **À supprimer** (rarement utilisé, recalcule à la volée) |

⚠️ **Attention à `sensorType`** : c'est le nom le plus ambigu (utilisé aussi dans les pages HTML JS). Pour ces fichiers, je détaille ci-dessous.

---

## Section D — Migrer `src/main.cpp`

📄 **Édite** : `src/main.cpp`

Cherche les occurrences des globales listées dans le tableau C et remplace selon le mapping. Le plus important :

- Le calcul de l'âge (~ligne 172) : `AgeGlycemie = ...` → `persons[0].ageSeconds = ...`
- L'aiguillage sensor : `if (sensorType == SENSOR_DEXCOM)` → `if (persons[0].sensorType == SENSOR_DEXCOM)`

### Ajouter l'appel à `InitPersons()`

Dans `setup()`, **avant** la lecture de la config depuis LittleFS (`DeserializeConfiguration` ou équivalent), ajoute :

```cpp
InitPersons();
```

Comme ça, si la config est manquante/corrompue, les `Person` ont au moins des valeurs sentinel par défaut.

---

## Section E — Migrer `src/Dexcom.cpp`

📄 **Édite** : `src/Dexcom.cpp`

Pareil : applique le mapping de la section C. Dans ce fichier, les renames les plus fréquents seront :

- `dexcomUsername` → `persons[0].dexcomUsername`
- `dexcomPassword` → `persons[0].dexcomPassword`
- `GlycemieVal` → `persons[0].glucoseMgDl`
- `TrendArrow` → `persons[0].trendArrow`
- `lastGlyUnixTime` → `persons[0].lastGlyUnixTime`
- `recurGlycMillis`, `lastDemandeGlycMillis`, etc.

### ⚠️ Ne touche PAS à ces statiques

```cpp
static String dexcomSessionId = "";
static String dexcomAccountId = "";
static String dexcomBaseURL = "https://shareous1.dexcom.com";
```

Ces 3 statiques restent au scope fichier pour étape 1. L'étape 3 les déplacera dans `Person` quand la boucle deviendra multi-personnes.

---

## Section F — Migrer `src/Libreview.cpp`

📄 **Édite** : `src/Libreview.cpp`

Même mapping. Note : Libreview n'est pas appelé en MVP Dexcom (mais le code doit compiler). Pareil, **ne touche pas** aux statiques du fichier (`AuthToken`, `userID`, `patientId`, etc.) — étape 3.

---

## Section G — Migrer `src/Server.cpp`

📄 **Édite** : `src/Server.cpp`

⚠️ **Attention aux lambdas** : ce fichier est plein de lambdas Arduino (`server.on(..., [](AsyncWebServerRequest *req) { ... })`). Les lambdas capturent `[]` (rien explicitement), mais accèdent aux globales par nom. Quand tu renommes `GlycemieVal` → `persons[0].glucoseMgDl`, la lambda continue de fonctionner (toujours global access). Aucune adaptation spéciale nécessaire.

Endpoints à migrer :

- `/ajaxGlycemie` → utiliser `persons[0]`
- `/dataGly` → utiliser `persons[0]` pour le pointCountGly et les arrays (ces arrays restent globaux pour cette étape)
- `/LoginJSON`, `/GraphJSON`, `/ConnectionJSON` → buffers restent globaux, pas de changement

---

## Section H — Migrer `src/Stock.cpp`

📄 **Édite** : `src/Stock.cpp`

Le **format JSON sur disque ne change pas** (étape 2 s'en occupe). On change juste **où** on lit/écrit en mémoire :

```diff
-conf["dexcomUsername"] = dexcomUsername;
-conf["dexcomPassword"] = dexcomPassword;
-conf["sensorType"] = sensorType;
+conf["dexcomUsername"] = persons[0].dexcomUsername;
+conf["dexcomPassword"] = persons[0].dexcomPassword;
+conf["sensorType"] = persons[0].sensorType;
```

Et pareil côté `Deserialize` :

```diff
-dexcomUsername = conf["dexcomUsername"] | "";
-dexcomPassword = conf["dexcomPassword"] | "";
-sensorType = conf["sensorType"] | SENSOR_DEXCOM;
+persons[0].dexcomUsername = conf["dexcomUsername"] | "";
+persons[0].dexcomPassword = conf["dexcomPassword"] | "";
+persons[0].sensorType = (SensorType)(conf["sensorType"] | SENSOR_DEXCOM);
+persons[0].configured = (persons[0].dexcomUsername.length() > 0);
+activePersonsCount = persons[0].configured ? 1 : 0;
```

⚠️ `dexcomRegion` reste global, pas de changement pour cette variable.

---

## Section I — Migrer les pages écran

📄 **Édite** : `src/Ecran/pageAccueil.cpp`, `src/Ecran/pageCompte.cpp`, `src/Ecran/pageClavier.cpp`

Applique le mapping de la section C. Notes :

- `pageAccueil.cpp` : affichage de `GlycemieVal`, `TrendArrow`, calcul d'âge. La courbe 24h utilise `glucoseValues[]` (globaux conservés). Ne réécris pas la page — juste les renames.
- `pageCompte.cpp` : édition des credentials. Les renames suffisent.
- `pageClavier.cpp` : saisie de texte qui écrit dans `dexcomUsername` ou `dexcomPassword` — renames suffisent.

---

## Section J — Build, flash, vérifier

### J.1 — Build

```bash
pio run
```

Tu auras très probablement des erreurs de compilation **manquantes** (oublis dans les renames). Le compilateur t'indiquera précisément :

- `'GlycemieVal' was not declared in this scope` → tu as oublié un renommage dans un fichier
- Cherche dans le fichier indiqué, remplace, recompile

Itère jusqu'à `[SUCCESS]`.

### J.2 — Flash

```bash
pio run -t upload
```

(Ou Upload via PlatformIO dans VSCode.)

### J.3 — Vérification du comportement

Sur l'écran, tu dois retrouver **exactement le même comportement qu'avant le refactor** :

- ✅ Affichage de la glycémie de ta fille (la valeur, la flèche, l'âge)
- ✅ Refresh toutes les ~5 min
- ✅ Page de config accessible
- ✅ Modification des credentials toujours possible
- ✅ Pas de reboot intempestif

Si l'app affiche `0 mg/dL` indéfiniment alors qu'elle marchait avant :

- Ouvre `http://gluco-monitor.local/Brute` → regarde si `ConnectionJSON` montre une erreur API
- Possible cause : un oubli de migration dans `Stock.cpp` (credentials non chargés dans `persons[0]`)

---

## ✅ Critère de sortie

- [ ] `pio run` compile sans warning ni erreur
- [ ] Flash réussi
- [ ] Affichage identique à l'étape 0 (glycémie de la fille, flèche, âge)
- [ ] La page `/Brute` montre toujours `accountId` valide et `glucoseValue` non null
- [ ] Commit propre sur la branche `refactor/struct-person`

Message de commit suggéré :

```
Refactor: introduce struct Person and persons[MAX_PERSONS]

Move all per-person globals (glucose, trend, timers, credentials)
into a single struct, accessed via persons[0]. Behavior unchanged
(still mono-account). Foundation for multi-person support.

PRD §10 étape 1.
```

---

## 🔄 Quand tu auras fini

Pingue-moi :

1. Si tu galères sur une erreur de compilation (copie-moi le message)
2. Quand tu as commit OK
3. Je préparerai alors le guide étape 2 (`parametres.json` multi-personnes)

Bon courage ! C'est le refactor le plus mécanique mais le plus pénible des 7 étapes. Une fois passé, les suivantes seront moins intrusives. 💪
