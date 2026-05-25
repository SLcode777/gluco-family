#include <Arduino.h>
#include <Heure.h>
#include <HTTPClient.h>
#include <ArduinoJson.h>
#include "mbedtls/md.h"
#include "Config.h"
#include "Libreview.h"
#include "Langues/Langue.h"

// variables globales
static String AuthToken = "";
static String userID = "";
static String patientId = "";
static String SHAuserID = "";
static bool UserOK = false;

// URL base de l’API (Europe ici, peut être différente)
// sur https://api.libreview.io", on reçoit plein d'info
// sur "https://api-eu.libreview.io"; il suggere une redirection ver fr
// const char *baseURL = "https://api-fr.libreview.io"; //"https://api-eu2.libreview.io";
// Proxy si problème
// const char *baseURL = "https://libreview-proxy.onrender.com/fr";

static String baseURL = ""; // "https://api-fr.libreview.io"; //"https://api-eu2.libreview.io";

String getSHA256(String payload);

bool loginLibreLinkUp()
{
  bool logUserOK = false;
  ServerConnu = false;
  baseURL = "https://api.libreview.io";
  if (libreZone != "")
    baseURL = "https://api-" + libreZone + ".libreview.io";

  Serial.println("Tentative de connexion à LibreLinkUp sur " + baseURL);
  delay(10);
  HTTPClient https;
  String loginURL = baseURL + "/llu/auth/login";

  https.begin(loginURL);
  https.setTimeout(15000); // HTTPClient en ms

  https.addHeader("Content-Type", "application/json");
  https.addHeader("Accept", "application/json");
  // Mes rajouts
  https.addHeader("User-Agent", "okhttp/4.9.0");
  https.addHeader("connection", "Keep-Alive");
  https.addHeader("product", "llu.android");
  https.addHeader("version", "4.17.0"); // Mettre le numéro de version de LinkLibre Up

  // JSON avec email & mot de passe
  String payload = "{\"email\":\"" + libreEmail + "\",\"password\":\"" + librePass + "\"}";

  int httpCode = https.POST(payload);
  String response = https.getString();
  LoginJSON = response;
  if (httpCode == HTTP_CODE_OK)
  {
    ServerConnu = true;
    Serial.println("Login OK: " + String(response.length()) + " caractères");
    JsonDocument doc;
    deserializeJson(doc, response);

    // Mon code
    AuthToken = doc["data"]["authTicket"]["token"].as<String>();

    Serial.println("Longueur : " + AuthToken.length());

    userID = doc["data"]["user"]["id"].as<String>();
    SHAuserID = getSHA256(userID);
    Serial.println("userID: " + userID);
    if (AuthToken.length() > 100)
      logUserOK = true;
  }
  else
  {
    String S = HEURE + T("LoginFailed") + String(httpCode);
    EcranPrintln(S, RGB565_ORANGE);
  }

  https.end();
  return logUserOK;
}
//============Obtention dernière Glycémie============
void getConnection()
{
  String S;
  HTTPClient https;
  String url = String(baseURL) + "/llu/connections";

  https.begin(url);
  https.setTimeout(15000); // HTTPClient en ms

  https.addHeader("Content-Type", "application/json");
  https.addHeader("Accept", "application/json");
  // Mes rajouts
  https.addHeader("User-Agent", "okhttp/4.9.0");
  https.addHeader("connection", "Keep-Alive");
  https.addHeader("product", "llu.android");
  https.addHeader("version", "4.17.0");

  // Header d’authentification

  https.addHeader("Authorization", "Bearer " + AuthToken);

  https.addHeader("Account-Id", SHAuserID);

  int httpCode = https.GET();

  if (httpCode == HTTP_CODE_OK)
  {
    String response = https.getString();
    Serial.println("Données connexion: " + String(response.length()) + " caractères");

    // StaticJsonDocument<512> doc;
    JsonDocument doc;
    deserializeJson(doc, response);
    ConnectionJSON = response;

    String glycemieStr = doc["data"][0]["glucoseItem"]["ValueInMgPerDl"].as<String>();
    if (glycemieStr == "")
    {
      persons[0].glucoseMgDl = 0;
    }
    else
    {
      persons[0].glucoseMgDl = glycemieStr.toInt();
    }
    String DateGly = doc["data"][0]["glucoseItem"]["Timestamp"].as<String>();
    persons[0].targetLow = doc["data"][0]["targetLow"];
    persons[0].targetHigh = doc["data"][0]["targetHigh"];
    S = HEURE + T("LastGlyco") + formatGlucoseValue(persons[0].glucoseMgDl) + " " + getGlucoseUnitLabel() + " " + T("le") + DateGly;
    EcranPrintln(S);
    persons[0].trendArrow = doc["data"][0]["glucoseItem"]["TrendArrow"].as<int8_t>();
    persons[0].lastReceptionMillis = millis();
    Serial.println();
    Serial.println("TrendArrow : " + String(persons[0].trendArrow));
    Serial.println();
    Serial.println("targetLow : " + String(persons[0].targetLow));
    Serial.println("targetHigh : " + String(persons[0].targetHigh));

    patientId = doc["data"][0]["patientId"].as<String>();

    const char *timestamp = doc["data"][0]["glucoseItem"]["Timestamp"].as<const char *>();
    persons[0].lastGlyUnixTime = convertToUnix(timestamp);
    persons[0].lastOkMillis = millis();
  }
  else
  {
    S = HEURE + T("GlucoFailed") + String(httpCode);
    EcranPrintln(S, RGB565_ORANGE);
  }

  https.end();
}

