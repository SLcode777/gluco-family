# Guide — Étape 4 : Nouvelle page d'accueil (3 zones empilées)

> **Référence PRD** : §5.2 (layout détaillé) + §10 étape 4
> **Objectif** : remplacer la page d'accueil actuelle (1 personne, jauge en arc, courbe 24h) par 3 zones empilées affichant chacune une personne, avec jauge verticale d'âge.
> **Fichiers touchés** : `Ecran/pageAccueil.cpp` (réécrit), `Config.h`/`.cpp` (cleanup), `Dexcom.cpp` (cleanup).

---

## 📚 Ce qu'on construit

### Avant

```
┌─────────────────────────────────┐
│              21:14              │  ← Heure
│                                 │
│       ╱─────────╲               │  ← Jauge en arc (bleu/vert/orange/rouge)
│      ╱   142     ╲              │
│      │  mg/dL    │ ↗            │  ← Glycémie + flèche
│       ╲_________╱               │
│                                 │
│      [Courbe historique 24h]    │  ← Graphique
│                                 │
└─────────────────────────────────┘
   ▓▓▓▓▓▓▓▓▓▓▓▓▓░░░░░░░░░░░░░░░    ← Barre progression
```

### Après

```
┌─────────────────────────────────┐
│  Léa                ↗         ┃ │
│   142  mg/dL                 ┃ │  ← Zone Léa
├─────────────────────────────────┤
│  Tom                →         ┃ │
│    98  mg/dL                 ┃ │  ← Zone Tom
├─────────────────────────────────┤
│  Marc               ↘         ┃ │
│   185  mg/dL                 ┃ │  ← Zone Marc
└─────────────────────────────────┘
```

3 zones de hauteur égale (~160 px chacune), chacune avec :
- **Prénom** en haut à gauche
- **Flèche** de tendance en haut à droite (logique de dessin existante réutilisée)
- **Valeur** centrée, en grande police, potentiellement colorée selon la zone glycémique
- **Unité** (`mg/dL`) discrète à côté de la valeur
- **Jauge verticale** (fine ligne) sur le bord droit, qui se remplit en 5 min puis change de couleur selon l'ancienneté

### Pourquoi cette étape est si différente des précédentes

Étapes 1-3 : **mécanique** (renommer, refactorer). Étape 4 : **design + dessin pixel**. Il faut penser layout, choisir des polices, calculer des coordonnées. C'est moins automatisable, mais une fois posé, c'est très visuel et satisfaisant.

---

## 🎯 Scope précis

### ✅ Dans cette étape

- Réécriture complète de `AccueiLoop()` dans `pageAccueil.cpp`
- Extraction de `drawArrow(canva, x, y, trend, color)` réutilisable (basée sur le code existant)
- Nouveau helper `drawAgeGauge(canva, x, yTop, yBottom, ageSeconds, hasMeasure)` pour la jauge verticale
- Nouveau helper `glucoseColorForValue(int mgdl, int targetLow, int targetHigh)` pour calculer la couleur
- **Cleanup** : suppression de `Trace_Gauge()`, du code de courbe 24h, des tableaux `glucoseValues[]`/`glucoseHeure[]`/`pointCountGly` dans `Config.h`/`.cpp` et leur alimentation dans `Dexcom.cpp` et `Libreview.cpp`
- Test avec les 3 personnes (re-mettre le scaffolding temporaire de l'étape 3 dans `main.cpp` si tu l'as viré)

### ❌ Hors scope

- UI tactile pour configurer les noms et credentials per-person (étape 5)
- Buffers debug multi-personnes (`/Brute`) — étape 7
- Comportement "alerte" / reboot si tous silencieux — étape 6
- Internationalisation des nouvelles chaînes — étape 7

---

## ⚠️ Pré-requis

- ✅ Étape 3 mergée sur `main` (3 personnes pollées en interne, validé)
- ✅ Tu as les 3 credentials Dexcom (pour scaffolding de test)
- ⚠️ **Branche dédiée** : `git checkout -b feat/page-accueil-3-zones`
- ⚠️ Si tu as viré le scaffolding `persons[1]`/`persons[2]` après étape 3, **remets-le temporairement** dans `setup()` de `main.cpp` (cf. guide étape 3 section F). Tu le re-vireras à la fin de l'étape 4.

---

## Section A — Réécrire `pageAccueil.cpp` (complet)

📄 **Édite** : `src/Ecran/pageAccueil.cpp`

