#pragma once
#include <Arduino.h>

#define HOSTNAME "GlucoMonit-"

// Sensor types
enum SensorType {
    SENSOR_LIBRE = 0,
    SENSOR_DEXCOM = 1
};

#define MAX_PERSONS 3

struct Person {
  String name;
  bool configured;

  SensorType sensorType;

  // Dexcom Credentials
  String dexcomUsername;
  String dexcomPassword;

  // Session cache (per-person, populated by loginDexcomShare)
  String dexcomSessionId;
  String dexcomAccountId;

  // Last measurement
  int16_t glucoseMgDl;               // 0 = no measurement
  int8_t trendArrow;                 // 0 = unknown
  unsigned long lastGlyUnixTime;     // unix timestamp
  long ageSeconds; 

  // poll timers (milliseconds)
  unsigned long lastDemandeMillis;
  unsigned long lastReceptionMillis;
  unsigned long lastOkMillis;
  unsigned long recurMillis;         // adaptative poll intervalle

  // glycemic targets
  int16_t targetLow;
  int16_t targetHigh;

};

extern Person persons[MAX_PERSONS];
extern int activePersonsCount;       // # of configured persons
extern int configPersonIndex;   // 0..2 : person currently being edited in config pages

enum GlucoseUnit {
    GLUCOSE_UNIT_MGDL = 0,
    GLUCOSE_UNIT_MMOLL = 1
};
//Couleur affichage glycemeie
enum GlucoseColor {
    GLUCOSE_BLANC = 0,
    GLUCOSE_COULEUR = 1
};

#define RecurrenceGlycemie 120000 // 2 minutes

//========= MACRO =========
#ifndef ARRAY_SIZE
#define ARRAY_SIZE(x) (sizeof(x) / sizeof((x)[0]))
#endif

//============ Version et Build ==========
extern const char* Version ;
extern const char* BuildDate ;

extern String ssid, password, hostname;
extern String MyIP;

extern String libreEmail;
extern String librePass;
extern String libreZone;
extern bool ServerConnu;

// Dexcom configuration
extern String dexcomRegion;

extern const char *regions[12];
extern const char *regionsCode[12];

extern int8_t idxFuseau; // Fuseau Horaire
extern int8_t Jour;      //-1=inconnu,0=dimanche,1=lundi...
extern bool HeureValide;
extern int16_t Int_Heure, Int_Minute;
extern String DATE, HEURE, DateAMJ, Hmn;
extern uint64_t T_On_seconde;

extern GlucoseUnit glucoseUnit;
extern GlucoseColor glucoseColor;

extern String ES, FS, GS, RS, US;

extern int16_t LuminositeNuit;

extern bool SetupEnCours;

//======= Page HTML Brute ============
extern bool AutorisationPageBrute;
extern unsigned long TimerAutorisationBruteMillis;

// PSRAM
extern EXT_RAM_BSS_ATTR char MessageEcran[];
extern EXT_RAM_BSS_ATTR String LoginJSON, GraphJSON, ConnectionJSON;

// Clear all data (glucose, Dexcom cache, LibreView cache) when switching accounts
void clearData();

// Initialize persons
void InitPersons();
bool allConfiguredPersonsSilent(unsigned long timeoutMs);

String formatGlucoseValue(int16_t mgdl);
String getGlucoseUnitLabel();
