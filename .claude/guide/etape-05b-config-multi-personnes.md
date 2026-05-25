# Guide — Étape 5b : Configuration multi-personnes

> **Référence PRD** : §10 étape 5
> **Objectif** : permettre de configurer **chacune des 3 personnes** via l'écran tactile — prénom + identifiants Dexcom — sans recompiler. Fini le scaffolding hardcodé.
> **Fichiers touchés** : `Config.h`, `Config.cpp`, `Ecran/Gestion.h`, `Ecran/Gestion.cpp`, `Ecran/pageConfiguration.cpp`, `Ecran/pageCompte.cpp`, `Ecran/pageClavier.cpp`.

---

## 📚 Ce qu'on construit

Aujourd'hui, toutes les pages de config travaillent en dur sur `persons[0]`. On veut pouvoir éditer `persons[0]`, `persons[1]` **et** `persons[2]`.

### Le mécanisme central : `configPersonIndex`

Plutôt que de dupliquer toutes les pages ×3, on introduit **une seule variable globale** `configPersonIndex` (0, 1 ou 2) qui dit "quelle personne suis-je en train de configurer". Les pages config liront/écriront `persons[configPersonIndex]` au lieu de `persons[0]`.

Le flux :
```
Menu Config → tap "Personne 2" → configPersonIndex = 1 → pageCompte
   → pageCompte édite persons[1]
   → clavier écrit dans persons[1].dexcomUsername, etc.
```

C'est élégant : une variable au lieu d'un enum explosé (`pageClavier_DexcomUsername_P0/P1/P2`...).

### Le champ prénom

On ajoute un champ "Prénom" sur `pageCompte`, qui ouvre le clavier pour saisir `persons[i].name`. Une fois saisi, il s'affiche sur la zone d'accueil (fini le "Person 1").

---

## 🎯 Scope

### ✅ Dans cette étape
- Global `configPersonIndex`
- Menu config : 3 entrées "Personne 1/2/3" + ré-agencement portrait (colonne unique)
- `pageCompte` : travaille sur `persons[configPersonIndex]` + champ prénom + titre indiquant la personne
- Clavier : écrit dans `persons[configPersonIndex]` + nouvelle cible "prénom"
- Suppression définitive du scaffolding de test

### ❌ Hors scope
- Ré-agencement portrait des autres pages annexes (`pageAffichage`, `pageInfos`…) → étape 7 polish si besoin
- Page web multi-personnes → étape 7

---

## ⚠️ Pré-requis
- ✅ Étape 5a mergée (clavier portrait fonctionnel)
- ✅ Les 3 credentials Dexcom sous la main
- ⚠️ **Branche dédiée** : `git checkout -b feat/config-multi-personnes`

---

## Section A — Global `configPersonIndex`

📄 **`src/Config.h`** — déclare la globale (près de `activePersonsCount`) :

```cpp
extern int configPersonIndex;   // 0..2 : person currently being edited in config pages
```

📄 **`src/Config.cpp`** — définis-la :

```cpp
int configPersonIndex = 0;
```

---

## Section B — Nouvelle page clavier "prénom"

📄 **`src/Ecran/Gestion.h`** — ajoute l'ID de page après `pageClavier_DexcomPwd` :

```diff
 #define pageClavier_DexcomUsername 24
 #define pageClavier_DexcomPwd 25
+#define pageClavier_PersonName 26
```

📄 **`src/Ecran/Gestion.cpp`** — ajoute la page au dispatch tactile (le `switch(PageActu)` vers la ligne 166, dans le groupe des claviers) :

```diff
           case pageClavier_DexcomUsername:
           case pageClavier_DexcomPwd:
+          case pageClavier_PersonName:
             handleTouch_clavier(touchX, touchY);
             break;
```

(Sans ça, taper sur le clavier prénom ne ferait rien.)

---

## Section C — Menu config : 3 entrées personnes + layout portrait

📄 **`src/Ecran/pageConfiguration.cpp`**

### C.1 — Agrandir le tableau de boutons (8 → 10)

Remplace la déclaration `static Bouton Boutons[8] = {...}` par :

