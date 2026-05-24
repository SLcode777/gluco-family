# Guide — Étape 2 : Persistance multi-personnes (`parametres.json`)

> **Référence PRD** : §10 étape 2
> **Objectif** : faire évoluer le format de `parametres.json` pour stocker 3 personnes au lieu d'une, avec migration automatique de l'ancien format.
> **Fichiers touchés** : 1 seul — `src/Stock.cpp`.

---

## 📚 Ce qu'on construit

Aujourd'hui, `parametres.json` contient les credentials Dexcom **à plat** au niveau racine :

```json
{
  "ssid": "MaBox",
  "password": "...",
  "dexcomUsername": "lea@gmail.com",
  "dexcomPassword": "...",
  "sensorType": 1,
  ...
}
```

C'est correct pour une seule personne, mais ça ne tient pas la route pour 3 — on aurait besoin d'inventer des clés comme `dexcomUsername2`, `dexcomUsername3`. Solution : **un tableau `persons`** :

```json
{
  "ssid": "MaBox",
  "password": "...",
  "dexcomRegion": "OUS",
  "persons": [
    { "name": "Léa",  "configured": true,  "sensorType": 1, "dexcomUsername": "lea@gmail.com",  "dexcomPassword": "..." },
    { "name": "Tom",  "configured": true,  "sensorType": 1, "dexcomUsername": "tom@gmail.com",  "dexcomPassword": "..." },
    { "name": "Marc", "configured": false, "sensorType": 1, "dexcomUsername": "",               "dexcomPassword": "" }
  ],
  ...
}
```

### Ce qui passe dans `persons[]` (per-person)
- `name` — affichage de la zone à l'écran
- `configured` — la zone est-elle "active" ou vide
- `sensorType` — `SENSOR_DEXCOM` pour le MVP
- `dexcomUsername`, `dexcomPassword`

### Ce qui reste à la racine (global)
- WiFi (`ssid`, `password`, `hostname`, `MyIP`)
- `dexcomRegion` — choix unique pour les 3 personnes (cf. PRD §5.1)
- `libreEmail`, `librePass`, `libreZone` — credentials Libre = 1 compte multi-patients (cf. asymétrie d'API qu'on a discutée)
- Préférences UI : `glucoseUnit`, `glucoseColor`, `LuminositeNuit`, `LaLangue`, `idxFuseau`, `rotation`

### La migration

