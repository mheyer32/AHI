* $Id$
* $Log$
* Revision 1.3  1997/02/01 19:44:18  lcs
* *** empty log message ***
*
* Revision 1.2  1997/01/04 20:19:56  lcs
* ahiac_EffChannelInfoStruct added
*
* Revision 1.1  1996/12/21 13:05:12  lcs
* Initial revision
*

DEBUG_DETAIL	SET	2

	include	exec/devices.i
	include	devices/ahi.i
	include	libraries/ahi_sub.i
	include	macros.i

AHI_UNITS	EQU	1			* Normal units, excluding AHI_NO_UNIT

* Message passed to the Unit Process at
* startup time.

	STRUCTURE StartupMessage,0
	STRUCT	Msg,MN_SIZE
	APTR	Unit
	LABEL	StartupMessage_SIZEOF

* AHIBase
	STRUCTURE AHIBase,LIB_SIZE
	UBYTE	ahib_Flags
	UBYTE	ahib_DebugLevel
	APTR	ahib_SysLib
	APTR	ahib_DosLib
	APTR	ahib_UtilityLib
	APTR	ahib_GadToolsLib
	APTR	ahib_IntuitionLib
	APTR	ahib_GraphicsLib
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
	LABEL	AHIBase_SIZEOF

	BITDEF	AHIB,NOSURROUND,0
	BITDEF	AHIB,NOECHO,1
	BITDEF	AHIB,FASTECHO,2

	STRUCTURE AHIEcho,0
	ULONG	ahiecho_Delay
	FPTR	ahiecho_Code		;The echo routine
	WORD	ahiecho_FeedbackDS	;Delayed signal to same channel
	WORD	ahiecho_FeedbackDO	;Delayed signal to other channel
	WORD	ahiecho_FeedbackNS	;Normal signal to same channel
	WORD	ahiecho_FeedbackNO	;Normal signal to other channel
	WORD	ahiecho_MixN		;Normal signal
	WORD	ahiecho_MixD		;Delayed signal
	UWORD	ahiecho_Pad
	ULONG	ahiecho_Offset		;(&Buffer-&SrcPtr)/sizeof(ahiecho_Buffer[0])
	APTR	ahiecho_SrcPtr		;Pointer to &Buffer
	APTR	ahiecho_DstPtr		;Pointer to &(Buffer[Delay])
	APTR	ahiecho_EndPtr		;Pointer to address after buffer
	ULONG	ahiecho_BufferSize	;Delay buffer size in bytes
	LABEL	ahiecho_Buffer		;Delay buffer
	LABEL	AHIEcho_SIZEOF

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
	Fixed	ahiac_MasterVolume;
	APTR	ahiac_EffOutputBufferStruct	* struct AHIEffOutputBuffer *
	APTR	ahiac_EffDSPEchoStruct		* struct AHIEcho *
	APTR	ahiac_EffChannelInfoStruct	* struct AHIChannelInfo *
	APTR	ahiac_WetList
	APTR	ahiac_DryList
	UBYTE	ahiac_WetOrDry
	UBYTE	ahiac_Pad1
	UWORD	ahiac_Channels2
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

* AHISoundData (private)
* sd_Addr and sd_Length depend on sd_Type.
* sd_Addr:
*  AHIST_SAMPLE (AHIST_M8S or AHIST_M16S): Address to array of samples.
*  AHIST_DYNAMICSAMPLE (AHIST_M8S or AHIST_M16S): Address to array of samples.
*  AHIST_INPUT: FIXIT!
*  AHIST_LOOP: Pointer to LW source sd_Addr followed by struct AHIMultiLoop[].
* sd_Length:
*  AHIST_SAMPLE (AHIST_M8S or AHIST_M16S): Length of array (in samples).
*  AHIST_DYNAMICSAMPLE (AHIST_M8S or AHIST_M16S): Length of array (in samples).
*  AHIST_INPUT: FIXIT!
*  AHIST_LOOP: NULL

	STRUCTURE AHISoundData,0
	ULONG	sd_Type
	APTR	sd_Addr
	ULONG	sd_Length
	LABEL	AHISoundData_SIZEOF


