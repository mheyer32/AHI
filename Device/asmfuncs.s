;/* $Id$
* $Log$
* Revision 4.10  1997/12/21 17:41:50  lcs
* Major source cleanup, moved some functions to separate files.
*
* Revision 4.9  1997/10/14 17:58:53  lcs
* Fixed some mistakes in the overview autodoc section
*
* Revision 4.8  1997/10/11 15:58:13  lcs
* Added the ahiac_UsedCPU field to the AHIAudioCtrl structure.
*

	IF	0

*******************************************************************************
** C function prototypes ******************************************************
*******************************************************************************

*/

#include <CompilerSpecific.h>
#include "ahi_def.h"

ASMCALL ULONG RecalcBuff ( REG(d1, ULONG freq), REG(a2, struct AHIPrivAudioCtrl *audioctrl) ) {}
ASMCALL BOOL PreTimer ( void ) {}
ASMCALL void PostTimer ( void ) {}
ASMCALL BOOL DummyPreTimer ( void ) {}
ASMCALL void DummyPostTimer ( void ) {}
ASMCALL LONG Fixed2Shift ( REG(d0, Fixed val) ) {}

;/*     Comment terminated at the end of the file!

	ENDC	* IF 0

*******************************************************************************
** Assembly code **************************************************************
*******************************************************************************



	include	exec/exec.i
	include	lvo/exec_lib.i
	include	lvo/timer_lib.i
	include	lvo/utility_lib.i

	include	ahi_def.i

	XDEF	_RecalcBuff
	XDEF	_PreTimer
	XDEF	_PostTimer
	XDEF	_DummyPreTimer
	XDEF	_DummyPostTimer

	XDEF	_Fixed2Shift
	XDEF	_UDivMod64

	XREF	_TimerBase
	XREF	_UtilityBase


;
; Simple version of the C "sprintf" function.  Assumes C-style
; stack-based function conventions.
;
;   long eyecount;
;   eyecount=2;
;   sprintf(string,"%s have %ld eyes.","Fish",eyecount);
;
; would produce "Fish have 2 eyes." in the string buffer.
;
        XDEF _Sprintf
_Sprintf:       ; ( ostring, format, {values} )
	movem.l	a2/a3/a6,-(sp)

	move.l	4*4(sp),a3	;Get the output string pointer
	move.l	5*4(sp),a0	;Get the FormatString pointer
	lea.l	6*4(sp),a1	;Get the pointer to the DataStream
	lea.l	stuffChar(pc),a2
	move.l	4.w,a6
	jsr	_LVORawDoFmt(a6)

	movem.l	(sp)+,a2/a3/a6
	rts

;------ PutChProc function used by RawDoFmt -----------
stuffChar:
	move.b	d0,(a3)+	;Put data to output string
	rts


;in:
* d1	Frequency (Fixed)
* a2	ptr to AHIAudioCtrl
;ut:
* d0	Samples/MixerPass (also in ahiac_BuffSamples)
*       ahiac_BuffSizeNow will also be updated (For mixing routine)
_RecalcBuff:
	move.l	ahiac_MixFreq(a2),d0
	beq	.error
	tst.l	d1
	beq	.error
	lsl.l	#8,d0				; Mix freq <<8 => 24.8
	cmp.l	#65536,d1
	bhs.b	.fixed
	swap.w	d1
.fixed
	lsr.l	#8,d1				; Freq >>8 => 24.8

 IFGE	__CPU-68020
	divu.l	d1,d0
 ELSE
	move.l	_UtilityBase(pc),a1
	jsr	_LVOUDivMod32(a1)
 ENDC
	and.l	#$ffff,d0
	move.l	d0,ahiac_BuffSamples(a2)

	move.l	d0,d1
	lsl.l	#1,d1				;always words
	btst.b	#AHIACB_STEREO,ahiac_Flags+3(a2)
	beq	.1
	lsl.l	#1,d1				;2 hw channels
.1
	btst	#AHIACB_HIFI,ahiac_Flags+3(a2)
	beq	.2
	lsl.l	#1,d1				;32 bit samples
.2
	btst.b	#AHIACB_POSTPROC-24,ahiac_Flags(a2)
	beq	.3
	lsl.l	#1,d1				;2 buffers
.3
	addq.l	#7,d1
	and.b	#~7,d1				;8 byte align
	add.l	#80,d1				;FIXIT! Kludge for Mungwall hits
	move.l	d1,ahiac_BuffSizeNow(a2)
.error
	rts



;in:
* a2	ptr to AHIAudioCtrl
;out:
* d0	TRUE if too much CPU is used
* Z	Updated
_PreTimer:
	pushm	d1-d2/a0-a1/a6
	move.l	_TimerBase(pc),a6
	lea	ahiac_Timer(a2),a0
	move.l	EntryTime+EV_LO(a0),d2
	call	ReadEClock
	move.l	EntryTime+EV_LO(a0),d1
	sub.l	d1,d2			; d2 = -(clocks since last entry)
	sub.l	ExitTime+EV_LO(a0),d1	; d1 = clocks since last exit
	add.l	d2,d1			; d1 = -(clocks spent mixing)
	beq	.ok
	neg.l	d1			; d1 = clocks spent mixing
	neg.l	d2			; d2 = clocks since last entry
	lsl.l	#8,d1
 IFGE	__CPU-68020
	divu.l	d2,d1
 ELSE
	move.l	d1,d0
	move.l	d2,d1
	move.l	_UtilityBase(pc),a1
	jsr	_LVOUDivMod32(a1)
	move.l	d0,d1
 ENDC
 	move.w	d1,ahiac_UsedCPU(a2)
	cmp.b	ahiac_MaxCPU(a2),d1
	bls	.ok
	moveq	#TRUE,d0
	bra	.exit
.ok
	moveq	#FALSE,d0
.exit
	popm	d1-d2/a0-a1/a6
	rts


_PostTimer:
	pushm	d0-d1/a0-a1/a6
	move.l	_TimerBase(pc),a6
	lea	ahiac_Timer+ExitTime(a2),a0
	call	ReadEClock
	popm	d0-d1/a0-a1/a6
	rts

_DummyPreTimer:
	moveq	#FALSE,d0
_DummyPostTimer:
	rts


;in:
* d0	Fixed
;out:
* d0	Shift value
_Fixed2Shift:
	push	d1
	moveq	#0,d1
	cmp.l	#$10000,d0
	bge	.exit
	addq.l	#1,d1
	cmp.l	#$8000,d0
	bge	.exit
	addq.l	#1,d1
	cmp.l	#$4000,d0
	bge	.exit
	addq.l	#1,d1
	cmp.l	#$2000,d0
	bge	.exit
	addq.l	#1,d1
	cmp.l	#$1000,d0
	bge	.exit
	addq.l	#1,d1
	cmp.l	#$800,d0
	bge	.exit
	addq.l	#1,d1
	cmp.l	#$400,d0
	bge	.exit
	addq.l	#1,d1
	cmp.l	#$200,d0
	bge	.exit
	addq.l	#1,d1
	cmp.l	#$100,d0
	bge	.exit
	addq.l	#1,d1
	cmp.l	#$80,d0
	bge	.exit
	addq.l	#1,d1
	cmp.l	#$40,d0
	bge	.exit
	addq.l	#1,d1
	cmp.l	#$20,d0
	bge	.exit
	addq.l	#1,d1
	cmp.l	#$10,d0
	bge	.exit
	addq.l	#1,d1
	cmp.l	#$8,d0
	bge	.exit
	addq.l	#1,d1
	cmp.l	#$4,d0
	bge	.exit
	addq.l	#1,d1
	cmp.l	#$2,d0
	bge	.exit
	addq.l	#1,d1
	cmp.l	#$1,d0
	bge	.exit
	addq.l	#1,d1
.exit
	move.l	d1,d0
	pop	d1
	rts

;UDivMod64 -- unsigned 64 by 32 bit division
;             64 bit quotient, 32 bit remainder.
; (d1:d2)/d0 = d0:d2, d1 remainder.

_UDivMod64:
	movem.l	d3-d7,-(sp)
	move.l	d0,d7
	moveq	#0,d0
	move.l	#$80000000,d3
	move.l	#$00000000,d4
	moveq	#0,d5			;result
	moveq	#0,d6			;result

.2
	lsl.l	#1,d2
	roxl.l	#1,d1
	roxl.l	#1,d0
	sub.l	d7,d0
	bmi	.3
	or.l	d3,d5
	or.l	d4,d6
	skipw
.3
	add.l	d7,d0

	lsr.l	#1,d3
	roxr.l	#1,d4
	bcc	.2

	move.l	d5,d2
	move.l	d0,d1
	move.l	d6,d0
	movem.l	(sp)+,d3-d7
	rts

;	C comment terminating here... */
