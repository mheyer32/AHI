/* $Id$ */

/*
     AHI - Hardware independent audio subsystem
     Copyright (C) 1996-1999 Martin Blom <martin@blom.org>
     
     This library is free software; you can redistribute it and/or
     modify it under the terms of the GNU Library General Public
     License as published by the Free Software Foundation; either
     version 2 of the License, or (at your option) any later version.
     
     This library is distributed in the hope that it will be useful,
     but WITHOUT ANY WARRANTY; without even the implied warranty of
     MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
     Library General Public License for more details.
     
     You should have received a copy of the GNU Library General Public
     License along with this library; if not, write to the
     Free Software Foundation, Inc., 59 Temple Place - Suite 330, Cambridge,
     MA 02139, USA.
*/

#ifndef _AHI_DEF_H_
#define _AHI_DEF_H_

/*** Debug stuff ***/

extern void KPrintF(char *fmt,...);
extern void kprintf(char *fmt,...);
#define HIT {char *a=0; *a=0;}

/*** Processor identification ****/

#ifdef mc68020
# define MC68020_PLUS
#endif

#ifdef mc68030
# define MC68020_PLUS
# define MC68030_PLUS
#endif

#ifdef mc68040
# define MC68020_PLUS
# define MC68030_PLUS
# define MC68040_PLUS
#endif

#ifdef mc68060
# define MC68020_PLUS
# define MC68030_PLUS
# define MC68040_PLUS
# define MC68060_PLUS
#endif

#ifdef MC68020_PLUS
# define HAVE_HIFI
# define HAVE_CLIPPING
#endif

/*** AHI include files ***/

#include <config.h>
#include <CompilerSpecific.h>

#include <exec/semaphores.h>
#include <exec/devices.h>
#include <devices/ahi.h>
#include <devices/timer.h>
#include <libraries/ahi_sub.h>
#include <utility/hooks.h>

struct Echo;
struct AHIDevUnit;


/*** Globals ***/

extern struct ExecBase          *SysBase;
extern struct AHIBase           *AHIBase;
extern struct DosLibrary        *DOSBase;
extern struct Library           *GadToolsBase;
extern struct GfxBase           *GfxBase;
extern struct Library           *IFFParseBase;
extern struct IntuitionBase     *IntuitionBase;
extern struct LocaleBase        *LocaleBase;
extern struct Device            *TimerBase;
extern struct UtilityBase       *UtilityBase;
extern struct Library           *PowerPCBase;
extern struct Library           *PPCLibBase;
extern void                     *PPCObject;


/*** Definitions ***/

#ifdef VERSION68K

struct Fixed64
{
        LONG    I;
        ULONG   F;
};

typedef struct Fixed64  Fixed64;

#else /* VERSION68K */

typedef long long int   Fixed64;

#endif /* VERSION68K */


#define AHI_UNITS       4       /* Normal units, excluding AHI_NO_UNIT */

#define AHIBB_NOSURROUND        (0)
#define AHIBF_NOSURROUND        (1L<<0)
#define AHIBB_NOECHO            (1)
#define AHIBF_NOECHO            (1L<<1)
#define AHIBB_FASTECHO          (2)
#define AHIBF_FASTECHO          (1L<<2)
#define AHIBB_CLIPPING          (3)
#define AHIBF_CLIPPING          (1L<<3)

/* AHIBase */
struct AHIBase
{
  struct Library           ahib_Library;
  UBYTE                    ahib_Flags;
  UBYTE                    ahib_DebugLevel;
  struct ExecBase         *ahib_SysLib;
  ULONG                    ahib_SegList;
  APTR                     ahib_AudioCtrl;
  struct AHIDevUnit       *ahib_DevUnits[AHI_UNITS];
  struct SignalSemaphore   ahib_Lock;
  ULONG                    ahib_AudioMode;
  ULONG                    ahib_Frequency;
  Fixed                    ahib_MonitorVolume;
  Fixed                    ahib_InputGain;
  Fixed                    ahib_OutputVolume;
  ULONG                    ahib_Input;
  ULONG                    ahib_Output;
  Fixed                    ahib_MaxCPU;
  ULONG                    ahib_AntiClickSamples;
};


