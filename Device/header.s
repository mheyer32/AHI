* $Id$
* $Log$
* Revision 4.5  1997/12/21 17:41:50  lcs
* Major source cleanup, moved some functions to separate files.
*
* Revision 4.4  1997/07/15 00:52:05  lcs
* This is the second bugfix release of AHI 4.
*


****** ahi.device/--background-- *******************************************
*
*   PURPOSE
*
*       The 'ahi.device' was first created because the lack of standards
*       when it comes to sound cards on the Amiga. Another reason was to
*       make it easier to write multi-channel music programs.
*
*       This device is by no means the final and perfect solution. But
*       hopefully, it can evolve into something useful until AT brings you
*       The Real Thing (TM).
*
*   OVERVIEW
*
*       Please see the document "AHI Developer's Guide" for more
*       information.
*
*
*       * Driver based
*
*       Each supported sound card is controlled by a library-based audio
*       driver. For a 'dumb' sound card, a new driver could be written in
*       a few hours. For a 'smart' sound card, it is possible to utilize an
*       on-board DSP, for example, to maximize performance and sound quality.
*       For sound cards with own DSP but little or no memory, it is possible
*       to use the main CPU to mix channels and do the post-processing
*       with the DSP. Drivers are available for most popular sound cards,
*       as well as an 8SVX (mono) and AIFF/AIFC (mono & stereo) sample render
*       driver.
*  
*       * Fast, powerful mixing routines (yeah, right... haha)
*  
*       The device's mixing routines mix 8- or 16-bit signed samples, both
*       mono and stereo, located in Fast-RAM and outputs 16-bit mono or stereo
*       (with stereo panning if desired) data, using any number of channels
*       (as long as 'any' means less than 128).  Tables can be used speed
*       the mixing up (especially when using 8-bit samples).  The samples can
*       have any length (including odd) and can have any number of loops.
*       There are also so-called HiFi mixing routines that can be used, that
*       use linear interpolation and gives 32 bit output.
*       
*       * Support for non-realtime mixing
*  
*       By providing a timing feature, it is possible to create high-
*       quality output even if the processing power is lacking, by saving
*       the output to disk, for example as an IFF AIFF or 8SXV file.
*  
*       * Audio database
*  
*       Uses ID codes, much like Screenmode IDs, to select the many
*       parameters that can be set. The functions to access the audio
*       database are not too different from those in 'graphics.library'.
*       The device also features a requester to get an ID code from the
*       user.
*  
*       * Both high- and low-level protocol
*  
*       By acting both like a device and a library, AHI gives the programmer
*       a choice between full control and simplicity. The device API allows
*       several programs to use the audio hardware at the same time, and
*       the AUDIO: dos-device driver makes playing and recording sound very
*       simple for both the programmer and user.
*  
*       * Future Compatible
*  
*       When AmigaOS gets device-independent audio worth it's name, it should
*       not be too difficult to write a driver for AHI, allowing applications
*       using 'ahi.device' to automatically use the new OS interface. At
*       least I hope it wont.
*
*
****************************************************************************
*
*


	include	devices/timer.i
	include	exec/exec.i
	include dos/dos.i
	include	graphics/gfxbase.i
	include	utility/utility.i

	include	lvo/exec_lib.i

	include devices/ahi.i
	include libraries/ahi_sub.i
	include ahi_def.i
	include ahi.device_rev.i

;	section	text,code

*******************************************************************************
** Start **********************************************************************
*******************************************************************************

Start:
	moveq	#-1,d0
	rts

*******************************************************************************
** RomTag *********************************************************************
*******************************************************************************

	XREF	EndCode

RomTag:
	DC.W	RTC_MATCHWORD
	DC.L	RomTag
	DC.L	EndCode
	DC.B	RTF_AUTOINIT
	DC.B	VERSION				;version
	DC.B	NT_DEVICE
	DC.B	0				;pri
	DC.L	_DevName
	DC.L	_IDString
	DC.L	InitTable

	XDEF	_DevName
	XDEF	_IDString

_DevName:	AHINAME
_IDString:	VSTRING

	dc.b	"$VER: "
	VERS
	dc.b	" ("
	DATE
	dc.b	") "
	dc.b	"©1994-1997 Martin Blom. "
 IFGE	__CPU-68020
  IFGE	__CPU-68060
	dc.b	"68060 version.",0
  ELSE
	dc.b	"68020+ version.",0
  ENDC
 ELSE
	dc.b	"68000 version.",0
 ENDC

*******************************************************************************
** Init & function tables *****************************************************
*******************************************************************************

	cnop	0,2

	XREF	_DevOpen
	XREF	_DevClose
	XREF	_DevBeginIO
	XREF	_DevAbortIO

	XREF	_AllocAudioA
	XREF	_FreeAudio
	XREF	_KillAudio
	XREF	_ControlAudioA
	XREF	_SetVol
	XREF	_SetFreq
	XREF	_SetSound
	XREF	_SetEffect
	XREF	_LoadSound
	XREF	_UnloadSound
	XREF	_NextAudioID
	XREF	_GetAudioAttrsA
	XREF	_BestAudioIDA
	XREF	_AllocAudioRequestA
	XREF	_AudioRequestA
	XREF	_FreeAudioRequest
	XREF	_PlayA
	XREF	_SampleFrameSize
	XREF	_AddAudioMode
	XREF	_RemoveAudioMode
	XREF	_LoadModeFile

