#include <Arduino.h>
#include <Heure.h>
#include <HTTPClient.h>
#include <ArduinoJson.h>
#include "mbedtls/md.h"
#include "Config.h"
#include "Libreview.h"
#include "Langues/Langue.h"

// ==============================================================
// LibreLinkUp: ONE follower account (global libreEmail/librePass)
// sees ALL followed patients. A single /llu/connections call
// returns every patient's latest glucose, which we distribute to
// the SENSOR_LIBRE persons by matching first names.
// ==============================================================

// Account-level session cache. The auth token is valid for months:
// we log in once and only again when the server rejects the token.
// (LibreLinkUp rate-limits logins and can lock accounts that
// re-authenticate on every poll, so never log in per reading.)
static String AuthToken = "";
static String SHAuserID = "";
static String baseURL = "";

// Account-level back-off after a server error (shared by all persons,
// since all Libre persons are served by the same requests).
static unsigned long libreBackoffUntilMillis = 0;

// One-shot warnings (avoid repeating the same message every poll)
static bool warnedNoAccount = false;
static bool warnedNoMatch[MAX_PERSONS] = {false};

String getSHA256(String payload);

static void addLibreHeaders(HTTPClient &https)
{
  https.addHeader("Content-Type", "application/json");
  https.addHeader("Accept", "application/json");
  https.addHeader("User-Agent", "okhttp/4.9.0");
  https.addHeader("connection", "Keep-Alive");
  https.addHeader("product", "llu.android");
  https.addHeader("version", "4.17.0"); // LibreLinkUp app version (server enforces a minimum)
}

// Lowercase + strip French/Latin accents + drop non-ASCII, so that
// "Léa" matches "lea" and screen names match LibreView first names.
static String normalizeName(const String &s)
{
  String out = "";
  for (unsigned int i = 0; i < s.length(); i++)
  {
    unsigned char c = s[i];
    if (c == 0xC3 && i + 1 < s.length())
    {
      // UTF-8 Latin-1 supplement (2 bytes): map accented letter to its base letter
      unsigned char d = s[++i];
      if ((d >= 0x80 && d <= 0x86) || (d >= 0xA0 && d <= 0xA6)) out += 'a';
      else if (d == 0x87 || d == 0xA7) out += 'c';
      else if ((d >= 0x88 && d <= 0x8B) || (d >= 0xA8 && d <= 0xAB)) out += 'e';
      else if ((d >= 0x8C && d <= 0x8F) || (d >= 0xAC && d <= 0xAF)) out += 'i';
      else if (d == 0x91 || d == 0xB1) out += 'n';
      else if ((d >= 0x92 && d <= 0x96) || (d >= 0xB2 && d <= 0xB6)) out += 'o';
      else if ((d >= 0x99 && d <= 0x9C) || (d >= 0xB9 && d <= 0xBC)) out += 'u';
    }
    else if (c < 0x80)
    {
      out += (char)tolower(c);
    }
  }
  out.trim();
  return out;
}

