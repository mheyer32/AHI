/*
 * $Id$
 * $Log$
 * Revision 1.1  1997/06/29 03:04:02  lcs
 * Initial revision
 *
 */

#include <devices/ahi.h>
#include <dos/dos.h>
#include <dos/dostags.h>
#include <exec/exec.h>

#include <proto/ahi.h>
#include <proto/dos.h>
#include <proto/exec.h>
#include <proto/utility.h>
#include <clib/toccata_protos.h>
#include <pragmas/toccata_pragmas.h>

#include <math.h>
#include "toccata.h"


/* Prototypes */

static ASM ULONG SoundFunc(REG(a0) struct Hook *hook,
                           REG(a2) struct AHIAudioCtrl *audioctrl,
                           REG(a1) struct AHISoundMessage *msg);
static ASM ULONG RecordFunc(REG(a0) struct Hook *hook,
                            REG(a2) struct AHIAudioCtrl *audioctrl,
                            REG(a1) struct AHIRecordMessage *msg);
ASM void RawReply(void);
static BOOL AllocAudio(void);
static void FreeAudio(void);
static BOOL TuneAudio(void);
static BOOL ControlAudio(void);

static void Pause(ULONG pause);
static void Stop(ULONG flags);
static BOOL RawPlayback(struct TagItem *tags);


/* Some flags */

BOOL SlaveInitialized = FALSE;
BOOL AudioInitialized = FALSE;
BOOL Playing          = FALSE;
BOOL Recording        = FALSE;
BOOL Leveling         = FALSE;
BOOL Pausing          = FALSE;
BOOL Sound0Loaded     = FALSE;
BOOL Sound1Loaded     = FALSE;
WORD SoundFlag        = 0;

/* RawPlayback variables */

struct Task      *ErrorTask, *RawTask, *SigTask;
ULONG             ErrorMask, RawMask, SigMask;
struct Interrupt *RawInt;
BYTE             *RawBuffer1, *RawBuffer2;
ULONG             RawBufferSize, RawIrqSize;
ULONG            *ByteCount;
ULONG             ByteSkip;

BYTE             *RawBufferPtr;
ULONG             RawBufferOffset;
ULONG             RawBufferLength;  /* RawBufferSize / samplesize */
ULONG             RawChunckLength;  /* RawIrqSize / samplesize */
BOOL              NextBufferOK;
LONG              ByteSkipCounter;

/* Audio stuff */

struct AHIAudioCtrl *audioctrl  = NULL;

Fixed MinMonVol, MaxMonVol, MinOutVol, MaxOutVol, MinGain, MaxGain;

struct Hook SoundHook  = {
  NULL, NULL, (ULONG (*)()) HookLoad, SoundFunc, NULL
};

struct Hook RecordHook = {
  NULL, NULL, (ULONG (*)()) HookLoad, RecordFunc, NULL
};

/* dB<->Fixed conversion */

const Fixed negboundaries[] = {
  65536,55141,46395,39037,32845,27636,23253,19565,16461,13850,11654,9805,8250,
  6941,5840,4914,4135,3479,2927,2463,2072,1743,1467,1234,1038,873,735,618,520,
  438,368,310,260,219,184,155,130,110,92,77,65,55,46,39,32,27,23,19,16,13,11,9,
  8,6,5,4,4,3,2,2,2,1,1,1,0
};

const Fixed posboundaries[] = {
  65536,77889,92572,110022,130761,155410,184705,219522,260903,
  310084,368536,438005,520570,618699,735326,873936
};



/*
 *  SoundFunc(): Called when a sample has just started
 */

