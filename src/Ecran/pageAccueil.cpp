#include "Ecran/pageAccueil.h"
#include "Config.h"
#include <Arduino.h>
#include "Ecran/Gestion.h"
#include "time.h"
#include "Langues/Langue.h"

// Layout constants
#define ZONE_COUNT       MAX_PERSONS
#define GAUGE_WIDTH_PX   5
#define GAUGE_MARGIN_PX  3   // gap between gauge and right edge

// Color thresholds (mg/dL) — same as legacy
static const int16_t HYPER_CRITICAL = 300;

// ---------- Helpers ----------

// Returns the glucose value's display color based on the person's targets.
// White if not configured to use colors, otherwise blue/green/orange/red.
static uint16_t glucoseColorForValue(int16_t mgdl, int16_t targetLow, int16_t targetHigh) {
    if (glucoseColor != GLUCOSE_COULEUR) return RGB565_WHITE;
    if (mgdl < targetLow)              return RGB565_BLUE;
    if (mgdl < targetHigh)             return RGB565_GREEN;
    if (mgdl < HYPER_CRITICAL)         return RGB565_ORANGE;
    return RGB565_RED;
}

// Draws the Dexcom trend arrow centered at (cx, cy), scaled by `scale`
// (1.0 = compact base size). Trend codes:
//   -1 DoubleDown, 1 SingleDown, 2 FortyFiveDown, 3 Flat,
//    4 FortyFiveUp, 5 SingleUp, 6 DoubleUp, 0 unknown (no draw)
static void drawArrow(Arduino_Canvas *canva, int16_t cx, int16_t cy, int8_t trend, uint16_t color, float scale) {
    if (trend == 0) return; // unknown trend → no arrow

    // Base coordinates relative to (cx, cy), multiplied by `scale` before drawing
    int16_t x0, y0, x1, y1, x2, y2, x3, y3, x4, y4;
    const int16_t offset = (int16_t)(22 * scale); // spacing between double arrows

    switch (trend) {
        case -1: // DoubleDown
        case 1:  // SingleDown
            x0 = -11; y0 = 0;  x1 = 0;  y1 = 11;  x2 = 11; y2 = 0;
            x3 = -5;  y3 = -27; x4 = 5; y4 = -27;
            break;
        case 2:  // FortyFiveDown
            x0 = 0;  y0 = 11; x1 = 11; y1 = 11; x2 = 11; y2 = 0;
            x3 = -16; y3 = -22; x4 = -22; y4 = -16;
            break;
        case 3:  // Flat
            x0 = 0;  y0 = 11; x1 = 11; y1 = 0; x2 = 0; y2 = -11;
            x3 = -27; y3 = -5; x4 = -27; y4 = 5;
            break;
        case 4:  // FortyFiveUp
            x0 = 11; y0 = 0;  x1 = 11; y1 = -11; x2 = 0; y2 = -11;
            x3 = -16; y3 = 22; x4 = -22; y4 = 16;
            break;
        case 5:  // SingleUp
        case 6:  // DoubleUp
            x0 = 11; y0 = 0;  x1 = 0;  y1 = -11; x2 = -11; y2 = 0;
            x3 = -5;  y3 = 27; x4 = 5;  y4 = 27;
            break;
        default:
            return;
    }

    // Apply scale to all coordinates
    x0 *= scale; y0 *= scale; x1 *= scale; y1 *= scale; x2 *= scale; y2 *= scale;
    x3 *= scale; y3 *= scale; x4 *= scale; y4 *= scale;

    canva->fillTriangle(cx + x0, cy + y0, cx + x1, cy + y1, cx + x2, cy + y2, color);
    canva->fillTriangle(cx + x3, cy + y3, cx + x1, cy + y1, cx + x4, cy + y4, color);

    // Second arrow for DoubleDown / DoubleUp
    if (trend == -1 || trend == 6) {
        int16_t dx = -offset;
        canva->fillTriangle(cx + x0 + dx, cy + y0, cx + x1 + dx, cy + y1, cx + x2 + dx, cy + y2, color);
        canva->fillTriangle(cx + x3 + dx, cy + y3, cx + x1 + dx, cy + y1, cx + x4 + dx, cy + y4, color);
    }
}

