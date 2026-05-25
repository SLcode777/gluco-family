# Guide — Étape 6 : Comportements globaux (reboot conditionnel)

> **Référence PRD** : §10 étape 6
> **Objectif** : ne plus redémarrer la carte dès qu'**une seule** personne n'a pas de glycémie. Le reboot ne doit se déclencher que si **toutes** les personnes configurées sont silencieuses en même temps (vrai problème réseau/système) — pas pendant un simple changement/chauffe de capteur.
> **Fichiers touchés** : `Config.h`, `Config.cpp`, `main.cpp`.

---

## 📚 Ce qu'on construit

Aujourd'hui, `main.cpp` (loop) redémarre la carte si **`persons[0]`** n'a pas de donnée :

```cpp
if (millis() - persons[0].lastOkMillis > 1210000)        // 20 min sans lecture réussie
    AlertePasdeGlycemie();                                // → reboot

if (persons[0].ageSeconds > 1800 && millis() > 300000)   // donnée > 30 min
    AlertePasdeGlycemie();                                // → reboot
```

**Le problème en conditions réelles** (vécu !) : tu changes les capteurs des enfants → 2h de chauffe sans valeur → la carte croit que "tout est cassé" et **reboote en boucle**. Alors que le système fonctionne très bien, c'est juste les capteurs qui n'émettent pas encore.

### La correction

On remplace les deux tests (basés sur `persons[0]`) par **un seul test global** : reboot **seulement si TOUTES les personnes configurées** sont silencieuses depuis 20 min. Tant qu'**au moins une** personne reçoit ses glycémies, le système est manifestement OK → pas de reboot.

Dans ton cas actuel : 2 enfants en chauffe + ton mari dont le capteur marche → le mari n'est pas silencieux → **pas de reboot**. ✅

---

## 🎯 Scope

### ✅ Dans cette étape
- Helper `allConfiguredPersonsSilent()` (ne regarde que les personnes configurées)
- Remplacer la logique de reboot de `main.cpp` par ce test global
- Calculer `ageSeconds` pour **les 3 personnes** dans la loop (pas seulement `persons[0]`)

### ❌ Hors scope
- `AlertePasdeGlycemie()` lui-même (écran + reboot générique) : inchangé
- Indicateurs visuels par zone : déjà gérés par la jauge d'âge (étape 4)
- Watchdog matériel (`esp_task_wdt`, 600 s) : inchangé, c'est un filet séparé pour les vrais blocages de boucle

---

## ⚠️ Pré-requis
- ✅ Étape 5 mergée
- ⚠️ **Branche dédiée** : `git checkout -b feat/reboot-conditionnel`

---

## Section A — Helper `allConfiguredPersonsSilent()`

📄 **`src/Config.h`** — déclare le prototype (près de `InitPersons`) :

```cpp
bool allConfiguredPersonsSilent(unsigned long timeoutMs);
```

📄 **`src/Config.cpp`** — ajoute l'implémentation (près de `InitPersons` / `clearData`) :

```cpp
// Returns true only if EVERY configured person has had no successful reading
// within timeoutMs (the whole acquisition looks stuck). Returns false as soon
// as one configured person is fresh, or if no person is configured at all.
bool allConfiguredPersonsSilent(unsigned long timeoutMs) {
  int configured = 0;
  int silent = 0;
  for (int i = 0; i < MAX_PERSONS; i++) {
    if (!persons[i].configured) continue;
    configured++;
    if (millis() - persons[i].lastOkMillis > timeoutMs) silent++;
  }
  return (configured > 0) && (silent == configured);
}
```

