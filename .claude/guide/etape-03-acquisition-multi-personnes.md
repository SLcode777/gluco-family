# Guide — Étape 3 : Acquisition multi-personnes (Dexcom)

> **Référence PRD** : §10 étape 3
> **Objectif** : faire boucler `LectureDexcom()` sur les 3 personnes configurées, avec un décalage temporel pour étaler la charge réseau. Chaque personne a son propre cache de session Dexcom.
> **Fichiers touchés** : `Config.h`, `Config.cpp`, `Dexcom.cpp`.

---

## 📚 Ce qu'on construit

Aujourd'hui, `LectureDexcom()` interroge **uniquement `persons[0]`** :

```cpp
void LectureDexcom() {
    persons[0].recurMillis = 315000;
    if (persons[0].ageSeconds < 315 ...) return;
    if (loginDexcomShare()) {
        getDexcomReadings();
    }
}
```

Le state de session (`dexcomSessionId`, `dexcomAccountId`) vit dans des `static` au scope du fichier. Pour 3 personnes, ces statiques deviennent un problème : on aurait **un seul** sessionId pour 3 comptes Dexcom différents → conflit.

### Refactor cible

1. **Déplacer `dexcomSessionId` et `dexcomAccountId` dans `struct Person`** : chaque personne a son propre cache de session.
2. **Faire prendre `Person&` en paramètre** aux fonctions `loginDexcomShare()` et `getDexcomReadings()`. Plus de hardcode `persons[0]`.
3. **Boucler dans `LectureDexcom()`** sur les 3 personnes, ne polling que celles qui sont configurées.
4. **Décaler les requêtes de 20 s** entre 2 polls consécutifs pour étaler la charge réseau et éviter de saturer le serveur Dexcom.

### Pourquoi le décalage

Si les 3 personnes arrivent à expiration en même temps (cas typique au premier boot : tout à zéro), on ferait 3 requêtes HTTPS simultanées sur Dexcom. Ça stresse le WiFi de la box, le SSL/TCP, et le serveur Dexcom qui pourrait rate-limiter. Avec 20 s d'écart, les requêtes s'étalent naturellement et le rythme persiste cycle après cycle (chaque personne se retrouve naturellement 20 s derrière la précédente).

---

## 🎯 Scope précis

### ✅ Dans cette étape

- Ajouter 2 champs dans `Person` : `dexcomSessionId`, `dexcomAccountId`
- Initialiser ces champs dans `InitPersons()`
- Refactor `loginDexcomShare(Person& person)`
- Refactor `getDexcomReadings(Person& person)`
- Refactor `LectureDexcom()` : boucle sur les 3, skip si non configuré, stagger 20 s
- Refactor `clearDexcomCache()` : vider le cache de **toutes** les personnes
- Scaffolding temporaire pour configurer `persons[1]` et `persons[2]` (test) — à virer après validation

### ❌ Hors scope

- UI tactile pour configurer les personnes 2 et 3 (étape 5)
- Multi-personnes sur les buffers debug `LoginJSON`/`GraphJSON`/`ConnectionJSON` (étape 7)
- Nouvelle page d'accueil avec affichage 3 zones (étape 4) — pour l'instant l'écran affichera toujours `persons[0]`
- Tableau `glucoseValues[300]` (historique 24h) — toujours rempli avec `persons[0]` uniquement, sera supprimé en étape 4
- Refactor Libreview (pas appelé en MVP)

### ⚠️ Effet visible attendu

À la fin de cette étape, **l'écran affichera toujours uniquement la glycémie de `persons[0]`** (l'étape 4 changera ça). Mais en interne, les 3 personnes seront pollées et leurs données seront stockées dans `persons[1].glucoseMgDl` et `persons[2].glucoseMgDl`. Tu valideras via le moniteur série et la page `/Brute`.

---

## ⚠️ Pré-requis