struct Timer
{
  struct EClockVal         EntryTime;
  struct EClockVal         ExitTime;
};


struct AHISoundData
{
  ULONG   sd_Type;
  APTR    sd_Addr;
  ULONG   sd_Length;
  APTR    sd_InputBuffer[3];
};

/* Private AHIChannelData */

#ifdef VERSION68K

struct AHIChannelData
{
  UWORD   cd_EOS;                 /* $FFFF: Sample has reached end */
  UBYTE   cd_FreqOK;              /* $00: Freq=0 ; $FF: Freq<>0 */
  UBYTE   cd_SoundOK;             /* $00: No sound set ; $FF: S. OK. */
  Fixed64 cd_Offset;
  Fixed64 cd_Add;
  APTR    cd_DataStart;
  Fixed64 cd_LastOffset;
  Fixed   cd_ScaleLeft;
  Fixed   cd_ScaleRight;
  APTR    cd_AddRoutine;
  Fixed   cd_VolumeLeft;
  Fixed   cd_VolumeRight;
  ULONG   cd_Type;

  UWORD   cd_NextEOS;             /* Not in use */
  UBYTE   cd_NextFreqOK;
  UBYTE   cd_NextSoundOK;
  Fixed64 cd_NextOffset;
  Fixed64 cd_NextAdd;
  APTR    cd_NextDataStart;
  Fixed64 cd_NextLastOffset;
  Fixed   cd_NextScaleLeft;
  Fixed   cd_NextScaleRight;
  APTR    cd_NextAddRoutine;
  Fixed   cd_NextVolumeLeft;
  Fixed   cd_NextVolumeRight;
  ULONG   cd_NextType;

  LONG    cd_Samples;             /* Samples left to store (down-counter) */
  LONG    cd_FirstOffsetI;        /* for linear interpolation routines */
  LONG    cd_LastSampleL;         /* for linear interpolation routines */
  LONG    cd_TempLastSampleL;     /* for linear interpolation routines */
  LONG    cd_LastSampleR;         /* for linear interpolation routines */
  LONG    cd_TempLastSampleR;     /* for linear interpolation routines */

  struct AHIChannelData *cd_Succ; /* For the wet and dry lists */
  UWORD   cd_ChannelNo;
  UWORD   cd_Pad;
} __attribute__  ((packed));

#else

struct AHIChannelData
{
  UWORD   cd_EOS;                 /* $FFFF: Sample has reached end */

  UBYTE   cd_FreqOK;              /* $00: Freq=0 ; $FF: Freq<>0 */
  UBYTE   cd_SoundOK;             /* $00: No sound set ; $FF: S. OK. */
  APTR    cd_DataStart;
  Fixed64 cd_Offset;
  Fixed64 cd_Add;
  Fixed64 cd_LastOffset;
  Fixed   cd_ScaleLeft;
  Fixed   cd_ScaleRight;
  Fixed   cd_VolumeLeft;
  Fixed   cd_VolumeRight;
  APTR    cd_AddRoutine;
  ULONG   cd_Type;

  UWORD   cd_Pad0;

  UBYTE   cd_NextFreqOK;
  UBYTE   cd_NextSoundOK;
  APTR    cd_NextDataStart;
  Fixed64 cd_NextOffset;
  Fixed64 cd_NextAdd;
  Fixed64 cd_NextLastOffset;
  Fixed   cd_NextScaleLeft;
  Fixed   cd_NextScaleRight;
  Fixed   cd_NextVolumeLeft;
  Fixed   cd_NextVolumeRight;
  APTR    cd_NextAddRoutine;
  ULONG   cd_NextType;

  UWORD   cd_Pad1;

  UBYTE   cd_DelayedFreqOK;
  UBYTE   cd_DelayedSoundOK;
  APTR    cd_DelayedDataStart;
  Fixed64 cd_DelayedOffset;
  Fixed64 cd_DelayedAdd;
  Fixed64 cd_DelayedLastOffset;
  Fixed   cd_DelayedScaleLeft;
  Fixed   cd_DelayedScaleRight;
  Fixed   cd_DelayedVolumeLeft;
  Fixed   cd_DelayedVolumeRight;
  APTR    cd_DelayedAddRoutine;
  ULONG   cd_DelayedType;

