/* $Id$
* $Log$
* Revision 1.20  1997/03/27 12:16:27  lcs
* Major bug in the device interface code fixed.
*
* Revision 1.19  1997/03/20 02:07:02  lcs
* Weiﬂ nicht?
*
* Revision 1.18  1997/03/15 09:51:52  lcs
* Dynamic sample loading in the device: No more alignment restrictions.
*
* Revision 1.17  1997/03/13 00:19:43  lcs
* Up to 4 device units are now available.
*
* Revision 1.16  1997/03/01 21:36:04  lcs
* Added docs for OpenDevice() and CloseDevice().
*
* Revision 1.15  1997/02/18 22:26:49  lcs
* Added flag for OpenDevice(): AHIDF_NOMODESCAN
*
* Revision 1.14  1997/02/12 15:32:45  lcs
* Moved each autodoc header to the file where the function is
*
* Revision 1.13  1997/02/10 02:23:06  lcs
* Infowindow in the requester added.
*
* Revision 1.12  1997/02/04 15:44:30  lcs
* AHIDB_MaxChannels didn't work in AHI_BestAudioID()
*
* Revision 1.11  1997/02/02 22:35:50  lcs
* Localized it
*
* Revision 1.10  1997/02/02 18:15:04  lcs
* Added protection against CPU overload
*
* Revision 1.9  1997/02/01 23:54:26  lcs
* Rewrote the library open code in C and removed the library bases
* from AHIBase
*
* Revision 1.8  1997/02/01 19:44:18  lcs
* Added stereo samples
*
* Revision 1.6  1997/01/15 14:59:50  lcs
* Added CMD_FLUSH, CMD_START, CMD_STOP and SMD_RESET
*
* Revision 1.5  1997/01/05 13:38:01  lcs
* Prettified the code a bit... ;)
*
* Revision 1.4  1997/01/04 20:19:56  lcs
* Changed the AHI_DEBUG levels
* CMD_WRITE seem to work as supposed now
*
* Revision 1.3  1997/01/04 13:26:41  lcs
* Debugged CMD_WRITE
*
* Revision 1.2  1996/12/21 23:06:35  lcs
* "Finished" the code for CMD_WRITE
*
* Revision 1.1  1996/12/21 13:05:12  lcs
* Initial revision
*
*/


//#define DEBUG

#include "ahi_def.h"
#include "localize.h"

#include <exec/alerts.h>
#include <exec/errors.h>
#include <exec/io.h>
#include <exec/devices.h>
#include <exec/memory.h>
#include <dos/dos.h>
#include <dos/dostags.h>
#include <libraries/iffparse.h>
#include <prefs/prefhdr.h>

#include <proto/exec.h>
#include <proto/dos.h>
#include <proto/iffparse.h>

#include <stddef.h>

#ifndef  noprotos
#ifndef _GENPROTO
#include "device_protos.h"
#endif

#include "devcommands_protos.h"
#endif

__asm BOOL OpenLibs(register __a6 struct ExecBase *);
__asm void CloseLibs(register __a6 struct ExecBase *);
static struct AHIDevUnit *InitUnit(ULONG , struct AHIBase *);
static void ExpungeUnit(struct AHIDevUnit *, struct AHIBase *);
BOOL ReadConfig( struct AHIDevUnit *, struct AHIBase *);
BOOL AllocHardware(struct AHIDevUnit *,struct AHIBase *);
void FreeHardware(struct AHIDevUnit *,struct AHIBase *);
static __asm __interrupt void PlayerFunc(
    register __a0 struct Hook *,
    register __a2 struct AHIAudioCtrl *,
    register __a1 APTR);
static __asm __interrupt BOOL RecordFunc(
    register __a0 struct Hook *,
    register __a2 struct AHIAudioCtrl *,
    register __a1 struct AHIRecordMessage *);
static __asm __interrupt void SoundFunc(
    register __a0 struct Hook *,
    register __a2 struct AHIAudioCtrl *,
    register __a1 struct AHISoundMessage *);
static __asm __interrupt void ChannelInfoFunc(
    register __a0 struct Hook *,
    register __a2 struct AHIAudioCtrl *,
    register __a1 struct AHIEffChannelInfo *);

extern __asm BPTR DevExpunge( register __a6 struct AHIBase * );
extern __asm void DevProcEntry(void);


/******************************************************************************
** OpenLibs *******************************************************************
******************************************************************************/

// This function is called by the device startup code when the device is
// first loaded into memory.

extern struct timerequest *TimerIO;
extern struct timeval     *timeval;