bool loginLibreLinkUp()
{
  ServerConnu = false;
  AuthToken = "";
  SHAuserID = "";

  if (baseURL == "")
  {
    baseURL = "https://api.libreview.io";
    if (libreZone != "")
      baseURL = "https://api-" + libreZone + ".libreview.io";
  }

  // Two attempts max: the first may answer "redirect" with the right
  // regional server, in which case we retry once on that server.
  for (int attempt = 0; attempt < 2; attempt++)
  {
    Serial.println("Connexion à LibreLinkUp: " + baseURL);
    HTTPClient https;
    https.begin(baseURL + "/llu/auth/login");
    https.setTimeout(15000);
    addLibreHeaders(https);

    String payload = "{\"email\":\"" + libreEmail + "\",\"password\":\"" + librePass + "\"}";
    int httpCode = https.POST(payload);
    String response = https.getString();
    https.end();
    LoginJSON = response;

    if (httpCode != HTTP_CODE_OK)
    {
      Serial.println("Login LibreLinkUp échoué: " + String(httpCode));
      Serial.println("Réponse: " + response);
      EcranPrintln(HEURE + T("LoginFailed") + String(httpCode), RGB565_ORANGE);
      // Server-side error, connection failure or rate-limit: back off >=120 s
      // (same policy as Dexcom).
      if (httpCode >= 500 || httpCode <= 0 || httpCode == 429)
        libreBackoffUntilMillis = millis() + 120000;
      return false;
    }

    ServerConnu = true;

    JsonDocument doc;
    if (deserializeJson(doc, response))
    {
      Serial.println("Erreur parsing JSON login LibreLinkUp");
      return false;
    }

    // Regional redirect: the server names the region our account lives in.
    if (doc["data"]["redirect"] | false)
    {
      String region = doc["data"]["region"].as<String>();
      if (region != "" && attempt == 0)
      {
        baseURL = "https://api-" + region + ".libreview.io";
        Serial.println("Redirection LibreLinkUp vers la région: " + region);
        continue;
      }
      Serial.println("Redirection LibreLinkUp invalide");
      return false;
    }

    // status 0 = OK; 2 = bad credentials; 4 = new terms of use must be
    // accepted once in the LibreLinkUp phone app before the API works again.
    int status = doc["status"] | 0;
    if (status != 0)
    {
      Serial.println("LibreLinkUp status: " + String(status));
      if (status == 4)
        EcranPrintln(HEURE + T("LibreToU"), RGB565_ORANGE);
      else
        EcranPrintln(HEURE + T("LoginFailed") + "status " + String(status), RGB565_ORANGE);
      return false;
    }

    AuthToken = doc["data"]["authTicket"]["token"].as<String>();
    String userID = doc["data"]["user"]["id"].as<String>();
    SHAuserID = getSHA256(userID);
    Serial.println("Login LibreLinkUp OK, token: " + String(AuthToken.length()) + " caractères");
    return AuthToken.length() > 100;
  }
  return false;
}

// One GET /llu/connections refreshes EVERY followed patient at once.
static void getLibreConnections()
{
  HTTPClient https;
  https.begin(baseURL + "/llu/connections");
  https.setTimeout(15000);
  addLibreHeaders(https);
  https.addHeader("Authorization", "Bearer " + AuthToken);
  https.addHeader("Account-Id", SHAuserID);

  int httpCode = https.GET();
  String response = https.getString();
  https.end();

  if (httpCode != HTTP_CODE_OK)
  {
    Serial.println("Erreur lecture LibreLinkUp: " + String(httpCode));
    EcranPrintln(HEURE + T("GlucoFailed") + String(httpCode), RGB565_ORANGE);
    // Token rejected: drop it so the next poll re-authenticates.
    if (httpCode == 401)
      AuthToken = "";
    if (httpCode >= 500 || httpCode <= 0 || httpCode == 429)
      libreBackoffUntilMillis = millis() + 120000;
    return;
  }

  ConnectionJSON = response;
  JsonDocument doc;
  if (deserializeJson(doc, response))
  {
    Serial.println("Erreur parsing JSON connections LibreLinkUp");
    return;
  }
  JsonArray connections = doc["data"].as<JsonArray>();
  Serial.println("LibreLinkUp: " + String(connections.size()) + " patient(s) suivi(s)");

  // Count the Libre persons first: with exactly one person and one
  // patient, we match them regardless of names (nothing to disambiguate).
  int librePersons = 0;
  for (int i = 0; i < MAX_PERSONS; i++)
    if (persons[i].configured && persons[i].sensorType == SENSOR_LIBRE)
      librePersons++;

  for (int i = 0; i < MAX_PERSONS; i++)
  {
    Person &person = persons[i];
    if (!person.configured || person.sensorType != SENSOR_LIBRE)
      continue;

    String target = normalizeName(person.name);
    JsonObject match;

    for (JsonObject conn : connections)
    {
      // A previous match pinned the patientId: reuse it (robust to renames).
      if (person.librePatientId != "" && person.librePatientId == conn["patientId"].as<String>())
      {
        match = conn;
        break;
      }
      String first = normalizeName(conn["firstName"].as<String>());
      // First-name match, accent/case-insensitive; prefix tolerated both
      // ways ("lea" vs "lea-marie", screen name "papa d." vs "papa").
      if (target != "" && first != "" &&
          (first == target || first.startsWith(target) || target.startsWith(first)))
      {
        match = conn;
        break;
      }
    }

    if (match.isNull() && librePersons == 1 && connections.size() == 1)
      match = connections[0];

    if (match.isNull())
    {
      if (!warnedNoMatch[i])
      {
        warnedNoMatch[i] = true;
        EcranPrintln(HEURE + T("LibreNoMatch") + person.name, RGB565_ORANGE);
        for (JsonObject conn : connections)
          Serial.println("  Patient disponible: " + conn["firstName"].as<String>() + " " + conn["lastName"].as<String>());
      }
      continue;
    }
    warnedNoMatch[i] = false;

    person.librePatientId = match["patientId"].as<String>();

    JsonObject g = match["glucoseItem"];
    person.glucoseMgDl = g["ValueInMgPerDl"] | 0;
    // Libre trend scale (1=falling fast ... 3=stable ... 5=rising fast)
    // maps 1:1 onto the display scale used for Dexcom (1=Down ... 5=Up).
    person.trendArrow = g["TrendArrow"] | 0;
    person.targetLow = match["targetLow"] | person.targetLow;
    person.targetHigh = match["targetHigh"] | person.targetHigh;

    const char *timestamp = g["Timestamp"]; // "M/D/YYYY H:MM:SS AM"
    if (timestamp != nullptr)
      person.lastGlyUnixTime = convertToUnix(timestamp);

    // The API answered for this patient: the acquisition chain is alive,
    // even during a sensor gap (value 0) — same rationale as Dexcom's
    // no-glucose reboot protection.
    person.lastOkMillis = millis();

    EcranPrintln(HEURE + person.name + T("LastGlyco") + formatGlucoseValue(person.glucoseMgDl) +
                 " " + getGlucoseUnitLabel() + " " + T("le") + unixToTimestamp(person.lastGlyUnixTime));
    Serial.println(person.name + ": " + formatGlucoseValue(person.glucoseMgDl) + " " +
                   getGlucoseUnitLabel() + ", TrendArrow " + String(person.trendArrow));
  }
}

