* $Id$

	IFND    AHI_DEF_I
AHI_DEF_I	SET	1

DEBUG_DETAIL	SET	2

	include exec/semaphores.i
	include	exec/devices.i
	include	devices/ahi.i
	include devices/timer.i
	include	libraries/ahi_sub.i
	include	macros.i


*** Processor identification ****

 IFGE	__CPU-68020

HAVE_HIFI	EQU	1
HAVE_CLIPPING	EQU	1

 ENDC


*** Definitions ***

	STRUCTURE LONGLONG_STRUCT,0
	LONG	LL_high
	ULONG	LL_low
	LABEL	LL_SIZEOF

	STRUCTURE ULONGLONG_STRUCT,0
	ULONG	ULL_high
	ULONG	ULL_low
	LABEL	ULL_SIZEOF

	STRUCTURE Fixed64_STRUCT,0
	LONG	F64_I
	ULONG	F64_F
	LABEL	F64_SIZEOF


LONGLONG    MACRO
\1          EQU     SOFFSET
SOFFSET     SET     SOFFSET+LL_SIZEOF
            ENDM

ULONGLONG   MACRO
\1          EQU     SOFFSET
SOFFSET     SET     SOFFSET+ULL_SIZEOF
            ENDM

Fixed64     MACRO
\1          EQU     SOFFSET
SOFFSET     SET     SOFFSET+F64_SIZEOF
            ENDM



AHI_UNITS	EQU	4			* Normal units, excluding AHI_NO_UNIT

	BITDEF	AHIB,NOSURROUND,0
	BITDEF	AHIB,NOECHO,1
	BITDEF	AHIB,FASTECHO,2
	BITDEF	AHIB,CLIPPING,3

* AHIBase
	STRUCTURE AHIBase,LIB_SIZE
	UBYTE	ahib_Flags
	UBYTE	ahib_DebugLevel
	APTR	ahib_SysLib
	ULONG	ahib_SegList
	APTR	ahib_AudioCtrl
	STRUCT	ahib_DevUnits,AHI_UNITS*4
	STRUCT  ahib_Lock,SS_SIZE
	ULONG	ahib_AudioMode
	ULONG	ahib_Frequency
	Fixed	ahib_MonitorVolume
	Fixed	ahib_InputGain
	Fixed	ahib_OutputVolume
	ULONG	ahib_Input
	ULONG	ahib_Output
	Fixed	ahib_MaxCPU
	ULONG	ahib_AntiClickSamples
	LABEL	AHIBase_SIZEOF



	STRUCTURE Timer,0
	STRUCT	EntryTime,EV_SIZE
	STRUCT	ExitTime,EV_SIZE
	LABEL	Timer_SIZEOF

	STRUCTURE AHISoundData,0
	ULONG	sd_Type
	APTR	sd_Addr
	ULONG	sd_Length
	APTR	sd_InputBuffer0
	APTR	sd_InputBuffer1
	APTR	sd_InputBuffer2
	LABEL	AHISoundData_SIZEOF

	BITDEF	AHIAC,NOMIXING,31		;private ahiac_Flags flag
	BITDEF	AHIAC,NOTIMING,30		;private ahiac_Flags flag
	BITDEF	AHIAC,POSTPROC,29		;private ahiac_Flags flag
	BITDEF	AHIAC,CLIPPING,28		;private ahiac_Flags flag

* Private AudioCtrl structure
	STRUCTURE AHIPrivAudioCtrl,AHIAudioCtrlDrv_SIZEOF
	APTR	ahiac_SubLib
	ULONG	ahiac_SubAllocRC
	APTR	ahiac_ChannelDatas
	APTR	ahiac_SoundDatas
	ULONG	ahiac_BuffSizeNow		* Now many bytes of the buffer are used?
	
	* For AHIST_INPUT
	APTR	ahiac_InputBuffer0		* Filling
	APTR	ahiac_InputBuffer1		* Filled
	APTR	ahiac_InputBuffer2		* Filled, older
	ULONG	ahiac_InputLength
	ULONG	ahiac_InputBlockLength
	APTR	ahiac_InputRecordPtr
	ULONG	ahiac_InputRecordCnt


	APTR	ahiac_MasterVolumeTable
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
	UWORD	ahiac_UsedCPU
	UWORD	ahiac_Pad
	APTR	ahiac_AntiClickBuffer
	ULONG	ahiac_AntiClickSize		* in bytes
	STRUCT	ahiac_DriverName,41+1		* sizeof("DEVS:ahi/                          .audio")
	LABEL	AHIPrivAudioCtrl_SIZEOF

* AHIChannelData (private)
	STRUCTURE AHIChannelData,0
	LABEL	cd_Flags
	UWORD	cd_EOS			;TRUE: Sample has reached end
	UBYTE	cd_FreqOK		;FALSE: Freq=0       ; TRUE: Freq<>0
	UBYTE	cd_SoundOK		;FALSE: No sound set ; TRUE: S. OK.
	Fixed64	cd_Offset
	Fixed64	cd_Add
	APTR	cd_DataStart
	Fixed64	cd_LastOffset
	LONG	cd_ScaleLeft
	LONG	cd_ScaleRight
	FPTR	cd_AddRoutine
	Fixed	cd_VolumeLeft
	Fixed	cd_VolumeRight
	ULONG	cd_Type

	LABEL	cd_NextFlags
	UWORD	cd_NextEOS		;Not in use
	UBYTE	cd_NextFreqOK
	UBYTE	cd_NextSoundOK
	Fixed64	cd_NextOffset
	Fixed64	cd_NextAdd
	APTR	cd_NextDataStart
	Fixed64	cd_NextLastOffset
	LONG	cd_NextScaleLeft
	LONG	cd_NextScaleRight
	FPTR	cd_NextAddRoutine
	Fixed	cd_NextVolumeLeft
	Fixed	cd_NextVolumeRight
	ULONG	cd_NextType

	LONG	cd_Samples		;Samples left to store (down-counter)
	LONG	cd_FirstOffsetI		;for linear interpolation routines
	LONG	cd_LastSampleL		;for linear interpolation routines
	LONG	cd_TempLastSampleL	;for linear interpolation routines
	LONG	cd_LastSampleR		;for linear interpolation routines
	LONG	cd_TempLastSampleR	;for linear interpolation routines
	LONG	cd_LastScaledSampleL	;for anticlick
	LONG	cd_LastScaledSampleR	;for anticlick

	APTR	cd_Succ			;For the wet and dry lists
	UWORD	cd_ChannelNo
	UWORD	cd_Pad
	LONG	cd_AntiClickCount

	LABEL	AHIChannelData_SIZEOF

	ENDC