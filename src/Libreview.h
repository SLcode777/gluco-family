#pragma once
#include "Config.h"
#include <Arduino.h>
#include "Heure.h"
#include "Ecran/Gestion.h"

// LibreLinkUp API functions (one follower account, multi-patient)
void LectureLibre();
bool loginLibreLinkUp();
void clearLibreViewCache();