Tu as déjà un `parametres.json` sur ta carte (config créée à l'étape 0). Quand tu flasheras la nouvelle version, elle doit :
1. Lire le vieux format à plat → charger les credentials dans `persons[0]`
2. **Immédiatement** réécrire en nouveau format → la migration est ainsi définitive après le 1er boot

Pour détecter quel format on a, on utilise un champ `format_version` (clean et extensible si jamais on a une v3 plus tard). Absence du champ ⇒ v1 (à plat).

---

## 🎯 Scope précis

### ✅ Dans cette étape

- Refondre `SerializeConfiguration()` pour produire le nouveau format avec tableau `persons`
- Refondre `DeserializeConfiguration()` pour lire **les deux formats** (auto-detect)
- Ajouter un champ `format_version: 2` dans le nouveau format
- Migration auto : si on lit du v1, on déclenche immédiatement une réécriture en v2
- Build, flash, vérifier que les credentials sont conservés après le boot

### ❌ Hors scope

- Pages de config multi-personnes (étape 5)
- Édition du nom (`name`) ou de la 2e/3e personne via l'UI (étape 5)
- Refactor de `Dexcom.cpp` pour boucler sur `persons[]` (étape 3)

---

## ⚠️ Pré-requis

- ✅ Étape 1 mergée sur `main`
- ⚠️ **Branche dédiée recommandée** : `git checkout -b feat/persistance-multi-personnes`
- ⚠️ Tu as un `parametres.json` existant sur la carte (avec ton compte Dexcom). On va vérifier que la migration ne le casse pas.

---

## Section A — Schéma JSON cible (référence, rien à éditer)

> 📖 **Section purement informative** — pas d'action à faire ici. Elle te montre la cible visuelle pour que tu sais à quoi tendre quand tu modifieras le code en Sections B et C.

Voici à quoi devra ressembler le fichier après l'étape 2 :

```json
{
  "format_version": 2,
  "ssid": "MaBox",
  "password": "...",
  "hostname": "gluco-monitor",
  "MyIP": "192.168.1.36",
  "idxFuseau": 1,
  "rotation": 0,
  "libreEmail": "",
  "librePass": "",
  "libreZone": "",
  "LuminositeNuit": 10,
  "LaLangue": 0,
  "dexcomRegion": "OUS",
  "glucoseUnit": 0,
  "glucoseColor": 1,
  "persons": [
    {
      "name": "",
      "configured": true,
      "sensorType": 1,
      "dexcomUsername": "lea@gmail.com",
      "dexcomPassword": "..."
    },
    {
      "name": "",
      "configured": false,
      "sensorType": 1,
      "dexcomUsername": "",
      "dexcomPassword": ""
    },
    {
      "name": "",
      "configured": false,
      "sensorType": 1,
      "dexcomUsername": "",
      "dexcomPassword": ""
    }
  ]
}
```

À la première migration, `persons[0]` contient tes credentials existants, `persons[1]` et `persons[2]` sont vides (`configured: false`). Tu remplis les autres plus tard via l'UI (étape 5).

---

## Section B — Refondre `SerializeConfiguration()`

📄 **Édite** : `src/Stock.cpp`, fonction `SerializeConfiguration()` (lignes ~99-126)

Remplace **toute la fonction** par :

```cpp
String SerializeConfiguration() {
  JsonDocument conf;

  // Schema version (used by DeserializeConfiguration to detect old format)
  conf["format_version"] = 2;

  // WiFi + network
  conf["ssid"] = ssid;
  conf["password"] = password;
  conf["hostname"] = hostname;
  conf["MyIP"] = MyIP;

  // UI / locale
  conf["idxFuseau"] = idxFuseau;
  conf["rotation"] = rotation;
  conf["LuminositeNuit"] = LuminositeNuit;
  conf["LaLangue"] = LaLangue;
  conf["glucoseUnit"] = (int)glucoseUnit;
  conf["glucoseColor"] = (int)glucoseColor;

  // LibreLinkUp (single multi-patient account — stays global)
  conf["libreEmail"] = libreEmail;
  conf["librePass"] = librePass;
  conf["libreZone"] = libreZone;

  // Dexcom (region is global, credentials are per-person)
  conf["dexcomRegion"] = dexcomRegion;

  // Per-person array
  JsonArray personsArray = conf["persons"].to<JsonArray>();
  for (int i = 0; i < MAX_PERSONS; i++) {
    JsonObject p = personsArray.add<JsonObject>();
    p["name"]           = persons[i].name;
    p["configured"]     = persons[i].configured;
    p["sensorType"]     = (int)persons[i].sensorType;
    p["dexcomUsername"] = persons[i].dexcomUsername;
    p["dexcomPassword"] = persons[i].dexcomPassword;
  }

  String Json;
  serializeJson(conf, Json);
  return Json;
}
```

### Points importants

- **`conf["persons"].to<JsonArray>()`** : API ArduinoJson 7.x pour créer un tableau JSON depuis un objet `JsonDocument`. (En v6 c'était `createNestedArray`, qui n'existe plus en v7.)
- **`personsArray.add<JsonObject>()`** : crée un objet JSON dans le tableau, qu'on remplit ensuite.
- **`(int)persons[i].sensorType`** : on cast l'enum en int avant sérialisation (pour avoir `1` plutôt qu'un nom symbolique).
- **On itère sur `MAX_PERSONS` (3)** quelle que soit la valeur de `configured` : on persiste les 3 slots même vides. Plus simple, plus prévisible, et permet à l'utilisateur de revenir éditer un slot vidé sans surprise.