void LectureGlycemie()
{

  if (UserOK)
  {
    getConnection(); // Dernière valeur de glycémie
    UserOK = false;
  }
  persons[0].recurMillis = RecurrenceGlycemie;
  if (persons[0].ageSeconds > 300)     // Si la dernière glycémie a plus de 5 minutes (300s), on tente une nouvelle lecture
    persons[0].recurMillis = 30000; // 30 secondes pour tenter de récupérer une nouvelle glycémie plus rapidement
  if (persons[0].ageSeconds > 500)     // On ne s'excite pas, on passe à 1.5 minute pour éviter de faire trop de requêtes si le serveur est en panne ou si on a un problème de connexion
    persons[0].recurMillis = 90000;
  if (millis() - persons[0].lastReceptionMillis > persons[0].recurMillis || persons[0].lastDemandeMillis == 0)
  {
    persons[0].lastDemandeMillis = millis(); // Met à jour le temps dernière demande de glycémie avant de faire la lecture
    if (libreEmail != "" && librePass != "")
    {
      Serial.println("On demande une nouvelle glycémie...");
      UserOK = loginLibreLinkUp();
    }
    else
    {
      EcranPrintln(T("LinkUpIndefini"));
    }
    persons[0].lastReceptionMillis = millis(); // Met à jour le temps du dernier relevé reussi ou pas  de glycémie
  }
}

String getSHA256(String payload)
{
  byte shaResult[32];
  mbedtls_md_context_t ctx;
  mbedtls_md_type_t md_type = MBEDTLS_MD_SHA256;

  mbedtls_md_init(&ctx);
  mbedtls_md_setup(&ctx, mbedtls_md_info_from_type(md_type), 0);
  mbedtls_md_starts(&ctx);

  // .c_str() convertit l'objet String en pointeur utilisable par mbedtls
  mbedtls_md_update(&ctx, (const unsigned char *)payload.c_str(), payload.length());

  mbedtls_md_finish(&ctx, shaResult);
  mbedtls_md_free(&ctx);

  // Conversion des 32 octets en une String Hexadécimale de 64 caractères
  String hashStr = "";
  for (int i = 0; i < 32; i++)
  {
    char str[3];
    sprintf(str, "%02x", (int)shaResult[i]);
    hashStr += str;
  }
  return hashStr;
}

void clearLibreViewCache()
{
  Serial.println("Clearing LibreView cache...");
  AuthToken = "";
  userID = "";
  patientId = "";
  SHAuserID = "";
  UserOK = false;
  baseURL = "";
}