- ✅ Étape 2 mergée sur `main`
- ✅ Tu as les **3 jeux de credentials Dexcom Share** (ta fille, Tom, ton mari)
- ⚠️ **Branche dédiée** : `git checkout -b feat/acquisition-multi-personnes`

---

## Section A — Ajouter les champs de session dans `Person`

📄 **Édite** : `src/Config.h`

Dans la `struct Person`, ajoute 2 champs dans la zone "État de session" (entre les credentials et la dernière mesure) :

```diff
   // Credentials Dexcom
   String dexcomUsername;
   String dexcomPassword;

+  // Session cache (per-person, populated by loginDexcomShare)
+  String dexcomSessionId;
+  String dexcomAccountId;
+
   // Dernière mesure
   int16_t glucoseMgDl;
```

📄 **Édite** : `src/Config.cpp`, fonction `InitPersons()`

Ajoute l'init des 2 nouveaux champs (ordre cohérent avec la struct) :

```diff
     persons[i].dexcomUsername = "";
     persons[i].dexcomPassword = "";
+    persons[i].dexcomSessionId = "";
+    persons[i].dexcomAccountId = "";
     persons[i].glucoseMgDl = 0;
```

---

## Section B — Refactor `loginDexcomShare()` pour prendre `Person&`

📄 **Édite** : `src/Dexcom.cpp`

### B.1 — Mettre à jour le prototype

Si tu as un fichier `Dexcom.h` avec le prototype, change-le :

```diff
-bool loginDexcomShare();
+bool loginDexcomShare(Person& person);
```

### B.2 — Mettre à jour l'implémentation

Remplace la signature de la fonction et toutes les références `persons[0]` par `person` :

```diff
-bool loginDexcomShare()
+bool loginDexcomShare(Person& person)
 {
     ServerConnu = false;
     ...
-    if (dexcomAccountId.length() == 0) {
+    if (person.dexcomAccountId.length() == 0) {
         ...
-        authDoc["accountName"] = persons[0].dexcomUsername;
-        authDoc["password"] = persons[0].dexcomPassword;
+        authDoc["accountName"] = person.dexcomUsername;
+        authDoc["password"] = person.dexcomPassword;
         ...
-        dexcomAccountId = response.substring(1, response.length() - 1);
+        person.dexcomAccountId = response.substring(1, response.length() - 1);
     }
     ...
-    loginDoc["accountId"] = dexcomAccountId;
-    loginDoc["password"] = persons[0].dexcomPassword;
+    loginDoc["accountId"] = person.dexcomAccountId;
+    loginDoc["password"] = person.dexcomPassword;
     ...
-    dexcomSessionId = response.substring(1, response.length() - 1);
+    person.dexcomSessionId = response.substring(1, response.length() - 1);
     ...
-    return dexcomSessionId.length() > 30;
+    return person.dexcomSessionId.length() > 30;
 }
```

### B.3 — Supprimer les statiques devenues inutiles

En haut de `Dexcom.cpp`, **supprime** ces 2 statiques (déplacées dans `Person`) :

```diff
-static String dexcomSessionId = "";
-static String dexcomAccountId = "";

 // Dexcom Share API base URL, Non-US (default)
 static String dexcomBaseURL = "https://shareous1.dexcom.com";
```

⚠️ **Laisse `dexcomBaseURL` en static** : c'est une fonction de `dexcomRegion` (global, identique pour les 3 personnes), donc pas besoin de la déplacer.

---

## Section C — Refactor `getDexcomReadings()` pour prendre `Person&`

📄 **Édite** : `src/Dexcom.cpp`

### C.1 — Mettre à jour le prototype

Dans `Dexcom.h` si présent :

```diff
-void getDexcomReadings();
+void getDexcomReadings(Person& person);
```

### C.2 — Mettre à jour l'implémentation