InitTable:
	DC.L	AHIBase_SIZEOF
	DC.L	funcTable
	DC.L	dataTable
	DC.L	initRoutine

funcTable:
	dc.l	_DevOpen
	dc.l	_DevClose
	dc.l	_DevExpunge
	dc.l	Null
*
	dc.l	_DevBeginIO
	dc.l	_DevAbortIO

	dc.l	_AllocAudioA
	dc.l	_FreeAudio
	dc.l	_KillAudio
	dc.l	_ControlAudioA
	dc.l	_SetVol
	dc.l	_SetFreq
	dc.l	_SetSound
	dc.l	_SetEffect
	dc.l	_LoadSound
	dc.l	_UnloadSound
	dc.l	_NextAudioID
	dc.l	_GetAudioAttrsA
	dc.l	_BestAudioIDA
	dc.l	_AllocAudioRequestA
	dc.l	_AudioRequestA
	dc.l	_FreeAudioRequest
	dc.l	_PlayA
	dc.l	_SampleFrameSize
	dc.l	_AddAudioMode
	dc.l	_RemoveAudioMode
	dc.l	_LoadModeFile
	dc.l	-1

dataTable:
	INITBYTE	LN_TYPE,NT_DEVICE
	INITLONG	LN_NAME,_DevName
	INITBYTE	LIB_FLAGS,LIBF_SUMUSED!LIBF_CHANGED
	INITWORD	LIB_VERSION,VERSION
	INITWORD	LIB_REVISION,REVISION
	INITLONG	LIB_IDSTRING,_IDString
	DC.L		0

	XDEF	_AHIBase
	XDEF	_DOSBase
	XDEF	_GadToolsBase
	XDEF	_GfxBase
	XDEF	_IFFParseBase
	XDEF	_IntuitionBase
	XDEF	_LocaleBase
	XDEF	_TimerBase
	XDEF	_UtilityBase


*******************************************************************************
** Globals ********************************************************************
*******************************************************************************

_AHIBase:	dc.l	0
_DOSBase:	dc.l	0
_GadToolsBase:	dc.l	0
_GfxBase:	dc.l	0
_IFFParseBase:	dc.l	0
_IntuitionBase:	dc.l	0
_LocaleBase:	dc.l	0
_TimerBase:	dc.l	0
_UtilityBase:	dc.l	0

	XDEF	_DriverVersion
	XDEF	_Version
	XDEF	_Revision

_DriverVersion:	dc.l	2
_Version:	dc.l	VERSION
_Revision:	dc.l	REVISION

	XDEF	_TimerIO
	XDEF	_timeval

_TimerIO:	dc.l	0
_timeval:	dc.l	0


*******************************************************************************
** initRoutine ****************************************************************
*******************************************************************************

	XREF	_initcode
	XREF	_OpenLibs

initRoutine:
	movem.l	d1-d2/a0-a1/a5-a6,-(sp)
	move.l	d0,_AHIBase
	move.l	d0,a5
	move.l	a6,ahib_SysLib(a5)
	move.l	a0,ahib_SegList(a5)

 IFGE	__CPU-68020
	move.w	AttnFlags(a6),d0
	and.w	#AFF_68020,d0
	bne.b	.cpuOK
	move.l	#$00068020,d0
	move.l	#$ABADC0DE,d1
	ALERT	(AN_Unknown|ACPU_InstErr&(~AT_DeadEnd))
	moveq	#0,d0
	bra.w	.exit
.cpuOK
 ENDC

	lea	ahib_Lock(a5),a0
	call	InitSemaphore

	jsr	_OpenLibs
	tst.l	d0
	beq	.exit

	jsr	_initcode

	move.l	a5,d0
.exit
	movem.l	(sp)+,d1-d2/a0-a1/a5-a6
	rts


*******************************************************************************
** DevExpunge *****************************************************************
*******************************************************************************

	XDEF	_DevExpunge
	XREF	_CloseLibs
;in:
* a6	device
_DevExpunge:
	movem.l	d1-d2/a0-a1/a5-a6,-(sp)
	move.l	a6,a5
	move.l	ahib_SysLib(a5),a6
	tst.w	LIB_OPENCNT(a5)
	beq.b	.notopen
	bset.b	#LIBB_DELEXP,LIB_FLAGS(a5)
	moveq	#0,d0
	bra.b	.Expunge_end
.notopen
	move.l	ahib_SegList(a5),d2
	move.l	a5,a1
	call	Remove

	jsr	_CloseLibs

	moveq	#0,d0
	move.l	a5,a1
	move.w	LIB_NEGSIZE(a5),d0
	sub.l	d0,a1
	add.w	LIB_POSSIZE(a5),d0
	call	FreeMem
	move.l	d2,d0
.Expunge_end
	movem.l	(sp)+,d1-d2/a0-a1/a5-a6
	rts

Null:
	moveq	#0,d0
	rts


*******************************************************************************
** DevProcEntry ***************************************************************
*******************************************************************************

	XREF	_DevProc
	XDEF	_DevProcEntry

_DevProcEntry:
	move.l	_AHIBase(pc),a6
	jmp	_DevProc


*******************************************************************************
** kprint_macro ***************************************************************
*******************************************************************************

	XDEF	kprint_macro
	XREF	KPrintF
kprint_macro:
	movem.l	d0-d1/a0-a1,-(sp)
	jsr	KPrintF
	movem.l	(sp)+,d0-d1/a0-a1
	rts