static ASM ULONG SoundFunc(REG(a0) struct Hook *hook,
                           REG(a2) struct AHIAudioCtrl *audioctrl,
                           REG(a1) struct AHISoundMessage *msg) {

  RawBufferOffset += RawChunckLength;

  if(RawBufferOffset >= RawBufferLength) {

    if(SoundFlag == 0) {
      SoundFlag = 1;
      RawBufferPtr = RawBuffer2;
    }
    else {
      SoundFlag = 0;
      RawBufferPtr = RawBuffer1;
    }

    RawBufferOffset = 0;

    if(!NextBufferOK && (ErrorTask != NULL)) {
      Signal(ErrorTask, ErrorMask);
      Signal((struct Task *) SlaveProcess, SIGBREAKF_CTRL_D);
    }

    if(RawInt != NULL) {
      Cause(RawInt);
    }
    else if(RawTask != NULL) {
      NextBufferOK = FALSE;
      Signal(RawTask, RawMask);
    }
  }

  AHI_SetSound(0, SoundFlag, RawBufferOffset, 
      min(RawChunckLength, RawBufferLength - RawBufferOffset), audioctrl, 0);


  if(ByteCount != NULL) {
    *ByteCount += RawIrqSize;
  }


  if(SigTask != NULL) {
    ByteSkipCounter -= RawIrqSize;
    if(ByteSkipCounter <= 0) {
      ByteSkipCounter = ByteSkip;
      Signal(SigTask, SigMask);
    }
  }

  return 0;
}


/*
 *  RecordFunc(): Called when a new block of recorded data is available
 */

static ASM ULONG RecordFunc(REG(a0) struct Hook *hook,
                            REG(a2) struct AHIAudioCtrl *audioctrl,
                            REG(a1) struct AHIRecordMessage *msg) {

  return 0;
}


/*
 *  RawRely(): Called by the library user when (s)he has filled a buffer
 */

ASM void RawReply(void) {
  NextBufferOK = TRUE;
}


/*
 *  SlaveTask(): The slave process
 */

void ASM SlaveTask(void) {
  struct MsgPort    *AHImp = NULL;
  struct AHIRequest *AHIio = NULL;
  BYTE AHIDevice = -1;
  struct Process    *me;
  ToccataBase->tb_HardInfo = NULL;

  me = (struct Process *) FindTask(NULL);

  if(AHImp=CreateMsgPort()) {
    if(AHIio=(struct AHIRequest *)CreateIORequest(AHImp,sizeof(struct AHIRequest))) {
      AHIio->ahir_Version = 4;
      AHIDevice = OpenDevice(AHINAME, AHI_NO_UNIT, (struct IORequest *)AHIio, NULL);
    }
  }

  if(AHIDevice == 0) {
    AHIBase = (struct Library *) AHIio->ahir_Std.io_Device;

    fillhardinfo();

    SlaveInitialized = TRUE;

    while(TRUE) {
      ULONG signals;
      struct slavemessage *msg;

      signals = Wait((1L << me->pr_MsgPort.mp_SigBit) |
                     SIGBREAKF_CTRL_C | SIGBREAKF_CTRL_D);

      if(signals & SIGBREAKF_CTRL_C) {
        break;
      }

      if(signals & SIGBREAKF_CTRL_D) {
        Playing = Recording = FALSE;
        ControlAudio();
      }

      if(signals & (1L << me->pr_MsgPort.mp_SigBit)) {
        while(msg = (struct slavemessage *) GetMsg(&me->pr_MsgPort)) {
          kprintf("gotmessage! %ld: 0x%08lx\n",msg->ID, msg->Data);
          switch(msg->ID) {
            case MSG_MODE:
              FreeAudio();
              AllocAudio();
              break;
            case MSG_HWPROP:
              TuneAudio();
              break;
            case MSG_RAWPLAY:
              msg->Data = (APTR) RawPlayback(msg->Data);
              break;
            case MSG_PLAY:
              kprintf("play\n");
              break;
            case MSG_RECORD:
              kprintf("record\n");
              break;
            case MSG_STOP:
              //kprintf("got stop!\n");
              Stop(*((ULONG *) msg->Data));
              break;
            case MSG_PAUSE:
              Pause(*((ULONG *) msg->Data));
              break;
            case MSG_LEVELON:
              kprintf("levelon\n");
              break;
            case MSG_LEVELOFF:
              kprintf("leveloff\n");
              break;
            default:
              kprintf("unknown\n");
              break;
          }
          ReplyMsg((struct Message *) msg);
        }
      }
    } /* while */
  }

  SlaveInitialized = FALSE;

  FreeAudio();

  AHIBase = NULL;
  if(!AHIDevice) {
    CloseDevice((struct IORequest *)AHIio);
  }
  DeleteIORequest((struct IORequest *)AHIio);
  DeleteMsgPort(AHImp);
  kprintf("(slave closed down)\n");

  Forbid();
  SlaveProcess = NULL;
}


