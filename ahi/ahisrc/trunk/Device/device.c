/* $Id$
* $Log$
* Revision 1.1  1996/12/21 13:05:12  lcs
* Initial revision
*
*/

#include "ahi_def.h"

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

#ifndef _GENPROTO
#include "device_protos.h"
#endif

#include "devcommands_protos.h"

static struct AHIDevUnit *InitUnit( ULONG unit, struct AHIBase *AHIBase );
static void ExpungeUnit(struct AHIDevUnit *iounit, struct AHIBase *AHIBase );
static BOOL ReadConfig( struct AHIDevUnit *iounit, struct AHIBase *AHIBase );
static BOOL AllocHardware(struct AHIDevUnit *,struct AHIBase *);
static void FreeHardware(struct AHIDevUnit *,struct AHIBase *);
static __asm __interrupt BOOL RecordFunc(
    register __a0 struct Hook *hook,
    register __a2 struct AHIAudioCtrl *actrl,
    register __a1 struct AHIRecordMessage *recmsg);
static __asm __interrupt void SoundFunc(
    register __a0 struct Hook *hook,
    register __a2 struct AHIAudioCtrl *actrl,
    register __a1 struct AHISoundMessage *sndmsg);

extern __asm BPTR DevExpunge( register __a6 struct AHIBase * );
extern __asm void DevProcEntry(void);


/******************************************************************************
** DevOpen ********************************************************************
******************************************************************************/

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

// Check if size includes the ahir_Version field
  if(ioreq->ahir_Std.io_Message.mn_Length < (sizeof(struct IOStdReq) + 2))
  {
    Alert(AT_Recovery|AG_BadParm);
    ioreq->ahir_Std.io_Error=IOERR_OPENFAIL;
    return IOERR_OPENFAIL;
  }

// One more check...
  if(ioreq->ahir_Version >= Version)
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

// Load database if not already loaded
  if(AHI_NextAudioID(AHI_INVALID_ID) EQ AHI_INVALID_ID)
  {
    AHI_LoadModeFile("DEVS:AudioModes");
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
    else if(unit EQ AHI_NO_UNIT)
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
  return rc;
}


/******************************************************************************
** DevClose *******************************************************************
******************************************************************************/

// This function is called by the system each time a unit is closed with
// exec.library/CloseDevice().

