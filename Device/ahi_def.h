/* $Id$ */
// :ts=8
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

extern struct ExecBase		*SysBase;
extern struct AHIBase		*AHIBase;
extern struct DosLibrary	*DOSBase;
extern struct Library		*GadToolsBase;
extern struct GfxBase		*GfxBase;
extern struct Library		*IFFParseBase;
extern struct IntuitionBase	*IntuitionBase;
extern struct LocaleBase	*LocaleBase;
extern struct Device		*TimerBase;
extern struct UtilityBase	*UtilityBase;
extern struct Library           *PPCLibBase;
extern void                     *AHIPPCObject;


/*** Definitions ***/

#ifdef VERSION68K

struct Fixed64
{
	LONG	I;
	ULONG	F;
};

typedef struct Fixed64	Fixed64;

#else /* VERSION68K */

typedef long long int	Fixed64;

#endif /* VERSION68K */


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
	struct AHIDevUnit		*ahib_DevUnits[AHI_UNITS];
	struct SignalSemaphore	 ahib_Lock;
	ULONG			 ahib_AudioMode;
	ULONG			 ahib_Frequency;
	Fixed			 ahib_MonitorVolume;
	Fixed			 ahib_InputGain;
	Fixed			 ahib_OutputVolume;
	ULONG			 ahib_Input;
	ULONG			 ahib_Output;
	Fixed			 ahib_MaxCPU;
	ULONG		 	 ahib_AntiClickSamples;
};


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
	Fixed64	cd_Offset;
	Fixed64	cd_Add;
	APTR	cd_DataStart;
	Fixed64	cd_LastOffset;
	Fixed	cd_ScaleLeft;
	Fixed	cd_ScaleRight;
	APTR	cd_AddRoutine;
	Fixed	cd_VolumeLeft;
	Fixed	cd_VolumeRight;
	ULONG	cd_Type;

	UWORD	cd_NextEOS;		/* Not in use */
	UBYTE	cd_NextFreqOK;
	UBYTE	cd_NextSoundOK;
	Fixed64	cd_NextOffset;
	Fixed64	cd_NextAdd;
	APTR	cd_NextDataStart;
	Fixed64	cd_NextLastOffset;
	Fixed	cd_NextScaleLeft;
	Fixed	cd_NextScaleRight;
	APTR	cd_NextAddRoutine;
	Fixed	cd_NextVolumeLeft;
	Fixed	cd_NextVolumeRight;
	ULONG	cd_NextType;

	LONG	cd_Samples;		/* Samples left to store (down-counter) */
	LONG	cd_FirstOffsetI;		/* for linear interpolation routines */
	LONG	cd_LastSampleL;		/* for linear interpolation routines */
	LONG	cd_TempLastSampleL;	/* for linear interpolation routines */
	LONG	cd_LastSampleR;		/* for linear interpolation routines */
	LONG	cd_TempLastSampleR;	/* for linear interpolation routines */
	LONG	cd_LastScaledSampleL;	/* for anticlick */
	LONG	cd_LastScaledSampleR;	/* for anticlick */

	struct AHIChannelData *cd_Succ;	/* For the wet and dry lists */
	UWORD	cd_ChannelNo;
	UWORD	cd_Pad;
	LONG	cd_AntiClickCount;
};

#define AHIACB_NOMIXING	31	/* private ahiac_Flags flag */
#define AHIACF_NOMIXING	(1L<<31)	/* private ahiac_Flags flag */
#define AHIACB_NOTIMING	30	/* private ahiac_Flags flag */
#define AHIACF_NOTIMING	(1L<<30)	/* private ahiac_Flags flag */
#define AHIACB_POSTPROC	29	/* private ahiac_Flags flag */
#define AHIACF_POSTPROC	(1L<<29)	/* private ahiac_Flags flag */
#define AHIACB_CLIPPING	28	/* private ahiac_Flags flag */
#define AHIACF_CLIPPING	(1L<<28)	/* private ahiac_Flags flag */

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
	UWORD			 ahiac_Com;		/* PPC communication variable */
	UWORD			 ahiac_ChannelNo;	/* PPC communication variable */
	UWORD			 ahiac_Pad;
	APTR			 ahiac_AntiClickBuffer;
	ULONG			 ahiac_AntiClickSize;	/* in bytes */
	char			 ahiac_DriverName[ 256 ];
};

#define AHIAC_COM_NONE		0
#define AHIAC_COM_ACK		1
#define AHIAC_COM_INIT		2
#define AHIAC_COM_SOUNDFUNC	3
#define AHIAC_COM_QUIT		4

#endif /* AHI_DEF_H_ */
