/* $Id$
* $Log$
* Revision 1.9  1997/03/13 00:19:43  lcs
* Up to 4 device units are now available.
*
* Revision 1.8  1997/02/02 22:35:50  lcs
* Localized it
*
* Revision 1.7  1997/02/02 18:15:04  lcs
* Added protection against CPU overload
*
* Revision 1.6  1997/02/01 23:54:26  lcs
* Rewrote the library open code in C and removed the library bases
* from AHIBase
*
* Revision 1.4  1997/01/04 20:19:56  lcs
* ahiac_EffChannelInfoStruct addded
*
* Revision 1.3  1997/01/04 13:26:41  lcs
* Debugged CMD_WRITE
*
* Revision 1.1  1996/12/21 13:05:12  lcs
* Initial revision
*
*/

/*** Debug stuff ***/

extern void KPrintF(char *fmt,...);
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


extern char __stdargs *Sprintf(char *, const char *, ...);

#define AHI_UNITS	4	/* Normal units, excluding AHI_NO_UNIT */

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

#define AHIBB_NOSURROUND	(0)
#define AHIBF_NOSURROUND	(1L<<0)
#define AHIBB_NOECHO		(1)
#define AHIBF_NOECHO		(1L<<1)
#define AHIBB_FASTECHO		(2)
#define AHIBF_FASTECHO		(1L<<2)

#define DRIVERNAME_SIZEOF sizeof("DEVS:ahi/                          .audio")

struct Timer
{
	struct EClockVal	 EntryTime;
	struct EClockVal	 ExitTime;
};

/* Private AudioCtrl structure */
struct AHIPrivAudioCtrl
{
	struct	AHIAudioCtrlDrv	 ac;
	struct	Library		*ahiac_SubLib;
	ULONG			 ahiac_SubAllocRC;
	APTR			 ahiac_ChannelDatas;
	APTR			 ahiac_SoundDatas;
	ULONG			 ahiac_BuffSizeNow;	/* How many bytes of the buffer are used? */

	APTR			 ahiac_MultTableS;
	APTR			 ahiac_MultTableU;
	struct Hook		*ahiac_RecordFunc;	/* AHIA_RecordFunc */
	ULONG			 ahiac_AudioID;
	Fixed			 ahiac_MasterVolume;
	struct AHIEffOutputBuffer *ahiac_EffOutputBufferStruct;
	APTR			*ahiac_EffDSPEchoStruct;
	struct AHIEffChannelInfo *ahiac_EffChannelInfoStruct;
	APTR			 ahiac_WetList;
	APTR			 ahiac_DryList;
	UBYTE			 ahiac_WetOrDry;
	UBYTE			 ahiac_MaxCPU;
	UWORD			 ahiac_Channels2;	/* Max virtual channels/hw channel */
	struct Timer		 ahiac_Timer;
	char			 ahiac_DriverName[DRIVERNAME_SIZEOF];
};

#define AHIACB_NOMIXING	31		/* private ahiac_Flags flag */
#define AHIACF_NOMIXING	(1L<<31)	/* private ahiac_Flags flag */
#define AHIACB_NOTIMING	30		/* private ahiac_Flags flag */
#define AHIACF_NOTIMING	(1L<<30)	/* private ahiac_Flags flag */
#define AHIACB_POSTPROC 29		/* private ahiac_Flags flag */
#define AHIACF_POSTPROC	(1L<<29)	/* private ahiac_Flags flag */
