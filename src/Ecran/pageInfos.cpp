#include "Ecran/pageInfos.h"
#include "Ecran/Gestion.h"
#include <U8g2lib.h>
#include "Config.h"
#include "Internet.h"
#include <WiFi.h>
#include "Langues/Langue.h"


void pageInfosSetup()
{
    PageActu = pageInfos;
    CanvaBase->setFont(u8g2_font_helvB18_tf);
    CanvaBase->setTextColor(RGB565_WHITE);
    CanvaBase->fillScreen(C_grisFonce);
    PrintCentre(CanvaBase, T("Infos"), EcranW / 2, 30, 1);
    CanvaBase->fillRoundRect(7, 50, EcranW - 14, EcranH - 60, 8, RGB565_NAVY);
    CanvaBase->drawRoundRect(7, 50, EcranW - 14, EcranH - 60, 8, RGB565_WHITE);
    CanvaBase->setFont(u8g2_font_10x20_mf);
    CanvaBase->setTextColor(RGB565_WHITE);

    const int16_t maxR = EcranW - 13; // inner right edge of the card
    const int16_t LH = 28;            // line spacing (also wrap interline)
    int16_t y = 80;

    y = PrintGaucheWrap(CanvaBase, T("VersionSoft") + String(Version) + "  " + String(BuildDate), 20, y, maxR, LH);
    y = PrintGaucheWrap(CanvaBase, T("Auteur") + " : Stella", 20, y, maxR, LH);
    if (WiFi.status() == WL_CONNECTED)
    {
        y = PrintGaucheWrap(CanvaBase, T("AdrIP") + WiFi.localIP().toString(), 20, y, maxR, LH);
    }
    else
    {
        y = PrintGaucheWrap(CanvaBase, T("NoWiFi") + ssid, 20, y, maxR, LH);
    }
    y = PrintGaucheWrap(CanvaBase, hostname + ".local", 20, y, maxR, LH);

    uint64_t Heures = T_On_seconde / 3600;
    uint64_t Minutes = (T_On_seconde % 3600) / 60;
    char value[60];
    String Gon = T("GlucoOn");
    sprintf(value, "%s%lluh %02llumn", Gon.c_str(), Heures, Minutes);
    y = PrintGaucheWrap(CanvaBase, String(value), 20, y, maxR, LH);

    PrintCentre(CanvaBase, "https://stellam.dev", EcranW2, EcranH - 40, 1);
    CanvaBase->flush();
}

void handleTouch_Infos(uint16_t touchX, uint16_t touchY)
{
}