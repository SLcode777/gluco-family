# Guide — Étape 5a : Clavier tactile en portrait

> **Référence PRD** : §10 étape 5 (sous-partie layout)
> **Objectif** : rendre le clavier tactile utilisable en portrait. Actuellement il est dimensionné pour le paysage (largeur ~473px) et déborde largement d'un écran de 320px — les touches de droite sont hors écran, donc impossible de saisir certains caractères.
> **Fichier touché** : `src/Ecran/pageClavier.cpp` uniquement.

---

## 📚 Ce qu'on construit

Le clavier dessine une grille de 10 colonnes. Les largeurs sont codées en dur pour le paysage :
- `KEY_W 44` → 10 touches × 44px + espaces ≈ **473px** de large
- Zone de texte : `fillRect(20, 50, 440, 40)` → **460px** de large

Sur un écran portrait de **320px**, tout ce qui dépasse 320 est invisible/intouchable. Résultat : les ~3 dernières colonnes de touches (dont des lettres et le bouton OK) sont hors écran.

**Le fix** : rendre les largeurs **proportionnelles à `EcranW`** au lieu de valeurs en dur. Ainsi le clavier s'adapte automatiquement, que l'écran soit en portrait (320) ou paysage (480).

### Pourquoi c'est un prérequis bloquant

L'étape 5b (config multi-personnes) demande de **saisir** les prénoms et credentials de Tom et ton mari au clavier tactile. Si le clavier est cassé, impossible de tester. Donc on le répare d'abord.

---

## 🎯 Scope

### ✅ Dans cette étape
- Rendre la grille de touches adaptative à `EcranW`
- Rendre la zone de texte adaptative à `EcranW`
- Améliorer un peu la hauteur des touches (on a de la place verticale en portrait)

### ❌ Hors scope
- Ré-agencement du menu config et de `pageCompte` (étape 5b)
- Logique multi-personnes (étape 5b)

---

## ⚠️ Pré-requis
- ✅ Étape 4 mergée (page d'accueil portrait OK)
- ⚠️ **Branche dédiée** : `git checkout -b feat/clavier-portrait`

---

## Section A — Rendre la zone de texte adaptative

📄 **Édite** : `src/Ecran/pageClavier.cpp`, fonction `drawTextBox()` (lignes ~49-50)

```diff
-  CanvaBase->fillRect(20, 50, 440, 40, RGB565_LIGHTGREY);
-  CanvaBase->drawRect(20, 50, 440, 40, RGB565_BLACK);
+  CanvaBase->fillRect(20, 50, EcranW - 40, 40, RGB565_LIGHTGREY);
+  CanvaBase->drawRect(20, 50, EcranW - 40, 40, RGB565_BLACK);
```

La zone de texte fait maintenant `EcranW - 40` de large (20px de marge de chaque côté), peu importe l'orientation.

---

## Section B — Rendre la grille de touches adaptative

📄 **Édite** : `src/Ecran/pageClavier.cpp`, fonction `Position()` (lignes ~238-248)

Remplace **toute** la fonction par :

```cpp
void Position(int row, int col, int &x, int &y, int &keyWidth, int &keyHeight)
{
  const int margin = 4;
  keyHeight = KEY_H;

  if (row < 3) {
    // First 3 rows: 10 keys spread across the full width
    keyWidth = (EcranW - 2 * margin - 9 * KEY_SPACING) / 10;
  } else {
    // Last row: 6 wider keys (SHIFT, SPACE, DEL, 123/ABC, Cancel, OK)
    keyWidth = (EcranW - 2 * margin - 5 * KEY_SPACING) / 6;
  }

  x = margin + col * (keyWidth + KEY_SPACING);
  y = row * (KEY_H + KEY_SPACING) + START_Y;
}
```

### Comment ça marche

- **Rangées 0-2** (les lettres/chiffres) : 10 touches. La largeur de chaque touche = `(EcranW - marges - espaces) / 10`. En portrait : `(320 - 8 - 18) / 10 = 29px`.
- **Rangée 3** (les fonctions) : seulement 6 touches utiles. On les fait plus larges : `(320 - 8 - 10) / 6 = 50px` → assez pour afficher "Cancel", "SHIFT", etc.
- **`x = margin + col * (keyWidth + spacing)`** : positionne chaque touche en partant de la marge gauche.

Comme `Position()` est utilisée **à la fois** pour dessiner (`drawKey`) ET pour détecter les taps (`handleTouch_clavier`), les deux restent automatiquement cohérents. Pas besoin de toucher ailleurs.

⚠️ **Important** : on a supprimé l'ancien `keyWidth += 31` (qui élargissait la dernière rangée en dur pour le paysage). C'est maintenant géré par la branche `else`.

---

## Section C — (Optionnel) Touches plus hautes pour le portrait

En portrait on a beaucoup de place verticale (480px). On peut rendre les touches plus hautes pour faciliter le tap (les touches font 29px de large, autant qu'elles soient confortables en hauteur).

📄 **Édite** : `src/Ecran/pageClavier.cpp`, les `#define` en haut (lignes ~12-15)

```diff
 #define KEY_W 44
-#define KEY_H 44
+#define KEY_H 50
 #define KEY_SPACING 3
-#define START_Y 110
+#define START_Y 130
```

(`KEY_W` n'est plus utilisé par `Position()` après la section B, mais laisse-le, il ne gêne pas.)

Tu peux ajuster `KEY_H` et `START_Y` à ton goût après le premier flash.

---

## Section D — Build, flash, test

### D.1 — Build + flash

Clique Build puis Upload.

### D.2 — Tester le clavier

Pour atteindre le clavier : **Configuration → WiFi** (ou Compte) → tape sur un champ qui ouvre le clavier (ex. le mot de passe WiFi).

Vérifie :
- ✅ Les 10 colonnes de touches sont **toutes visibles** dans la largeur de l'écran
- ✅ La rangée du bas (SHIFT, SPACE, DEL, 123, Cancel, OK) est entièrement visible
- ✅ La zone de texte ne déborde pas à droite
- ✅ Taper une touche écrit bien le bon caractère (le tap est aligné avec le visuel)
- ✅ Le bouton **OK** est accessible et fonctionne

### D.3 — Test de saisie complet

Tape quelques caractères, teste SHIFT (majuscule/minuscule), 123 (chiffres), DEL, puis OK. Tout doit être accessible et aligné.

⚠️ **Ne valide pas un vrai mot de passe WiFi par erreur** (ça redémarre la carte). Pour tester sans risque, utilise un champ comme le username Dexcom, ou tape Cancel à la fin.

---

## ✅ Critère de sortie

- [ ] Build OK
- [ ] Toutes les touches visibles et alignées en portrait
- [ ] Le tap correspond visuellement à la touche pressée
- [ ] OK / Cancel accessibles

Message de commit suggéré :

```
Make on-screen keyboard adaptive (portrait fix)

Key grid and text box now sized from EcranW instead of
hardcoded landscape widths. Keyboard usable in portrait (320px).

PRD §10 étape 5 (layout).
```

---

## 🔄 Quand tu auras fini

Pingue-moi et je rédige le **guide 5b** (config multi-personnes) — il s'appuiera sur le clavier réparé pour permettre la saisie des prénoms + credentials de chaque personne via l'écran. C'est la dernière grosse étape avant que ton device soit pleinement autonome. 💪