**Remplace TOUT le contenu du fichier** par ce qui suit. C'est plus simple que de diffs ligne par ligne vu qu'on jette 90 % de l'ancien code.

```cpp
#include "Ecran/pageAccueil.h"
#include "Config.h"
#include <Arduino.h>
#include "Ecran/Gestion.h"
#include "time.h"
#include "Langues/Langue.h"

// Layout constants
#define ZONE_COUNT       MAX_PERSONS
#define GAUGE_WIDTH_PX   5
#define GAUGE_MARGIN_PX  3   // gap between gauge and right edge

// Color thresholds (mg/dL) — same as legacy
static const int16_t HYPER_CRITICAL = 300;

// ---------- Helpers ----------

// Returns the glucose value's display color based on the person's targets.
// White if not configured to use colors, otherwise blue/green/orange/red.
static uint16_t glucoseColorForValue(int16_t mgdl, int16_t targetLow, int16_t targetHigh) {
    if (glucoseColor != GLUCOSE_COULEUR) return RGB565_WHITE;
    if (mgdl < targetLow)              return RGB565_BLUE;
    if (mgdl < targetHigh)             return RGB565_GREEN;
    if (mgdl < HYPER_CRITICAL)         return RGB565_ORANGE;
    return RGB565_RED;
}

// Draws the Dexcom trend arrow centered at (cx, cy). Scale ~half of legacy version
// so it fits in the smaller per-person zone. Trend codes:
//   -1 DoubleDown, 1 SingleDown, 2 FortyFiveDown, 3 Flat,
//    4 FortyFiveUp, 5 SingleUp, 6 DoubleUp, 0 unknown (no draw)
static void drawArrow(Arduino_Canvas *canva, int16_t cx, int16_t cy, int8_t trend, uint16_t color) {
    if (trend == 0) return; // unknown trend → no arrow

    // Coordinates relative to (cx, cy), scaled down from the legacy version
    int16_t x0, y0, x1, y1, x2, y2, x3, y3, x4, y4;
    const int16_t offset = 22; // spacing between double arrows

    switch (trend) {
        case -1: // DoubleDown
        case 1:  // SingleDown
            x0 = -11; y0 = 0;  x1 = 0;  y1 = 11;  x2 = 11; y2 = 0;
            x3 = -5;  y3 = -27; x4 = 5; y4 = -27;
            break;
        case 2:  // FortyFiveDown
            x0 = 0;  y0 = 11; x1 = 11; y1 = 11; x2 = 11; y2 = 0;
            x3 = -16; y3 = -22; x4 = -22; y4 = -16;
            break;
        case 3:  // Flat
            x0 = 0;  y0 = 11; x1 = 11; y1 = 0; x2 = 0; y2 = -11;
            x3 = -27; y3 = -5; x4 = -27; y4 = 5;
            break;
        case 4:  // FortyFiveUp
            x0 = 11; y0 = 0;  x1 = 11; y1 = -11; x2 = 0; y2 = -11;
            x3 = -16; y3 = 22; x4 = -22; y4 = 16;
            break;
        case 5:  // SingleUp
        case 6:  // DoubleUp
            x0 = 11; y0 = 0;  x1 = 0;  y1 = -11; x2 = -11; y2 = 0;
            x3 = -5;  y3 = 27; x4 = 5;  y4 = 27;
            break;
        default:
            return;
    }

    canva->fillTriangle(cx + x0, cy + y0, cx + x1, cy + y1, cx + x2, cy + y2, color);
    canva->fillTriangle(cx + x3, cy + y3, cx + x1, cy + y1, cx + x4, cy + y4, color);

    // Second arrow for DoubleDown / DoubleUp
    if (trend == -1 || trend == 6) {
        int16_t dx = (trend == -1) ? -offset : -offset;
        canva->fillTriangle(cx + x0 + dx, cy + y0, cx + x1 + dx, cy + y1, cx + x2 + dx, cy + y2, color);
        canva->fillTriangle(cx + x3 + dx, cy + y3, cx + x1 + dx, cy + y1, cx + x4 + dx, cy + y4, color);
    }
}

// Draws the vertical age gauge on the right edge of the zone.
// Fill cycle: 0 → 5 min the bar fills (white). After 5 min it stays full
// and changes color: orange (5-15 min), red (>15 min or no measure).
static void drawAgeGauge(Arduino_Canvas *canva, int16_t xLeft, int16_t yTop, int16_t yBottom,
                        long ageSeconds, bool hasMeasure) {
    const int16_t zoneHeight = yBottom - yTop;
    const int16_t FILL_PERIOD_S = 300;   // 5 min
    const int16_t ORANGE_THRESHOLD_S = 300;
    const int16_t RED_THRESHOLD_S = 900; // 15 min

    int16_t fillHeight;
    uint16_t color;

    if (!hasMeasure) {
        // No measure yet → full red gauge
        fillHeight = zoneHeight;
        color = RGB565_RED;
    } else if (ageSeconds < FILL_PERIOD_S) {
        // Normal cycle: filling, white
        fillHeight = (int16_t)((long)zoneHeight * ageSeconds / FILL_PERIOD_S);
        color = RGB565_WHITE;
    } else if (ageSeconds < RED_THRESHOLD_S) {
        // Refresh late: full, orange
        fillHeight = zoneHeight;
        color = RGB565_ORANGE;
    } else {
        // Critical: full, red
        fillHeight = zoneHeight;
        color = RGB565_RED;
    }

    // Draw gauge background (dark grey thin line, full height)
    canva->fillRect(xLeft, yTop, GAUGE_WIDTH_PX, zoneHeight, C_grisFonce);
    // Draw filled portion from bottom up
    if (fillHeight > 0) {
        canva->fillRect(xLeft, yBottom - fillHeight, GAUGE_WIDTH_PX, fillHeight, color);
    }
}

// Draws one person's zone (or empty placeholder if not configured).
static void drawPersonZone(Arduino_Canvas *canva, int zoneIndex) {
    const int16_t zoneHeight = EcranH / ZONE_COUNT;
    const int16_t yTop = zoneIndex * zoneHeight;
    const int16_t yBottom = yTop + zoneHeight;
    const int16_t yCenter = yTop + zoneHeight / 2;

    // Separator line (skip for the first zone)
    if (zoneIndex > 0) {
        canva->drawFastHLine(0, yTop, EcranW, C_grisFonce);
    }

    Person& p = persons[zoneIndex];
    const int16_t gaugeX = EcranW - GAUGE_MARGIN_PX - GAUGE_WIDTH_PX;

    if (!p.configured) {
        // Empty zone: just the separator and a very subtle placeholder
        canva->setFont(u8g2_font_helvR10_tf);
        canva->setTextColor(C_grisMoyen);
        PrintCentre(canva, T("PersonEmpty"), EcranW / 2, yCenter, 1);
        return;
    }

    // ---- Name (top-left) ----
    canva->setFont(u8g2_font_helvB18_tf);
    canva->setTextColor(RGB565_WHITE);
    String displayName = (p.name.length() > 0) ? p.name : String("Person ") + String(zoneIndex + 1);
    PrintGauche(canva, displayName, 8, yTop + 22, 1);

    // ---- Arrow (top-right area, before gauge) ----
    drawArrow(canva, gaugeX - 35, yTop + 22, p.trendArrow, RGB565_WHITE);

    // ---- Glucose value (centered) ----
    bool hasMeasure = (p.glucoseMgDl > 0);
    if (hasMeasure) {
        uint16_t valueColor = glucoseColorForValue(p.glucoseMgDl, p.targetLow, p.targetHigh);
        canva->setTextColor(valueColor);
        canva->setFont(u8g2_font_inb46_mn);
        PrintCentre(canva, formatGlucoseValue(p.glucoseMgDl), EcranW / 2 - 25, yCenter + 18, 1);

        // ---- Unit label (right of value, small) ----
        canva->setFont(u8g2_font_helvR10_tf);
        canva->setTextColor(RGB565_WHITE);
        PrintGauche(canva, getGlucoseUnitLabel(), EcranW / 2 + 50, yCenter + 8, 1);
    } else {
        // No data yet
        canva->setFont(u8g2_font_helvB18_tf);
        canva->setTextColor(C_grisMoyen);
        PrintCentre(canva, T("WaitGluco"), EcranW / 2, yCenter + 8, 1);
    }

    // ---- Age gauge (right edge) ----
    drawAgeGauge(canva, gaugeX, yTop + 2, yBottom - 2, p.ageSeconds, hasMeasure);
}

// ---------- Page lifecycle ----------

void AccueilInit() {
    // Nothing to init for now
}

void AccueiLoop() {
    CanvaAccueil->fillScreen(RGB565_BLACK);

    // Update per-person age in seconds (used by gauge and color logic)
    if (HeureValide) {
        time_t now;
        time(&now);
        for (int i = 0; i < MAX_PERSONS; i++) {
            if (persons[i].lastGlyUnixTime > 0) {
                persons[i].ageSeconds = (long)now - persons[i].lastGlyUnixTime;
            }
        }
    }

    for (int i = 0; i < ZONE_COUNT; i++) {
        drawPersonZone(CanvaAccueil, i);
    }
}
```

