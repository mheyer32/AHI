* $Id$
* $Log$
* Revision 1.8  1997/03/24 18:03:10  lcs
* Rewrote AHI_LoadSound() and AHI_UnloadSound() in C
*
* Revision 1.7  1997/03/24 12:41:51  lcs
* Echo rewritten
*
* Revision 1.6  1997/03/13 00:19:43  lcs
* Up to 4 device units are now available.
*
* Revision 1.5  1997/02/02 18:15:04  lcs
* Added protection against CPU overload
*
* Revision 1.4  1997/02/01 23:54:26  lcs
* Rewrote the library open code in C and removed the library bases
* from AHIBase
*
* Revision 1.2  1997/01/04 20:19:56  lcs
* ahiac_EffChannelInfoStruct added
*
* Revision 1.1  1996/12/21 13:05:12  lcs
* Initial revision
*

	IFND    AHI_DEF_I
AHI_DEF_I	SET	1

DEBUG_DETAIL	SET	2

	include exec/semaphores.i
	include	exec/devices.i
	include	devices/ahi.i
	include devices/timer.i
	include	libraries/ahi_sub.i
	include	macros.i

AHI_UNITS	EQU	4			* Normal units, excluding AHI_NO_UNIT

* Message passed to the Unit Process at
* startup time.

	STRUCTURE StartupMessage,0
	STRUCT	Msg,MN_SIZE
	APTR	Unit
	LABEL	StartupMessage_SIZEOF

	BITDEF	AHIB,NOSURROUND,0
	BITDEF	AHIB,NOECHO,1
	BITDEF	AHIB,FASTECHO,2

* AHIBase
	STRUCTURE AHIBase,LIB_SIZE
	UBYTE	ahib_Flags
	UBYTE	ahib_DebugLevel
	APTR	ahib_SysLib
	ULONG	ahib_SegList
	APTR	ahib_AudioCtrl
	STRUCT	ahib_DevUnits,AHI_UNITS*4
	STRUCT  ahib_Lock,SS_SIZE
	STRUCT	ahib_Startup,StartupMessage_SIZEOF
	ULONG	ahib_AudioMode
	ULONG	ahib_Frequency
	Fixed	ahib_MonitorVolume
	Fixed	ahib_InputGain
	Fixed	ahib_OutputVolume
	ULONG	ahib_Input
	ULONG	ahib_Output
	Fixed	ahib_MaxCPU
	LABEL	AHIBase_SIZEOF



	STRUCTURE Timer,0
	STRUCT	EntryTime,EV_SIZE
	STRUCT	ExitTime,EV_SIZE
	LABEL	Timer_SIZEOF

	STRUCTURE AHISoundData,0
	ULONG	sd_Type
	APTR	sd_Addr
	ULONG	sd_Length
	LABEL	AHISoundData_SIZEOF

	BITDEF	AHIAC,NOMIXING,31		;private ahiac_Flags flag
	BITDEF	AHIAC,NOTIMING,30		;private ahiac_Flags flag
	BITDEF	AHIAC,POSTPROC,29		;private ahiac_Flags flag

* Private AudioCtrl structure
	STRUCTURE AHIPrivAudioCtrl,AHIAudioCtrlDrv_SIZEOF
	APTR	ahiac_SubLib
	ULONG	ahiac_SubAllocRC
	APTR	ahiac_ChannelDatas
	APTR	ahiac_SoundDatas
	ULONG	ahiac_BuffSizeNow		;Now many bytes of the buffer are used?
	
	APTR	ahiac_MultTableS
	APTR	ahiac_MultTableU
	APTR	ahiac_RecordFunc		* AHIA_RecordFunc
	ULONG	ahiac_AudioID
	Fixed	ahiac_MasterVolume;		* Real
	Fixed	ahiac_SetMasterVolume;		* Set by user
	Fixed	ahiac_EchoMasterVolume;		* Set by dspecho
	APTR	ahiac_EffOutputBufferStruct	* struct AHIEffOutputBuffer *
	APTR	ahiac_EffDSPEchoStruct		* struct Echo *
	APTR	ahiac_EffChannelInfoStruct	* struct AHIChannelInfo *
	APTR	ahiac_WetList
	APTR	ahiac_DryList
	UBYTE	ahiac_WetOrDry
	UBYTE	ahiac_MaxCPU
	UWORD	ahiac_Channels2
	STRUCT	ahiac_Timer,Timer_SIZEOF
	STRUCT	ahiac_DriverName,41+1		* sizeof("DEVS:ahi/                          .audio")
	LABEL	AHIPrivAudioCtrl_SIZEOF

* AHIChannelData (private)
	STRUCTURE AHIChannelData,0
	LABEL	cd_Flags
	UWORD	cd_EOS			;$FFFF: Sample has reached end
	UBYTE	cd_FreqOK		;$00: Freq=0 ; $FF: Freq<>0
	UBYTE	cd_SoundOK		;$00: No sound set ; $FF: S. OK.
	ULONG	cd_OffsetI
	UWORD	cd_Pad1
	UWORD	cd_OffsetF
	ULONG	cd_AddI
	UWORD	cd_Pad2
	UWORD	cd_AddF
	APTR	cd_DataStart
	ULONG	cd_LastOffsetI
	UWORD	cd_Pad3
	UWORD	cd_LastOffsetF
	ULONG	cd_ScaleLeft
	ULONG	cd_ScaleRight
	FPTR	cd_AddRoutine
	LONG	cd_VolumeLeft		;Fixed
	LONG	cd_VolumeRight		;Fixed
	ULONG	cd_Type

	LABEL	cd_NextFlags
	UWORD	cd_NextEOS		;Not in use
	UBYTE	cd_NextFreqOK
	UBYTE	cd_NextSoundOK
	ULONG	cd_NextOffsetI
	UWORD	cd_NextPad1
	UWORD	cd_NextOffsetF
	ULONG	cd_NextAddI
	UWORD	cd_NextPad2
	UWORD	cd_NextAddF
	APTR	cd_NextDataStart
	ULONG	cd_NextLastOffsetI
	UWORD	cd_NextPad3
	UWORD	cd_NextLastOffsetF
	ULONG	cd_NextScaleLeft
	ULONG	cd_NextScaleRight
	FPTR	cd_NextAddRoutine
	LONG	cd_NextVolumeLeft	;Fixed
	LONG	cd_NextVolumeRight	;Fixed
	ULONG	cd_NextType

	ULONG	cd_LCommand		;for loops
	ULONG	cd_LAddress		;for loops

	ULONG	cd_Samples		;Samples left to store (down-counter)
	ULONG	cd_FirstOffsetI		;for linear interpolation routines
	LONG	cd_LastSampleL		;for linear interpolation routines
	LONG	cd_TempLastSampleL	;for linear interpolation routines
	LONG	cd_LastSampleR		;for linear interpolation routines
	LONG	cd_TempLastSampleR	;for linear interpolation routines

	APTR	cd_Succ			;For the wet and dry lists
	UWORD	cd_ChannelNo
	UWORD	cd_Pad

	LABEL	AHIChannelData_SIZEOF

	ENDC
