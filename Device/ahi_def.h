/* :ts= 8 */

/* $Id$
* $Log$
* Revision 4.5  1997/12/21 17:41:50  lcs
* Major source cleanup, moved some functions to separate files.
*
* Revision 4.4  1997/10/11 15:58:13  lcs
* Added the ahiac_UsedCPU field to the AHIAudioCtrl structure.
*
*/

#ifndef AHI_DEF_H
#define AHI_DEF_H

/*** Debug stuff ***/

extern void KPrintF(char *fmt,...);
extern void kprintf(char *fmt,...);
#define HIT {char *a=0; *a=0;}

/*** AHI include files ***/

#include <devices/ahi.h>
#include <devices/timer.h>
#include <libraries/ahi_sub.h>
#include <utility/hooks.h>
#include <pragmas/ahi_pragmas.h>
#include <clib/ahi_protos.h>
#include <proto/ahi_sub.h>


#include <exec/semaphores.h>
#include <exec/devices.h>

#include "ahi_device.h"
#include "dsp.h"

extern __far struct AHIBase		*AHIBase;
extern __far struct DosLibrary		*DOSBase;
extern __far struct Library		*GadToolsBase;
extern __far struct GfxBase		*GfxBase;
extern __far struct Library		*IFFParseBase;
extern __far struct IntuitionBase	*IntuitionBase;
extern __far struct Library		*LocaleBase;
extern __far struct Library		*TimerBase;
extern __far struct Library		*UtilityBase;

extern __far ULONG			 DriverVersion;
extern __far ULONG			 Version;
extern __far ULONG			 Revision;
extern __far UBYTE			*DevName;
extern __far UBYTE			*IDString;

extern __far ULONG			 AHIChannelData_SIZEOF;
extern __far ULONG			 AHISoundData_SIZEOF;


extern __stdargs char *Sprintf(char *, const char *, ...);

#define AHI_UNITS	4	/* Normal units, excluding AHI_NO_UNIT */

#define AHIBB_NOSURROUND	(0)
#define AHIBF_NOSURROUND	(1L<<0)
#define AHIBB_NOECHO		(1)
#define AHIBF_NOECHO		(1L<<1)
#define AHIBB_FASTECHO		(2)
#define AHIBF_FASTECHO		(1L<<2)
#define AHIBB_CLIPPING		(3)
#define AHIBF_CLIPPING		(1L<<3)

/* AHIBase */
struct AHIBase
{
	struct Library		 ahib_Library;
	UBYTE			 ahib_Flags;
	UBYTE			 ahib_DebugLevel;
	struct ExecBase		*ahib_SysLib;
	ULONG			 ahib_SegList;
	APTR			 ahib_AudioCtrl;
	struct AHIDevUnit	*ahib_DevUnits[AHI_UNITS];
	struct SignalSemaphore	 ahib_Lock;
	struct StartupMessage	 ahib_Startup;
	ULONG			 ahib_AudioMode;
	ULONG			 ahib_Frequency;
	Fixed			 ahib_MonitorVolume;
	Fixed			 ahib_InputGain;
	Fixed			 ahib_OutputVolume;
	ULONG			 ahib_Input;
	ULONG			 ahib_Output;
	Fixed			 ahib_MaxCPU;
};


#define DRIVERNAME_SIZEOF sizeof("DEVS:ahi/                          .audio")

struct Timer
{
	struct EClockVal	 EntryTime;
	struct EClockVal	 ExitTime;
};

struct AHISoundData
{
	ULONG	sd_Type;
	APTR	sd_Addr;
	ULONG	sd_Length;
	APTR	sd_InputBuffer[3];
};

/* Private AHIChannelData */

struct AHIChannelData
{
	UWORD	cd_EOS;			/* $FFFF: Sample has reached end */
	UBYTE	cd_FreqOK;		/* $00: Freq=0 ; $FF: Freq<>0 */
	UBYTE	cd_SoundOK;		/* $00: No sound set ; $FF: S. OK. */
	ULONG	cd_OffsetI;
	UWORD	cd_Pad1;
	UWORD	cd_OffsetF;
	ULONG	cd_AddI;
	UWORD	cd_Pad2;
	UWORD	cd_AddF;
	APTR	cd_DataStart;
	ULONG	cd_LastOffsetI;
	UWORD	cd_Pad3;
	UWORD	cd_LastOffsetF;
	ULONG	cd_ScaleLeft;
	ULONG	cd_ScaleRight;
	APTR	cd_AddRoutine;
	Fixed	cd_VolumeLeft;
	Fixed	cd_VolumeRight;
	ULONG	cd_Type;

