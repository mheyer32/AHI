/* $Id$
* $Log$
* Revision 1.5  1997/01/15 14:59:50  lcs
* Added CMD_FLUSH, CMD_START, CMD_STOP and SMD_RESET
*
* Revision 1.4  1997/01/04 20:19:56  lcs
* PLAYERFREQ added
*
* Revision 1.3  1997/01/04 13:26:41  lcs
* Debugged CMD_WRITE
*
* Revision 1.2  1996/12/21 23:08:47  lcs
* AHICMD_FINISHED => AHICMD_WRITTEN
* New stuff in the Voice structure
*
* Revision 1.1  1996/12/21 13:05:12  lcs
* Initial revision
*
*/

/*** New Style Device definitions ***/

#define NSCMD_DEVICEQUERY   0x4000

#define NSDEVTYPE_UNKNOWN       0   /* No suitable category, anything */
#define NSDEVTYPE_GAMEPORT      1   /* like gameport.device */
#define NSDEVTYPE_TIMER         2   /* like timer.device */
#define NSDEVTYPE_KEYBOARD      3   /* like keyboard.device */
#define NSDEVTYPE_INPUT         4   /* like input.device */
#define NSDEVTYPE_TRACKDISK     5   /* like trackdisk.device */
#define NSDEVTYPE_CONSOLE       6   /* like console.device */
#define NSDEVTYPE_SANA2         7   /* A >=SANA2R2 networking device */
#define NSDEVTYPE_AUDIOARD      8   /* like audio.device */
#define NSDEVTYPE_CLIPBOARD     9   /* like clipboard.device */
#define NSDEVTYPE_PRINTER       10  /* like printer.device */
#define NSDEVTYPE_SERIAL        11  /* like serial.device */
#define NSDEVTYPE_PARALLEL      12  /* like parallel.device */

struct NSDeviceQueryResult
{
  /*
  ** Standard information
  */
  ULONG   DevQueryFormat;         /* this is type 0               */
  ULONG   SizeAvailable;          /* bytes available              */

  /*
  ** Common information (READ ONLY!)
  */
  UWORD   DeviceType;             /* what the device does         */
  UWORD   DeviceSubType;          /* depends on the main type     */
  UWORD   *SupportedCommands;     /* 0 terminated list of cmd's   */

  /* May be extended in the future! Check SizeAvailable! */
};

#define DRIVE_NEWSTYLE  (0x4E535459L)   /* 'NSTY' */
#define NSCMD_TD_READ64     0xc000
#define NSCMD_TD_WRITE64    0xc001
#define NSCMD_TD_SEEK64     0xc002
#define NSCMD_TD_FORMAT64   0xc003


/*** My own stuff ***/

#define AHI_UNITS	1	/* Normal units, excluding AHI_NO_UNIT */
#define AHI_PRI		50	/* Priority for the device process */

#define SND8  0
#define SND16 1

#define PLAYERFREQ	50	/* How often the PlayerFunc is called */

#define AHICMD_END	CMD_NONSTD

#define AHICMD_WRITTEN	(0x8000 | CMD_WRITE)

#define ahir_Channel	ahir_Pad1
#define NOCHANNEL	65535

struct Voice
{
	UWORD			 NextSound;
	UBYTE			 Pad[2];
	Fixed			 NextVolume;
	Fixed			 NextPan;
	ULONG			 NextFrequency;
	ULONG			 NextOffset;
	ULONG			 NextLength;
	struct AHIRequest	*NextRequest;
	
	struct AHIRequest	*QueuedRequest;
	struct AHIRequest	*PlayingRequest;
};

/* Special Offset values */

#define FREE	0	/* Channel is not playing anything */
#define	MUTE	-1	/* Channel will be muted when current sound is finished */
#define PLAY	-2	/* Channel will play more when current sound is finished */

struct AHIDevUnit
{
	struct Unit		 Unit;
	UBYTE			 UnitNum;
	BYTE			 PlaySignal;
	BYTE			 RecordSignal;
	BYTE			 SampleSignal;
	struct Process		*Process;
	struct Process		*Master;
	struct Hook		 PlayerHook;
	struct Hook 		 RecordHook;
	struct Hook 		 SoundHook;
	struct Hook		 ChannelInfoHook;
	
	struct AHIEffChannelInfo *ChannelInfoStruct;

	WORD			*RecordBuffer;
	ULONG			 RecordSize;

	BOOL			 IsPlaying;	// Currently playing (or want to)
	BOOL			 IsRecording;	// Currently recording
	BOOL			 ValidRecord;	// The record buffer contains valid data
	BOOL			 FullDuplex;	// Mode is full duplex
	UWORD			 StopCnt;	// CMD_STOP count

	struct SignalSemaphore	 ListLock;
	struct MinList		 ReadList;
	struct MinList		 PlayingList;
	struct MinList		 SilentList;
	struct MinList		 WaitingList;

	struct MinList		 RequestQueue;	// Not locked by ListLock!

	struct Voice		*Voices;

	struct AHIAudioCtrl	*AudioCtrl;
	ULONG			 AudioMode;
	ULONG			 Frequency;
	UWORD			 Channels;
	UWORD			 Pad2;
	Fixed			 MonitorVolume;
	Fixed			 InputGain;
	Fixed			 OutputVolume;
	ULONG			 Input;
	ULONG			 Output;
};

/*
** Message passed to the Unit Process at
** startup time.
*/
struct StartupMessage
{
	struct Message		 Msg;
	struct AHIDevUnit	*Unit;
};