### Comment ça marche
- On ne compte que les personnes **configurées** (les slots vides sont ignorés).
- `lastOkMillis` = dernière **lecture réussie** d'une glycémie pour cette personne (mis à jour dans `getDexcomReadings`).
- On renvoie `true` seulement si **toutes** les personnes configurées sont silencieuses → reboot justifié.
- Si **une seule** est fraîche → `false` → pas de reboot (cas du capteur en chauffe d'un autre membre).

---

## Section B — Remplacer la logique de reboot dans `main.cpp`

📄 **`src/main.cpp`**, dans `loop()`

Remplace ce bloc (les deux tests basés sur `persons[0]`) :

```cpp
  //== Tests si fonctionnement nominal ============
  if (millis() - persons[0].lastOkMillis > 1210000) // Si on n'a pas réussi à récupérer une glycémie depuis plus de 20 minutes, on redémarre le module pour tenter de résoudre les problèmes de communication
    AlertePasdeGlycemie();

  if (HeureValide && persons[0].lastGlyUnixTime > 0)
  {

    time_t now;
    time(&now);
    persons[0].ageSeconds = (long)now - persons[0].lastGlyUnixTime;
    if (persons[0].ageSeconds > 1800 && millis() > 300000)
      AlertePasdeGlycemie(); // Pas de nouvelle mesure depuis 30mn. Exemple changement de capteur
  }
```

par :

```cpp
  //== Update each person's data age (used by display + retry timing) =====
  if (HeureValide)
  {
    time_t now;
    time(&now);
    for (int i = 0; i < MAX_PERSONS; i++)
    {
      if (persons[i].lastGlyUnixTime > 0)
        persons[i].ageSeconds = (long)now - persons[i].lastGlyUnixTime;
    }
  }

  //== Reboot ONLY if the whole acquisition is stuck =====================
  // A single sensor in warmup / being changed must not trigger a reboot.
  // We reboot only when every configured person has had no successful
  // reading for 20 min — that points to a real network/system problem.
  if (allConfiguredPersonsSilent(1210000) && millis() > 300000)
    AlertePasdeGlycemie();
```

### Pourquoi calculer `ageSeconds` pour tout le monde
`ageSeconds` sert à 2 choses : la **jauge d'âge** (page d'accueil) et la **cadence de retry** dans `LectureDexcom`. Avant, seul `persons[0].ageSeconds` était calculé dans la loop. Maintenant on le fait pour les 3, sinon les jauges/retry des personnes 1 et 2 seraient faux quand on n'est pas sur la page d'accueil.

---

## Section C — Build, flash, test

### C.1 — Build
Le compilateur signalera si `allConfiguredPersonsSilent` n'est pas bien déclaré/inclus.

### C.2 — Test du comportement
Le scénario idéal (le tien en ce moment !) :
- 1+ personne avec glycémie fraîche, 1+ personne en chauffe (pas de valeur)
- ✅ **La carte ne doit PAS redémarrer** tant qu'au moins une personne reçoit ses données

Pour vérifier sans attendre 20 min, tu peux temporairement baisser le seuil (ex. `allConfiguredPersonsSilent(60000)` = 1 min) pour tester, puis le remettre à `1210000`.

### C.3 — Test du reboot légitime (optionnel)
Si tu coupes le WiFi (ou que toutes les personnes perdent leur capteur), après 20 min la carte doit redémarrer (récupération d'un vrai blocage réseau). Pas indispensable à tester.

---

## ⚠️ Cas limite connu

Si **les 3 personnes** sont en chauffe **en même temps** (ex. tu changes les 3 capteurs le même jour), elles seront toutes silencieuses → la carte rebootera quand même toutes les ~20 min pendant la chauffe.

Pour ton usage c'est rare (rarement les 3 capteurs changés simultanément). Si ça devient gênant, deux options pour plus tard :
- **Simple** : augmenter le seuil (ex. `allConfiguredPersonsSilent(2400000)` = 40 min) pour réduire la fréquence
- **Robuste** : distinguer "pas de **valeur**" (chauffe — normal) de "pas de **communication**" (vrai bug réseau) en traçant un `lastCommMillis` mis à jour à chaque réponse HTTP OK de Dexcom, même vide. Plus de code, à faire seulement si le cas limite te pose vraiment problème.

Dis-moi si tu veux qu'on fasse la version robuste maintenant ou si la version simple suffit.

---

## ✅ Critère de sortie
- [ ] Build OK
- [ ] Avec 1 personne fraîche + autres en chauffe : **pas de reboot**
- [ ] `ageSeconds` correct pour les 3 (jauges d'âge à jour sur l'accueil)

Message de commit suggéré :

```
Reboot only when all configured persons are silent

Replace the persons[0]-based reboot checks with a global condition:
the device reboots only if every configured person had no successful
reading for 20 min (real network/system failure). A single sensor in
warmup or being changed no longer triggers spurious reboots.
Also compute ageSeconds for all persons each loop (display + retry).

PRD §10 étape 6.
```

---

## 🔄 Quand tu auras fini

Pingue-moi. Restera l'**étape 7 (polish)** : pages web multi-personnes, mDNS `gluco-family.local`, README, suppression du code migration v1→v2, et quelques retouches portrait des pages annexes (ex. le bouton de la question de config au premier boot qui fait 430px de large). Ce sera la dernière ligne droite avant un projet partageable. 🏁