// Draws the vertical age gauge on the right edge of the zone.
// Fill cycle: 0 → 5 min the bar fills (white). After 5 min it stays full
// and changes color: orange (5-15 min), red (>15 min or no measure).
static void drawAgeGauge(Arduino_Canvas *canva, int16_t xLeft, int16_t yTop, int16_t yBottom,
                        long ageSeconds, bool hasMeasure) {
    const int16_t zoneHeight = yBottom - yTop;
    const int16_t FILL_PERIOD_S = 300;   // 5 min
    const int16_t ORANGE_THRESHOLD_S = 300;
    const int16_t RED_THRESHOLD_S = 900; // 15 min

    int16_t fillHeight;
    uint16_t color;

    if (!hasMeasure) {
        // No measure yet → full red gauge
        fillHeight = zoneHeight;
        color = RGB565_RED;
    } else if (ageSeconds < FILL_PERIOD_S) {
        // Normal cycle: filling, white
        fillHeight = (int16_t)((long)zoneHeight * ageSeconds / FILL_PERIOD_S);
        color = RGB565_WHITE;
    } else if (ageSeconds < RED_THRESHOLD_S) {
        // Refresh late: full, orange
        fillHeight = zoneHeight;
        color = RGB565_ORANGE;
    } else {
        // Critical: full, red
        fillHeight = zoneHeight;
        color = RGB565_RED;
    }

    // Draw gauge background (dark grey thin line, full height)
    canva->fillRect(xLeft, yTop, GAUGE_WIDTH_PX, zoneHeight, C_grisFonce);
    // Draw filled portion from bottom up
    if (fillHeight > 0) {
        canva->fillRect(xLeft, yBottom - fillHeight, GAUGE_WIDTH_PX, fillHeight, color);
    }
}

// Draws one person's zone (or empty placeholder if not configured).
static void drawPersonZone(Arduino_Canvas *canva, int zoneIndex) {
    const int16_t zoneHeight = EcranH / ZONE_COUNT;
    const int16_t yTop = zoneIndex * zoneHeight;
    const int16_t yBottom = yTop + zoneHeight;
    const int16_t yCenter = yTop + zoneHeight / 2;

    // Separator line (skip for the first zone)
    if (zoneIndex > 0) {
        canva->drawFastHLine(0, yTop, EcranW, C_grisFonce);
    }

    Person& p = persons[zoneIndex];
    const int16_t gaugeX = EcranW - GAUGE_MARGIN_PX - GAUGE_WIDTH_PX;

    if (!p.configured) {
        // Empty zone: just the separator and a very subtle placeholder
        canva->setFont(u8g2_font_helvR10_tf);
        canva->setTextColor(C_grisMoyen);
        PrintCentre(canva, T("PersonEmpty"), EcranW / 2, yCenter, 1);
        return;
    }

    // ---- Name (top-left, small) ----
    canva->setFont(u8g2_font_helvB14_tf);
    canva->setTextColor(RGB565_WHITE);
    String displayName = (p.name.length() > 0) ? p.name : String("Person ") + String(zoneIndex + 1);
    PrintGauche(canva, displayName, 6, yTop + 16, 1);

    bool hasMeasure = (p.glucoseMgDl > 0);
    if (hasMeasure) {
        // ---- Glucose value: huge, left-aligned, baseline near zone bottom ----
        uint16_t valueColor = glucoseColorForValue(p.glucoseMgDl, p.targetLow, p.targetHigh);
        canva->setTextColor(valueColor);
        canva->setFont(u8g2_font_logisoso92_tn); // big → swap to _logisoso78_tn for smaller
        PrintGauche(canva, formatGlucoseValue(p.glucoseMgDl), 6, yBottom - 18, 1);

        // ---- Unit label (small, top-right near gauge) ----
        canva->setFont(u8g2_font_helvR10_tf);
        canva->setTextColor(RGB565_WHITE);
        PrintDroite(canva, getGlucoseUnitLabel(), gaugeX - 6, yTop + 16, 1);

        // ---- Arrow (right side, tall, vertically aligned with the value) ----
        drawArrow(canva, gaugeX - 50, yCenter + 15, p.trendArrow, RGB565_WHITE, 2.0);
    } else {
        // No data yet
        canva->setFont(u8g2_font_helvB18_tf);
        canva->setTextColor(C_grisMoyen);
        PrintCentre(canva, T("WaitGluco"), EcranW / 2, yCenter + 8, 1);
    }

    // ---- Age gauge (right edge) ----
    drawAgeGauge(canva, gaugeX, yTop + 2, yBottom - 2, p.ageSeconds, hasMeasure);
}

// ---------- Page lifecycle ----------

void AccueilInit() {
    // Nothing to init for now
}

void AccueiLoop() {
    CanvaAccueil->fillScreen(RGB565_BLACK);

    // Update per-person age in seconds (used by gauge and color logic)
    if (HeureValide) {
        time_t now;
        time(&now);
        for (int i = 0; i < MAX_PERSONS; i++) {
            if (persons[i].lastGlyUnixTime > 0) {
                persons[i].ageSeconds = (long)now - persons[i].lastGlyUnixTime;
            }
        }
    }

    for (int i = 0; i < ZONE_COUNT; i++) {
        drawPersonZone(CanvaAccueil, i);
    }
}