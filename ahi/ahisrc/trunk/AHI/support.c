/* $Id$
 * $Log$
 * Revision 4.2  1997/04/07 01:36:51  lcs
 * Localized it, bug fixes
 *
 */

#include <devices/ahi.h>
#include <exec/memory.h>
#include <prefs/prefhdr.h>
#include <workbench/workbench.h>
#include <proto/ahi.h>
#include <proto/dos.h>
#include <proto/exec.h>
#include <proto/icon.h>
#include <proto/iffparse.h>
#include <proto/utility.h>
#include <math.h>
#include <stdio.h>
#include <string.h>

#include "ahi.h"
#include "ahiprefs_Cat.h"

#ifndef _GENPROTO
#include "support_protos.h"
#endif

static BOOL AddUnit(struct List *, int);
static void FillUnitName(struct UnitNode *);


struct AHIGlobalPrefs   globalprefs;

struct Library           *AHIBase   = NULL;
static struct MsgPort    *AHImp     = NULL;
static struct AHIRequest *AHIio     = NULL;
static BYTE               AHIDevice = -1;


static char deftoolname[] = {"AHI"};

static UWORD chip DiskI1Data[] =
{
/* Plane */
  0x0000,0x0000,0x0004,0x0000,0x0000,0x0000,0x0001,0x0000,
  0x0000,0x07ff,0x8000,0x4000,0x0000,0x1800,0x6000,0x1000,
  0x0000,0x20fc,0x1000,0x0800,0x0000,0x4102,0x0800,0x0c00,
  0x0000,0x4082,0x0800,0x0c00,0x0000,0x4082,0x0800,0x0c00,
  0x0000,0x2104,0x0800,0x0c00,0x0000,0x1e18,0x1000,0x0c00,
  0x0000,0x0060,0x2000,0x0c00,0x0000,0x0080,0xc000,0x0c00,
  0x0000,0x0103,0x0000,0x0c00,0x0000,0x021c,0x0000,0x0c00,
  0x0000,0x0108,0x0000,0x0c00,0x0000,0x00f0,0x0000,0x0c00,
  0x0000,0x0108,0x0000,0x0c00,0x0000,0x0108,0x0000,0x0c00,
  0x4000,0x00f0,0x0000,0x0c00,0x1000,0x0000,0x0000,0x0c00,
  0x0400,0x0000,0x0000,0x0c00,0x01ff,0xffff,0xffff,0xfc00,
/* Plane */
  0xffff,0xffff,0xfff8,0x0000,0xd555,0x5555,0x5556,0x0000,
  0xd555,0x5000,0x5555,0x8000,0xd555,0x47ff,0x9555,0x6000,
  0xd555,0x5f03,0xe555,0x5000,0xd555,0x3e55,0xf555,0x5000,
  0xd555,0x3f55,0xf555,0x5000,0xd555,0x3f55,0xf555,0x5000,
  0xd555,0x5e53,0xf555,0x5000,0xd555,0x4147,0xe555,0x5000,
  0xd555,0x551f,0xd555,0x5000,0xd555,0x557f,0x1555,0x5000,
  0xd555,0x54fc,0x5555,0x5000,0xd555,0x55e1,0x5555,0x5000,
  0xd555,0x54f5,0x5555,0x5000,0xd555,0x5505,0x5555,0x5000,
  0xd555,0x54f5,0x5555,0x5000,0xd555,0x54f5,0x5555,0x5000,
  0x3555,0x5505,0x5555,0x5000,0x0d55,0x5555,0x5555,0x5000,
  0x0355,0x5555,0x5555,0x5000,0x0000,0x0000,0x0000,0x0000

};

static struct Image image1 =
{
    0,0,
    54,22,2,
    DiskI1Data,
    0x0004,0x0000,
    NULL
};

static char *toolTypes[] = {
  "ACTION=USE",
  NULL
};

static struct DiskObject projIcon = {
  WB_DISKMAGIC,                   /* Magic Number */
  WB_DISKVERSION,                 /* Version */
  {                               /* Embedded Gadget Structure */
    NULL,                         /* Next Gadget Pointer */
    97,12,52,23,                  /* Left,Top,Width,Height */
    GADGIMAGE|GADGHBOX,           /* Flags */
    GADGIMMEDIATE|RELVERIFY,      /* Activation Flags */
    BOOLGADGET,                   /* Gadget Type */
    (APTR)&image1,                /* Render Image */
    NULL,                         /* Select Image */
    NULL,                         /* Gadget Text */
    NULL,                         /* Mutual Exclude */
    NULL,                         /* Special Info */
    0,                            /* Gadget ID */
    NULL                          /* User Data */
  },  
  WBPROJECT,                      /* Icon Type */
  deftoolname,                    /* Default Tool */
  toolTypes,                      /* Tool Type Array */
  NO_ICON_POSITION,               /* Current X */
  NO_ICON_POSITION,               /* Current Y */
  NULL,                           /* Drawer Structure */
  NULL,                           /* Tool Window */
  4096                            /* Stack Size */
};


