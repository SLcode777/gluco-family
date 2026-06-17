#include <Arduino.h>
#include <Heure.h>
#include <HTTPClient.h>
#include <ArduinoJson.h>
#include "Config.h"
#include "Dexcom.h"
#include "Langues/Langue.h"

// Dexcom Share API base URL, Non-US (default)
static String dexcomBaseURL = "https://shareous1.dexcom.com";

// Dexcom Share API endpoints
const char* DEXCOM_LOGIN_ENDPOINT = "/ShareWebServices/Services/General/LoginPublisherAccountById";
const char* DEXCOM_AUTHENTICATE_ENDPOINT = "/ShareWebServices/Services/General/AuthenticatePublisherAccount";
const char* DEXCOM_GLUCOSE_ENDPOINT = "/ShareWebServices/Services/Publisher/ReadPublisherLatestGlucoseValues";

const char* APP_ID = "d89443d2-327c-4a6f-89e5-496bbb0317db";

bool loginDexcomShare(Person& person)
{
    ServerConnu = false;
    
    if (dexcomRegion == "US") {
        dexcomBaseURL = "https://share2.dexcom.com";
        APP_ID = "d89443d2-327c-4a6f-89e5-496bbb0317db";
    } else if (dexcomRegion == "JP") {
        dexcomBaseURL = "https://share.dexcom.jp";
        APP_ID = "d8665ade-9673-4e27-9ff6-92db4ce13d13";
    } else { // Non-US (default) — always reset to the right server (dexcomBaseURL is static)
        dexcomBaseURL = "https://shareous1.dexcom.com";
        APP_ID = "d89443d2-327c-4a6f-89e5-496bbb0317db";
    }
    Serial.println("Connexion à Dexcom Share: " + dexcomBaseURL);
    
    HTTPClient https;
    String payload;
    int httpCode;
    String response;
    
    // Step 1: Authenticate to get account ID (only if not cached)
    if (person.dexcomAccountId.length() == 0) {
        Serial.println("Récupération de l'Account ID...");
        https.begin(dexcomBaseURL + String(DEXCOM_AUTHENTICATE_ENDPOINT));
        https.setTimeout(15000);
        https.addHeader("Content-Type", "application/json");
        https.addHeader("Accept", "application/json");
        https.addHeader("User-Agent", "Dexcom Share/3.0.2.11 CFNetwork/711.2.23 Darwin/14.0.0");
        
        JsonDocument authDoc;
        authDoc["accountName"] = person.dexcomUsername;
        authDoc["password"] = person.dexcomPassword;
        authDoc["applicationId"] = APP_ID;
        
        serializeJson(authDoc, payload);
        
        httpCode = https.POST(payload);
        response = https.getString();
        https.end();

        // Validate HTTP status BEFORE touching ConnectionJSON, so an error
        // body (e.g. a 504 gateway timeout) never gets stored as an accountId.
        if (httpCode != HTTP_CODE_OK) {
            Serial.println("Authentification échouée: " + String(httpCode));
            Serial.println("Réponse Dexcom: " + response);
            EcranPrintln(HEURE + T("LoginFailed") + String(httpCode), RGB565_ORANGE);
            // Server-side error (5xx) or connection failure (<=0): Dexcom asks
            // us to wait >=120 s before retrying. Honour that back-off.
            if (httpCode >= 500 || httpCode <= 0)
                person.backoffUntilMillis = millis() + 120000;
            return false;
        }

        response.trim();
        if (!response.startsWith("\"") || !response.endsWith("\"")) {
            Serial.println("Format de réponse invalide");
            return false;
        }

        // Valid response only: format ConnectionJSON with the accountId field
        JsonDocument connDoc;
        connDoc["accountId"] = response.substring(1, response.length() - 1);
        serializeJson(connDoc, ConnectionJSON);

        person.dexcomAccountId = response.substring(1, response.length() - 1);
        Serial.println("Account ID obtenu: " + String(person.dexcomAccountId));
    } else {
        Serial.println("Utilisation de l'Account ID en cache");
    }
    
    ServerConnu = true;
    
    // Step 2: Login with account ID to get session ID
    https.begin(dexcomBaseURL + String(DEXCOM_LOGIN_ENDPOINT));
    https.setTimeout(15000);
    https.addHeader("Content-Type", "application/json");
    https.addHeader("Accept", "application/json");
    https.addHeader("User-Agent", "Dexcom Share/3.0.2.11 CFNetwork/711.2.23 Darwin/14.0.0");
    
    JsonDocument loginDoc;
    loginDoc["accountId"] = person.dexcomAccountId;
    loginDoc["password"] = person.dexcomPassword;
    loginDoc["applicationId"] = APP_ID;
    
    serializeJson(loginDoc, payload);
    
    httpCode = https.POST(payload);
    response = https.getString();
    https.end();

    // Validate HTTP status BEFORE touching LoginJSON, so an error body
    // (e.g. a 504 gateway timeout) never gets stored as a sessionId.
    if (httpCode != HTTP_CODE_OK) {
        Serial.println("Login échoué: " + String(httpCode));
        Serial.println("Réponse Dexcom: " + response);
        EcranPrintln(HEURE + T("LoginFailed") + String(httpCode), RGB565_ORANGE);
        // Server-side error (5xx) or connection failure (<=0): back off >=120 s.
        if (httpCode >= 500 || httpCode <= 0)
            person.backoffUntilMillis = millis() + 120000;
        return false;
    }

    response.trim();
    if (!response.startsWith("\"") || !response.endsWith("\"")) {
        Serial.println("Format de réponse invalide");
        return false;
    }

    // Valid response only: format LoginJSON with the sessionId field
    JsonDocument loginJsonDoc;
    loginJsonDoc["sessionId"] = response.substring(1, response.length() - 1);
    serializeJson(loginJsonDoc, LoginJSON);

    person.dexcomSessionId = response.substring(1, response.length() - 1);
    Serial.println("Session ID: " + String(person.dexcomSessionId));
    
    return person.dexcomSessionId.length() > 30;
}