/*
 *  AllocAudio(): Allocate the audio hardware
 */

static BOOL AllocAudio(void) {

  kprintf("(AllocAudio())...\n");

  /* Set up for HookFunc */
  SoundHook.h_Data = ToccataBase;
  RecordHook.h_Data = ToccataBase;

  MinMonVol = MaxMonVol = 0;
  MinOutVol = MaxOutVol = 0x10000;
  MinGain   = MaxGain   = 0x10000;

  audioctrl = AHI_AllocAudio(
      AHIA_AudioID,       (tprefs.Mode & TMODEF_STEREO ?
                           tprefs.StereoMode : tprefs.MonoMode),
      AHIA_MixFreq,       tprefs.Frequency,
      AHIA_Channels,      1,
      AHIA_Sounds,        2,
      AHIA_SoundFunc,     &SoundHook,

/*    AHIA_PlayerFreq,    (tprefs.Frequency / 512) << 16, */
      AHIA_PlayerFreq,    PLAYERFREQ << 16,
      AHIA_MinPlayerFreq, min(PLAYERFREQ, tprefs.Frequency / 512) << 16,
      AHIA_MaxPlayerFreq, max(PLAYERFREQ, tprefs.Frequency / 32) << 16,
      AHIA_RecordFunc,    &RecordHook,
      TAG_DONE);

  if(audioctrl != NULL) {
    AHI_GetAudioAttrs(AHI_INVALID_ID, audioctrl,
        AHIDB_MinMonitorVolume, &MinMonVol,
        AHIDB_MaxMonitorVolume, &MaxMonVol,
        AHIDB_MinOutputVolume,  &MinOutVol,
        AHIDB_MaxOutputVolume,  &MaxOutVol,
        AHIDB_MinInputGain,     &MinGain,
        AHIDB_MaxInputGain,     &MaxGain,
        TAG_DONE);

    fillhardinfo();
    TuneAudio();

    AudioInitialized = TRUE;
    kprintf("ok!\n");
    return TRUE;
  }
  kprintf("Nope!\n");
  return FALSE;
}


/*
 *  FreeAudio(): Release the audio hardware
 */

static void FreeAudio(void) {

  kprintf("(FreeAudio())...\n");

  if(audioctrl != NULL) {
    AHI_ControlAudio(audioctrl,
        AHIC_Play,   FALSE,
        AHIC_Record, FALSE,
        TAG_DONE);

    AHI_FreeAudio(audioctrl);
  }
  audioctrl = NULL;
  AudioInitialized = FALSE;
  kprintf("ok!\n");
}


/*
 *  TuneAudio(): Change (hardware) properties of the allocated audio mode
 */

static BOOL TuneAudio() {
  Fixed MonVol, OutVol, Gain;
  ULONG Input;
  BOOL rc = FALSE;

  kprintf("(TuneAudio())\n");

  if(audioctrl != NULL) {

    MonVol = negboundaries[tprefs.LoopbackVolume];
    OutVol = negboundaries[tprefs.OutputVolumeLeft];
    Gain   = posboundaries[tprefs.InputVolumeLeft];

    MonVol = min( max(MonVol, MinMonVol), MaxMonVol);
    OutVol = min( max(OutVol, MinOutVol), MaxOutVol);
    Gain   = min( max(Gain,   MinGain),   MaxGain);

    switch(tprefs.Input) {

      case TINPUT_Line:
        Input = tprefs.LineInput;
        break;

      case TINPUT_Aux1:
        Input = tprefs.Aux1Input;
        break;

      case TINPUT_Mic:
        if(tprefs.MicGain) {
          Input = tprefs.MicGainInput;
        }
        else {
          Input = tprefs.MicInput;
        }
        break;

      case TINPUT_Mix:
        Input = tprefs.MixInput;
        break;
    }

    rc = AHI_ControlAudio(audioctrl,
        AHIC_MonitorVolume, MonVol,
        AHIC_OutputVolume,  OutVol,
        AHIC_InputGain,     Gain,
        AHIC_Input,         Input,
        TAG_DONE);
    rc = (rc == AHIE_OK ? TRUE : FALSE);
  }

  return rc;
}


