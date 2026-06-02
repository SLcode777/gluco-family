#include "pageCompte.h"
#include "Config.h"
#include <Arduino.h>
#include <WiFi.h>
#include <Ecran/Gestion.h>
#include <Ecran/pageClavier.h>
#include <Ecran/pageLibreServeur.h>
#include <Stock.h>
#include "Langues/Langue.h"
#include "Libreview.h"
#include "Dexcom.h"

static Bouton Boutons[5] = {
    {15, 40, 110, 30, "Modifier"},
    {15, 288, 110, 30, "Modifier"},
    {15, 288, 110, 30, "Modifier"},
    {15, 288, 110, 30, "Modifier"},
    {85, 288, 150, 35, "Tester"}};
void drawPara(String Titre, String Valeur, int H0, int index);

void CompteSetup()
{
    PageActu = pageCompte;
    Boutons[0].Texte=T("Modifier");
    Boutons[1].Texte=T("Modifier");
    Boutons[2].Texte=T("Modifier");
    Boutons[3].Texte=T("Modifier");
    Boutons[4].Texte=T("Tester");
    CanvaBase->setFont(u8g2_font_helvB18_tf);
    CanvaBase->setTextColor(RGB565_WHITE);
    CanvaBase->fillScreen(C_grisFonce);

    // Person being edited — centered title at the very top
    String personLabel = (persons[configPersonIndex].name.length() > 0)
                             ? persons[configPersonIndex].name
                             : String("Personne ") + String(configPersonIndex + 1);
    CanvaBase->setFont(u8g2_font_helvB18_tf);
    CanvaBase->setTextColor(RGB565_WHITE);
    PrintCentre(CanvaBase, personLabel, EcranW / 2, 20, 1);

    // First name field at the top
    drawPara(T("FirstName"), persons[configPersonIndex].name, 32, 0);

    // Sensor type selector - two buttons side by side
    CanvaBase->setFont(u8g2_font_helvB14_tf);

    // FreeStyle button (left)
    int buttonWidth = (EcranW - 21) / 2;  // 21 = 7 + 7 + 7 (margins and spacing)
    uint16_t libreColor = (persons[configPersonIndex].sensorType == SENSOR_LIBRE) ? RGB565_GREEN : RGB565_NAVY;
    CanvaBase->fillRoundRect(7, 98, buttonWidth, 38, 8, libreColor);
    CanvaBase->drawRoundRect(7, 98, buttonWidth, 38, 8, RGB565_WHITE);
    CanvaBase->setFont(u8g2_font_helvB14_tf);
    PrintCentre(CanvaBase, "FreeStyle", 7 + buttonWidth / 2, 122, 1);

    // Dexcom button (right)
    uint16_t dexcomColor = (persons[configPersonIndex].sensorType == SENSOR_DEXCOM) ? RGB565_GREEN : RGB565_NAVY;
    CanvaBase->fillRoundRect(14 + buttonWidth, 98, buttonWidth, 38, 8, dexcomColor);
    CanvaBase->drawRoundRect(14 + buttonWidth, 98, buttonWidth, 38, 8, RGB565_WHITE);
    PrintCentre(CanvaBase, "Dexcom", 14 + buttonWidth + buttonWidth / 2, 122, 1);
    
    // Display appropriate account info based on sensor type
    if (persons[configPersonIndex].sensorType == SENSOR_LIBRE)
    {
        PrintCentre(CanvaBase, T("Compte") + " LibreLinkUp", EcranW / 2, 152, 1);
        drawPara("Email", libreEmail, 160, 1);
        drawPara(T("Password"), librePass, 226, 2);
        String zoneAffichee = T("Undefined");
        for (int i = 0; i < 12; i++)
        {
            if (strlen(regions[i]) > 0)
            {
                if (String(regionsCode[i]) == libreZone)
                {
                    zoneAffichee = regions[i];
                    break;
                }
            }
        }
        drawPara(T("ServerZone"), zoneAffichee, 292, 3);
    }
    else // SENSOR_DEXCOM
    {
        PrintCentre(CanvaBase, T("Compte") + " Dexcom Share", EcranW / 2, 152, 1);
        drawPara(T("Username"), persons[configPersonIndex].dexcomUsername, 160, 1);
        drawPara(T("Password"), persons[configPersonIndex].dexcomPassword, 226, 2);

        // Region selection with radio buttons (centered)
        CanvaBase->setFont(u8g2_font_helvB14_tf);
        CanvaBase->fillRoundRect(7, 292, EcranW - 14, 58, 8, RGB565_NAVY);
        CanvaBase->drawRoundRect(7, 292, EcranW - 14, 58, 8, RGB565_WHITE);
        PrintCentre(CanvaBase, T("Region"), EcranW2, 312, 1);

        // Calculate positions to center the entire group
        // Approximate text widths: "Non-US" ~60px, "US" ~25px, "JP" ~25px
        int radioY = 332;
        int radioR = 8;
        int textOffset = 15; // Space between radio and text
        
        // Total width calculation: radio + text + spacing + radio + text + spacing + radio + text
        // Non-US: 8 + 15 + 60 = 83
        // US: 8 + 15 + 25 = 48
        // JP: 8 + 15 + 25 = 48
        // Spacing between groups: 20px each
        int totalWidth = 83 + 20 + 48 + 20 + 48; // ~219px
        int startX = EcranW2 - (totalWidth / 2);
        
        int radioX1 = startX + 8; // Non-US button
        int radioX2 = radioX1 + 83 + 20 + 8; // US button
        int radioX3 = radioX2 + 48 + 20 + 8; // JP button
        
        // Non-US button
        CanvaBase->fillCircle(radioX1, radioY, radioR, RGB565_WHITE);
        if (dexcomRegion != "US" && dexcomRegion != "JP") {
            CanvaBase->fillCircle(radioX1, radioY, radioR - 3, RGB565_GREEN);
        } else {
            CanvaBase->fillCircle(radioX1, radioY, radioR - 3, RGB565_NAVY);
        }
        CanvaBase->setFont(u8g2_font_helvB14_tf);
        CanvaBase->setCursor(radioX1 + textOffset, radioY + 5);
        CanvaBase->print("Non-US");
        
        // US button
        CanvaBase->fillCircle(radioX2, radioY, radioR, RGB565_WHITE);
        if (dexcomRegion == "US") {
            CanvaBase->fillCircle(radioX2, radioY, radioR - 3, RGB565_GREEN);
        } else {
            CanvaBase->fillCircle(radioX2, radioY, radioR - 3, RGB565_NAVY);
        }
        CanvaBase->setCursor(radioX2 + textOffset, radioY + 5);
        CanvaBase->print("US");
        
        // JP button
        CanvaBase->fillCircle(radioX3, radioY, radioR, RGB565_WHITE);
        if (dexcomRegion == "JP") {
            CanvaBase->fillCircle(radioX3, radioY, radioR - 3, RGB565_GREEN);
        } else {
            CanvaBase->fillCircle(radioX3, radioY, radioR - 3, RGB565_NAVY);
        }
        CanvaBase->setCursor(radioX3 + textOffset, radioY + 5);
        CanvaBase->print("JP");
    }

    // Test button (centered, below the fields)
    Boutons[4].X0 = (EcranW - 150) / 2;
    Boutons[4].Y0 = 362;
    Bouton_Trace(Boutons[4]); // Tester
    if (SetupEnCours)
    { // First-boot wizard: tell the user how to finish / move on
        CanvaBase->setFont(u8g2_font_helvB14_tf);
        CanvaBase->setTextColor(RGB565_WHITE);
        PrintCentre(CanvaBase, T("SwipeNext"), EcranW / 2, EcranH - 20, 1);
    }
    CanvaBase->flush();
}
void drawPara(String Titre, String Valeur, int H0, int index)
{
    const int boxH = 58;
    CanvaBase->fillRoundRect(7, H0, EcranW - 14, boxH, 8, RGB565_NAVY);
    CanvaBase->drawRoundRect(7, H0, EcranW - 14, boxH, 8, RGB565_WHITE);

    // Title: top-left (left-aligned so it never collides with the right-side button)
    CanvaBase->setFont(u8g2_font_helvB14_tf);
    CanvaBase->setTextColor(RGB565_WHITE);
    PrintGauche(CanvaBase, Titre, 12, H0 + 20, 1);
    // Value: below the title, larger font
    CanvaBase->setFont(u8g2_font_10x20_mf);
    PrintGauche(CanvaBase, Valeur, 12, H0 + 48, 1);
    // Edit button: top-right corner of the box
    Boutons[index].X0 = EcranW - 98;
    Boutons[index].Y0 = H0 + 14;
    Boutons[index].W = 86;
    Boutons[index].H = 30;
    Bouton_Trace(Boutons[index]);
}