__asm BOOL OpenLibs(register __a6 struct ExecBase *SysBase)
{

  /* DOS Library */

  if((DOSBase = (struct DosLibrary *) OpenLibrary("dos.library", 37)) == NULL)
  {
    Alert(AN_Unknown|AG_OpenLib|AO_DOSLib);
    return FALSE;
  }

  /* Graphics Library */

  if((GfxBase = (struct GfxBase *) OpenLibrary("graphics.library", 37)) == NULL)
  {
    Alert(AN_Unknown|AG_OpenLib|AO_GraphicsLib);
    return FALSE;
  }

  /* GadTools Library */

  if((GadToolsBase = OpenLibrary("gadtools.library", 37)) == NULL)
  {
    Alert(AN_Unknown|AG_OpenLib|AO_GadTools);
    return FALSE;
  }

  /* IFFParse Library */

  if((IFFParseBase = OpenLibrary("iffparse.library", 37)) == NULL)
  {
    Alert(AN_Unknown|AG_OpenLib|AO_Unknown);
    return FALSE;
  }

  /* Intuition Library */

  if((IntuitionBase = (struct IntuitionBase *) 
      OpenLibrary("intuition.library", 37)) == NULL)
  {
    Alert(AN_Unknown|AG_OpenLib|AO_Intuition);
    return FALSE;
  }

  /* Locale Library */

  LocaleBase = OpenLibrary("locale.library", 38);

  /* Timer Device */

  if((TimerIO = (struct timerequest *) AllocVec(sizeof(struct timerequest),
      MEMF_PUBLIC | MEMF_CLEAR)) == NULL)
  {
    Alert(AN_Unknown|AG_NoMemory);
    return FALSE;
  }

  if((timeval = (struct timeval *) AllocVec(sizeof(struct timeval),
      MEMF_PUBLIC | MEMF_CLEAR)) == NULL)
  {
    Alert(AN_Unknown|AG_NoMemory);
    return FALSE;
  }

  if(OpenDevice("timer.device", UNIT_MICROHZ, (struct IORequest *) TimerIO, 0))
  {
    Alert(AN_Unknown|AG_OpenDev|AO_TimerDev);
    return FALSE;
  }

  TimerBase = (struct Library *) TimerIO->tr_node.io_Device;

  /* Utility Library */

  if((UtilityBase = OpenLibrary("utility.library", 37)) == NULL)
  {
    Alert(AN_Unknown|AG_OpenLib|AO_UtilityLib);
    return FALSE;
  }

  OpenahiCatalog(NULL, NULL);

  return TRUE;
}


/******************************************************************************
** CloseLibs *******************************************************************
******************************************************************************/

// This function is called by DevExpunge() when the device is about to be
// flushed

__asm void CloseLibs(register __a6 struct ExecBase *SysBase)
{

  CloseahiCatalog();
  CloseLibrary(UtilityBase);
  if(TimerIO)
    CloseDevice((struct IORequest *) TimerIO);
  FreeVec(timeval);
  FreeVec(TimerIO);
  CloseLibrary(LocaleBase);
  CloseLibrary((struct Library *) IntuitionBase);
  CloseLibrary(IFFParseBase);
  CloseLibrary(GadToolsBase);
  CloseLibrary((struct Library *) GfxBase);
  CloseLibrary((struct Library *) DOSBase);

}


/******************************************************************************
** DevOpen ********************************************************************
******************************************************************************/

/****** ahi.device/OpenDevice **********************************************
*
*   NAME
*       OpenDevice -- Open the device
*
*   SYNOPSIS
*       error = OpenDevice(AHINAME, unit, ioRequest, flags)
*       D0                 A0       D0    A1         D1
*
*       BYTE OpenDevice(STRPTR, ULONG, struct AHIRequest *, ULONG);
*
*   FUNCTION
*       This is an exec call.  Exec will search for the ahi.device, and
*       if found, will pass this call on to the device.
*
*   INPUTS
*       AHINAME - pointer to the string "ahi.device".
*       unit - Either AHI_DEFAULT_UNIT (0), AHI_NO_UNIT (255) or any other
*           unit the user has requested, for example with a UNIT tooltype.
*           AHI_NO_UNIT should be used when you're using the low-level
*           API.
*       ioRequest - a pointer to a struct AHIRequest, initialized by
*           exec.library/CreateIORequest().
*       flags - There is only one flag defined, AHIDF_NOMODESCAN, which
*           asks ahi.device not to build the audio mode database if not
*           already initialized. It should not be used by applications
*           without good reasons (AddAudioModes uses this flag).
*
*   RESULT
*       error - Same as io_Error.
*       io_Error - If the call succeeded, io_Error will be 0, else
*           an error code as defined in <exec/errors.h> and
*           <devices/ahi.h>.
*       io_Device - A pointer to the device base, which can be used
*           to call the functions the device provides.
*
*   EXAMPLE
*
*   NOTES
*
*   BUGS
*
*   SEE ALSO
*      CloseDevice(), exec.library/OpenDevice(), <exec/errors.h>,
*      <devices/ahi.h>.
*
****************************************************************************
*
*/