  BOOL    cd_SoundDelayed;
  BOOL    cd_FreqDelayed;
  BOOL    cd_VolDelayed;
  UWORD   cd_Pad2;

  LONG    cd_Samples;             /* Samples left to store (down-counter) */
  LONG    cd_FirstOffsetI;        /* for linear interpolation routines */

  LONG    cd_DelayedSamples;
  LONG    cd_DelayedFirstOffsetI;

  LONG    cd_StartPointL;         /* for linear interpolation routines */
  LONG    cd_TempStartPointL;     /* for linear interpolation routines */
  LONG    cd_StartPointR;         /* for linear interpolation routines */
  LONG    cd_TempStartPointR;     /* for linear interpolation routines */

  struct AHIChannelData *cd_Succ; /* For the wet and dry lists */
  UWORD   cd_ChannelNo;
  UWORD   cd_Pad3;
  LONG    cd_AntiClickCount;
};

#endif

#define AHIACB_NOMIXING 31              /* private ahiac_Flags flag */
#define AHIACF_NOMIXING (1L<<31)        /* private ahiac_Flags flag */
#define AHIACB_NOTIMING 30              /* private ahiac_Flags flag */
#define AHIACF_NOTIMING (1L<<30)        /* private ahiac_Flags flag */
#define AHIACB_POSTPROC 29              /* private ahiac_Flags flag */
#define AHIACF_POSTPROC (1L<<29)        /* private ahiac_Flags flag */
#define AHIACB_CLIPPING 28              /* private ahiac_Flags flag */
#define AHIACF_CLIPPING (1L<<28)        /* private ahiac_Flags flag */

/* Private AudioCtrl structure */

struct AHIPrivAudioCtrl
{
  struct  AHIAudioCtrlDrv    ac;
  struct  Library           *ahiac_SubLib;
  ULONG                      ahiac_SubAllocRC;
  struct AHIChannelData     *ahiac_ChannelDatas;
  struct AHISoundData       *ahiac_SoundDatas;
  ULONG                      ahiac_BuffSizeNow;     /* How many bytes of the buffer are used? */

  APTR                       ahiac_MasterVolumeTable;
  APTR                       ahiac_MultTableS;
  APTR                       ahiac_MultTableU;
  struct Hook               *ahiac_RecordFunc;      /* AHIA_RecordFunc */
  ULONG                      ahiac_AudioID;
  Fixed                      ahiac_MasterVolume;    /* Real */
  Fixed                      ahiac_SetMasterVolume; /* Set by user */
  Fixed                      ahiac_EchoMasterVolume;/* Set by dspecho */
  struct AHIEffOutputBuffer *ahiac_EffOutputBufferStruct;
  struct Echo               *ahiac_EffDSPEchoStruct;
  struct AHIEffChannelInfo  *ahiac_EffChannelInfoStruct;
  struct AHIChannelData     *ahiac_WetList;
  struct AHIChannelData     *ahiac_DryList;
  UBYTE                      ahiac_WetOrDry;
  UBYTE                      ahiac_MaxCPU;
  UWORD                      ahiac_Channels2;       /* Max virtual channels/hw channel */
  struct Timer               ahiac_Timer;
  UWORD                      ahiac_UsedCPU;
  volatile UWORD             ahiac_PPCCommand;
  volatile void             *ahiac_PPCArgument;
  APTR                       ahiac_PPCMixBuffer;
  struct Interrupt          *ahiac_PPCMixInterrupt;
  APTR                       ahiac_M68KPort;
  APTR                       ahiac_PPCStartupMsg;
  APTR                       ahiac_PPCTask;
  char                       ahiac_DriverName[ 256 ];
};  

#define AHIAC_COM_NONE          0
#define AHIAC_COM_START         1
#define AHIAC_COM_ACK           2
#define AHIAC_COM_INIT          3
#define AHIAC_COM_SOUNDFUNC     4
#define AHIAC_COM_QUIT          5
#define AHIAC_COM_DEBUG         6
#define AHIAC_COM_FINISHED      7

#endif /* AHI_DEF_H_ */