void handleTouch_Compte(uint16_t touchX, uint16_t touchY)
{
    if (Bouton_Appui(Boutons[0], touchX, touchY)) // First name
    {
        PageActu = pageClavier_PersonName;
        setup_clavier();
        return;
    }

    // Sensor type selection - two buttons
    if (touchY >= 98 && touchY <= 136)
    {
        int buttonWidth = (EcranW - 21) / 2;
        
        // FreeStyle Libre button (left)
        if (touchX >= 7 && touchX <= 7 + buttonWidth)
        {
            if (persons[configPersonIndex].sensorType != SENSOR_LIBRE) {
                persons[configPersonIndex].sensorType = SENSOR_LIBRE;
                
                // Clear all cached data when switching accounts
                clearData();
                
                RecordFichierParametres();
                CompteSetup();
            }
            return;
        }
        
        // Dexcom button (right)
        if (touchX >= 14 + buttonWidth && touchX <= EcranW - 7)
        {
            if (persons[configPersonIndex].sensorType != SENSOR_DEXCOM) {
                persons[configPersonIndex].sensorType = SENSOR_DEXCOM;
                
                // Clear all cached data when switching accounts
                clearData();
                
                RecordFichierParametres();
                CompteSetup();
            }
            return;
        }
    }

    if (Bouton_Appui(Boutons[1], touchX, touchY)) // Email/Username
    {
        if (persons[configPersonIndex].sensorType == SENSOR_LIBRE)
        {
            PageActu = pageClavier_CompteEmail;
        }
        else
        {
            PageActu = pageClavier_DexcomUsername;
        }
        setup_clavier();
    }
    else if (Bouton_Appui(Boutons[2], touchX, touchY)) // Password
    {
        if (persons[configPersonIndex].sensorType == SENSOR_LIBRE)
        {
            PageActu = pageClavier_ComptePwd;
        }
        else
        {
            PageActu = pageClavier_DexcomPwd;
        }
        setup_clavier();
    }
    else if (persons[configPersonIndex].sensorType == SENSOR_DEXCOM && touchY >= 292 && touchY <= 350)
    {
        // Handle Dexcom region radio buttons (check this BEFORE Bouton_Appui)
        // Calculate positions to match display (same calculation as in CompteLoop)
        int totalWidth = 83 + 20 + 48 + 20 + 48; // ~219px
        int startX = EcranW2 - (totalWidth / 2);
        
        int radioX1 = startX + 8; // Non-US button
        int radioX2 = radioX1 + 83 + 20 + 8; // US button
        int radioX3 = radioX2 + 48 + 20 + 8; // JP button
        int touchRadius = 50; // Touch area
        
        // Check if Non-US radio button was clicked
        if (touchX >= radioX1 - touchRadius && touchX <= radioX1 + touchRadius) {
            if (dexcomRegion != "US" && dexcomRegion != "JP") {
                // Already Non-US, no change needed
            } else {
                dexcomRegion = "Non-US";
                RecordFichierParametres();
                CompteSetup();
            }
            return;
        }
        // Check if US radio button was clicked
        else if (touchX >= radioX2 - touchRadius && touchX <= radioX2 + touchRadius) {
            if (dexcomRegion != "US") {
                dexcomRegion = "US";
                RecordFichierParametres();
                CompteSetup();
            }
            return;
        }
        // Check if JP radio button was clicked
        else if (touchX >= radioX3 - touchRadius && touchX <= radioX3 + touchRadius) {
            if (dexcomRegion != "JP") {
                dexcomRegion = "JP";
                RecordFichierParametres();
                CompteSetup();
            }
            return;
        }
    }
    else if (Bouton_Appui(Boutons[3], touchX, touchY)) // Zone serveur (LibreView only)
    {
        if (persons[configPersonIndex].sensorType == SENSOR_LIBRE)
        {
            pageLibreServeurSetup();
        }
    }
    else if (Bouton_Appui(Boutons[4], touchX, touchY)) // Tester la connexion
    {
        CanvaBase->fillRect(0, 405, EcranW, 60, C_grisFonce);
        CanvaBase->setTextColor(RGB565_WHITE);
        bool loginSuccess = false;

        if (persons[configPersonIndex].sensorType == SENSOR_LIBRE)
        {
            loginSuccess = loginLibreLinkUp();
        }
        else
        {
            loginSuccess = loginDexcomShare(persons[configPersonIndex]);
        }

        if (loginSuccess)
        {
            CanvaBase->setFont(u8g2_font_helvB18_tf);
            PrintCentre(CanvaBase, T("ConnectOK"), EcranW / 2, 432, 1);
        }
        else
        { //Problème
            CanvaBase->setFont(u8g2_font_10x20_tf);
            if (ServerConnu){
                 PrintCentre(CanvaBase, T("UserUnknown"), EcranW / 2, 432, 1);
            } else {
                 PrintCentre(CanvaBase, T("ServerNoAccess"), EcranW / 2, 432, 1);
            }

        }
        CanvaBase->flush();
    }
}