// This function is called by the system each time a unit is opened with
// exec.library/OpenDevice().

__asm ULONG DevOpen(
    register __d0 ULONG unit,
    register __d1 ULONG flags,
    register __a1 struct AHIRequest *ioreq,
    register __a6 struct AHIBase *AHIBase)
{
  ULONG rc=NULL;
  BOOL  error=FALSE;
  struct AHIDevUnit *iounit=NULL;

  if(AHIBase->ahib_DebugLevel >= AHI_DEBUG_LOW)
  {
    KPrintF("OpenDevice(%ld, 0x%08lx, %ld)", unit, ioreq, flags);
  }

// Check if size includes the ahir_Version field

  if(ioreq->ahir_Std.io_Message.mn_Length < (sizeof(struct IOStdReq) + 2))
  {
    Alert(AT_Recovery|AG_BadParm);
    ioreq->ahir_Std.io_Error=IOERR_OPENFAIL;
    return IOERR_OPENFAIL;
  }

// One more check...

  if((unit != AHI_NO_UNIT) && (ioreq->ahir_Version >= Version))
  {
    if(ioreq->ahir_Std.io_Message.mn_Length < sizeof(struct AHIRequest))
    {
      Alert(AT_Recovery|AG_BadParm);
      ioreq->ahir_Std.io_Error=IOERR_OPENFAIL;
      return IOERR_OPENFAIL;
    }
  }

  AHIBase->ahib_Library.lib_OpenCnt++;
  ObtainSemaphore(&AHIBase->ahib_Lock);

  if( ! (flags & AHIDF_NOMODESCAN))
  {
    // Load database if not already loaded

    if(AHI_NextAudioID(AHI_INVALID_ID) == AHI_INVALID_ID)
    {
      AHI_LoadModeFile("DEVS:AudioModes");
    }
  }

  if( ioreq->ahir_Version > Version)
    error=TRUE;
  else
  {
    if(unit < AHI_UNITS)
    {
      iounit=InitUnit(unit,AHIBase);
      if(!iounit) 
        error=TRUE;
    }
    else if(unit == AHI_NO_UNIT)
      InitUnit(unit,AHIBase);
  }

  if(!error)
  {
    ioreq->ahir_Std.io_Unit=(struct Unit *) iounit;
    if(iounit)    // Is NULL for AHI_NO_UNIT
      iounit->Unit.unit_OpenCnt++;
    AHIBase->ahib_Library.lib_OpenCnt++;
    AHIBase->ahib_Library.lib_Flags &=~LIBF_DELEXP;
  }
  else
  {
    rc=IOERR_OPENFAIL;
    ioreq->ahir_Std.io_Error=IOERR_OPENFAIL;
    ioreq->ahir_Std.io_Device=(struct Device *) -1;
    ioreq->ahir_Std.io_Unit=(struct Unit *) -1;
  }
  ReleaseSemaphore(&AHIBase->ahib_Lock);
  AHIBase->ahib_Library.lib_OpenCnt--;

  if(AHIBase->ahib_DebugLevel >= AHI_DEBUG_LOW)
  {
    KPrintF("=>%ld\n",rc);
  }
  return rc;
}


/******************************************************************************
** DevClose *******************************************************************
******************************************************************************/

/****** ahi.device/CloseDevice *********************************************
*
*   NAME
*       CloseDevice -- Close the device
*
*   SYNOPSIS
*       CloseDevice(ioRequest)
*                   A1
*
*       void CloseDevice(struct IORequest *);
*
*   FUNCTION
*       This is an exec call that closes the device. Every OpenDevice()
*       must be mached with a call to CloseDevice().
*
*       The user must ensure that all outstanding IO Requests have been
*       returned before closing the device.
*
*   INPUTS
*       ioRequest - a pointer to the same struct AHIRequest that was used
*           to open the device.
*
*   RESULT
*
*   EXAMPLE
*
*   NOTES
*
*   BUGS
*
*   SEE ALSO
*      OpenDevice(), exec.library/CloseDevice()
*
****************************************************************************
*
*/

