; $Id$

;    AHI - Hardware independent audio subsystem
;    Copyright (C) 1996-2001 Martin Blom <martin@blom.org>
;     
;    This library is free software; you can redistribute it and/or
;    modify it under the terms of the GNU Library General Public
;    License as published by the Free Software Foundation; either
;    version 2 of the License, or (at your option) any later version.
;     
;    This library is distributed in the hope that it will be useful,
;    but WITHOUT ANY WARRANTY; without even the implied warranty of
;    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
;    Library General Public License for more details.
;     
;    You should have received a copy of the GNU Library General Public
;    License along with this library; if not, write to the
;    Free Software Foundation, Inc., 59 Temple Place - Suite 330, Cambridge,
;    MA 02139, USA.

	include exec/types.i
	include	macros.i

	XDEF	_RecM8S
	XDEF	_RecS8S
	XDEF	_RecM16S
	XDEF	_RecS16S
	XDEF	_RecM32S
	XDEF	_RecS32S

	section	.text,code

* Not the best routines (fraction does not get saved between calls,
* loads of byte writes, no interpolation etc), but who cares? 

;in:
* d0	Number of SAMPLES to fill. (Max 131071)
* d1	Add interger.fraction in samples (2×16 bit)
* a0	Source (AHIST_S16S)
* a2	Pointer to Source Offset in bytes (will be updated)
* a3	Pointer to Destination (will be updated)
;out

Cnt	EQUR	d0
Tmp	EQUR	d1
OffsI	EQUR	d2
OffsF	EQUR	d3
AddI	EQUR	d4
AddF	EQUR	d5
Src	EQUR	a0
Dst	EQUR	a1

*******************************************************************************
** RecM8S *********************************************************************
*******************************************************************************

_RecM8S:
	pushm	d2-d5,-(sp)
	move.w	d1,AddF
	moveq	#0,AddI
	swap.w	d1
	move.w	d1,AddI
	moveq	#0,OffsI
	moveq	#0,OffsF
	add.l	(a2),Src		;Get Source
	move.l	(a3),Dst		;Get Dest
	lsr.l	#1,Cnt			;Unroll one time
	bcs	.1
	subq.w	#1,Cnt
	bmi.b	.exit
.nextsample
 IFGE	__CPU-68020
	move.b	(Src,OffsI.l*4),(Dst)+
 ELSE
	move.l	OffsI,Tmp
	add.l	Tmp,Tmp
	add.l	Tmp,Tmp
	move.b	(Src,Tmp.l),(Dst)+
 ENDC
	add.w	AddF,OffsF
	addx.l	AddI,OffsI
.1
 IFGE	__CPU-68020
	move.b	(Src,OffsI.l*4),(Dst)+
 ELSE
	move.l	OffsI,Tmp
	add.l	Tmp,Tmp
	add.l	Tmp,Tmp
	move.b	(Src,Tmp.l),(Dst)+
 ENDC
	add.w	AddF,OffsF
	addx.l	AddI,OffsI
	dbf	Cnt,.nextsample
	lsl.l	#2,OffsI
	add.l	OffsI,(a2)		;Update Offset
	move.l	Dst,(a3)		;Update Dest
.exit
	popm	d2-d5,-(sp)
	rts

*******************************************************************************
** RecS8S *********************************************************************
*******************************************************************************

_RecS8S:
	pushm	d2-d5,-(sp)
	move.w	d1,AddF
	moveq	#0,AddI
	swap.w	d1
	move.w	d1,AddI
	moveq	#0,OffsI
	moveq	#0,OffsF
	add.l	(a2),Src		;Get Source
	move.l	(a3),Dst		;Get Dest
	lsr.l	#1,Cnt			;Unroll one time
	bcs	.1
	subq.w	#1,Cnt
	bmi.b	.exit