```cpp
static Bouton Boutons[10] = {
    {0, 0, 0, 0, "WiFi"},
    {0, 0, 0, 0, "Personne 1"},
    {0, 0, 0, 0, "Personne 2"},
    {0, 0, 0, 0, "Personne 3"},
    {0, 0, 0, 0, "Affichage"},
    {0, 0, 0, 0, "Informations"},
    {0, 0, 0, 0, "Langue"},
    {0, 0, 0, 0, "Fuseau Horaire"},
    {0, 0, 0, 0, "About"},
    {0, 0, 0, 0, "Restart"}};
```

(Les coordonnées sont calculées dans `ParaInit()`, donc on met 0 ici.)

### C.2 — Réécrire `ParaInit()`

Remplace **toute** la fonction `ParaInit()` par :

```cpp
void ParaInit()
{
  String Titre = T("ParaConfig");
  CanvaConfig->fillScreen(C_grisFonce);
  CanvaConfig->setFont(u8g2_font_helvB18_tf);
  CanvaConfig->setTextColor(RGB565_WHITE);
  PrintCentre(CanvaConfig, Titre, EcranW / 2, 30, 1);
  CanvaConfig->fillRoundRect(7, 50, EcranW - 14, EcranH - 60, 8, RGB565_NAVY);
  CanvaConfig->drawRoundRect(7, 50, EcranW - 14, EcranH - 60, 8, RGB565_WHITE);

  // Person entries show their name if set, else "Personne N"
  for (int i = 0; i < MAX_PERSONS; i++) {
    Boutons[1 + i].Texte = (persons[i].name.length() > 0)
                               ? persons[i].name
                               : T("Person") + String(" ") + String(i + 1);
  }
  Boutons[0].Texte = "WiFi";
  Boutons[4].Texte = T("Display");
  Boutons[5].Texte = T("Infos");
  Boutons[6].Texte = T("Lang");
  Boutons[7].Texte = T("F_Hor");
  Boutons[8].Texte = T("Apropos");
  Boutons[9].Texte = T("Restart");

  // Single-column portrait layout
  const int16_t X = 25;
  const int16_t W = EcranW - 50;
  const int16_t H = 34;
  const int16_t firstY = 64;
  const int16_t step = 38;
  for (int i = 0; i < 10; i++) {
    Boutons[i].X0 = X;
    Boutons[i].Y0 = firstY + i * step;
    Boutons[i].W = W;
    Boutons[i].H = H;
    Bouton_Trace(Boutons[i], RGB565_WHITE, CanvaConfig);
  }
}
```

### C.3 — Réécrire le routage des taps

Remplace **toute** la fonction `pageConfigurationChoix()` par :

```cpp
void pageConfigurationChoix(uint16_t touchX, uint16_t touchY, int16_t DeltaTouchX, int16_t DeltaTouchY)
{
  for (int i = 0; i < 10; i++)
  {
    if (Bouton_Appui(Boutons[i], touchX, touchY, CanvaConfig))
    {
      switch (i)
      {
      case 0:
        WifiListSetup();
        break;
      case 1: // Personne 1
      case 2: // Personne 2
      case 3: // Personne 3
        configPersonIndex = i - 1;
        CompteSetup();
        break;
      case 4:
        pageAffichageSetup();
        break;
      case 5:
        pageInfosSetup();
        break;
      case 6:
        pageLangueSetup();
        break;
      case 7:
        pageFuseauSetup();
        break;
      case 8:
        pageAboutSetup();
        break;
      case 9:
        ESP.restart();
        break;
      }
      delay(100);
      return;
    }
  }
}
```

⚠️ Il te faudra une chaîne de traduction `Person` (ex. FR "Personne", EN "Person"). Si elle n'existe pas, ajoute-la dans `Langues/` ou remplace `T("Person")` par `"Personne"` en dur pour l'instant.

---

## Section D — `pageCompte` : per-person + champ prénom

📄 **`src/Ecran/pageCompte.cpp`**

### D.1 — Remplacer `persons[0]` par `persons[configPersonIndex]`

