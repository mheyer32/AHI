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

/* ppc

LONG	  Samples		r3     long		Samples
LONG	  ScaleLeft		r4     long		ScaleLeft
LONG	  ScaleRight		r5     long		ScaleRight
LONG	 *StartPointLeft	r6     long		StartPointLeft
LONG	 *StartPointRight	r7     long		StartPointRight
void	 *Src			r8     long		Src
void	**Dst			r9     long		Dst
LONG	  FirstOffsetI		r10    long		FirstOffsetI
Fixed64	  Add			 8(r1) long long	Add
Fixed64	 *Offset		16(r1) long		Offset
BOOL	  StopAtZero		20(r1) word		StopAtZero

*/

AddI		= 48 + 8
AddF		= 48 + 12
Offset		= 48 + 16
StopAtZero	= 48 + 20

	.text

	.globl	AddByteMono
	.globl	AddByteStereo
	.globl	AddBytesMono
	.globl	AddBytesStereo
	.globl	AddWordMono
	.globl	AddWordStereo
	.globl	AddWordsMono
	.globl	AddWordsStereo
	.globl	AddByteMonoB
	.globl	AddByteStereoB
	.globl	AddBytesMonoB
	.globl	AddBytesStereoB
	.globl	AddWordMonoB
	.globl	AddWordStereoB
	.globl	AddWordsMonoB
	.globl	AddWordsStereoB

	.type	AddByteMono,@function
	.type	AddByteStereo,@function
	.type	AddBytesMono,@function
	.type	AddBytesStereo,@function
	.type	AddWordMono,@function
	.type	AddWordStereo,@function
	.type	AddWordsMono,@function
	.type	AddWordsStereo,@function
	.type	AddByteMonoB,@function
	.type	AddByteStereoB,@function
	.type	AddBytesMonoB,@function
	.type	AddBytesStereoB,@function
	.type	AddWordMonoB,@function
	.type	AddWordStereoB,@function
	.type	AddWordsMonoB,@function
	.type	AddWordsStereoB,@function

AddByteStereo:
AddBytesMono:
AddBytesStereo:
AddWordMono:
AddWordStereo:
AddWordsMono:
AddWordsStereo:
AddByteMonoB:
AddByteStereoB:
AddBytesMonoB:
AddBytesStereoB:
AddWordMonoB:
AddWordStereoB:
AddWordsMonoB:
AddWordsStereoB:
	stwu	r1,-36(r1)
	stw	r14,8(r1)
	stw	r16,12(r1)
	stw	r17,16(r1)
	stw	r18,20(r1)
	stw	r19,24(r1)

	lwz	r14,Offset(r1)
	lwz	r18,AddI(r1)
	lwz	r19,AddF(r1)
	lwz	r16,0(r14)
	lwz	r17,4(r14)

	lwz	r16,8(r1)
	lwz	r17,12(r1)
	lwz	r18,16(r1)
	lwz	r19,20(r1)
	addi	r1,r1,36

	blr



AddSilenceMono:
	blr

/*

Register usage:

r3	samples
r4	scaleleft
r5	scaleright
r16	int offset
r17	fract offset
r18	int add
r19	fract add
r8	src
r20	dst
r10	firstoffset
r6	left lastpoint
r7	right lastpoint

*/

AddByteMono:
	stwu	r1,-48(r1)
	stw	r14,8(r1)
	stw	r15,12(r1)
	stw	r16,16(r1)
	stw	r17,20(r1)
	stw	r18,24(r1)
	stw	r19,28(r1)
	stw	r20,32(r1)
	stw	r21,36(r1)
	stw	r22,40(r1)

	lwz	r14,Offset(r1)
	lwz	r18,AddI(r1)
	lwz	r19,AddF(r1)
	lwz	r16,0(r14)
	lwz	r17,4(r14)

	lwz	r20,0(r9)
	subi	r20,r20,4

	mtctr	r3			# Number of loop times to CTR
	b	first_sample

next_sample:
	addc	r17,r17,r19		# Add fraction
	adde	r16,r16,r18		# Add integer
first_sample:
	cmp	cr0,1,r16,r10		# Offset == FirstOffset?
	add	r14,r8,r16		# (Calculate &src[ offset ])
	bne+	not_first
	lwz	r15,0(r6)		# Fetch left lastpoint (normalized)
	lbz	r22,0(r14)		# Fetch src[ offset ]
	b	got_sample
not_first:
	lbz	r15,-1(r14)		# Fetch src[ offset - 1 ]
	lbz	r22,0(r14)		# Fetch src[ offset ]
	slwi	r15,r15,8		# Normalize...
	extsh	r15,r15			# ...src[ offset - 1 ]
got_sample:
	slwi	r22,r22,8		# Normalize...
	extsh	r22,r22			# ...src[ offset ]
	srwi	r14,r17,17		# Get linear high word / 2
	sub	r22,r22,r15
	mullw	r14,r14,r22		# Linear interpolation
	lwz	r22,4(r20)		# Fetsh *dst
	srawi	r14,r14,15
	add	r14,r14,r15
	mullw	r14,r14,r4		# Volume scale
	add	r14,r14,r22
	stwu	r14,4(r20)		# Store to *dst, dst++

	bdnz	next_sample
	b	exit

abort:
	li	r18,0
	li	r19,0
exit:
	add	r14,r8,r16		# (Calculate &src[ offset ])
	lbz	r22,0(r14)		# Fetch src[ offset ]
	slwi	r22,r22,8		# Normalize...
	extsh	r22,r22			# ...src[ offset ]
	stw	r22,0(r6)

	addc	r17,r17,r19		# Add fraction
	adde	r16,r16,r18		# Add integer

	lwz	r14,Offset(r1)
	stw	r16,0(r14)
	stw	r17,4(r14)

	addi	r20,r20,4
	stw	r20,0(r9)

	mfctr	r21
	sub	r3,r3,r21

	lwz	r14,8(r1)
	lwz	r15,12(r1)
	lwz	r16,16(r1)
	lwz	r17,20(r1)
	lwz	r18,24(r1)
	lwz	r19,28(r1)
	lwz	r20,32(r1)
	lwz	r21,36(r1)
	lwz	r22,40(r1)

	addi	r1,r1,48
	blr