```diff
-void getDexcomReadings()
+void getDexcomReadings(Person& person)
 {
     HTTPClient https;

-    Serial.println("getDexcomReadings - Session ID: " + dexcomSessionId);
+    Serial.println("getDexcomReadings for " + person.dexcomUsername + " - Session ID: " + person.dexcomSessionId);

     String url = dexcomBaseURL + String(DEXCOM_GLUCOSE_ENDPOINT) +
-                 "?sessionId=" + dexcomSessionId +
+                 "?sessionId=" + person.dexcomSessionId +
                  "&minutes=1440&maxCount=288";
```

Et remplace **toutes les occurrences de `persons[0]` par `person`** dans le corps de la fonction. Le mapping :

| Avant | Après |
|---|---|
| `persons[0].glucoseMgDl` | `person.glucoseMgDl` |
| `persons[0].trendArrow` | `person.trendArrow` |
| `persons[0].lastGlyUnixTime` | `person.lastGlyUnixTime` |
| `persons[0].lastReceptionMillis` | `person.lastReceptionMillis` |
| `persons[0].lastOkMillis` | `person.lastOkMillis` |

### C.3 — Cas particulier : `glucoseValues[]` et `pointCountGly`

Ces 2 globales restent inchangées (toujours alimentées par les readings, pas par `person.*`). MAIS comme l'historique 24h n'a pas vraiment de sens en multi-personnes, on va **ne le populer que pour `persons[0]`** (sera supprimé en étape 4). Encadre le bloc de remplissage des tableaux avec une garde :

```cpp
// Historical arrays kept only for person 0 (to be deleted in étape 4)
if (&person == &persons[0]) {
    pointCountGly = 0;
    for (int i = readings.size() - 1; i > -1; i--) {
        // ... existing code ...
    }
    Serial.println("Nombre de points Dexcom: " + String(pointCountGly));
}
```

`&person == &persons[0]` compare les **adresses mémoire** : on vérifie que c'est bien la personne 0 (et pas une copie). Comme on passe `Person&` (référence), `&person` est bien l'adresse de la vraie struct dans le tableau.

---

## Section D — Refactor `LectureDexcom()` : la boucle multi-personnes

📄 **Édite** : `src/Dexcom.cpp`

Remplace **toute** la fonction `LectureDexcom()` par :

```cpp
void LectureDexcom()
{
    // Global stagger: at least 20s between any 2 Dexcom polls to spread network load
    static unsigned long lastAnyPollMillis = 0;
    const unsigned long STAGGER_MS = 20000;

    if (millis() - lastAnyPollMillis < STAGGER_MS && lastAnyPollMillis > 0) {
        return; // too soon since last poll, wait
    }

    // Find the person most overdue for a refresh and poll them
    for (int i = 0; i < MAX_PERSONS; i++) {
        Person& person = persons[i];

        if (!person.configured) continue;
        if (person.sensorType != SENSOR_DEXCOM) continue;
        if (person.dexcomUsername == "" || person.dexcomPassword == "") continue;

        // Default polling interval: 5 min 15 s (Dexcom updates every 5 min + safety margin)
        person.recurMillis = 315000;

        // Skip if we already have recent data
        if (person.ageSeconds < 315 && person.lastGlyUnixTime > 0) continue;

        // Adaptive retry intervals
        if (person.ageSeconds > 500) {
            person.recurMillis = 90000; // 1.5 min if very stale (server might be down)
        } else if (person.ageSeconds > 315) {
            person.recurMillis = 30000; // 30 s if slightly overdue
        }

        // Is it time to poll this person?
        bool firstPoll = (person.lastDemandeMillis == 0);
        bool intervalElapsed = (millis() - person.lastReceptionMillis > person.recurMillis);

        if (firstPoll || intervalElapsed) {
            Serial.println("Polling Dexcom for: " + person.dexcomUsername);
            person.lastDemandeMillis = millis();

            if (loginDexcomShare(person)) {
                getDexcomReadings(person);
            }

            person.lastReceptionMillis = millis();
            lastAnyPollMillis = millis();

            // Only poll one person per call (to enforce stagger naturally)
            return;
        }
    }
}
```

