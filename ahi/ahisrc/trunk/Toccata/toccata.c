/*
 * $Id$
 * $Log$
 * Revision 1.1  1997/06/25 19:45:09  lcs
 * Initial revision
 *
 *
 */

#include <devices/ahi.h>
#include <dos/dos.h>
#include <dos/dostags.h>
#include <exec/exec.h>
#include <libraries/toccata.h>

#include <proto/ahi.h>
#include <proto/dos.h>
#include <proto/exec.h>
#include <clib/toccata_protos.h>
#include <pragmas/toccata_pragmas.h>

#include <string.h>

struct toccataprefs {
  UBYTE ID[8];
  LONG  InputVolume;
  LONG  OutputVolume;
  LONG  LoopbackVolume;
  ULONG Input;
  BOOL  MicGain;
  ULONG Mode;
  ULONG Frequency;
};

#define ENVPREFS    "ENV:toccata-emul.prefs"
#define ENVARCPREFS "ENVARC:toccata-emul.prefs"
#define IDCODE    "TOCEMUL"

/* Force 32 bit results */

#define BOOL LONG

/* Arguments in registers */

#define ASM     __asm
#define REG(x)  register __ ## x

/* Externals */

extern long __far _LibVersion;
extern long __far _LibRevision;
extern char __far _LibID[];
extern char __far _LibName[];
extern struct HardInfo hardinfo;

ASM void SlaveTaskEntry(void);

/* Globals */

struct Library        *UtilityBase = NULL;
struct Library        *AHIBase     = NULL;
struct ToccataBase    *ToccataBase = NULL;
struct DosLibrary     *DOSBase     = NULL;

struct Process        *SlaveProcess = NULL;

ULONG MonoMode       = 0x0002000B ^ 0xC0DECAFE;
ULONG StereoMode     = 0x00020007 ^ 0xDEADBEEF;

struct toccataprefs tprefs = {
  IDCODE,
  0,
  0,
  0,
  TINPUT_Line,
  FALSE,
  TMODE_LINEAR_8,
  11025,
};

ULONG error = TIOERR_UNKNOWN;


/*
 *  UserLibInit(): Library init
 */

int ASM __UserLibInit (REG(a6) struct Library *libbase)
{
  ToccataBase = (struct ToccataBase *) libbase;

  ToccataBase->tb_BoardAddress = NULL;

  if(!(DOSBase = (struct DosLibrary *)OpenLibrary("dos.library",37)))
  {
    Alert(AN_Unknown|AG_OpenLib|AO_DOSLib);
    return 1;
  }

  if(!(UtilityBase = OpenLibrary("utility.library",37)))
  {
    Alert(AN_Unknown|AG_OpenLib|AO_UtilityLib);
    return 1;
  }

  Forbid();
  if(SlaveProcess = CreateNewProcTags(
      NP_Entry,     SlaveTaskEntry,
      NP_Name,      _LibName,
      NP_Priority,  1,
      TAG_DONE)) {
    SlaveProcess->pr_Task.tc_UserData = ToccataBase;
  }
  Permit();

  if(SlaveProcess == NULL) {
    return 1;
  }

  return 0;
}

/*
 *  UserLibCleanup(): Library cleanup
 */

void ASM __UserLibCleanup (REG(a6) struct Library *libbase)
{
  if(DOSBase) {
    CloseLibrary((struct Library *)DOSBase);
    DOSBase = NULL;
  }

  if(UtilityBase) {
    CloseLibrary(UtilityBase);
    UtilityBase = NULL;
  }
  
  Signal((struct Task *) SlaveProcess, SIGBREAKF_CTRL_C);
}


/*
 *  HardInfo
 */

struct HardInfo hardinfo;

/*
 *  SlaveTask(): The slave process
 */