/******************************************************************************
**** Call to open ahi.device etc. *********************************************
******************************************************************************/

BOOL Initialize(void) {
  if(AHImp=CreateMsgPort()) {
    if(AHIio = (struct AHIRequest *)CreateIORequest(
        AHImp,sizeof(struct AHIRequest))) {
      AHIio->ahir_Version = 4;
      AHIDevice = OpenDevice(AHINAME,AHI_NO_UNIT,(struct IORequest *)AHIio,NULL);
      if(AHIDevice == 0) {
        AHIBase   = (struct Library *)AHIio->ahir_Std.io_Device;
        return TRUE;
      }
    }
  }

  Printf((char *) msgTextNoOpen, "ahi.device", 4);
  Printf("\n");
  return FALSE;
}

/******************************************************************************
**** Call to close ahi.device etc. ********************************************
******************************************************************************/

void CleanUp(void) {
  if(!AHIDevice)
    CloseDevice((struct IORequest *)AHIio);
  DeleteIORequest((struct IORequest *)AHIio);
  DeleteMsgPort(AHImp);

  AHIBase = NULL; AHImp = NULL; AHIio = NULL; AHIDevice = -1;
}


/***** Local function to create a Unit structure  ****************************/

static BOOL AddUnit(struct List *list, int unit) {
  struct UnitNode *u;

  u = AllocVec(sizeof(struct UnitNode), MEMF_CLEAR);

  if(u == NULL) {
    return FALSE;
  }

  u->prefs.ahiup_Unit           = unit;
  u->prefs.ahiup_Channels       = 1;
  u->prefs.ahiup_AudioMode      = AHI_BestAudioID(AHIDB_Realtime, TRUE, TAG_DONE);
  u->prefs.ahiup_Frequency      = 8000;
  u->prefs.ahiup_MonitorVolume  = 0x00000;
  u->prefs.ahiup_InputGain      = 0x10000;
  u->prefs.ahiup_OutputVolume   = 0x10000;
  u->prefs.ahiup_Input          = 0;
  u->prefs.ahiup_Output         = 0;

  FillUnitName(u);

  u->node.ln_Pri = -unit;
  Enqueue(list, (struct Node *) u);

  return TRUE;
}

/***** Local function to create a Unit name **********************************/

static void FillUnitName(struct UnitNode *u) {
  if(u->prefs.ahiup_Unit != AHI_NO_UNIT) {
    sprintf((char *) &u->name, msgUnitDevice, u->prefs.ahiup_Unit);
  }
  else {
    sprintf((char *) &u->name, msgUnitMusic);
  }
  u->node.ln_Name = (char *) &u->name;
}

/******************************************************************************
**** Returns a list with all avalable units, fills globalprefs ****************
******************************************************************************/

#define UNITNODES 4

// Call with name == NULL to get defaults

struct List *GetUnits(char *name) {
  struct List *list;
  struct IFFHandle *iff;
  BOOL devnodes[UNITNODES] = { FALSE, FALSE, FALSE, FALSE } ;
  BOOL lownode = FALSE;
  int i;

  globalprefs.ahigp_MaxCPU = (90 << 16) / 100;

  list = AllocVec(sizeof(struct List), MEMF_CLEAR);
  