void LectureLibre()
{
  // Adaptive polling: is any Libre person due for a refresh?
  bool anyLibre = false;
  bool due = false;
  for (int i = 0; i < MAX_PERSONS; i++)
  {
    Person &person = persons[i];
    if (!person.configured || person.sensorType != SENSOR_LIBRE)
      continue;
    anyLibre = true;

    // Base cadence 2 min (Libre pushes ~every minute); retry faster when
    // the reading is overdue, then relax if the server looks down.
    person.recurMillis = RecurrenceGlycemie;
    if (person.ageSeconds > 300)
      person.recurMillis = 30000;
    if (person.ageSeconds > 500)
      person.recurMillis = 90000;

    if (person.lastDemandeMillis == 0 ||
        millis() - person.lastReceptionMillis > person.recurMillis)
      due = true;
  }
  if (!anyLibre || !due)
    return;

  if (libreEmail == "" || librePass == "")
  {
    if (!warnedNoAccount)
    {
      warnedNoAccount = true;
      EcranPrintln(T("LinkUpIndefini"), RGB565_ORANGE);
    }
    return;
  }

  // Respect a server-requested back-off (rollover-safe comparison).
  if (libreBackoffUntilMillis != 0 &&
      (long)(libreBackoffUntilMillis - millis()) > 0)
    return;

  // One shared poll serves every Libre person: stamp them all.
  for (int i = 0; i < MAX_PERSONS; i++)
  {
    if (persons[i].configured && persons[i].sensorType == SENSOR_LIBRE)
      persons[i].lastDemandeMillis = millis();
  }

  Serial.println("On demande les glycémies LibreLinkUp...");
  if (AuthToken == "" && !loginLibreLinkUp())
  {
    // Login failed: still stamp the attempt so the adaptive intervals apply.
    for (int i = 0; i < MAX_PERSONS; i++)
      if (persons[i].configured && persons[i].sensorType == SENSOR_LIBRE)
        persons[i].lastReceptionMillis = millis();
    return;
  }

  getLibreConnections();

  for (int i = 0; i < MAX_PERSONS; i++)
    if (persons[i].configured && persons[i].sensorType == SENSOR_LIBRE)
      persons[i].lastReceptionMillis = millis();
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
  SHAuserID = "";
  baseURL = "";
  libreBackoffUntilMillis = 0;
  warnedNoAccount = false;
  for (int i = 0; i < MAX_PERSONS; i++)
  {
    persons[i].librePatientId = "";
    warnedNoMatch[i] = false;
  }
}