__asm BPTR DevClose(
    register __a1 struct AHIRequest *ioreq,
    register __a6 struct AHIBase *AHIBase)
{
  struct AHIDevUnit *iounit;
  BPTR  seglist=0;

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

  if( unit EQ AHI_NO_UNIT )
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

      if(ReadConfig(iounit,AHIBase))
      {
        if(iounit->Voices=AllocVec(
            sizeof(struct Voice)*iounit->Channels,MEMF_PUBLIC|MEMF_CLEAR))
        {
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

static BOOL ReadConfig( struct AHIDevUnit *iounit, struct AHIBase *AHIBase )
{
  struct IFFHandle *iff;
  struct StoredProperty *prhd,*ahig;
  struct CollectionItem *ci;

  if(iounit)
  {
    /* Internal defaults for device unit 0 */
    iounit->AudioMode       = AHI_BestAudioID(
                                  AHIDB_Realtime, TRUE,
                                  AHIDB_MaxChannels, 4,
                                  TAG_DONE);
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
          if(ParseIFF(iff,IFFPARSE_SCAN) EQ IFFERR_EOC)
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

            }
            ci=FindCollection(iff,ID_PREF,ID_AHIU);
            while(ci)
            {
              struct AHIUnitPrefs *unitprefs;

              unitprefs = (struct AHIUnitPrefs *)ci->ci_Data;

              if(iounit)
              {
                if(unitprefs->ahiup_Unit EQ iounit->UnitNum)
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
                if(unitprefs->ahiup_Unit EQ AHI_NO_UNIT)
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

static BOOL AllocHardware(struct AHIDevUnit *iounit,struct AHIBase *AHIBase)
{
  BOOL rc = FALSE;
  struct AHISampleInfo snd8info   = {AHIST_M8S,0,0xffffffff};
  struct AHISampleInfo snd16info  = {AHIST_M16S,0,0xffffffff};
  ULONG fullduplex=FALSE;

  /* Allocate the hardware */
  iounit->AudioCtrl = AHI_AllocAudio(
      AHIA_AudioID,       iounit->AudioMode,
      AHIA_MixFreq,       iounit->Frequency,
      AHIA_Channels,      iounit->Channels,
      AHIA_Sounds,        2,
//    AHIA_PlayerFunc,    &iounit->PlayerHook,
      AHIA_RecordFunc,    &iounit->RecordHook,
      AHIA_SoundFunc,     &iounit->SoundHook,
      TAG_DONE);

  if(iounit->AudioCtrl != NULL)
  {
    /* Full duplex? */
    AHI_GetAudioAttrs(AHI_INVALID_ID,iounit->AudioCtrl,
      AHIDB_FullDuplex, &fullduplex,
      TAG_DONE);
    iounit->FullDuplex=fullduplex;

    /* Load sounds */
    if((!AHI_LoadSound(SND8,AHIST_DYNAMICSAMPLE,&snd8info,iounit->AudioCtrl))
    && (!AHI_LoadSound(SND16,AHIST_DYNAMICSAMPLE,&snd16info,iounit->AudioCtrl)))
    {
      /* Set hardware properties */
      AHI_ControlAudio(iounit->AudioCtrl,
          (iounit->MonitorVolume EQ -1 ? TAG_IGNORE : AHIC_MonitorVolume),
          iounit->MonitorVolume,

          (iounit->InputGain EQ -1 ? TAG_IGNORE : AHIC_InputGain),
          iounit->InputGain,

          (iounit->OutputVolume EQ -1 ? TAG_IGNORE : AHIC_OutputVolume),
          iounit->OutputVolume,

          (iounit->Input EQ -1 ? TAG_IGNORE : AHIC_Input),
          iounit->Input,

          (iounit->Output EQ -1 ? TAG_IGNORE : AHIC_Output),
          iounit->Output,

          TAG_DONE);
      rc = TRUE;
    }
  }
  return rc;
}


/******************************************************************************
** FreeHardware ***************************************************************
******************************************************************************/

// Take a wild guess!

static void FreeHardware(struct AHIDevUnit *iounit,struct AHIBase *AHIBase)
{
  if(iounit->AudioCtrl)
  {
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

  iounit->RecordHook.h_Entry=(ULONG (*)())RecordFunc;
  iounit->RecordHook.h_Data=iounit;

  iounit->SoundHook.h_Entry=(ULONG (*)())SoundFunc;
  iounit->SoundHook.h_Data=iounit;

  iounit->Master=proc;

  signalbit = AllocSignal(-1);
  iounit->PlaySignal = AllocSignal(-1);
  iounit->RecordSignal = AllocSignal(-1);

  if((signalbit != -1)
  && (iounit->PlaySignal != -1)
  && (iounit->RecordSignal != -1))
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
             | (1L << iounit->RecordSignal);

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
      
      if(signals & (1L << iounit->RecordSignal))
      {
        iounit->ValidRecord = TRUE;
        FeedReaders(iounit,AHIBase);
      }
    }
  }

  FreeHardware(iounit, AHIBase);
  FreeSignal(iounit->RecordSignal);
  iounit->RecordSignal = -1;
  FreeSignal(iounit->PlaySignal);
  iounit->PlaySignal = -1;

  if(iounit->Process)
  {
    Forbid();
    Signal((struct Task *) iounit->Process, SIGBREAKF_CTRL_F);
  }
  FreeSignal(signalbit);
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
  
  if(recmsg->ahirm_Type EQ AHIST_S16S)
  {
    iounit = (struct AHIDevUnit *) hook->h_Data;
    iounit->RecordBuffer = recmsg->ahirm_Buffer;
    iounit->RecordSize = recmsg->ahirm_Length<<2;
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
  switch(voice->Offset)
  {
    case FREE:
      break;
    case MUTE:
      /* A AHI_NOSOUND is done, channel is silent */
      voice->Offset=FREE;
      break;
    case PLAY:
      /* A normal sound is done and playing, no other sound is queued */
      AHI_SetSound(sndmsg->ahism_Channel,AHI_NOSOUND,0,0,actrl,NULL);
      voice->Offset=MUTE;
      break;
    default:
      /* A normal sound is done, and another is waiting */
      AHI_SetSound(sndmsg->ahism_Channel,
          voice->Sound,
          voice->Offset,
          voice->Length,
          actrl,NULL);
      AHI_SetFreq(sndmsg->ahism_Channel,
          voice->Frequency,
          actrl,NULL);
      AHI_SetVol(sndmsg->ahism_Channel,
          voice->Volume,
          voice->Pan,
          actrl,NULL);
      voice->Offset=PLAY;
      voice->FIFO[1]=TRUE;
      break;
  }
/*
  if(voice->SignalDone && voice->FIFO[0])
  {
    *voice->SignalDone=AHICMD_FINISHED;
    voice->SignalDone=NULL;
  }
  voice->FIFO[0]=voice->FIFO[1];
  voice->FIFO[1]=FALSE;
*/
  Signal((struct Task *) iounit->Master, (1L << iounit->PlaySignal));
}