// This function is called by the system each time a unit is closed with
// exec.library/CloseDevice().

__asm BPTR DevClose(
    register __a1 struct AHIRequest *ioreq,
    register __a6 struct AHIBase *AHIBase)
{
  struct AHIDevUnit *iounit;
  BPTR  seglist=0;

  if(AHIBase->ahib_DebugLevel >= AHI_DEBUG_LOW)
  {
    KPrintF("CloseDevice(0x%08lx)\n", ioreq);
  }

  ObtainSemaphore(&AHIBase->ahib_Lock);

  iounit= (struct AHIDevUnit *) ioreq->ahir_Std.io_Unit;
  ioreq->ahir_Std.io_Device = (struct Device *) -1;
  ioreq->ahir_Std.io_Unit = (struct Unit *) -1;

  if(iounit)
  {
    iounit->Unit.unit_OpenCnt--;
    if(!iounit->Unit.unit_OpenCnt)
      ExpungeUnit(iounit,AHIBase);
  }

  AHIBase->ahib_Library.lib_OpenCnt--;

  ReleaseSemaphore(&AHIBase->ahib_Lock);

  if(!AHIBase->ahib_Library.lib_OpenCnt)
  {
    if(AHIBase->ahib_Library.lib_Flags & LIBF_DELEXP)
      seglist=DevExpunge(AHIBase);
  }
  return seglist;
}


/******************************************************************************
** InitUnit *******************************************************************
******************************************************************************/

// This function is called by DevOpen() to initialize a unit

static struct AHIDevUnit *InitUnit( ULONG unit, struct AHIBase *AHIBase )
{
  struct AHIDevUnit *iounit;
  struct TagItem NPTags[]=
  {
    NP_Entry,     0,
    NP_Name,      0,
    NP_Priority,  AHI_PRI ,
    TAG_DONE,     0
  };
  struct MsgPort *replyport;

  if( unit == AHI_NO_UNIT )
  {
    ReadConfig(NULL,AHIBase);
    return NULL;
  }
  else if(!AHIBase->ahib_DevUnits[unit])
  {
    if(iounit = AllocVec(sizeof(struct AHIDevUnit), MEMF_CLEAR|MEMF_PUBLIC))
    {
      NewList(&iounit->Unit.unit_MsgPort.mp_MsgList);

      iounit->Unit.unit_MsgPort.mp_Node.ln_Type = NT_MSGPORT;
      iounit->Unit.unit_MsgPort.mp_Flags = PA_IGNORE;
      iounit->Unit.unit_MsgPort.mp_Node.ln_Name = DevName;
      iounit->UnitNum = unit;
      InitSemaphore(&iounit->ListLock);
      NewList((struct List *)&iounit->ReadList);
      NewList((struct List *)&iounit->PlayingList);
      NewList((struct List *)&iounit->SilentList);
      NewList((struct List *)&iounit->WaitingList);
      NewList((struct List *)&iounit->RequestQueue);
      if(ReadConfig(iounit,AHIBase))
      {
        if(iounit->Voices = AllocVec(
            sizeof(struct Voice)*iounit->Channels,MEMF_PUBLIC|MEMF_CLEAR))
        {
          int i;
          struct Voice *v = iounit->Voices;
          
          // Mark all channels as free
          for(i = 0 ; i < iounit->Channels; i++)
          {
            v->NextOffset = FREE;
            v++;
          }
          
          if(replyport = CreateMsgPort())
          {
            AHIBase->ahib_Startup.Msg.mn_ReplyPort = replyport;
            AHIBase->ahib_Startup.Unit = iounit;

            NPTags[0].ti_Data = (ULONG) &DevProcEntry;
            NPTags[1].ti_Data = (ULONG) &DevName;       /* Process name */

            if(iounit->Process = CreateNewProc(NPTags))
            {
  
                PutMsg(&iounit->Process->pr_MsgPort,&AHIBase->ahib_Startup.Msg);
                WaitPort(replyport);
                GetMsg(replyport);
            }
            DeleteMsgPort(replyport);
          }
        }
      }

      if(!iounit->Process)
        FreeVec(iounit);
      else
        AHIBase->ahib_DevUnits[unit] = iounit;

    }
  }
  return AHIBase->ahib_DevUnits[unit];
}


