#include "Ecran/pageConfiguration.h"
#include "Config.h"
#include <Arduino.h>
#include "Ecran/Gestion.h"
#include "Ecran/pageWifiList.h"
#include "Ecran/pageClavier.h"
#include "Ecran/pageCompte.h"
#include "Ecran/pageInfos.h"
#include "Ecran/pageAffichage.h"
#include "Ecran/pageFuseauH.h"
#include "Ecran/pageLangue.h"
#include "Ecran/pageAbout.h"
#include "Langues/Langue.h"

static Bouton Boutons[10] = {
    {0, 0, 0, 0, "WiFi"},
    {0, 0, 0, 0, "Person 1"},
    {0, 0, 0, 0, "Person 2"},
    {0, 0, 0, 0, "Person 3"},
    {0, 0, 0, 0, "Display"},
    {0, 0, 0, 0, "Infos"},
    {0, 0, 0, 0, "Language"},
    {0, 0, 0, 0, "Time Zone"},
    {0, 0, 0, 0, "About"},
    {0, 0, 0, 0, "Restart"}};

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
      case 1: // Person 1
      case 2: // Person 2
      case 3: // Person 3
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