### Points importants à comprendre

- **`drawArrow()`** : code des flèches refactorisé — scales divisés par ~2 pour tenir dans une zone plus petite. Tu pourras ajuster les `offset`/`scale` selon ton goût après le premier flash.
- **`drawAgeGauge()`** : implémente exactement la logique du PRD §5.2 — remplissage linéaire en 5 min, puis pleine et colorée. Le calcul `(long)zoneHeight * ageSeconds / FILL_PERIOD_S` utilise `(long)` pour éviter un overflow `int16_t` sur la multiplication.
- **`drawPersonZone()`** : gère les 3 cas — non configurée, configurée mais sans mesure, configurée avec mesure.
- **`AccueiLoop()`** : très court maintenant. Recalcule juste l'âge des 3 personnes, puis dessine 3 zones. C'est tout.
- **Constantes en haut** (`ZONE_COUNT`, `GAUGE_WIDTH_PX`, etc.) : tu pourras les ajuster facilement si tu veux modifier le design sans chasser des nombres magiques dans le code.

---

## Section B — Ajouter la chaîne `PersonEmpty` aux langues

📄 **Édite** : `src/Langues/Langue.h` (ou les fichiers de chaque langue selon la structure)

Cherche où est défini `T("WaitGluco")` ou similaire, et ajoute la nouvelle clé `PersonEmpty` :