/******************************************************************************
** ExpungeUnit ****************************************************************
******************************************************************************/

// This function is called by DevClose() to remove a unit.

static void ExpungeUnit(struct AHIDevUnit *iounit, struct AHIBase *AHIBase )
{
  struct Task *unittask;

  unittask = (struct Task *) iounit->Process;
  iounit->Process = (struct Process *) FindTask(NULL);
  Signal(unittask,SIGBREAKF_CTRL_F);
  Wait(SIGBREAKF_CTRL_F);
  AHIBase->ahib_DevUnits[iounit->UnitNum]=NULL;
  FreeVec(iounit->Voices);
  FreeVec(iounit);
}


/******************************************************************************
** ReadConfig *****************************************************************
******************************************************************************/

// This functions loads the users settings for AHI.

BOOL ReadConfig( struct AHIDevUnit *iounit, struct AHIBase *AHIBase )
{
  struct IFFHandle *iff;
  struct StoredProperty *prhd,*ahig;
  struct CollectionItem *ci;

  if(iounit)
  {
    /* Internal defaults for device unit 0 */
    iounit->AudioMode       = AHI_BestAudioID(AHIDB_Realtime, TRUE, TAG_DONE);
    iounit->Frequency       = 10000;
    iounit->Channels        = 4;
    iounit->MonitorVolume   = -1;
 	  iounit->InputGain       = -1;
    iounit->OutputVolume    = -1;
 	  iounit->Input           = -1;
    iounit->Output          = -1;
  }
  else
  {
    /* Internal defaults for low-level mode */
    AHIBase->ahib_AudioMode       = AHI_BestAudioID( AHIDB_Realtime, TRUE, TAG_DONE);
    AHIBase->ahib_Frequency       = 10000;
    AHIBase->ahib_MonitorVolume   = 0x00000;
  	AHIBase->ahib_InputGain       = 0x10000;
    AHIBase->ahib_OutputVolume    = 0x10000;
  	AHIBase->ahib_Input           = 0;
    AHIBase->ahib_Output          = 0;
  }

  if(iff=AllocIFF())
  {
    iff->iff_Stream=Open("ENV:Sys/ahi.prefs", MODE_OLDFILE);
    if(iff->iff_Stream)
    {
      InitIFFasDOS(iff);
      if(!OpenIFF(iff,IFFF_READ))
      {

        if(!(PropChunk(iff,ID_PREF,ID_PRHD)
          || PropChunk(iff,ID_PREF,ID_AHIG)
          || CollectionChunk(iff,ID_PREF,ID_AHIU)
          || StopOnExit(iff,ID_PREF,ID_FORM)))
        {
          if(ParseIFF(iff,IFFPARSE_SCAN) == IFFERR_EOC)
          {
            prhd=FindProp(iff,ID_PREF,ID_PRHD);
            ahig=FindProp(iff,ID_PREF,ID_AHIG);
            
            if(ahig)
            {
              struct AHIGlobalPrefs *globalprefs;
              
              globalprefs = (struct AHIGlobalPrefs *)ahig->sp_Data;

              AHIBase->ahib_DebugLevel = globalprefs->ahigp_DebugLevel;

              AHIBase->ahib_Flags = 0;

              if(globalprefs->ahigp_DisableSurround)
                AHIBase->ahib_Flags |= AHIBF_NOSURROUND;

              if(globalprefs->ahigp_DisableEcho)
                AHIBase->ahib_Flags |= AHIBF_NOECHO;

              if(globalprefs->ahigp_FastEcho)
                AHIBase->ahib_Flags |= AHIBF_FASTECHO;
                
              if(ahig->sp_Size > offsetof(struct AHIGlobalPrefs, ahigp_MaxCPU))
              {
                AHIBase->ahib_MaxCPU = globalprefs->ahigp_MaxCPU;
              }
              else
              {
                AHIBase->ahib_MaxCPU = 0x10000 * 90 / 100;
              }
            }
            ci=FindCollection(iff,ID_PREF,ID_AHIU);
            while(ci)
            {
              struct AHIUnitPrefs *unitprefs;

              unitprefs = (struct AHIUnitPrefs *)ci->ci_Data;

              if(iounit)
              {
                if(unitprefs->ahiup_Unit == iounit->UnitNum)
                {
                  iounit->AudioMode       = unitprefs->ahiup_AudioMode;
                  iounit->Frequency       = unitprefs->ahiup_Frequency;
                  iounit->Channels        = unitprefs->ahiup_Channels;
                  iounit->MonitorVolume   = unitprefs->ahiup_MonitorVolume;
                	iounit->InputGain       = unitprefs->ahiup_InputGain;
              	  iounit->OutputVolume    = unitprefs->ahiup_OutputVolume;
                	iounit->Input           = unitprefs->ahiup_Input;
                  iounit->Output          = unitprefs->ahiup_Output;
                }
              }
              else
              {
                if(unitprefs->ahiup_Unit == AHI_NO_UNIT)
                {
                  AHIBase->ahib_AudioMode       = unitprefs->ahiup_AudioMode;
                  AHIBase->ahib_Frequency       = unitprefs->ahiup_Frequency;
                  AHIBase->ahib_MonitorVolume   = unitprefs->ahiup_MonitorVolume;
                	AHIBase->ahib_InputGain       = unitprefs->ahiup_InputGain;
              	  AHIBase->ahib_OutputVolume    = unitprefs->ahiup_OutputVolume;
                	AHIBase->ahib_Input           = unitprefs->ahiup_Input;
                  AHIBase->ahib_Output          = unitprefs->ahiup_Output;
                }
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
  return TRUE;
}


/******************************************************************************
** AllocHardware **************************************************************
******************************************************************************/

// Allocates the audio hardware

BOOL AllocHardware(struct AHIDevUnit *iounit,struct AHIBase *AHIBase)
{
  BOOL rc = FALSE;
  ULONG fullduplex=FALSE;

  /* Allocate the hardware */
  iounit->AudioCtrl = AHI_AllocAudio(
      AHIA_AudioID,       iounit->AudioMode,
      AHIA_MixFreq,       iounit->Frequency,
      AHIA_Channels,      iounit->Channels,
      AHIA_Sounds,        MAXSOUNDS,
      AHIA_PlayerFunc,    &iounit->PlayerHook,
      AHIA_RecordFunc,    &iounit->RecordHook,
      AHIA_SoundFunc,     &iounit->SoundHook,
      TAG_DONE);

  if(iounit->AudioCtrl != NULL)
  {
    /* Full duplex? */
    AHI_GetAudioAttrs(AHI_INVALID_ID,iounit->AudioCtrl,
      AHIDB_FullDuplex, &fullduplex,
      TAG_DONE);
    iounit->FullDuplex = fullduplex;

    /* Set hardware properties */
    AHI_ControlAudio(iounit->AudioCtrl,
        (iounit->MonitorVolume == -1 ? TAG_IGNORE : AHIC_MonitorVolume),
        iounit->MonitorVolume,

        (iounit->InputGain == -1 ? TAG_IGNORE : AHIC_InputGain),
        iounit->InputGain,

        (iounit->OutputVolume == -1 ? TAG_IGNORE : AHIC_OutputVolume),
        iounit->OutputVolume,

        (iounit->Input == -1 ? TAG_IGNORE : AHIC_Input),
        iounit->Input,

        (iounit->Output == -1 ? TAG_IGNORE : AHIC_Output),
        iounit->Output,

        TAG_DONE);

    iounit->ChannelInfoStruct->ahie_Effect = AHIET_CHANNELINFO;
    iounit->ChannelInfoStruct->ahieci_Func = &iounit->ChannelInfoHook;
    iounit->ChannelInfoStruct->ahieci_Channels = iounit->Channels;
    if(!AHI_SetEffect(iounit->ChannelInfoStruct, iounit->AudioCtrl))
    {
      rc = TRUE;
    }
  }
  return rc;
}


/******************************************************************************
** FreeHardware ***************************************************************
******************************************************************************/

// Take a wild guess!

void FreeHardware(struct AHIDevUnit *iounit,struct AHIBase *AHIBase)
{
  if(iounit->AudioCtrl)
  {
    if(iounit->ChannelInfoStruct)
    {
      iounit->ChannelInfoStruct->ahie_Effect = (AHIET_CANCEL | AHIET_CHANNELINFO);
      AHI_SetEffect(iounit->ChannelInfoStruct, iounit->AudioCtrl);
    }
    AHI_FreeAudio(iounit->AudioCtrl);
    iounit->AudioCtrl = NULL;
    iounit->IsRecording = FALSE;
    iounit->IsPlaying = FALSE;
    iounit->ValidRecord = FALSE;
  }
}


/******************************************************************************
** DevProc ********************************************************************
******************************************************************************/

__asm void DevProc(register __a6 struct AHIBase *AHIBase)
{
  struct Process *proc;
  struct StartupMessage *sm;
  struct AHIDevUnit *iounit;
  UBYTE  signalbit;

  proc = (struct Process *)FindTask(NULL);
  WaitPort(&proc->pr_MsgPort);
  sm = (struct StartupMessage *)GetMsg(&proc->pr_MsgPort);
  iounit = sm->Unit;

  iounit->Process = NULL;

  iounit->PlayerHook.h_Entry=(ULONG (*)())PlayerFunc;
  iounit->PlayerHook.h_Data=iounit;

  iounit->RecordHook.h_Entry=(ULONG (*)())RecordFunc;
  iounit->RecordHook.h_Data=iounit;

  iounit->SoundHook.h_Entry=(ULONG (*)())SoundFunc;
  iounit->SoundHook.h_Data=iounit;

  iounit->ChannelInfoHook.h_Entry=(ULONG (*)())ChannelInfoFunc;
  iounit->ChannelInfoHook.h_Data=iounit;

  iounit->ChannelInfoStruct = AllocVec(
      sizeof(struct AHIEffChannelInfo) + (iounit->Channels * sizeof(ULONG)),
      MEMF_PUBLIC | MEMF_CLEAR);

  iounit->Master=proc;

  signalbit = AllocSignal(-1);
  iounit->PlaySignal = AllocSignal(-1);
  iounit->RecordSignal = AllocSignal(-1);
  iounit->SampleSignal = AllocSignal(-1);

  if((signalbit != -1)
  && (iounit->PlaySignal != -1)
  && (iounit->RecordSignal != -1)
  && (iounit->SampleSignal != -1)
  && (iounit->ChannelInfoStruct != NULL)
  )
  {
    /* Set up our Unit's MsgPort. */
    iounit->Unit.unit_MsgPort.mp_SigBit = signalbit;
    iounit->Unit.unit_MsgPort.mp_SigTask = (struct Task *)proc;
    iounit->Unit.unit_MsgPort.mp_Flags = PA_SIGNAL;

    /* Allocate the hardware */
    if(AllocHardware(iounit, AHIBase))
    {

      /* Set iounit->Process to pointer to our unit process.
         This will let the Unit init code know that were
         are okay. */
      iounit->Process = proc;
    }
  }

  /* Reply to our startup message */
  ReplyMsg(&sm->Msg);

  if(iounit->Process)
  {
    ULONG  waitmask,signals;

    waitmask = (1L << signalbit)
             | SIGBREAKF_CTRL_E    // Dummy signal to wake up task
             | SIGBREAKF_CTRL_F    // Quit signal
             | (1L << iounit->PlaySignal)
             | (1L << iounit->RecordSignal)
             | (1L << iounit->SampleSignal);

    while(TRUE)
    {
      signals = Wait(waitmask);

      /* Have we been signaled to shut down? */
      if(signals & SIGBREAKF_CTRL_F)
        break;

      if(signals & (1L << signalbit))
      {
        struct AHIRequest *ioreq;

        while(ioreq = (struct AHIRequest *) GetMsg(&iounit->Unit.unit_MsgPort))
        {
          PerformIO(ioreq,AHIBase);
        }
      }
      
      if(signals & (1L << iounit->PlaySignal))
      {
        ObtainSemaphore(&iounit->ListLock);
        UpdateSilentPlayers(iounit,AHIBase);
        ReleaseSemaphore(&iounit->ListLock);
      }

      if(signals & (1L << iounit->RecordSignal))
      {
        iounit->ValidRecord = TRUE;
        FeedReaders(iounit,AHIBase);
      }

      if(signals & (1L << iounit->SampleSignal))
      {
        RethinkPlayers(iounit,AHIBase);
      }
    }
  }

  FreeHardware(iounit, AHIBase);
  FreeSignal(iounit->SampleSignal);
  iounit->SampleSignal = -1;
  FreeSignal(iounit->RecordSignal);
  iounit->RecordSignal = -1;
  FreeSignal(iounit->PlaySignal);
  iounit->PlaySignal = -1;
  FreeVec(iounit->ChannelInfoStruct);

  if(iounit->Process)
  {
    Forbid();
    Signal((struct Task *) iounit->Process, SIGBREAKF_CTRL_F);
  }
  FreeSignal(signalbit);
}


/******************************************************************************
** PlayerFunc *****************************************************************
******************************************************************************/

static __asm __interrupt void PlayerFunc(
    register __a0 struct Hook *hook,
    register __a2 struct AHIAudioCtrl *actrl,
    register __a1 APTR null)
{
  struct AHIDevUnit *iounit = (struct AHIDevUnit *) hook->h_Data;

  if(AttemptSemaphore(&iounit->ListLock))
  {
    UpdateSilentPlayers(iounit,AHIBase);
    ReleaseSemaphore(&iounit->ListLock);
  }
  else
  { // Do it later instead
    Signal((struct Task *) iounit->Master, (1L << iounit->PlaySignal));
  }
  return;
}


/******************************************************************************
** RecordFunc *****************************************************************
******************************************************************************/

static __asm __interrupt BOOL RecordFunc(
    register __a0 struct Hook *hook,
    register __a2 struct AHIAudioCtrl *actrl,
    register __a1 struct AHIRecordMessage *recmsg)
{
  struct AHIDevUnit *iounit;
  
  if(recmsg->ahirm_Type == AHIST_S16S)
  {
    iounit = (struct AHIDevUnit *) hook->h_Data;
    iounit->RecordBuffer = recmsg->ahirm_Buffer;
    iounit->RecordSize = recmsg->ahirm_Length<<2;
#ifdef DEBUG
    KPrintF("Buffer Filled again...\n");
#endif
    Signal((struct Task *) iounit->Master, (1L << iounit->RecordSignal));
  }
  return NULL;
}


/******************************************************************************
** SoundFunc ******************************************************************
******************************************************************************/

static __asm __interrupt void SoundFunc(
    register __a0 struct Hook *hook,
    register __a2 struct AHIAudioCtrl *actrl,
    register __a1 struct AHISoundMessage *sndmsg)
{
  struct AHIDevUnit *iounit;
  struct Voice *voice;  

  iounit = (struct AHIDevUnit *) hook->h_Data;
  voice = &iounit->Voices[(WORD)sndmsg->ahism_Channel];

#ifdef DEBUG
  KPrintF("Playing on channel %ld: 0x%08lx\n", sndmsg->ahism_Channel, voice->PlayingRequest);
#endif

  if(voice->PlayingRequest)
  {
    voice->PlayingRequest->ahir_Std.io_Command = AHICMD_WRITTEN;
#ifdef DEBUG
    KPrintF("Marked 0x%08lx as written\n", voice->PlayingRequest);
#endif
  }
  voice->PlayingRequest = voice->QueuedRequest;
#ifdef DEBUG
  KPrintF("New player: 0x%08lx\n", voice->PlayingRequest);
#endif
  voice->QueuedRequest  = NULL;

  switch(voice->NextOffset)
  {
    case FREE:
      break;
    case MUTE:
      /* A AHI_NOSOUND is done, channel is silent */
      voice->NextOffset = FREE;
      break;
    case PLAY:
      /* A normal sound is done and playing, no other sound is queued */
      AHI_SetSound(sndmsg->ahism_Channel,AHI_NOSOUND,0,0,actrl,NULL);
      voice->NextOffset = MUTE;
      break;
    default:
      /* A normal sound is done, and another is waiting */
      AHI_SetSound(sndmsg->ahism_Channel,
          voice->NextSound,
          voice->NextOffset,
          voice->NextLength,
          actrl,NULL);
      AHI_SetFreq(sndmsg->ahism_Channel,
          voice->NextFrequency,
          actrl,NULL);
      AHI_SetVol(sndmsg->ahism_Channel,
          voice->NextVolume,
          voice->NextPan,
          actrl,NULL);
      voice->QueuedRequest = voice->NextRequest;
      voice->NextRequest = NULL;
      voice->NextOffset = PLAY;
      break;
  }

  Signal((struct Task *) iounit->Master, (1L << iounit->SampleSignal));
}


/******************************************************************************
** ChannelInfoFunc ************************************************************
******************************************************************************/

// This hook keeps updating the io_Actual field of each playing requests

static __asm __interrupt void ChannelInfoFunc(
    register __a0 struct Hook *hook,
    register __a2 struct AHIAudioCtrl *actrl,
    register __a1 struct AHIEffChannelInfo *cimsg)
{
  struct AHIDevUnit *iounit = (struct AHIDevUnit *) hook->h_Data;
  struct Voice      *voice;
  ULONG             *offsets = (ULONG *) &cimsg->ahieci_Offset;
  int i;

  Disable();    // Not needed?
  voice = iounit->Voices;
  for(i = 0; i < iounit->Channels; i++)
  {
    if(voice->PlayingRequest)
    {
      voice->PlayingRequest->ahir_Std.io_Actual = *offsets;
    }
    voice++;
    offsets++;
  }
  Enable();
  return;
}