  if(list) {
    NewList(list);
    
    if(name && (iff = AllocIFF())) {
      iff->iff_Stream = Open(name, MODE_OLDFILE);
      if(iff->iff_Stream) {
        InitIFFasDOS(iff);
        if(!OpenIFF(iff, IFFF_READ)) {
          if(!(PropChunk      (iff, ID_PREF, ID_AHIG) ||
               CollectionChunk(iff, ID_PREF, ID_AHIU) ||
               StopOnExit     (iff, ID_PREF, ID_FORM))) {
            if(ParseIFF(iff, IFFPARSE_SCAN) == IFFERR_EOC) {
              struct StoredProperty *global = FindProp(iff, ID_PREF, ID_AHIG);
              struct CollectionItem *ci = FindCollection(iff, ID_PREF, ID_AHIU);

              if(global != NULL) {
                CopyMem(global->sp_Data, &globalprefs, 
                    min( sizeof(struct AHIGlobalPrefs), global->sp_Size ));
              }

              while(ci) {
                struct AHIUnitPrefs *p = ci->ci_Data;
                struct UnitNode     *u;

                u = AllocVec(sizeof(struct UnitNode), MEMF_CLEAR);
                if(u == NULL)
                  break;
                CopyMem(p, &u->prefs, 
                    min( sizeof(struct AHIUnitPrefs), ci->ci_Size ));

                FillUnitName(u);
                
                u->node.ln_Pri = -(u->prefs.ahiup_Unit);
                Enqueue(list, (struct Node *) u);
                
                if(u->prefs.ahiup_Unit == AHI_NO_UNIT) {
                  lownode = TRUE;
                }
                else if(u->prefs.ahiup_Unit < UNITNODES) {
                  devnodes[u->prefs.ahiup_Unit] = TRUE;
                }
                
                ci=ci->ci_Next;
              }
            }
          }
          CloseIFF(iff);
        }
        Close(iff->iff_Stream);
      }
      FreeIFF(iff);
    }


    // Fill up to lowlevel + UNITNODES device nodes, if not found in prefs file

    if(!lownode) AddUnit(list, AHI_NO_UNIT);
    for(i = 0; i < UNITNODES; i++) {
      if(!devnodes[i]) AddUnit(list, i);
    }

  }

  return list;
}

/******************************************************************************
**** Returns a list with all available modes **********************************
******************************************************************************/

struct List *GetModes(struct AHIUnitPrefs *prefs) {
  struct List *list;

  list = AllocVec(sizeof(struct List), MEMF_CLEAR);
  
  if(list) {
    ULONG id = AHI_NextAudioID(AHI_INVALID_ID);

    NewList(list);

    while(id != AHI_INVALID_ID) {
      struct ModeNode *t;
      struct Node     *node;
      
      if(t = AllocVec( sizeof(struct ModeNode), MEMF_CLEAR)) {
        LONG realtime;

        t->node.ln_Name = t->name;
        t->ID = id;
        
        realtime = FALSE;
        
        AHI_GetAudioAttrs(id, NULL,
            AHIDB_BufferLen,80,
            AHIDB_Name, t->node.ln_Name,
            AHIDB_Realtime, &realtime,
            TAG_DONE);

        if((prefs->ahiup_Unit == AHI_NO_UNIT) || realtime ) {
          // Insert node alphabetically
          for(node = list->lh_Head;
              node->ln_Succ;
              node = node->ln_Succ) {
            if(Stricmp(t->node.ln_Name,node->ln_Name) < 0)
              break;
          }
          Insert(list, (struct Node *) t, node->ln_Pred);
        }
        else {
          FreeVec(t);
        }
      }
      id = AHI_NextAudioID(id);
    }
  }
  return list;
}

/******************************************************************************
**** Creates a char* array from a list ****************************************
******************************************************************************/

char **List2Array(struct List *list) {
  char **strings, **rstrings;
  int i;
  struct Node *n;

  for(n = list->lh_Head, i = 0; n->ln_Succ; n = n->ln_Succ) {
    i++;
  }

  strings = AllocVec(sizeof(char *) * (i + 1), MEMF_CLEAR);
  rstrings = strings;

  if(strings) {
    for(n = list->lh_Head; n->ln_Succ; n = n->ln_Succ) {
      *strings++ = n->ln_Name;
    }
    *strings = NULL;
  }
  return rstrings;
}

/******************************************************************************
**** Returns a char* array with inputs names **********************************
******************************************************************************/

char **GetInputs(ULONG id) {
  char **strings, **rstrings;
  LONG inputs = 0, i;


  AHI_GetAudioAttrs(id, NULL,
      AHIDB_Inputs, &inputs,
      TAG_DONE);

  strings = AllocVec(sizeof(char *) * (inputs + 1) + (32 * inputs), MEMF_CLEAR);
  rstrings = strings;

  if(strings) {
    char *string = (char *) &strings[inputs + 1];

    for(i = 0; i < inputs; i++) {
      if(AHI_GetAudioAttrs(id, NULL,
          AHIDB_BufferLen, 32,
          AHIDB_InputArg,  i,
          AHIDB_Input,     string,
          TAG_DONE)) {
        *strings++ = string;
        while(*string++);
      }
    }
    *strings = NULL;
  }
  return rstrings;
}

/******************************************************************************
**** Returns a char* array with outputs names *********************************
******************************************************************************/

