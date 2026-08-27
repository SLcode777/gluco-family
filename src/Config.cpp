#include "Config.h"
#include <Arduino.h>
#include "Dexcom.h"
#include "Libreview.h"

//============ Version et Build ==========
const char* Version = PROG_VERSION;
const char* BuildDate = BUILD_DATE;

//======= VARIABLES ========
String ssid = "", password = "", hostname = "";
String MyIP = "0.0.0.0";

String  libreEmail = "";
String librePass = "";
String libreZone = "";
bool ServerConnu = false;

Person persons[MAX_PERSONS];
int activePersonsCount = 0;
int configPersonIndex = 0;

// Dexcom configuration
String dexcomRegion = "Non-US"; // Default to "Non-US" region

//Regions possibles pour LibreLinkUp
const char *regions[12] = {"General", "", "Europe", " Europe 2", "France", "Germany", "USA", "Canada", "Australia", "Japan ", "Asia Pacific", "UAE"};
const char *regionsCode[12] = {"", "", "eu", "eu2", "fr", "de", "us", "ca", "au", "jp", "ap", "ae"};

// Heure
int8_t idxFuseau = 2; // Fuseau Horaire
int8_t Jour;        //-1=inconnu,0=dimanche,1=lundi...
bool HeureValide=false;
int16_t Int_Heure, Int_Minute;
String DATE, HEURE, DateAMJ, Hmn;
uint64_t T_On_seconde = 0;




// Glycémie
GlucoseUnit glucoseUnit = GLUCOSE_UNIT_MGDL;
GlucoseColor glucoseColor = GLUCOSE_COULEUR;


//Generaux
String ES = String((char)27); // ESC Separator
String FS = String((char)28); // File Separator
String GS = String((char)29); // Group Separator
String RS = String((char)30); // Record Separator
String US = String((char)31); // Unit Separator

int16_t LuminositeNuit=255; //Maximum

bool SetupEnCours=true;

//======= Page HTML Brute ============
bool AutorisationPageBrute=false;
unsigned long TimerAutorisationBruteMillis=0;

// PSRAM
EXT_RAM_BSS_ATTR char MessageEcran[8192];
EXT_RAM_BSS_ATTR String LoginJSON = "", GraphJSON = "",ConnectionJSON = "";

String formatGlucoseValue(int16_t mgdl)
{
    if (glucoseUnit == GLUCOSE_UNIT_MMOLL)
    {
        return String(float(mgdl) / 18.0f, 1);
    }
    return String(mgdl);
}

String getGlucoseUnitLabel()
{
    if (glucoseUnit == GLUCOSE_UNIT_MMOLL)
    {
        return "mmol/L";
    }
    return "mg/dL";
}

void clearData()
{
    Serial.println("Clearing all data (glucose, Dexcom cache, LibreView cache)...");
    
  for (int i = 0; i < MAX_PERSONS; i++) {
    persons[i].glucoseMgDl = 0;
    persons[i].trendArrow = 0;
    persons[i].lastGlyUnixTime = 0;
    persons[i].ageSeconds = 0;
    persons[i].lastReceptionMillis = 0;
    persons[i].lastOkMillis = 0;
    // credentials et name conservés
  }

  // Buffers debug
  LoginJSON = "";
  GraphJSON = "";
  ConnectionJSON = "";

  // Clear Dexcom cache
  clearDexcomCache();
    
  // Clear LibreView cache
  clearLibreViewCache();
}

void InitPersons() {
  for (int i = 0; i < MAX_PERSONS; i++) {
    persons[i].name = "";
    persons[i].configured = false;
    persons[i].sensorType = SENSOR_DEXCOM;  // MVP default
    persons[i].dexcomUsername = "";
    persons[i].dexcomPassword = "";
    persons[i].dexcomSessionId = "";
    persons[i].dexcomAccountId = "";
    persons[i].librePatientId = "";
    persons[i].glucoseMgDl = 0;
    persons[i].trendArrow = 0;
    persons[i].lastGlyUnixTime = 0;
    persons[i].ageSeconds = 0;
    persons[i].lastDemandeMillis = 0;
    persons[i].lastReceptionMillis = 0;
    persons[i].lastOkMillis = 0;
    persons[i].recurMillis = 120000;        // 2 min initial (legacy)
    persons[i].targetLow = 70;              // sentinel values, overwritten by config
    persons[i].targetHigh = 180;
  }
  activePersonsCount = 0;
}

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
    