	UWORD	cd_NextEOS;		/* Not in use */
	UBYTE	cd_NextFreqOK;
	UBYTE	cd_NextSoundOK;
	ULONG	cd_NextOffsetI;
	UWORD	cd_NextPad1;
	UWORD	cd_NextOffsetF;
	ULONG	cd_NextAddI;
	UWORD	cd_NextPad2;
	UWORD	cd_NextAddF;
	APTR	cd_NextDataStart;
	ULONG	cd_NextLastOffsetI;
	UWORD	cd_NextPad3;
	UWORD	cd_NextLastOffsetF;
	ULONG	cd_NextScaleLeft;
	ULONG	cd_NextScaleRight;
	APTR	cd_NextAddRoutine;
	Fixed	cd_NextVolumeLeft;
	Fixed	cd_NextVolumeRight;
	ULONG	cd_NextType;

	ULONG	cd_Samples;		/* Samples left to store (down-counter) */
	ULONG	cd_FirstOffsetI;	/* for linear interpolation routines */
	LONG	cd_LastSampleL;		/* for linear interpolation routines */
	LONG	cd_TempLastSampleL;	/* for linear interpolation routines */
	LONG	cd_LastSampleR;		/* for linear interpolation routines */
	LONG	cd_TempLastSampleR;	/* for linear interpolation routines */

	struct AHIChannelData *cd_Succ;	/* For the wet and dry lists */
	UWORD	cd_ChannelNo;
	UWORD	cd_Pad;
};

#define AHIACB_NOMIXING	31		/* private ahiac_Flags flag */
#define AHIACF_NOMIXING	(1L<<31)	/* private ahiac_Flags flag */
#define AHIACB_NOTIMING	30		/* private ahiac_Flags flag */
#define AHIACF_NOTIMING	(1L<<30)	/* private ahiac_Flags flag */
#define AHIACB_POSTPROC 29		/* private ahiac_Flags flag */
#define AHIACF_POSTPROC	(1L<<29)	/* private ahiac_Flags flag */
#define AHIACB_CLIPPING 28		/* private ahiac_Flags flag */
#define AHIACF_CLIPPING (1L<<28)	/* private ahiac_Flags flag */

/* Private AudioCtrl structure */

struct AHIPrivAudioCtrl
{
	struct	AHIAudioCtrlDrv	 ac;
	struct	Library		*ahiac_SubLib;
	ULONG			 ahiac_SubAllocRC;
	struct AHIChannelData	*ahiac_ChannelDatas;
	struct AHISoundData	*ahiac_SoundDatas;
	ULONG			 ahiac_BuffSizeNow;	/* How many bytes of the buffer are used? */

	/* For AHIST_INPUT */
	APTR			 ahiac_InputBuffer[3];  /* Filling, filled, old filled */
	ULONG			 ahiac_InputLength;
	ULONG			 ahiac_InputBlockLength;
	APTR			 ahiac_InputRecordPtr;
	ULONG			 ahiac_InputRecordCnt;

	APTR			 ahiac_MasterVolumeTable;
	APTR			 ahiac_MultTableS;
	APTR			 ahiac_MultTableU;
	struct Hook		*ahiac_RecordFunc;	/* AHIA_RecordFunc */
	ULONG			 ahiac_AudioID;
	Fixed			 ahiac_MasterVolume;	/* Real */
	Fixed			 ahiac_SetMasterVolume;	/* Set by user */
	Fixed			 ahiac_EchoMasterVolume;/* Set by dspecho */
	struct AHIEffOutputBuffer *ahiac_EffOutputBufferStruct;
	struct Echo		*ahiac_EffDSPEchoStruct;
	struct AHIEffChannelInfo *ahiac_EffChannelInfoStruct;
	struct AHIChannelData	*ahiac_WetList;
	struct AHIChannelData	*ahiac_DryList;
	UBYTE			 ahiac_WetOrDry;
	UBYTE			 ahiac_MaxCPU;
	UWORD			 ahiac_Channels2;	/* Max virtual channels/hw channel */
	struct Timer		 ahiac_Timer;
	UWORD			 ahiac_UsedCPU;
	UWORD			 ahiac_Pad;
	char			 ahiac_DriverName[DRIVERNAME_SIZEOF];
};



#endif /* AHI_DEF_H */