Fais un **Find/Replace dans ce fichier** (Ctrl+H, fichier courant) :
- Chercher : `persons[0]`
- Remplacer : `persons[configPersonIndex]`
- Remplacer tout — **sans exception**

Toutes les occurrences doivent être remplacées, y compris la ligne du bouton Tester qui devient `loginDexcomShare(persons[configPersonIndex])` (c'est correct : on teste la connexion de la personne en cours d'édition).

### D.2 — Repurposer `Boutons[0]` pour le prénom

Le `Boutons[0]` actuel ("Sensor Type") n'est jamais utilisé comme bouton (le sélecteur de capteur dessine ses propres rectangles). On le recycle pour le champ prénom.

### D.3 — Ajouter le titre personne + le champ prénom dans `CompteSetup()`

Juste après `CanvaBase->fillScreen(C_grisFonce);` (vers ligne 30), insère ce bloc pour afficher en haut quelle personne on édite :

```cpp
    String personLabel = (persons[configPersonIndex].name.length() > 0)
                             ? persons[configPersonIndex].name
                             : String("Personne ") + String(configPersonIndex + 1);
    CanvaBase->setFont(u8g2_font_helvB14_tf);
    CanvaBase->setTextColor(RGB565_WHITE);
    PrintGauche(CanvaBase, personLabel, 8, 18, 1);
```

Puis, **juste avant** `Bouton_Trace(Boutons[4]);` (le bouton Tester, vers ligne 132), ajoute le champ prénom :

```cpp
    // Name field (uses Boutons[0]) — placed in the free space below the account fields
    drawPara(T("FirstName"), persons[configPersonIndex].name, 330, 0);
```

`330` = position verticale (Y) du champ. En portrait il reste de la place sous les champs existants (Region finit vers 280, Test vers 288 — tu ajusteras peut-être ces Y, voir D.5).

### D.4 — Gérer le tap sur le champ prénom dans `handleTouch_Compte()`