void getDexcomReadings(Person& person)
{
    HTTPClient https;
    
    Serial.println("getDexcomReadings - Session ID: " + person.dexcomUsername + " - Session ID: " + person.dexcomSessionId);
    
    String url = dexcomBaseURL + String(DEXCOM_GLUCOSE_ENDPOINT) +
                 "?sessionId=" + person.dexcomSessionId +
                 "&minutes=1440&maxCount=288"; // 288 = 24h of 5-min readings

    Serial.println("URL Dexcom: " + url);
    https.begin(url);
    https.setTimeout(15000);

    https.addHeader("Content-Type", "application/json");
    https.addHeader("Accept", "application/json");
    https.addHeader("User-Agent", "Dexcom Share/3.0.2.11 CFNetwork/711.2.23 Darwin/14.0.0");

    int httpCode = https.GET();
    String response = https.getString();

    Serial.println("HTTP Code: " + String(httpCode));
    Serial.println("Response: " + response);
    
    if (httpCode == HTTP_CODE_OK) {
        Serial.println("Données Dexcom: " + String(response.length()) + " caractères");
        GraphJSON = response;

        if (response.length() == 0) {
            Serial.println("Réponse vide - aucune donnée disponible");
            EcranPrintln(HEURE + T("GlucoFailed") + " (empty response)", RGB565_ORANGE);
            https.end();
            return;
        }

        JsonDocument doc;
        DeserializationError error = deserializeJson(doc, response);
        
        if (error) {
            Serial.println("Erreur parsing JSON Dexcom: " + String(error.c_str()));
            https.end();
            return;
        }

        JsonArray readings = doc.as<JsonArray>();
        
        if (readings.size() > 0) {
            // Get the most recent reading (first in array)
            JsonObject latestReading = readings[0];
            
            int mgdl = latestReading["Value"];
            const char* trend = latestReading["Trend"];
            const char* timestamp = latestReading["WT"]; // Wall Time
            
            person.glucoseMgDl = mgdl;
            
            // Map Dexcom trend
            // -1=DoubleDown, 0=undefined, 1=Down, 2=DownRight, 3=Flat, 4=UpRight, 5=Up, 6=DoubleUp
            person.trendArrow = 0; // Default to undefined
            if (trend != nullptr) {
                String trendStr = String(trend);
                if (trendStr == "DoubleUp") person.trendArrow = 6;        // DoubleUp
                else if (trendStr == "SingleUp") person.trendArrow = 5;   // Up
                else if (trendStr == "FortyFiveUp") person.trendArrow = 4; // UpRight
                else if (trendStr == "Flat") person.trendArrow = 3;       // Right (Flat)
                else if (trendStr == "FortyFiveDown") person.trendArrow = 2; // DownRight
                else if (trendStr == "SingleDown") person.trendArrow = 1; // Down
                else if (trendStr == "DoubleDown") person.trendArrow = -1; // DoubleDown
            }
            
            // Parse timestamp - Dexcom format: "Date(1234567890000)"
            if (timestamp != nullptr) {
                String tsStr = String(timestamp);
                int startIdx = tsStr.indexOf('(') + 1;
                int endIdx = tsStr.indexOf(')');
                if (startIdx > 0 && endIdx > startIdx) {
                    person.lastGlyUnixTime = tsStr.substring(startIdx, endIdx - 3).toInt();
                }
            }
            
            String DateGly = unixToTimestamp(person.lastGlyUnixTime);
            EcranPrintln(HEURE + T("LastGlyco") + formatGlucoseValue(person.glucoseMgDl) + " " + getGlucoseUnitLabel() + " " + T("le") + DateGly);
            person.lastReceptionMillis = millis();
            
            Serial.println("Glycémie: " + formatGlucoseValue(person.glucoseMgDl) + " " + getGlucoseUnitLabel());
            Serial.println("TrendArrow: " + String(person.trendArrow));
            Serial.println("Timestamp: " + String(person.lastGlyUnixTime));
            
            person.lastOkMillis = millis();
        } else {
            EcranPrintln(HEURE + T("GlucoFailed") + " (no data)", RGB565_ORANGE);
        }
    } else {
        EcranPrintln(HEURE + T("GlucoFailed") + String(httpCode), RGB565_ORANGE);
        Serial.println("Erreur lecture Dexcom: " + response);
    }

    https.end();
}

