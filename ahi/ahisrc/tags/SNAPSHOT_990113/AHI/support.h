
#ifndef _SUPPORT_H_
#define _SUPPORT_H_

#include <exec/types.h>
#include <exec/lists.h>
#include <devices/ahi.h>

extern struct AHIGlobalPrefs globalprefs;

BOOL Initialize(void);
void CleanUp(void);

struct List * GetUnits(char * );
struct List * GetModes(struct AHIUnitPrefs * );
char ** List2Array(struct List * );
char ** GetInputs(ULONG );
char ** GetOutputs(ULONG );
BOOL SaveSettings(char * , struct List * );
BOOL WriteIcon(char * );
void FreeList(struct List * );
struct Node * GetNode(int , struct List * );

#endif _SUPPORT_H_