/*
 *  ControlAudio(): Start/Stop/Pause playing and recording
 */

static BOOL ControlAudio(void) {
  BOOL rc;

  kprintf("(ControlAudio())\n");

  if(audioctrl) {
    rc = AHI_ControlAudio(audioctrl,
        AHIC_Play,   (Playing && !Pausing),
        AHIC_Record, (Recording && !Pausing),
        TAG_DONE);
  }

  rc = (rc == AHIE_OK ? TRUE : FALSE);
  return rc;
}


/*
 *  Pause(): Take care of the T_Pause() unctionf
 */

static void Pause(ULONG pause) {

  kprintf("(Pause %ld)\n", pause);
  Pausing = pause;
  ControlAudio();
}


/*
 *  Stop(): Take care of the T_Stop() function
 */

static void Stop(ULONG flags) {

  kprintf("Stop(%lx)...\n", flags);

  Playing = FALSE;
  Recording = FALSE;
  ControlAudio();

  if(Sound0Loaded) {
    AHI_UnloadSound(0, audioctrl);
  }

  if(Sound1Loaded) {
    AHI_UnloadSound(1, audioctrl);
  }

  Sound0Loaded = Sound1Loaded = FALSE;

  if((flags & TSF_DONTSAVECACHE) == 0) {
    /* Save cache here... */
  }

  /* Check if a record/play file is open, and close them if so */
  kprintf("ok\n");
}


/*
 *  RawPlayback(): Take care of the T_RawPlayback() function
 */