char **GetOutputs(ULONG id) {
  char **strings, **rstrings;
  LONG outputs = 0, i;


  AHI_GetAudioAttrs(id, NULL,
      AHIDB_Outputs, &outputs,
      TAG_DONE);

  strings = AllocVec(sizeof(char *) * (outputs + 1) + (32 * outputs), MEMF_CLEAR);
  rstrings = strings;
  
  if(strings) {
    char *string = (char *) &strings[outputs + 1];

    for(i = 0; i < outputs; i++) {
      if(AHI_GetAudioAttrs(id, NULL,
          AHIDB_BufferLen, 32,
          AHIDB_OutputArg,  i,
          AHIDB_Output,     string,
          TAG_DONE)) {
        *strings++ = string;
        while(*string++);
      }
    }
    *strings = NULL;
  }
  return rstrings;
}

/******************************************************************************
**** Saves all units and globalprefs ******************************************
******************************************************************************/

BOOL SaveSettings(char *name, struct List *list) {
  struct IFFHandle  *iff;
  struct PrefHeader  header = { 0, 0, 0 };
  struct Node       *n;
  BOOL               success = FALSE;

  if(name && (iff = AllocIFF())) {
    iff->iff_Stream = Open(name, MODE_NEWFILE);
    if(iff->iff_Stream) {
      InitIFFasDOS(iff);
      if(!OpenIFF(iff, IFFF_WRITE)) {
        if(! PushChunk(iff, ID_PREF, ID_FORM, IFFSIZE_UNKNOWN)) {

          success = TRUE;

          // Prefs header
          if(! PushChunk(iff, ID_PREF, ID_PRHD, sizeof header)) {
            WriteChunkBytes(iff, &header, sizeof header);
            PopChunk(iff);
          }
          else success = FALSE;

          // Global prefs
          if(! PushChunk(iff, ID_PREF, ID_AHIG, sizeof globalprefs)) {
            WriteChunkBytes(iff, &globalprefs, sizeof globalprefs);
            PopChunk(iff);
          }
          else success = FALSE;

          // Units
          if(list != NULL) {    
            for(n = list->lh_Head; n->ln_Succ; n = n->ln_Succ) {
              if(! PushChunk(iff, ID_PREF, ID_AHIU, sizeof(struct AHIUnitPrefs))) {
                WriteChunkBytes(iff, &((struct UnitNode *) n)->prefs,
                    sizeof(struct AHIUnitPrefs));
                PopChunk(iff);
              }
              else success = FALSE;
            }
          }
        }
        CloseIFF(iff);
      }
      Close(iff->iff_Stream);
    }
    FreeIFF(iff);
  }
  return success;
}


/******************************************************************************
**** Write a project icon to disk *********************************************
******************************************************************************/

BOOL WriteIcon(char *name) {
  struct DiskObject *dobj;
  char *olddeftool;
  char **oldtooltypes;
  BOOL success = FALSE;

  /* Use the already present icon */
  if(dobj=GetDiskObject(name)) {
    oldtooltypes = dobj->do_ToolTypes;
    olddeftool = dobj->do_DefaultTool;

    dobj->do_ToolTypes = toolTypes;
    dobj->do_DefaultTool = deftoolname;

    success = PutDiskObject(name,dobj);

    /* we must restore the original pointers before freeing */
    dobj->do_ToolTypes = oldtooltypes;
    dobj->do_DefaultTool = olddeftool;
    FreeDiskObject(dobj);
  }

  /* Try the user's default prefs icon */
  if((! success) && (dobj=GetDiskObject("ENV:Sys/def_Pref"))) {
    oldtooltypes = dobj->do_ToolTypes;
    olddeftool = dobj->do_DefaultTool;

    dobj->do_ToolTypes = toolTypes;
    dobj->do_DefaultTool = deftoolname;

    success = PutDiskObject(name,dobj);

    /* we must restore the original pointers before freeing */
    dobj->do_ToolTypes = oldtooltypes;
    dobj->do_DefaultTool = olddeftool;
    FreeDiskObject(dobj);
  }

  /* Else, put our default icon */
  if(! success) {
    success = PutDiskObject(name,&projIcon);
  }

  return success;
}


/******************************************************************************
**** Frees a list and all nodes ***********************************************
******************************************************************************/

void FreeList(struct List *list) {
  struct Node *n;

  if(list == NULL)
    return;

  while(n = RemHead(list)) {
    FreeVec(n);
  }
  FreeVec(list);
}

/******************************************************************************
**** Returns the index:th node in a list  *************************************
******************************************************************************/

struct Node *GetNode(int index, struct List *list) {
  struct Node *n;

  if(list == NULL)
    return NULL;
    
  for(n = list->lh_Head; n->ln_Succ; n = n->ln_Succ) {
    if(index == 0) {
      break;
    }
    index--;
  }
  return n;
}