Au **début** de `handleTouch_Compte()` (juste après l'accolade ouvrante), ajoute :

```cpp
    if (Bouton_Appui(Boutons[0], touchX, touchY)) // First name
    {
        PageActu = pageClavier_PersonName;
        setup_clavier();
        return;
    }
```

### D.5 — Ajustements de position (à faire au flash)

Les champs existants (Username 110, Password 170, Region 230, Test 288) ont été pensés pour le paysage. En portrait tu as de la place. Selon le rendu :
- Si le champ prénom (Y=330) chevauche le bouton Tester ou sort de l'écran, ajuste son Y.
- Idéalement : remonte un peu les champs ou place le prénom au-dessus des credentials. À toi de voir au flash — c'est de l'itération visuelle.

> 💡 Suggestion d'ordre logique : Prénom **en premier** (juste sous le sélecteur de capteur), puis Username, Password, Region. Si tu veux ça, mets le `drawPara(T("FirstName")...)` avec un Y bas (~80) et décale Username/Password/Region/Test vers le bas. Mais commence simple (prénom en bas, Y=330) et affine ensuite.

---

## Section E — Clavier : per-person + cible prénom

📄 **`src/Ecran/pageClavier.cpp`**

### E.1 — Remplacer `persons[0]` par `persons[configPersonIndex]`

Find/Replace dans ce fichier : `persons[0]` → `persons[configPersonIndex]` (remplacer tout). Ça couvre les écritures Dexcom username/password et les resets de timer.

### E.2 — Charger le prénom dans `setup_clavier()`

Ajoute un cas (près des autres, vers ligne 280) :

```cpp
  if (PageActu == pageClavier_PersonName)
  {
    Titre = T("FirstName");
    textBuffer = persons[configPersonIndex].name;
  }
```

### E.3 — Écrire le prénom à la validation (OK) dans `handleTouch_clavier()`

Ajoute un cas dans le bloc `if (key == "OK")` (près des autres `if (PageActu == ...)`) :

```cpp
          if (PageActu == pageClavier_PersonName)
          {
            persons[configPersonIndex].name = textBuffer;
            RecordFichierParametres();
            CompteSetup();
            return;
          }
```

### E.4 — Gérer Cancel pour la page prénom

Dans le bloc `if (key == "Cancel")`, ajoute `pageClavier_PersonName` au groupe qui retourne vers `CompteSetup()` :

```diff
           if (PageActu == pageClavier_CompteEmail || PageActu == pageClavier_ComptePwd ||
-              PageActu == pageClavier_DexcomUsername || PageActu == pageClavier_DexcomPwd)
+              PageActu == pageClavier_DexcomUsername || PageActu == pageClavier_DexcomPwd ||
+              PageActu == pageClavier_PersonName)
           {
             CompteSetup();
             return;
           }
```

### E.5 — (Optionnel) Curseur clignotant sur la page prénom

Dans `Gestion.cpp`, le `switch` d'affichage (vers ligne 253) n'appelle `loop_touch_clavier()` que pour certaines pages clavier. Pour que le curseur clignote sur la page prénom, tu peux ajouter `case pageClavier_PersonName: loop_touch_clavier(); break;`. Pas critique (la saisie marche sans).

---

## Section F — Chaînes de traduction

Ajoute deux clés dans `src/Langues/` (au minimum FR et EN), ou hardcode temporairement :
- `Person` → FR "Personne", EN "Person"
- `FirstName` → FR "Prénom", EN "First name"

Si tu veux aller vite : remplace `T("Person")` et `T("FirstName")` par les chaînes en dur dans le code, et on les internationalisera en étape 7.

---

## Section G — Supprimer le scaffolding de test

📄 **`src/main.cpp`** — si tu avais re-collé le bloc `TEMP TEST SCAFFOLDING` pour tester les étapes précédentes, **supprime-le définitivement**. Désormais les 3 personnes se configurent via l'écran.

---

## Section H — Build, flash, test

### H.1 — Build

Le compilateur t'aidera à attraper un `persons[0]` oublié ou une référence manquante.

### H.2 — Test du flux complet

1. **Configuration → Personne 1**
2. Tape le champ **Prénom** → clavier → saisis "Léa" → OK
3. Tape **Username** → saisis l'identifiant Dexcom → OK
4. Tape **Password** → saisis le mot de passe → OK
5. **Tester la connexion** → doit afficher "OK"
6. Reviens au menu (swipe), refais pour **Personne 2** (Tom) et **Personne 3** (ton mari)
7. Retourne sur l'accueil : les 3 prénoms s'affichent, les 3 glycémies arrivent

### H.3 — Vérifier la persistance

Reboot la carte. Les 3 personnes (prénoms + credentials) doivent être rechargées depuis `parametres.json` (format v2 multi-personnes de l'étape 2). Plus besoin de scaffolding !

---

## ✅ Critère de sortie

- [ ] Build OK
- [ ] Menu config affiche 3 entrées personnes (avec prénoms si saisis)
- [ ] Chaque personne configurable indépendamment (prénom + credentials)
- [ ] Test connexion OK pour les 3
- [ ] Prénoms affichés sur l'accueil
- [ ] Persistance après reboot
- [ ] Scaffolding supprimé de `main.cpp`

Message de commit suggéré :

```
Add per-person touch configuration

- Introduce global configPersonIndex (0..2) to select the person
  being edited in config pages
- Config menu: 3 person entries (showing names) + single-column
  portrait layout
- pageCompte / keyboard now operate on persons[configPersonIndex]
- New keyboard target for first-name entry (pageClavier_PersonName)
- Remove hardcoded test scaffolding; all 3 persons configured via UI

PRD §10 étape 5.
```

---

## 🔄 Quand tu auras fini

Pingue-moi avec le résultat (idéalement une photo du menu config + d'une page personne). Ton device sera alors **pleinement autonome** : ajout/modif d'une personne sans recompiler.

Restera ensuite :
- **Étape 6** : comportements globaux (reboot seulement si les 3 sont muettes, etc.)
- **Étape 7** : polish (pages web, mDNS `gluco-family.local`, README, nettoyage migration v1, traductions)

Bravo, c'est la dernière grosse étape fonctionnelle ! 🎉