void ASM SlaveTask(void) {
  struct MsgPort    *AHImp = NULL;
  struct AHIRequest *AHIio = NULL;
  BYTE AHIDevice = -1;

  ToccataBase->tb_HardInfo = NULL;

  if(AHImp=CreateMsgPort()) {
    if(AHIio=(struct AHIRequest *)CreateIORequest(AHImp,sizeof(struct AHIRequest))) {
      AHIio->ahir_Version = 4;
      AHIDevice = OpenDevice(AHINAME, AHI_NO_UNIT, (struct IORequest *)AHIio, NULL);
    }
  }

  if(AHIDevice == 0) {
    AHIBase = (struct Library *) AHIio->ahir_Std.io_Device;

    hardinfo.hi_Version      = 0;
    hardinfo.hi_Revision     = 0;
    hardinfo.hi_Frequencies  = 14;
    hardinfo.hi_MinFrequency = 5513;
    hardinfo.hi_MaxFrequency = 48000;
    hardinfo.hi_Flags    = 0;

    ToccataBase->tb_HardInfo = &hardinfo;
    Wait(SIGBREAKF_CTRL_C);
  }

  SlaveProcess = NULL;

  if(!AHIDevice)
    CloseDevice((struct IORequest *)AHIio);
  DeleteIORequest((struct IORequest *)AHIio);
  DeleteMsgPort(AHImp);
}


/*
 *  IoErr()
 */

ASM ULONG t_IoErr(void) {
  return error;
}


/*
 *  RawPlayback()
 */

ASM BOOL t_RawPlayback(REG(a0) struct TagItem *tags) {
  return FALSE;
}


/*
 *  SaveSettings()
 */

static BOOL savesettings(STRPTR name) {
  BOOL rc = FALSE;
  BPTR file;

  file = Open(name, MODE_NEWFILE);
  if(file) {
    if(Write(file, &tprefs, sizeof tprefs) == sizeof tprefs) {
      rc = TRUE;
    }
    Close(file);
  }
  return rc;
}

ASM BOOL t_SaveSettings(REG(d0) ULONG flags) {
  BOOL rc = TRUE;

  if(flags == 1) {
    rc = savesettings(ENVARCPREFS);
  }

  if(rc) {
    rc = savesettings(ENVPREFS);
  }

  return rc;
}


/*
 *  LoadSettings()
 */

static BOOL loadsettings(STRPTR name) {
  BOOL rc = FALSE;
  BPTR file;
  struct toccataprefs tempprefs;

  file = Open(name, MODE_OLDFILE);
  if(file) {
    if(Read(file, &tempprefs, sizeof tempprefs) == sizeof tempprefs) {
      if(strcmp(tempprefs.ID, IDCODE) == 0) {
        memcpy(&tprefs, &tempprefs, sizeof tempprefs);
        rc = TRUE;
      }
    }
    Close(file);
  }
  return rc;
}

ASM BOOL t_LoadSettings(REG(d0) ULONG flags) {
  BOOL rc = FALSE;

  if(flags == 1) {
    rc = loadsettings(ENVARCPREFS);
  }
  else if(flags == 0) {
    rc = loadsettings(ENVPREFS);
  }

  T_SetPartTags(
      PAT_InputVolumeLeft,  tprefs.InputVolume,
      PAT_OutputVolumeLeft, tprefs.OutputVolume,
      PAT_LoopbackVolume,   tprefs.LoopbackVolume,
      PAT_Input,            tprefs.Input,
      PAT_MicGain,          tprefs.MicGain,
      PAT_Mode,             tprefs.Mode,
      PAT_Frequency,        tprefs.Frequency,
      TAG_DONE);

  return rc;
}


/*
 *  Expand()
 */

ASM WORD t_Expand(REG(d0) UBYTE value, REG(d1) ULONG mode) {
  return 0;
}


/*
 *  StartLevel()
 */

ASM BOOL t_StartLevel(REG(a0) struct TagItem *tags) {
  return FALSE;
}


/*
 *  Capture()
 */

ASM BOOL t_Capture(REG(a0) struct TagItem *tags) {
  return FALSE;
}


/*
 *  Playback()
 */

ASM BOOL t_Playback(REG(a0) struct TagItem *tags) {
  return FALSE;
}


/*
 *  Pause()
 */

