* $Id$
* $Log$
* Revision 1.3  1997/02/02 22:35:50  lcs
* Localized it
*
* Revision 1.2  1997/02/01 23:54:26  lcs
* Rewrote the library open code in C and removed the library bases
* from AHIBase
*
* Revision 1.1  1996/12/21 13:05:12  lcs
* Initial revision
*

	incdir	include:

	include	devices/timer.i
	include	exec/exec.i
	include dos/dos.i
	include	graphics/gfxbase.i
	include	utility/utility.i

	include	lvo/exec_lib.i

	include devices/ahi.i
	include libraries/ahi_sub.i
	include ahi_def.i
	include ahi_rev.i

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

	VERSTAG


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

	XREF	initcode
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

	jsr	_OpenLibs(pc)
	tst.l	d0
	beq	.exit

	bsr	initcode

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

	jsr	_CloseLibs(pc)

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
	jmp	_DevProc(pc)


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
