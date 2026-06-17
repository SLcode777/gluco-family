#include "Server.h"
#include "Config.h"
#include <Arduino.h>
#include <WiFi.h>
#include <HTTPClient.h>
#include <ArduinoJson.h>
#include <AsyncTCP.h>
#include <ESPAsyncWebServer.h>
#include <ESPmDNS.h>
#include <Update.h>

#include "HTML/pageMain.h"
#include "HTML/pageBrute.h"
#include "HTML/pageAutorisationBrute.h"
#include "HTML/pageOTA.h"
#include "HTML/pageSettings.h"
#include "HTML/JS_Commun.js.h"
#include "HTML/JS_Main.js.h"
#include "Ecran/pageAutBrute.h"
#include "Langues/Langue.h"
#include "Langues/en.h"
#include "Langues/fr.h"
#include "Langues/de.h"
#include "Langues/it.h"
#include "Langues/es.h"

// Serveur Web
static AsyncWebServer server(80);

// Prototypes
void notFound(AsyncWebServerRequest *request);
void handleDoUpdate(AsyncWebServerRequest *request, const String &filename, size_t index, uint8_t *data, size_t len, bool final);

void Init_Server()
{

  // Main Page
  //*********
  server.on("/", HTTP_GET, [](AsyncWebServerRequest *request)
            { request->send(200, "text/html", MainHtml); });
  server.on("/Brute", HTTP_GET, [](AsyncWebServerRequest *request)
            {
                  if (AutorisationPageBrute)
                  {                      
                      request->send(200, "text/html", BruteHtml);
                  }
                  else
                  {
                      PageActu = pageAutBrute;
                      request->send(200, "text/html", AutBruteHtml);
                  }
                  TimerAutorisationBruteMillis = millis(); });
  server.on("/OTA", HTTP_GET, [](AsyncWebServerRequest *request)
            { 
                if (AutorisationPageBrute)
                  {
                      request->send(200, "text/html", OTAupdateHtml);
                  }
                  else
                  {
                      PageActu = pageAutBrute;
                      request->send(200, "text/html", AutBruteHtml);
                  }
                  TimerAutorisationBruteMillis = millis(); });
  server.on(
      "/update", HTTP_POST,
      [](AsyncWebServerRequest *request) {},
      [](AsyncWebServerRequest *request, const String &filename, size_t index, uint8_t *data,
         size_t len, bool final)
      {
        handleDoUpdate(request, filename, index, data, len, final);
      });

  server.on("/JS_Commun", HTTP_GET, [](AsyncWebServerRequest *request)
            { String complement="\nconst Version = '" + String(Version) +"' ;\nconst glucoseUnit = " + String(glucoseUnit) + ";";
              request->send(200, "text/javascript", String(JS_Commun) + complement); });
  server.on("/JS_Traduction", HTTP_GET, [](AsyncWebServerRequest *request)
            {  String file;
              switch(LaLangue)
                  {
                      case LANG_EN:
                          file=String(LangEN);
                          break;
                      case LANG_FR:
                          file=String(LangFR);
                          break;
                      case LANG_DE:
                          file=String(LangDE);
                          break;
                      case LANG_ES:
                          file=String(LangES);
                          break;
                      case LANG_IT:
                          file=String(LangIT);
                          break;
                  }
              
              request->send(200, "text/javascript",  "Traduction =" + file +";"); });
  server.on("/JS_Main", HTTP_GET, [](AsyncWebServerRequest *request)
            { request->send(200, "text/javascript", JS_Main); });
  server.on("/LoginJSON", HTTP_GET, [](AsyncWebServerRequest *request)
            { request->send(200, "application/json", LoginJSON); });
  server.on("/ConnectionJSON", HTTP_GET, [](AsyncWebServerRequest *request)
            { request->send(200, "application/json", ConnectionJSON); });
  server.on("/GraphJSON", HTTP_GET, [](AsyncWebServerRequest *request)
            { request->send(200, "application/json", GraphJSON); });
  server.on("/ajaxGlycemie", HTTP_GET, [](AsyncWebServerRequest *request)
            {JsonDocument doc;
                doc["GlucoseUnitLabel"] = getGlucoseUnitLabel();
                JsonArray arr = doc["persons"].to<JsonArray>();
                for (int i = 0; i < MAX_PERSONS; i++)
                {
                    JsonObject p = arr.add<JsonObject>();
                    p["name"] = persons[i].name;
                    p["configured"] = persons[i].configured;
                    p["GlycemieVal"] = persons[i].glucoseMgDl;
                    p["TrendArrow"] = persons[i].trendArrow;
                    p["lastGlyUnixTime"] = persons[i].lastGlyUnixTime;
                    p["targetLow"] = persons[i].targetLow;
                    p["targetHigh"] = persons[i].targetHigh;
                    p["sensorType"] = (int)persons[i].sensorType;
                }
                String Json;
                serializeJson(doc, Json);
                request->send(200, "application/json", Json); });
  // ===== Settings (gated by the physical "Accept" button, like /Brute) =====
  server.on("/Settings", HTTP_GET, [](AsyncWebServerRequest *request)
            {
                if (AutorisationPageBrute)
                {
                    request->send(200, "text/html", SettingsHtml);
                }
                else
                {
                    PageActu = pageAutBrute;
                    request->send(200, "text/html", AutBruteHtml);
                }
                TimerAutorisationBruteMillis = millis(); });
  server.on("/api/config", HTTP_GET, [](AsyncWebServerRequest *request)
            {
                if (!AutorisationPageBrute)
                {
                    request->send(403, "application/json", "{\"error\":\"unauthorized\"}");
                    return;
                }
                JsonDocument doc;
                doc["dexcomRegion"] = dexcomRegion;
                doc["libreEmail"] = libreEmail;
                doc["libreZone"] = libreZone;
                doc["librePass"] = librePass;
                JsonArray arr = doc["persons"].to<JsonArray>();
                for (int i = 0; i < MAX_PERSONS; i++)
                {
                    JsonObject p = arr.add<JsonObject>();
                    p["name"] = persons[i].name;
                    p["sensorType"] = (int)persons[i].sensorType;
                    p["dexcomUsername"] = persons[i].dexcomUsername;
                    p["dexcomPass"] = persons[i].dexcomPassword;
                    p["targetLow"] = persons[i].targetLow;
                    p["targetHigh"] = persons[i].targetHigh;
                    p["configured"] = persons[i].configured;
                }
                String Json;
                serializeJson(doc, Json);
                TimerAutorisationBruteMillis = millis();
                request->send(200, "application/json", Json); });
  server.on("/api/settings", HTTP_POST, [](AsyncWebServerRequest *request)
            {
                if (!AutorisationPageBrute)
                {
                    request->send(403, "application/json", "{\"error\":\"unauthorized\"}");
                    return;
                }
                auto P = [&](const String &n) -> String
                { return request->hasParam(n, true) ? request->getParam(n, true)->value() : String(); };

                // Global account fields
                if (request->hasParam("region", true)) dexcomRegion = P("region");
                if (request->hasParam("lemail", true)) libreEmail = P("lemail");
                if (request->hasParam("lzone", true)) libreZone = P("lzone");
                { String lp = P("lpass"); if (lp.length() > 0) librePass = lp; }

                for (int i = 0; i < MAX_PERSONS; i++)
                {
                    String s = String(i);
                    persons[i].name = P("name" + s);
                    persons[i].sensorType = (SensorType)(P("sensor" + s).toInt());
                    persons[i].dexcomUsername = P("duser" + s);
                    String dp = P("dpass" + s);
                    if (dp.length() > 0) persons[i].dexcomPassword = dp;
                    if (request->hasParam("low" + s, true)) persons[i].targetLow = P("low" + s).toInt();
                    if (request->hasParam("high" + s, true)) persons[i].targetHigh = P("high" + s).toInt();
                    // configured = does the chosen sensor have credentials?
                    if (persons[i].sensorType == SENSOR_DEXCOM)
                        persons[i].configured = (persons[i].dexcomUsername.length() > 0);
                    else
                        persons[i].configured = (libreEmail.length() > 0);
                    // force a fresh login on next poll
                    persons[i].dexcomSessionId = "";
                    persons[i].dexcomAccountId = "";
                    persons[i].lastDemandeMillis = 0;
                }
                activePersonsCount = 0;
                for (int i = 0; i < MAX_PERSONS; i++)
                    if (persons[i].configured) activePersonsCount++;

                RecordFichierParametres();
                TimerAutorisationBruteMillis = millis();
                request->send(200, "application/json", "{\"status\":\"ok\"}"); });
  server.on("/Restart", HTTP_GET, [](AsyncWebServerRequest *request)
            { request->send(200, "text/html", RestartHtml); 
                 delay(1000);
                 ESP.restart(); });
  server.on("/favicon.ico", HTTP_GET, [](AsyncWebServerRequest *request)
            { request->send(200, "image/svg+xml", Favicon); });
  server.on("/favicon192.ico", HTTP_GET, [](AsyncWebServerRequest *request)
            { request->send(200, "image/svg+xml", Favicon192); });
  server.on("/manifest.json", HTTP_GET, [](AsyncWebServerRequest *request)
            { request->send(200, "application/json", Manifest); });
  server.onNotFound(notFound);

  server.begin();
  EcranPrintln(T("Serveur80"));
}

void notFound(AsyncWebServerRequest *request)
{
  request->send(404, "text/plain", "Not found");
}

void handleDoUpdate(AsyncWebServerRequest *request, const String &filename, size_t index, uint8_t *data, size_t len, bool final)
{ // Mise à jour par OTA
  char progress[30];
  if (!index)
  {
    EcranPrintln(T("Update"));
    // content_len = request->contentLength();
    if (!Update.begin(UPDATE_SIZE_UNKNOWN))
    {
      Update.printError(Serial);
    }
  }

  if (Update.write(data, len) != len)
  {
    Update.printError(Serial);
    sprintf(progress, "Progress: %d%%\n", (Update.progress() * 100) / Update.size());
    EcranPrintln(String(progress));
  }

  if (final)
  {
    AsyncWebServerResponse *response = request->beginResponse(302, "text/plain", "Please wait while the device reboots");
    response->addHeader("Refresh", "20");
    response->addHeader("Location", "/");
    request->send(response);
    if (!Update.end(true))
    {
      Update.printError(Serial);
    }
    else
    {
      EcranPrintln(T("UpdateComplete"));
      Serial.flush();
      ESP.restart();
    }
  }
}