ASM void t_Pause(REG(d0) ULONG pause) {

}


/*
 *  Stop()
 */

ASM void t_Stop(REG(d0) ULONG flags) {

}


/*
 *  StopLevel()
 */

ASM void t_StopLevel(void) {

}


/*
 *  FindFrequency()
 */

ASM ULONG t_FindFrequency(REG(d0) ULONG frequency) {
  ULONG index = 0;
  ULONG freq  = 0;

  

  return freq;
}


/*
 *  NextFrequency()
 */

ASM ULONG t_NextFrequency(REG(d0) ULONG frequency) {
  return 0;
}


/*
 *  SetPart()
 */

ASM void t_SetPart(REG(a0) struct TagItem *tags) {

}


/*
 *  GetPart()
 */

ASM void t_GetPart(REG(a0) struct TagItem *tags) {

}


/*
 *  Open()
 */

ASM struct TocHandle * t_Open(REG(a0) UBYTE *name, REG(a1) struct TagItem *tags) {
  return NULL;
}


/*
 *  Close()
 */

ASM void t_Close(REG(a0) struct TocHandle *handle) {

}


/*
 *  Play()
 */

ASM BOOL t_Play(REG(a0) struct TocHandle *handle, REG(a1) struct TagItem *tags) {
  return FALSE;
}


/*
 *  Record()
 */

ASM BOOL t_Record(REG(a0) struct TocHandle *handle, REG(a1) struct TagItem *tags) {
  return FALSE;
}

/*
 *  Convert()
 */

ASM void t_Convert(REG(a0) APTR src, REG(a1) APTR dest, REG(d0) ULONG samples,
                 REG(d1) ULONG srcmode, REG(d2) ULONG destmode) {

}


/*
 *  BytesPerSample()
 */

ASM ULONG t_BytesPerSample(REG(d0) ULONG mode) {
  const static ULONG table[] = {
    1,    // TMODE_LINEAR_8
    2,    // TMODE_LINEAR_16
    1,    // TMODE_ALAW
    1,    // TMODE_ULAW
    2,    // TMODE_RAW_16
    0,
    0,
    0,
    2,    // TMODE_LINEAR_8_S
    4,    // TMODE_LINEAR_16_S
    2,    // TMODE_ALAW_S
    2,    // TMODE_ULAW_S
    4,    // TMODE_RAW_16_S
    0,
    0,
    0
  };

  return table[mode];
}




/* No documentation available for the following functions */


/*
 *  OpenFile()
 */

ASM struct MultiSoundHandle * t_OpenFile(REG(a0) UBYTE *name, REG(d0) ULONG flags) {
  return NULL;
}


/*
 *  CloseFile()
 */

ASM void t_CloseFile(REG(a0) struct MultiSoundHandle *handle) {

}


/*
 *  ReadFile()
 */

ASM LONG t_ReadFile(REG(a0) struct MultiSoundHandle *handle,
                    REG(a1) UBYTE *dest, REG(d0) ULONG length) {
  return 0;
}





/* No prototypes available for the following functions... */


/*
 *  WriteSmpte()
 */

ASM ULONG t_WriteSmpte(void) {
  return 0;
}


/*
 *  StopSmpte()
 */

ASM ULONG t_StopSmpte(void) {
  return 0;
}


/*
 *  Reserved1()
 */

ASM ULONG t_Reserved1(void) {
  return 0;
}


/*
 *  Reserved2()
 */

ASM ULONG t_Reserved2(void) {
  return 0;
}


/*
 *  Reserved3()
 */

ASM ULONG t_Reserved3(void) {
  return 0;
}


/*
 *  Reserved4()
 */

ASM ULONG t_Reserved4(void) {
  return 0;
}


/*
 *  Own()
 */

ASM void t_Own(void) {
}


/*
 *  Disown()
 */

ASM void t_Disown(void) {
}


/*
 *  SetReg()
 */

ASM void t_SetReg(void) {
}


/*
 *  GetReg()
 */

ASM ULONG t_GetReg(void) {
  return 0;
}
