
#include <devices/ahi.h>

struct UnitNode {
  struct Node           node;
  char                  name[32];
  struct AHIUnitPrefs   prefs;
};

struct ModeNode {
  struct Node           node;
  ULONG                 ID;
  char                  name[80];
};

#define HELPFILE    "ahi.guide"
#define ENVARCFILE  "ENVARC:Sys/ahi.prefs"
#define ENVFILE     "ENV:Sys/ahi.prefs"