### Points importants

- **`Person& person = persons[i];`** : on prend une référence, pas une copie. Toutes les modifs (`person.glucoseMgDl = ...`) écrivent directement dans `persons[i]`.
- **`return;` en fin de boucle** quand on a polled quelqu'un : assure qu'on ne fait qu'**un seul poll par appel** de `LectureDexcom()`. Le main loop appelle `LectureDexcom()` à chaque tour, donc à l'appel suivant on traitera la prochaine personne (si elle est due).
- **`lastAnyPollMillis`** : statique à la fonction, garde la trace du dernier poll global. Garantit l'écart de 20 s entre 2 polls quelconque.
- **Filtres en cascade** (`!configured`, `!= SENSOR_DEXCOM`, credentials vides) : robuste face aux personnes mal initialisées.

---

## Section D-bis — Adapter l'appelant dans `pageCompte.cpp`

📄 **Édite** : `src/Ecran/pageCompte.cpp`

Le bouton "Tester la connexion" appelle `loginDexcomShare()` sans argument. Comme on a changé la signature, il faut passer `persons[0]` (cette page n'édite que la personne 0 pour l'instant — l'étape 5 ajoutera la sélection de personne) :

```diff
-            loginSuccess = loginDexcomShare();
+            loginSuccess = loginDexcomShare(persons[0]);
```

---

## Section E — Refactor `clearDexcomCache()`

📄 **Édite** : `src/Dexcom.cpp`

Remplace par :

```cpp
void clearDexcomCache()
{
    Serial.println("Clearing Dexcom cache for all persons...");
    for (int i = 0; i < MAX_PERSONS; i++) {
        persons[i].dexcomSessionId = "";
        persons[i].dexcomAccountId = "";
    }
    dexcomBaseURL = "https://shareous1.dexcom.com";
}
```

---

## Section F — Scaffolding temporaire pour tester les 3 personnes

⚠️ **Ce code est TEMPORAIRE** — il sera supprimé/remplacé à l'étape 5 quand on aura l'UI tactile pour configurer les personnes 2 et 3.

📄 **Édite** : `src/main.cpp`

Dans `setup()`, **juste après** `ReadFichierParametres()`, ajoute le bloc suivant :

```cpp
// =========== TEMP TEST SCAFFOLDING — REMOVE BEFORE COMMIT (étape 5 will add UI) =========
// Hardcoded credentials for persons[1] and persons[2] to validate multi-person polling.
persons[1].name = "Tom";
persons[1].dexcomUsername = "tom@example.com";   // ← replace with real
persons[1].dexcomPassword = "RealPasswordHere";  // ← replace with real
persons[1].sensorType = SENSOR_DEXCOM;
persons[1].configured = true;

persons[2].name = "Marc";
persons[2].dexcomUsername = "marc@example.com";  // ← replace with real
persons[2].dexcomPassword = "RealPasswordHere";  // ← replace with real
persons[2].sensorType = SENSOR_DEXCOM;
persons[2].configured = true;

activePersonsCount = 3;
// =========== END TEMP TEST SCAFFOLDING ====================================================
```

### ⚠️ Sécurité

Tu vas mettre des mots de passe en clair dans `main.cpp`. **Ne commit pas ce fichier dans cet état.** Quelques options :

1. **Le plus simple** : tu testes, tu vires le bloc, puis tu commit. (Recommandé pour cette étape.)
2. **Si tu veux être paranoïaque** : tu peux mettre `main.cpp` temporairement dans `.gitignore` le temps des tests (mais attention de le re-tracker après !).

Pour cette étape, **option 1**. Tu valides → tu vires les 14 lignes → tu commit le reste.

---

## Section G — Build, flash, valider

### G.1 — Build

Click sur **Build** dans PlatformIO. Doit compiler sans erreur.

⚠️ Si tu as un `Dexcom.h` qui déclare les prototypes (`loginDexcomShare()`, `getDexcomReadings()`), n'oublie pas de mettre à jour les signatures aussi sinon tu auras une erreur de linkage.

### G.2 — Brancher le moniteur série + Flash

Lance le moniteur série **avant** le flash pour voir tout le boot.

### G.3 — Observer le polling multi-personnes

Au boot, tu dois voir dans le moniteur :

```
Active persons: 3
...
Polling Dexcom for: lea@example.com
Account ID obtenu: ...
Session ID: ...
Glycémie: 142 mg/dL
...
(20s plus tard)
Polling Dexcom for: tom@example.com
Account ID obtenu: ...
Session ID: ...
Glycémie: 98 mg/dL
...
(20s plus tard)
Polling Dexcom for: marc@example.com
Account ID obtenu: ...
Session ID: ...
Glycémie: 185 mg/dL
```

Les 3 polls se font à 20 s d'écart. ✅

### G.4 — Vérifier l'état en mémoire

Va sur `http://gluco-monitor.local/Brute`. Tu verras **seulement la dernière personne** pollée (les buffers debug sont encore globaux, à corriger en étape 7). C'est OK.

Pour vérifier les 3 glycémies, l'idéal est d'ajouter un `Serial.println` temporaire dans la loop ou dans `LectureDexcom` qui affiche les 3 valeurs. Exemple à coller dans `setup()` ou en debug temporaire :

```cpp
// Temporary debug — print all 3 persons' state every 30s
static unsigned long lastDebugMs = 0;
if (millis() - lastDebugMs > 30000) {
    lastDebugMs = millis();
    for (int i = 0; i < MAX_PERSONS; i++) {
        Serial.printf("persons[%d]: name=%s gluc=%d age=%lds\n",
            i, persons[i].name.c_str(),
            persons[i].glucoseMgDl, persons[i].ageSeconds);
    }
}
```

Tu dois voir progressivement les 3 valeurs se remplir.

### G.5 — Note : l'écran continue d'afficher uniquement persons[0]

C'est attendu. L'étape 4 réécrira la page d'accueil pour montrer les 3.

---

## ✅ Critère de sortie

- [ ] Build sans erreur
- [ ] 3 polls Dexcom observés dans le moniteur série, à ~20 s d'écart
- [ ] Les 3 glycémies sont récupérées et stockées dans `persons[0..2].glucoseMgDl`
- [ ] Pas de reboot intempestif
- [ ] **Scaffolding temporaire supprimé de `main.cpp`** avant commit
- [ ] Glycémie de `persons[0]` toujours affichée à l'écran comme avant

### Avant le commit

Vire bien le bloc `TEMP TEST SCAFFOLDING` ! Si tu commit avec, les credentials de Tom et ton mari finissent dans l'historique git pour toujours. Pas grave si le repo est privé, plus emmerdant s'il est public.

Une fois le scaffolding viré, tu commit. Suggestion de message :

```
Acquisition multi-personnes: loop Dexcom polling over persons[]

- Move dexcomSessionId/dexcomAccountId into Person struct
- loginDexcomShare/getDexcomReadings now take Person&
- LectureDexcom loops over MAX_PERSONS, polls one per call,
  with 20s global stagger
- Adaptive retry intervals (315s normal, 30s if late, 90s if very stale)

Display still single-person (persons[0]) — multi-person UI in étape 4.

PRD §10 étape 3.
```

---

## 🔄 Quand tu auras fini

Pingue-moi avec :
- Confirmation que les 3 polls se déroulent bien à 20 s d'écart (extrait du log série)
- Les 3 glycémies récupérées (extrait du log de debug)
- Le scaffolding viré et le commit prêt

Ensuite : **étape 4** (nouvelle page d'accueil avec les 3 zones empilées) — c'est l'étape la plus "visuelle", tu verras enfin les 3 glycémies sur ton écran ! 🎯