---

## Section C — Refondre `DeserializeConfiguration()`

📄 **Édite** : `src/Stock.cpp`, fonction `DeserializeConfiguration()` (lignes ~52-97)

Remplace **toute la fonction** par :

```cpp
void DeserializeConfiguration(String json) {
  Serial.print("Json reçu:");
  Serial.println(json);
  JsonDocument conf;
  DeserializationError error = deserializeJson(conf, json);

  if (error) {
    Serial.print("Erreur de parsing des paramètres: ");
    Serial.println(error.c_str());
    return;
  }

  // Detect schema version (missing field = legacy v1 with flat dexcom credentials)
  int formatVersion = conf["format_version"] | 1;
  Serial.printf("Config format_version: %d\n", formatVersion);

  // -------- Common fields (same in v1 and v2) --------
  ssid = conf["ssid"].as<String>();
  password = conf["password"].as<String>();
  hostname = conf["hostname"] | hostname;
  MyIP = conf["MyIP"] | MyIP;
  idxFuseau = conf["idxFuseau"].isNull() ? idxFuseau : conf["idxFuseau"];
  rotation = conf["rotation"].isNull() ? rotation : conf["rotation"];
  libreEmail = conf["libreEmail"].as<String>();
  librePass = conf["librePass"].as<String>();
  libreZone = conf["libreZone"].as<String>();
  LuminositeNuit = conf["LuminositeNuit"] | LuminositeNuit;
  LaLangue = conf["LaLangue"] | LaLangue;
  dexcomRegion = conf["dexcomRegion"] | dexcomRegion;

  int glucoseUnitInt = conf["glucoseUnit"] | GLUCOSE_UNIT_MGDL;
  glucoseUnit = (GlucoseUnit)glucoseUnitInt;
  int glucoseColorInt = conf["glucoseColor"] | GLUCOSE_BLANC;
  glucoseColor = (GlucoseColor)glucoseColorInt;

  // -------- Per-person fields (v1 vs v2) --------
  if (formatVersion >= 2) {
    // v2: read from "persons" array
    JsonArray personsArray = conf["persons"].as<JsonArray>();
    for (int i = 0; i < MAX_PERSONS; i++) {
      JsonVariant p = personsArray[i];
      if (p.isNull()) {
        // Slot missing in JSON: leave InitPersons defaults
        continue;
      }
      persons[i].name           = p["name"].as<String>();
      persons[i].configured     = p["configured"] | false;
      int sensorTypeInt         = p["sensorType"] | SENSOR_DEXCOM;
      persons[i].sensorType     = (SensorType)sensorTypeInt;
      persons[i].dexcomUsername = p["dexcomUsername"].as<String>();
      persons[i].dexcomPassword = p["dexcomPassword"].as<String>();
    }
  } else {
    // v1 legacy: flat fields at root, migrate into persons[0] only
    Serial.println("Legacy v1 config detected — migrating to v2 format");
    persons[0].name           = "";
    persons[0].dexcomUsername = conf["dexcomUsername"].as<String>();
    persons[0].dexcomPassword = conf["dexcomPassword"].as<String>();
    int sensorTypeInt         = conf["sensorType"] | SENSOR_DEXCOM;
    persons[0].sensorType     = (SensorType)sensorTypeInt;
    persons[0].configured     = (persons[0].dexcomUsername.length() > 0);
    // persons[1] and [2] keep InitPersons defaults (configured = false)
  }

  // Recompute activePersonsCount from the loaded data
  activePersonsCount = 0;
  for (int i = 0; i < MAX_PERSONS; i++) {
    if (persons[i].configured) activePersonsCount++;
  }
  Serial.printf("Active persons: %d\n", activePersonsCount);

  // If we migrated from v1, immediately rewrite in v2 format so the disk is up-to-date
  if (formatVersion < 2) {
    Serial.println("Rewriting parametres.json in v2 format");
    RecordFichierParametres();
  }
}
```