static BOOL RawPlayback(struct TagItem *tags) {
  BOOL rc = TRUE;
  BOOL newmode = FALSE;
  struct TagItem *tstate;
  struct TagItem *tag;

  kprintf("RawPlayback()...\n");

  if(Playing || Recording) {
    return FALSE;
  }

  /* Fill defaults */

  ErrorTask       =
  RawTask         =
  SigTask         = NULL;
  RawInt          = NULL;
  RawBuffer1      =
  RawBuffer2      = NULL;
  RawBufferSize   = 32768;
  RawIrqSize      = 512;
  ByteCount       = NULL;
  ByteSkip        =
  ByteSkipCounter = 2048;


  /* Check arguments */

  tstate = tags;

  while (tag = NextTagItem(&tstate)) {
    //kprintf("%ld, 0x%08lx,\n", tag->ti_Tag - TT_Min, tag->ti_Data);
    switch (tag->ti_Tag) {

      case TT_IrqPri:
        break;

      case TT_Mode:
        if(tag->ti_Data != tprefs.Mode) {
          tprefs.Mode = tag->ti_Data;
          newmode = TRUE;
        }
        break;

      case TT_Frequency:
        if(tag->ti_Data != tprefs.Frequency) {
          tprefs.Frequency = tag->ti_Data;
          newmode = TRUE;
        }
        break;

      case TT_ErrorTask:
        ErrorTask = (struct Task *) tag->ti_Data;
        break;
        
      case TT_ErrorMask:
        ErrorMask = tag->ti_Data;
        break;

      case TT_RawTask:
        RawTask = (struct Task *) tag->ti_Data;
        break;

      case TT_RawMask:
        RawMask = tag->ti_Data;
        break;

      case TT_RawReply:
      {
        ULONG *p = (ULONG *) tag->ti_Data;
        
        *p = GetRawReply(ToccataBase);
        kprintf("Rawreply is 0x%08lx\n", *p);
        break;
      }

      case TT_RawInt:
        RawInt = (struct Interrupt *) tag->ti_Data;
        break;

      case TT_RawBuffer1:
        RawBuffer1 = (BYTE *) tag->ti_Data;
        break;

      case TT_RawBuffer2:
        RawBuffer2 = (BYTE *) tag->ti_Data;
        break;

      case TT_BufferSize:
        RawBufferSize = tag->ti_Data;
        break;

      case TT_RawIrqSize:
        RawIrqSize = tag->ti_Data;
        break;

      case TT_ByteCount:
        ByteCount = (ULONG *) tag->ti_Data;
        break;

      case TT_ByteSkip:
        ByteSkip = tag->ti_Data;
        break;

      case TT_SigTask:
        SigTask = (struct Task *) tag->ti_Data;
        break;

      case TT_SigMask:
        SigMask = tag->ti_Data;
        break;

    } /* switch */
  } /* while */

  if((ErrorTask == NULL) ||
     ((RawTask == NULL) && (RawInt == NULL)) ||
     (RawBuffer1 == NULL) ||
     (RawBuffer2 == NULL)) {

    rc = FALSE;
  }

  Stop(0);

  if(rc && (newmode || !AudioInitialized)) {
    FreeAudio();
    rc = AllocAudio();
  }


  if(rc) {
    ULONG sampletype = AHIST_NOTYPE;
    struct AHISampleInfo s0, s1;

    switch(tprefs.Mode) {
      case TMODE_LINEAR_8:
          sampletype = AHIST_M8S;
          break;
      case TMODE_LINEAR_16:
          sampletype = AHIST_M16S;
          break;
      case TMODE_ALAW:
      case TMODE_ULAW:
      case TMODE_RAW_16:
          rc = FALSE;
          break;
      case TMODE_LINEAR_8_S:
          sampletype = AHIST_S8S;
          break;
      case TMODE_LINEAR_16_S:
          sampletype = AHIST_S16S;
          break;
      case TMODE_ALAW_S:
      case TMODE_ULAW_S:
      case TMODE_RAW_16_S:
          rc = FALSE;
          break;
      default:
          rc = FALSE;
    }

    if(sampletype != AHIST_NOTYPE) {

      s0.ahisi_Type    =
      s1.ahisi_Type    = sampletype;
      s0.ahisi_Address = RawBuffer1;
      s1.ahisi_Address = RawBuffer2;
      s0.ahisi_Length  =
      s1.ahisi_Length  = RawBufferSize / AHI_SampleFrameSize(sampletype);

      if(AHI_LoadSound(0, AHIST_DYNAMICSAMPLE, &s0, audioctrl) == AHIE_OK) {
        Sound0Loaded = TRUE;
      }

      if(AHI_LoadSound(1, AHIST_DYNAMICSAMPLE, &s1, audioctrl) == AHIE_OK) {
        Sound1Loaded = TRUE;
      }
      
      if(!(Sound0Loaded && Sound1Loaded)) {
        rc = FALSE;
      }

      RawChunckLength = RawIrqSize / AHI_SampleFrameSize(sampletype);
      RawBufferLength = RawBufferSize / AHI_SampleFrameSize(sampletype);
    }
  }

  if(rc) {
    Playing         = TRUE;
    SoundFlag       = 0;
    NextBufferOK    = TRUE;
    RawBufferPtr    = RawBuffer1;
    RawBufferOffset = 0;;

    ControlAudio();
    AHI_Play(audioctrl,
      AHIP_BeginChannel,  0,
      AHIP_Freq,          tprefs.Frequency,
      AHIP_Vol,           0x10000,
      AHIP_Pan,           0x8000,
      AHIP_Sound,         0,
      AHIP_Offset,        0,
      AHIP_Length,        RawChunckLength,
      AHIP_EndChannel,    NULL,
      TAG_DONE);
  }

  kprintf("ok %ld\n", rc);
  return rc;
}
