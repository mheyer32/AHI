/* $Id$ */

/*
     AHI - Hardware independent audio subsystem
     Copyright (C) 1996-1999 Martin Blom <martin@blom.org>
     
     This library is free software; you can redistribute it and/or
     modify it under the terms of the GNU Library General Public
     License as published by the Free Software Foundation; either
     version 2 of the License, or (at your option) any later version.
     
     This library is distributed in the hope that it will be useful,
     but WITHOUT ANY WARRANTY; without even the implied warranty of
     MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
     Library General Public License for more details.
     
     You should have received a copy of the GNU Library General Public
     License along with this library; if not, write to the
     Free Software Foundation, Inc., 59 Temple Place - Suite 330, Cambridge,
     MA 02139, USA.
*/

/******************************************************************************
** Add-Routines ***************************************************************
******************************************************************************/

/* m68k:

LONG	  Samples		 4(sp) long		Samples
LONG	  ScaleLeft		 8(sp) long		ScaleLeft
LONG	  ScaleRight		12(sp) long		ScaleRight
LONG	 *StartPointLeft	16(sp) long		StartPointLeft
LONG	 *StartPointRight	20(sp) long		StartPointRight
void	 *Src			24(sp) long		Src
void	**Dst			28(sp) long		Dst
LONG	  FirstOffsetI		32(sp) long		FirstOffsetI
Fixed64	  Add			36(sp) long long	Add
Fixed64	 *Offset		44(sp) long		Offset
BOOL	  StopAtZero		50(sp) word		StopAtZero

*/

Samples		= 11*4 + 4
ScaleLeft	= 11*4 + 8
ScaleRight	= 11*4 + 12
StartPointLeft	= 11*4 + 16
StartPointRight	= 11*4 + 20
Src		= 11*4 + 24
Dst		= 11*4 + 28
FirstOffsetI	= 11*4 + 32
AddI		= 11*4 + 36
AddF		= 11*4 + 40
Offset		= 11*4 + 44
StopAtZero	= 11*4 + 50

/*

Register usage:

d0	counter
d1	scaleleft
d2	scaleright
d3	int offset
d4.w	fract offset
d5	int add
d6.w	fract offset
a0	src
a1	dst
a2	firstoffset
a5	left lastpoint
a6	right lastpoint

*/

	.text

	.globl	_AddByteMono


_AddByteMono:
	movem.l	d2-d7/a2-a6,-(sp)
	move.l	(Samples,sp),d0			/* counter */
	move.l	(ScaleLeft,sp),d1
# 	move.l	(ScaleRight,sp),d2
	move.l	(Src,sp),a0
	move.l	([Dst,sp]),a1
	move.l	(FirstOffsetI,sp),a2
	move.l	(AddI,sp),d5			/* Integer add */
	moveq	#0,d6
	move.w	(AddF+2,sp),d6			/* Fraction add (16 bit) */
	move.l	([Offset,sp],0),d3		/* Integer offset */
	moveq	#0,d5
	move.w	([Offset,sp],4),d4		/* Fraction offset (16 bit) */
	suba.l	a5,a5
#	suba.l	a6,a6

	tst.w	(StopAtZero,sp)
	bne.b	Lfirst_sampleZ
	bra.b	Lfirst_sample

Lnext_sampleZ:
	add.w	d6,d4
	addx.l	d5,d3
Lfirst_sampleZ:
	cmp.l	a2,d3
	bne.b	Lnot_firstZ
	move.l	([StartPointLeft,sp]),a3
	bra.b	Lgot_sampleZ
Lnot_firstZ:
	move.b	(-1,a0,d3.l),d7
	lsl.w	#8,d7
	move.w	d7,a3				/* sign extend */
Lgot_sampleZ:
	move.b	(0,a0,d3.l),d7
	lsl.w	#8,d7
	ext.l	d7
	sub.l	a3,d7
	asr.l	#1,d7
	muls.l	d4,d7
	asr.l	#7,d7
	asr.l	#8,d7
	add.l	a3,d7

	tst.l	a5
	bgt.b	Llastpoint_gtZ
	beq.b	Llastpoint_checkedZ
	tst.l	d7
	bge.b	Labort
	bra.b	Llastpoint_checkedZ
Llastpoint_gtZ:
	tst.l	d7
	ble.b	Labort
Llastpoint_checkedZ:
	move.l	d7,a5				/* update lastsample */

	muls.l	d1,d7
	add.l	d7,(a1)+

	subq.l	#1,d0
	bne.b	Lnext_sampleZ
	bra.b	Lexit

Lnext_sample:
	add.w	d6,d4
	addx.l	d5,d3
Lfirst_sample:
	cmp.l	a2,d3
	bne.b	Lnot_first
	move.l	([StartPointLeft,sp]),a3
	bra.b	Lgot_sample
Lnot_first:
	move.b	(-1,a0,d3.l),d7
	lsl.w	#8,d7
	move.w	d7,a3				/* sign extend */
Lgot_sample:
	move.b	(0,a0,d3.l),d7
	lsl.w	#8,d7
	ext.l	d7
	sub.l	a3,d7
	asr.l	#1,d7
	muls.l	d4,d7
	asr.l	#7,d7
	asr.l	#8,d7
	add.l	a3,d7

	muls.l	d1,d7
	add.l	d7,(a1)+

	subq.l	#1,d0
	bne.b	Lnext_sample
	bra.b	Lexit

Labort:
	moveq	#0,d5				/* Prevent the last add */
	moveq	#0,d6
Lexit:
	move.b	(0,a0,d3.l),d7			/* Fetch last endpoint */
	lsl.w	#8,d7
	ext.l	d7
	move.l	d7,([StartPointLeft,sp])

	add.w	d6,d4
	addx.l	d5,d3

	move.l	d3,([Offset,sp],0)		/* Integer offset */
	move.w	d4,([Offset,sp],4)		/* Fraction offset (16 bit) */

	move.l	a1,([Dst,sp])

	sub.l	(Samples,sp),d0
	neg.l	d0				/* Return Samples - d0 */
	movem.l	(sp)+,d2-d7/a2-a6
	rts
