

#ifndef _AHI_H_
#define _AHI_H_

#include <exec/types.h>
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

struct state 
{
  LONG UnitSelected;
  LONG ModeSelected;
  LONG FreqSelected;
  LONG ChannelsSelected;
  LONG InputSelected;
  LONG OutputSelected;
  LONG OutVolSelected;
  LONG MonVolSelected;
  LONG GainSelected;

  LONG Frequencies;
  LONG Channels;
  LONG Inputs;
  LONG Outputs;
  LONG OutVols;
  LONG MonVols;
  LONG Gains;

  BOOL ChannelsDisabled;
  BOOL OutVolMute;
  BOOL MonVolMute;
  BOOL GainMute;

  float OutVolOffset;
  float MonVolOffset;
  float GainOffset;
};

struct args
{
  STRPTR  from;
  ULONG   edit;
  ULONG   use;
  ULONG   save;
  STRPTR  pubscreen;
};

extern char const *Version;

extern struct List *UnitList;
extern struct List *ModeList;

extern char **Units;
extern char **Modes;
extern char **Inputs;
extern char **Outputs;


extern struct state state;
extern struct args args;

extern BOOL SaveIcons;

void NewSettings(char * );
void NewUnit(int );
void NewMode(int );

void FillUnit(void);

char *getFreq(void);
char *getChannels(void);
char *getOutVol(void);
char *getMonVol(void);
char *getGain(void);
char *getInput(void);
char *getOutput(void);
ULONG getAudioMode(void);
char *getRecord(void);
char *getAuthor(void);
char *getCopyright(void);
char *getDriver(void);
char *getVersion(void);

#endif _AHI_H_