.nextsample
 IFGE	__CPU-68020
	move.b	(Src,OffsI.l*4),(Dst)+
	move.b	2(Src,OffsI.l*4),(Dst)+
 ELSE
	move.l	OffsI,Tmp
	add.l	Tmp,Tmp
	add.l	Tmp,Tmp
	move.b	(Src,Tmp.l),(Dst)+
	move.b	2(Src,Tmp.l),(Dst)+
 ENDC
	add.w	AddF,OffsF
	addx.l	AddI,OffsI
.1
 IFGE	__CPU-68020
	move.b	(Src,OffsI.l*4),(Dst)+
	move.b	2(Src,OffsI.l*4),(Dst)+
 ELSE
	move.l	OffsI,Tmp
	add.l	Tmp,Tmp
	add.l	Tmp,Tmp
	move.b	(Src,Tmp.l),(Dst)+
	move.b	2(Src,Tmp.l),(Dst)+
 ENDC
	add.w	AddF,OffsF
	addx.l	AddI,OffsI
	dbf	Cnt,.nextsample
	lsl.l	#2,OffsI
	add.l	OffsI,(a2)		;Update Offset
	move.l	Dst,(a3)		;Update Dest
.exit
	popm	d2-d5,-(sp)
	rts

*******************************************************************************
** RecM16S ********************************************************************
*******************************************************************************

_RecM16S:
	pushm	d2-d5,-(sp)
	move.w	d1,AddF
	moveq	#0,AddI
	swap.w	d1
	move.w	d1,AddI
	moveq	#0,OffsI
	moveq	#0,OffsF
	add.l	(a2),Src		;Get Source
	move.l	(a3),Dst		;Get Dest
	lsr.l	#1,Cnt			;Unroll one time
	bcs	.1
	subq.w	#1,Cnt
	bmi.b	.exit
.nextsample
 IFGE	__CPU-68020
	move.w	(Src,OffsI.l*4),(Dst)+
 ELSE
	move.l	OffsI,Tmp
	add.l	Tmp,Tmp
	add.l	Tmp,Tmp
	move.w	(Src,Tmp.l),(Dst)+
 ENDC
	add.w	AddF,OffsF
	addx.l	AddI,OffsI
.1
 IFGE	__CPU-68020
	move.w	(Src,OffsI.l*4),(Dst)+
 ELSE
	move.l	OffsI,Tmp
	add.l	Tmp,Tmp
	add.l	Tmp,Tmp
	move.w	(Src,Tmp.l),(Dst)+
 ENDC
	add.w	AddF,OffsF
	addx.l	AddI,OffsI
	dbf	Cnt,.nextsample
	lsl.l	#2,OffsI
	add.l	OffsI,(a2)		;Update Offset
	move.l	Dst,(a3)		;Update Dest
.exit
	popm	d2-d5,-(sp)
	rts

*******************************************************************************
** RecS16S ********************************************************************
*******************************************************************************

_RecS16S:
	pushm	d2-d5,-(sp)
	move.w	d1,AddF
	moveq	#0,AddI
	swap.w	d1
	move.w	d1,AddI
	moveq	#0,OffsI
	moveq	#0,OffsF
	add.l	(a2),Src		;Get Source
	move.l	(a3),Dst		;Get Dest
	lsr.l	#1,Cnt			;Unroll one time
	bcs	.1
	subq.w	#1,Cnt
	bmi.b	.exit
.nextsample
 IFGE	__CPU-68020
	move.l	(Src,OffsI.l*4),(Dst)+
 ELSE
	move.l	OffsI,Tmp
	add.l	Tmp,Tmp
	add.l	Tmp,Tmp
	move.l	(Src,Tmp.l),(Dst)+
 ENDC
	add.w	AddF,OffsF
	addx.l	AddI,OffsI
.1
 IFGE	__CPU-68020
	move.l	(Src,OffsI.l*4),(Dst)+
 ELSE
	move.l	OffsI,Tmp
	add.l	Tmp,Tmp
	add.l	Tmp,Tmp
	move.l	(Src,Tmp.l),(Dst)+
 ENDC
	add.w	AddF,OffsF
	addx.l	AddI,OffsI
	dbf	Cnt,.nextsample
	lsl.l	#2,OffsI
	add.l	OffsI,(a2)		;Update Offset
	move.l	Dst,(a3)		;Update Dest