void LectureDexcom()
{
    // Global stagger: at least 20s between any 2 Dexcom polls to spread network load
    static unsigned long lastAnyPollMillis = 0;
    const unsigned long STAGGER_MS = 20000;

    if (millis() - lastAnyPollMillis < STAGGER_MS && lastAnyPollMillis > 0) {
        return; // too soon since last poll, wait
    }

    // Find the person most overdue for a refresh and poll them
    for (int i = 0; i < MAX_PERSONS; i++) {
        Person& person = persons[i];

        if (!person.configured) continue;
        if (person.sensorType != SENSOR_DEXCOM) continue;
        if (person.dexcomUsername == "" || person.dexcomPassword == "") continue;

        // Respect a server-requested back-off (e.g. after a 504). Rollover-safe:
        // (long)(target - now) > 0 means the target is still in the future.
        if (person.backoffUntilMillis != 0 &&
            (long)(person.backoffUntilMillis - millis()) > 0) continue;

        // Default polling interval: 5 min 15 s (Dexcom updates every 5 min + safety margin)
        person.recurMillis = 315000;

        // Skip if we already have recent data
        if (person.ageSeconds < 315 && person.lastGlyUnixTime > 0) continue;

        // Adaptive retry intervals
        if (person.ageSeconds > 500) {
            person.recurMillis = 90000; // 1.5 min if very stale (server might be down)
        } else if (person.ageSeconds > 315) {
            person.recurMillis = 30000; // 30 s if slightly overdue
        }

        // Is it time to poll this person?
        bool firstPoll = (person.lastDemandeMillis == 0);
        bool intervalElapsed = (millis() - person.lastReceptionMillis > person.recurMillis);

        if (firstPoll || intervalElapsed) {
            Serial.println("Polling Dexcom for: " + person.dexcomUsername);
            person.lastDemandeMillis = millis();

            if (loginDexcomShare(person)) {
                getDexcomReadings(person);
            }

            person.lastReceptionMillis = millis();
            lastAnyPollMillis = millis();

            // Only poll one person per call (to enforce stagger naturally)
            return;
        }
    }
}
void clearDexcomCache()
{
    Serial.println("Clearing Dexcom cache for all persons...");
    for (int i = 0; i < MAX_PERSONS; i++) {
        persons[i].dexcomSessionId = "";
        persons[i].dexcomAccountId = "";
    }
    dexcomBaseURL = "https://shareous1.dexcom.com";
}
