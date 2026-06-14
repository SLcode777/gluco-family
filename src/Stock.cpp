#include "Stock.h"
#include "Config.h"
#include <Arduino.h>
#include "LittleFS.h"
#include <ArduinoJson.h>
#include "Heure.h"
#include "Langues/Langue.h"
#include "Ecran/Gestion.h"


void ReadFichierParametres();
String SerializeConfiguration();   
void DeserializeConfiguration(String json) ;
void RecordFichierParametres();


void InitStock()
{
  if (!LittleFS.begin(true))
  {
    Serial.println("Erreur de montage de LittleFS");
    return;
  }
  Serial.println("LittleFS monté avec succès");
  delay(500);
}   

// *************************************************************
// Stockage parametres en zone SPIFFS (methode 2026 depuis V17)
// *************************************************************
void RecordFichierParametres() {
  File file = LittleFS.open("/parametres.json", FILE_WRITE);
  file.print(SerializeConfiguration());  //Fichier au format JSON
  file.close();
  Serial.println("Ecriture fichier parametres");
}
void ReadFichierParametres() {
  if (!LittleFS.exists("/parametres.json")) {  //Fichier pas encore crée
    RecordFichierParametres();
  }

  File file = LittleFS.open("/parametres.json", "r");
  Serial.println("Lecture du fichier paramètres");
  String content = file.readString();  // lit tout le fichier
  file.close();
  DeserializeConfiguration(content);
}
void RemoveParametres(){
  LittleFS.remove("/parametres.json");
}

void DeserializeConfiguration(String json) {
  Serial.print("Json reçu:");
  Serial.println(json);
  JsonDocument conf;
  DeserializationError error = deserializeJson(conf, json);

  if (error) {
    Serial.print("Erreur de parsing des paramètres: ");
    Serial.println(error.c_str());
    return;
  }

  // Detect schema version (missing field = legacy v1 with flat dexcom credentials)
  int formatVersion = conf["format_version"] | 1;
  Serial.printf("Config format_version: %d\n", formatVersion);

  // -------- Common fields (same in v1 and v2) --------
  ssid = conf["ssid"].as<String>();
  password = conf["password"].as<String>();
  hostname = conf["hostname"] | hostname;
  MyIP = conf["MyIP"] | MyIP;
  idxFuseau = conf["idxFuseau"].isNull() ? idxFuseau : conf["idxFuseau"];
  rotation = conf["rotation"].isNull() ? rotation : conf["rotation"];
  libreEmail = conf["libreEmail"].as<String>();
  librePass = conf["librePass"].as<String>();
  libreZone = conf["libreZone"].as<String>();
  LuminositeNuit = conf["LuminositeNuit"] | LuminositeNuit;
  LaLangue = conf["LaLangue"] | LaLangue;
  dexcomRegion = conf["dexcomRegion"] | dexcomRegion;

  int glucoseUnitInt = conf["glucoseUnit"] | GLUCOSE_UNIT_MGDL;
  glucoseUnit = (GlucoseUnit)glucoseUnitInt;
  int glucoseColorInt = conf["glucoseColor"] | GLUCOSE_COULEUR;
  glucoseColor = (GlucoseColor)glucoseColorInt;

  // -------- Per-person fields (v1 vs v2) --------
  if (formatVersion >= 2) {
    // v2: read from "persons" array
    JsonArray personsArray = conf["persons"].as<JsonArray>();
    for (int i = 0; i < MAX_PERSONS; i++) {
      JsonVariant p = personsArray[i];
      if (p.isNull()) {
        // Slot missing in JSON: leave InitPersons defaults
        continue;
      }
      persons[i].name           = p["name"].as<String>();
      persons[i].configured     = p["configured"] | false;
      int sensorTypeInt         = p["sensorType"] | SENSOR_DEXCOM;
      persons[i].sensorType     = (SensorType)sensorTypeInt;
      persons[i].dexcomUsername = p["dexcomUsername"].as<String>();
      persons[i].dexcomPassword = p["dexcomPassword"].as<String>();
      persons[i].targetLow      = p["targetLow"]  | persons[i].targetLow;
      persons[i].targetHigh     = p["targetHigh"] | persons[i].targetHigh;
    }
  } else {
    // v1 legacy: flat fields at root, migrate into persons[0] only
    Serial.println("Legacy v1 config detected — migrating to v2 format");
    persons[0].name           = "";
    persons[0].dexcomUsername = conf["dexcomUsername"].as<String>();
    persons[0].dexcomPassword = conf["dexcomPassword"].as<String>();
    int sensorTypeInt         = conf["sensorType"] | SENSOR_DEXCOM;
    persons[0].sensorType     = (SensorType)sensorTypeInt;
    persons[0].configured     = (persons[0].dexcomUsername.length() > 0);
    // persons[1] and [2] keep InitPersons defaults (configured = false)
  }

  // Recompute activePersonsCount from the loaded data
  activePersonsCount = 0;
  for (int i = 0; i < MAX_PERSONS; i++) {
    if (persons[i].configured) activePersonsCount++;
  }
  Serial.printf("Active persons: %d\n", activePersonsCount);

  // If we migrated from v1, immediately rewrite in v2 format so the disk is up-to-date
  if (formatVersion < 2) {
    Serial.println("Rewriting parametres.json in v2 format");
    RecordFichierParametres();
  }
}

String SerializeConfiguration() {
  JsonDocument conf;

  // Schema version (used by DeserializeConfiguration to detect old format)
  conf["format_version"] = 2;

  // wifi + network
  conf["ssid"] = ssid;
  conf["password"] = password;
  conf["hostname"] = hostname;
  conf["MyIP"] = MyIP;

  // UI / locale
  conf["idxFuseau"] = idxFuseau;
  conf["rotation"] = rotation;
  conf["LuminositeNuit"] = LuminositeNuit;
  conf["LaLangue"]=LaLangue;
  conf["glucoseUnit"] = (int) glucoseUnit;
  conf["glucoseColor"] = (int) glucoseColor;

  // LibreLinkUp config (single multi-patient account - stays global)
  conf["libreEmail"] = libreEmail;
  conf["librePass"] = librePass;
  conf["libreZone"] = libreZone;
  
  // Dexcom configuration
  conf["dexcomRegion"] = dexcomRegion;

  // Per person array
   JsonArray personsArray = conf["persons"].to<JsonArray>();
  for (int i = 0; i < MAX_PERSONS; i++) {
    JsonObject p = personsArray.add<JsonObject>();
    p["name"]           = persons[i].name;
    p["configured"]     = persons[i].configured;
    p["sensorType"]     = (int)persons[i].sensorType;
    p["dexcomUsername"] = persons[i].dexcomUsername;
    p["dexcomPassword"] = persons[i].dexcomPassword;
    p["targetLow"]      = persons[i].targetLow;
    p["targetHigh"]     = persons[i].targetHigh;
  }
  
  String Json;
  serializeJson(conf, Json);
  return Json;
}