.exit
	popm	d2-d5,-(sp)
	rts

*******************************************************************************
** RecM32S ********************************************************************
*******************************************************************************

_RecM32S:
	pushm	d2-d5,-(sp)
	move.w	d1,AddF
	moveq	#0,AddI
	swap.w	d1
	move.w	d1,AddI
	moveq	#0,OffsI
	moveq	#0,OffsF
	add.l	(a2),Src		;Get Source
	move.l	(a3),Dst		;Get Dest
	lsr.l	#1,Cnt			;Unroll one time
	bcs	.1
	subq.w	#1,Cnt
	bmi.b	.exit
.nextsample
 IFGE	__CPU-68020
	move.w	(Src,OffsI.l*4),(Dst)+
 ELSE
	move.l	OffsI,Tmp
	add.l	Tmp,Tmp
	add.l	Tmp,Tmp
	move.w	(Src,Tmp.l),(Dst)+
 ENDC
	clr.w	(Dst)+
	add.w	AddF,OffsF
	addx.l	AddI,OffsI
.1
 IFGE	__CPU-68020
	move.w	(Src,OffsI.l*4),(Dst)+
 ELSE
	move.l	OffsI,Tmp
	add.l	Tmp,Tmp
	add.l	Tmp,Tmp
	move.w	(Src,Tmp.l),(Dst)+
 ENDC
	clr.w	(Dst)+
	add.w	AddF,OffsF
	addx.l	AddI,OffsI
	dbf	Cnt,.nextsample
	lsl.l	#2,OffsI
	add.l	OffsI,(a2)		;Update Offset
	move.l	Dst,(a3)		;Update Dest
.exit
	popm	d2-d5,-(sp)
	rts

*******************************************************************************
** RecS32S ********************************************************************
*******************************************************************************

_RecS32S:
	pushm	d2-d5,-(sp)
	move.w	d1,AddF
	moveq	#0,AddI
	swap.w	d1
	move.w	d1,AddI
	moveq	#0,OffsI
	moveq	#0,OffsF
	add.l	(a2),Src		;Get Source
	move.l	(a3),Dst		;Get Dest
	lsr.l	#1,Cnt			;Unroll one time
	bcs	.1
	subq.w	#1,Cnt
	bmi.b	.exit
.nextsample
 IFGE	__CPU-68020
	move.w	(Src,OffsI.l*4),(Dst)+
	clr.w	(Dst)+
	move.w	2(Src,OffsI.l*4),(Dst)+
	clr.w	(Dst)+
 ELSE
	move.l	OffsI,Tmp
	add.l	Tmp,Tmp
	add.l	Tmp,Tmp
	move.w	(Src,Tmp.l),(Dst)+
	clr.w	(Dst)+
	move.w	2(Src,Tmp.l),(Dst)+
	clr.w	(Dst)+
 ENDC
	add.w	AddF,OffsF
	addx.l	AddI,OffsI
.1
 IFGE	__CPU-68020
	move.w	(Src,OffsI.l*4),(Dst)+
	clr.w	(Dst)+
	move.w	2(Src,OffsI.l*4),(Dst)+
	clr.w	(Dst)+
 ELSE
	move.l	OffsI,Tmp
	add.l	Tmp,Tmp
	add.l	Tmp,Tmp
	move.w	(Src,Tmp.l),(Dst)+
	clr.w	(Dst)+
	move.w	2(Src,Tmp.l),(Dst)+
	clr.w	(Dst)+
 ENDC
	add.w	AddF,OffsF
	addx.l	AddI,OffsI
	dbf	Cnt,.nextsample
	lsl.l	#2,OffsI
	add.l	OffsI,(a2)		;Update Offset
	move.l	Dst,(a3)		;Update Dest
.exit
	popm	d2-d5,-(sp)
	rts