### Points importants

- **`int formatVersion = conf["format_version"] | 1;`** : si le champ n'existe pas (cas du vieux JSON), on retombe sur `1` par défaut. C'est ça qui déclenche la branche migration.
- **`if (formatVersion >= 2)`** plutôt que `== 2` : prêt pour les versions futures (v3, v4) si on en a un jour besoin.
- **`personsArray[i]` retourne un `JsonVariant`** : si l'index n'existe pas (tableau plus court que MAX_PERSONS), `isNull()` renvoie true → on garde les defaults posés par `InitPersons()` sans planter.
- **`RecordFichierParametres()` en fin de migration** : on déclenche immédiatement une réécriture pour migrer la flash de v1 vers v2. Comme ça la migration n'a lieu **qu'une seule fois**.
- **`activePersonsCount`** : recalculé à la fin, indépendamment du chemin v1/v2. Pour étape 1 le code ne l'utilise pas encore, mais étape 4 oui.

---

## Section D — Build & flash

### D.1 — Build

Clique sur **Build** dans PlatformIO. Doit compiler sans erreur ni warning.

⚠️ Si erreur du genre `'createNestedArray' is not a member of 'ArduinoJson::JsonDocument'` → tu as un reste de syntaxe v6, vérifie que tu utilises bien `to<JsonArray>()`.

### D.2 — Flash

**Avant de flasher**, branche le **Monitor série** (PlatformIO → Monitor) pour voir les logs du boot. Tu vas vouloir observer la migration en direct.

Puis Upload.

### D.3 — Observer la migration au premier boot

Dans le moniteur série, tu dois voir successivement :

```
Json reçu: {...ancien JSON v1...}
Config format_version: 1
Legacy v1 config detected — migrating to v2 format
Active persons: 1
Rewriting parametres.json in v2 format
Ecriture fichier parametres
```

Si tu vois ces lignes ✅ → la migration s'est faite proprement.

### D.4 — Vérifier que la glycémie marche toujours

L'écran doit afficher la glycémie de ta fille comme avant. Si oui, on est good.

### D.5 — Vérifier le nouveau format sur disque

**Reboote la carte** (débranche/rebranche, ou bouton RESET).

Dans le moniteur série au second boot, tu dois maintenant voir :

```
Json reçu: {"format_version":2,"ssid":"...","persons":[{...},{...},{...}],...}
Config format_version: 2
Active persons: 1
```

Plus de `migrating`, plus de `Rewriting`. Le fichier est désormais en v2 sur la flash. 🎯

---

## ✅ Critère de sortie

- [ ] Build sans erreur
- [ ] Flash + boot OK
- [ ] Log moniteur série montre `format_version: 1` puis `migrating` au 1er boot
- [ ] Log moniteur série montre `format_version: 2` aux boots suivants
- [ ] La glycémie de ta fille s'affiche toujours correctement
- [ ] `persons[0]` est `configured`, `persons[1]` et `persons[2]` sont `!configured`

Message de commit suggéré :

```
Persistance multi-personnes: JSON v2 format with persons array

- Add format_version field for future schema evolution
- Read both v1 (legacy flat) and v2 (persons array) configs
- Auto-migrate v1 → v2 on first boot of new firmware
- persons[1] and persons[2] initialized empty (configured by UI in étape 5)

PRD §10 étape 2.
```

---

## 🔄 Quand tu auras fini

Pingue-moi avec :
- Le log du moniteur série au premier boot (pour valider la migration)
- Le log du second boot (pour valider le nouveau format)
- Et bien sûr, confirmation que la glycémie s'affiche toujours

Ensuite : guide **étape 3** (refactor `Dexcom.cpp` pour boucler sur les 3 personnes — c'est là que ça devient vraiment "multi-personnes" en runtime). 💪