- FR : `"Non configuré"` (ou simplement `"—"`)
- EN : `"Not configured"` (ou `"—"`)
- Autres langues : ce que tu veux

Si tu préfères ne pas toucher aux langues pour l'instant, remplace dans `pageAccueil.cpp` la ligne :
```cpp
PrintCentre(canva, T("PersonEmpty"), EcranW / 2, yCenter, 1);
```
par :
```cpp
PrintCentre(canva, "—", EcranW / 2, yCenter, 1);
```
On internationalisera proprement en étape 7.

---

## Section C — Cleanup : supprimer le code de l'historique 24h

### C.1 — `src/Config.h`

Supprime ces 3 lignes :

```diff
-#define MAX_POINTS 300
-extern int16_t glucoseValues[];
-extern unsigned long glucoseHeure[];
-extern int16_t pointCountGly;
```

(Garde `MAX_PERSONS` bien sûr — c'est différent.)

### C.2 — `src/Config.cpp`

Supprime les définitions correspondantes :

```diff
-int16_t glucoseValues[MAX_POINTS];
-unsigned long glucoseHeure[MAX_POINTS];
-int16_t pointCountGly = 0;
```

Et dans `clearData()`, supprime la boucle qui les remet à zéro :

```diff
-for (int i = 0; i < MAX_POINTS; i++) {
-    glucoseValues[i] = 0;
-    glucoseHeure[i] = 0;
-}
-pointCountGly = 0;
```

### C.3 — `src/main.cpp`

Dans `setup()`, supprime la boucle d'init des tableaux :

```diff
-for (int i = 0; i < MAX_POINTS; i++) {
-    glucoseValues[i] = 0;
-    glucoseHeure[i] = 0;
-}
```

### C.4 — `src/Dexcom.cpp`

Dans `getDexcomReadings()`, supprime tout le bloc encadré par `if (&person == &persons[0])` que tu avais ajouté en étape 3 (qui remplissait les tableaux). C'est devenu du code mort.

```diff
-// Historical arrays kept only for person 0 (to be deleted in étape 4)
-if (&person == &persons[0]) {
-    pointCountGly = 0;
-    for (int i = readings.size() - 1; i > -1; i--) {
-        ...
-    }
-    Serial.println("Nombre de points Dexcom: " + String(pointCountGly));
-}
```

### C.5 — `src/Libreview.cpp` ✅ (fait par Claude)

Dans la fonction de lecture LibreView, on a supprimé le bloc qui décalait et stockait dans `glucoseValues[]`/`glucoseHeure[]` (gestion du buffer circulaire de 300 points). On garde `persons[0].lastGlyUnixTime = convertToUnix(timestamp);` et `persons[0].lastOkMillis = millis();` qui l'encadraient.

### C.6 — `src/Server.cpp` ✅ (fait par Claude)

Deux suppressions :
1. **L'endpoint `/dataGly`** (`server.on("/dataGly", ...)`) qui servait les tableaux d'historique en binaire — n'a plus de sens sans les tableaux.
2. **La déclaration `uint8_t MonBuffer[4 + MAX_POINTS * 6];`** (ligne ~29) — ce buffer ne servait qu'à `/dataGly`, et surtout il référençait `MAX_POINTS` qu'on supprime en C.1, donc il aurait cassé le build.

⚠️ Le frontend HTML (`pageMain.h`) appelle peut-être encore `/dataGly` → il aura un 404 silencieux. On nettoiera la page HTML proprement en étape 7. Ça n'empêche rien de fonctionner pour l'instant.

---

## Section D — Build & flash & valider

### D.1 — Re-mettre le scaffolding temporaire (si viré)

Si tu as commit l'étape 3 sans le scaffolding `persons[1]`/`persons[2]`, remets-le dans `setup()` de `main.cpp` (cf. guide étape 3 section F). Sans ça tu n'auras qu'1 zone remplie sur 3.

### D.2 — Build

Click **Build** dans PlatformIO. Le compilateur t'aidera à trouver les références oubliées si tu en as laissé.

### D.3 — Flash + observer

Upload et regarde l'écran. Tu dois voir :
- **3 zones empilées**, séparées par des lignes grises horizontales
- **Zone 1 (Léa)** : son prénom, sa valeur, sa flèche, sa jauge qui se remplit
- **Zone 2 (Tom)** : pareil
- **Zone 3 (Marc)** : pareil (ou "—" si tu n'as pas mis ses creds)
- Les jauges se remplissent en ~5 min, puis se vident d'un coup à la réception d'une nouvelle mesure

### D.4 — Cas de test à observer

- ✅ **Polling normal** : la jauge d'une zone est en train de monter, blanche, et se vide quand un nouveau poll arrive
- ✅ **Refresh en retard** : si tu laisses tourner > 5 min sans poll, la jauge reste pleine et passe orange
- ✅ **Personne non configurée** : si tu enlèves le scaffolding pour `persons[2]`, sa zone affiche "—" en gris

### D.5 — Ajustements typographiques

Si la valeur déborde de la zone, ou si les flèches sont trop grosses/petites, c'est normal — tu vas devoir bidouiller :
- **Taille de la valeur** : remplace `u8g2_font_inb46_mn` par `u8g2_font_inb38_mn` (plus petite) ou `u8g2_font_inb53_mn` (plus grande)
- **Position du nom/flèche** : ajuste les constantes `yTop + 22` etc.
- **Taille des flèches** : modifie les coordonnées dans `drawArrow()` proportionnellement

Itère par flash successifs jusqu'à ce que ça te plaise visuellement. C'est normal de faire 3-5 itérations design.

---

## ✅ Critère de sortie

- [ ] Build sans erreur (warnings éventuels OK)
- [ ] 3 zones visibles à l'écran
- [ ] Chaque zone affiche : nom, valeur, flèche, unité, jauge
- [ ] La jauge se remplit et se vide selon la logique du PRD §5.2
- [ ] Une zone non configurée affiche "—" ou "Non configuré"
- [ ] Le code de la courbe 24h est complètement supprimé (pas de référence à `glucoseValues[]` qui traîne)
- [ ] Scaffolding de test retiré de `main.cpp` **avant le commit**

Message de commit suggéré :

```
Nouvelle page d'accueil: 3 zones empilées avec jauge d'âge

- Réécriture complète de pageAccueil.cpp
- drawArrow() refactorisé en helper réutilisable
- Nouvelle jauge verticale d'âge (remplissage 5 min, blanc/orange/rouge)
- Cleanup : suppression de Trace_Gauge, courbe 24h, tableaux historiques
  (glucoseValues, glucoseHeure, pointCountGly) et endpoint /dataGly

L'UI tactile multi-personnes sera ajoutée en étape 5.

PRD §5.2 + §10 étape 4.
```

---

## 🔄 Quand tu auras fini

Pingue-moi avec :
- Une photo (ou description) de ton écran avec les 3 zones
- Tes ajustements typographiques éventuels (police choisie, positions tweakées)
- Le commit prêt

Ensuite : **étape 5** — les pages tactiles de configuration par personne (saisie prénom + credentials Dexcom pour Tom et ton mari via l'écran tactile, sans passer par le scaffolding hardcodé). 🎯
