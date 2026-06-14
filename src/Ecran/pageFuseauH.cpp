#include "Ecran/pageFuseauH.h"
#include "Config.h"
#include <Arduino.h>
#include <WiFi.h>
#include "Ecran/Gestion.h"
#include "Stock.h"
#include "Langues/Langue.h"

static int16_t idxVisu = 0, NbBoutons = 0;
static RadioBouton Rboutons[10];


void ImpressionZones();

void pageFuseauSetup()
{
    PageActu = pageFuseauH;
    CanvaBase->setFont(u8g2_font_helvB18_tf);
    CanvaBase->setTextColor(RGB565_WHITE);
    CanvaBase->fillScreen(C_grisFonce);
    PrintCentre(CanvaBase, T("F_Hor"), EcranW / 2, 30, 1);
    //============= FUSEAU ==========================
    CanvaBase->fillRoundRect(7, 50, EcranW - 14, 260, 8, RGB565_NAVY);
    CanvaBase->drawRoundRect(7, 50, EcranW - 14, 260, 8, RGB565_WHITE);
    idxVisu = idxFuseau - 2;
    ImpressionZones();
    if (SetupEnCours)
    { // First-boot wizard: tell the user how to reach the next step
        CanvaBase->setFont(u8g2_font_helvB14_tf);
        CanvaBase->setTextColor(RGB565_WHITE);
        PrintCentre(CanvaBase, T("SwipeNext"), EcranW / 2, EcranH - 20, 1);
        CanvaBase->flush();
    }
}

void handleTouch_Fuseau(uint16_t touchX, uint16_t touchY, int16_t DeltaTouchY)
{
    int16_t delta = DeltaTouchY / 2;

    if (delta != 0)
    {
        idxVisu = idxVisu - delta;
        ImpressionZones();
    }
    else
    {
        for (int i = 0; i < NbBoutons; i++)
        {
            if (RadioBouton_Appui(Rboutons[i], touchX, touchY))
            {
                idxFuseau = i + idxVisu;
                ImpressionZones();
                RecordFichierParametres();
                i = 100;
            }
        }
    }
}
void ImpressionZones()
{
    if (idxVisu < 0)
        idxVisu = 0;
    if (idxVisu > NB_TZ - 3)
        idxVisu = NB_TZ - 3;
    CanvaBase->fillRoundRect(7, 50, EcranW - 14, 260, 8, RGB565_NAVY);
    CanvaBase->drawRoundRect(7, 50, EcranW - 14, 260, 8, RGB565_WHITE);
    int Y = 60;
    NbBoutons = 0;
    for (int i = idxVisu; i < NB_TZ; i++)
    {

        Rboutons[NbBoutons].X0 = 15;
        Rboutons[NbBoutons].Y0 = Y;
        Rboutons[NbBoutons].R = 15;
        Rboutons[NbBoutons].Texte = nomTZ[i];
        // maxRight = inner right edge of the blue card; 26px line spacing.
        if (i == idxFuseau)
        {
            RadioBouton_TraceWrap(Rboutons[NbBoutons], EcranW - 13, 26, RGB565_BLUE);
        }
        else
        {
            RadioBouton_TraceWrap(Rboutons[NbBoutons], EcranW - 13, 26);
        }
        NbBoutons++;
        // 64px row pitch leaves room for labels that wrap onto a 2nd line
        // (long timezone names) without overlapping the next entry.
        Y = Y + 64;
        if (Y > 290)
            break;
    }
    CanvaBase->flush();
}