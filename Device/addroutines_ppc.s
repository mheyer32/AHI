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

AddI		.equ	56 + 8
AddF		.equ	56 + 12
Offset		.equ	56 + 16
StopAtZero	.equ	56 + 20

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
AddByteStereoB:
AddBytesMonoB:
AddBytesStereoB:
AddWordMonoB:
AddWordStereoB:
AddWordsMonoB:
AddWordsStereoB:
	blr



AddSilenceMono:
	mullw   r14,r18,r3		# (add:high * samples) : low
	mullw	r15,r19,r3		# (add:low * samples) : low 
	add	r16,r16,r14
	addc	r17,r17,r15
	mulhw	r14,r19,r3		# (add:low * samples) : high 
	adde	r16,r16,r14

	slwi	r15,r3,2
	add	r20,r20,r15
	li	r3,0
	blr

AddSilenceStereo:
	mullw   r14,r18,r3		# (add:high * samples) : low
	mullw	r15,r19,r3		# (add:low * samples) : low 
	add	r16,r16,r14
	addc	r17,r17,r15
	mulhw	r14,r19,r3		# (add:low * samples) : high 
	adde	r16,r16,r14

	slwi	r15,r3,3
	add	r20,r20,r15
	li	r3,0
	blr

AddSilenceMonoB:
	mullw   r14,r18,r3		# (add:high * samples) : low
	mullw	r15,r19,r3		# (add:low * samples) : low 
	sub	r16,r16,r14
	subc	r17,r17,r15
	mulhw	r14,r19,r3		# (add:low * samples) : high 
	subfe	r16,r14,r16

	slwi	r15,r3,2
	add	r20,r20,r15
	li	r3,0
	blr

AddSilenceStereoB:
	mullw   r14,r18,r3		# (add:high * samples) : low
	mullw	r15,r19,r3		# (add:low * samples) : low 
	sub	r16,r16,r14
	subc	r17,r17,r15
	mulhw	r14,r19,r3		# (add:low * samples) : high 
	subfe	r16,r14,r16

	slwi	r15,r3,3
	add	r20,r20,r15
	li	r3,0
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
r23	lastpoint left
r24	lastpoint right

*/

	.macro	prelude

	stwu	r1,-56(r1)
	stw	r14,8(r1)
	stw	r15,12(r1)
	stw	r16,16(r1)
	stw	r17,20(r1)
	stw	r18,24(r1)
	stw	r19,28(r1)
	stw	r20,32(r1)
	stw	r21,36(r1)
	stw	r22,40(r1)
	stw	r23,44(r1)
	stw	r24,48(r1)

	lwz	r14,Offset(r1)
	lwz	r18,AddI(r1)
	lwz	r19,AddF(r1)
	lwz	r16,0(r14)
	lwz	r17,4(r14)

	lwz	r20,0(r9)
	subi	r20,r20,4

	li	r23,0
	li	r24,0

	mtctr	r3			# Number of loop times to CTR

	.endm


	.macro	postlude

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
	lwz	r23,44(r1)
	lwz	r24,48(r1)

	addi	r1,r1,56

	.endm



x:	.ASSIGNC "AddByteMono"

\&x:
	prelude

	lhz	r15,StopAtZero(r1)	# Test StopAtZero
	cmpwi	cr0,r15,0
	bne+	.Lfirst_sampleZ\&x
	cmpwi	cr0,r4,0		# Test if volume == 0
	bne+	.Lfirst_sample\&x
#	bl	AddSilenceMono
#	b	.Labort\&x
	b	.Lfirst_sample\&x

.Lnext_sampleZ\&x:
	addc	r17,r17,r19		# Add fraction
	adde	r16,r16,r18		# Add integer
.Lfirst_sampleZ\&x:
	cmp	cr0,1,r16,r10		# Offset == FirstOffset?
	add	r14,r8,r16		# (Calculate &src[ offset ])
	bne+	.Lnot_firstZ\&x
	lwz	r15,0(r6)		# Fetch left lastpoint (normalized)
	lbz	r22,0(r14)		# Fetch src[ offset ]
	b	.Lgot_sampleZ\&x
.Lnot_firstZ\&x:
	lbz	r15,-1(r14)		# Fetch src[ offset - 1 ]
	lbz	r22,0(r14)		# Fetch src[ offset ]
	slwi	r15,r15,8		# Normalize...
	extsh	r15,r15			# ...src[ offset - 1 ]
.Lgot_sampleZ\&x:
	slwi	r22,r22,8		# Normalize...
	extsh	r22,r22			# ...src[ offset ]
	srwi	r14,r17,17		# Get linear high word / 2
	sub	r22,r22,r15
	mullw	r14,r14,r22		# Linear interpolation
	lwz	r22,4(r20)		# Fetsh *dst
	srawi	r14,r14,15
	add	r14,r14,r15

	cmpwi	cr0,r23,0
	bgt	.Llastpoint_gtZ\&x
	beq	.Llastpoint_checkedZ\&x
	cmpwi	cr0,r14,0
	bge	.Labort\&x
	b	.Llastpoint_checkedZ\&x
.Llastpoint_gtZ\&x:
	cmpwi	cr0,r14,0
	ble	.Labort\&x
.Llastpoint_checkedZ\&x:
	mr	r23,r14			# Update lastsample

	mullw	r14,r14,r4		# Volume scale
	add	r14,r14,r22
	stwu	r14,4(r20)		# Store to *dst, dst++

	bdnz	.Lnext_sampleZ\&x
	b	.Lexit\&x

.Lnext_sample\&x:
	addc	r17,r17,r19		# Add fraction
	adde	r16,r16,r18		# Add integer
.Lfirst_sample\&x:
	cmp	cr0,1,r16,r10		# Offset == FirstOffset?
	add	r14,r8,r16		# (Calculate &src[ offset ])
	bne+	.Lnot_first\&x
	lwz	r15,0(r6)		# Fetch left lastpoint (normalized)
	lbz	r22,0(r14)		# Fetch src[ offset ]
	b	.Lgot_sample\&x
.Lnot_first\&x:
	lbz	r15,-1(r14)		# Fetch src[ offset - 1 ]
	lbz	r22,0(r14)		# Fetch src[ offset ]
	slwi	r15,r15,8		# Normalize...
	extsh	r15,r15			# ...src[ offset - 1 ]
.Lgot_sample\&x:
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

	bdnz	.Lnext_sample\&x
	b	.Lexit\&x

.Labort\&x:
	li	r18,0
	li	r19,0
.Lexit\&x:
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

	postlude
	blr


x:	.ASSIGNC "AddByteMonoB"

\&x:
	prelude

	lhz	r15,StopAtZero(r1)	# Test StopAtZero
	cmpwi	cr0,r15,0
	bne+	.Lfirst_sampleZ\&x
	cmpwi	cr0,r4,0		# Test if volume == 0
	bne+	.Lfirst_sample\&x
#	bl	AddSilenceMonoB
#	b	.Labort\&x
	b	.Lfirst_sample\&x

.Lnext_sampleZ\&x:
	subfc	r17,r19,r17		# Subtract fraction
	subfe	r16,r18,r16		# Subtract integer
.Lfirst_sampleZ\&x:
	cmp	cr0,1,r16,r10		# Offset == FirstOffset?
	add	r14,r8,r16		# (Calculate &src[ offset ])
	bne+	.Lnot_firstZ\&x
	lwz	r22,0(r6)		# Fetch left lastpoint (normalized)
	lbz	r15,0(r14)		# Fetch src[ offset ]
	b	.Lgot_sampleZ\&x
.Lnot_firstZ\&x:
	lbz	r22,1(r14)		# Fetch src[ offset + 1 ]
	lbz	r15,0(r14)		# Fetch src[ offset ]
	slwi	r22,r22,8		# Normalize...
	extsh	r22,r22			# ...src[ offset + 1 ]
.Lgot_sampleZ\&x:
	slwi	r15,r15,8		# Normalize...
	extsh	r15,r15			# ...src[ offset ]
	srwi	r14,r17,17		# Get linear high word / 2
	sub	r22,r22,r15
	mullw	r14,r14,r22		# Linear interpolation
	lwz	r22,4(r20)		# Fetsh *dst
	srawi	r14,r14,15
	add	r14,r14,r15

	cmpwi	cr0,r23,0
	bgt	.Llastpoint_gtZ\&x
	beq	.Llastpoint_checkedZ\&x
	cmpwi	cr0,r14,0
	bge	.Labort\&x
	b	.Llastpoint_checkedZ\&x
.Llastpoint_gtZ\&x:
	cmpwi	cr0,r14,0
	ble	.Labort\&x
.Llastpoint_checkedZ\&x:
	mr	r23,r14			# Update lastsample

	mullw	r14,r14,r4		# Volume scale
	add	r14,r14,r22
	stwu	r14,4(r20)		# Store to *dst, dst++

	bdnz	.Lnext_sampleZ\&x
	b	.Lexit\&x

.Lnext_sample\&x:
	subfc	r17,r19,r17		# Subtract fraction
	subfe	r16,r18,r16		# Subtract integer
.Lfirst_sample\&x:
	cmp	cr0,1,r16,r10		# Offset == FirstOffset?
	add	r14,r8,r16		# (Calculate &src[ offset ])
	bne+	.Lnot_first\&x
	lwz	r22,0(r6)		# Fetch left lastpoint (normalized)
	lbz	r15,0(r14)		# Fetch src[ offset ]
	b	.Lgot_sample\&x
.Lnot_first\&x:
	lbz	r22,1(r14)		# Fetch src[ offset + 1 ]
	lbz	r15,0(r14)		# Fetch src[ offset ]
	slwi	r22,r22,8		# Normalize...
	extsh	r22,r22			# ...src[ offset + 1 ]
.Lgot_sample\&x:
	slwi	r15,r15,8		# Normalize...
	extsh	r15,r15			# ...src[ offset ]
	srwi	r14,r17,17		# Get linear high word / 2
	sub	r22,r22,r15
	mullw	r14,r14,r22		# Linear interpolation
	lwz	r22,4(r20)		# Fetsh *dst
	srawi	r14,r14,15
	add	r14,r14,r15
	mullw	r14,r14,r4		# Volume scale
	add	r14,r14,r22
	stwu	r14,4(r20)		# Store to *dst, dst++

	bdnz	.Lnext_sample\&x
	b	.Lexit\&x

.Labort\&x:
	li	r18,0
	li	r19,0
.Lexit\&x:
	add	r14,r8,r16		# (Calculate &src[ offset ])
	lbz	r22,0(r14)		# Fetch src[ offset ]
	slwi	r22,r22,8		# Normalize...
	extsh	r22,r22			# ...src[ offset ]
	stw	r22,0(r6)

	subfc	r17,r19,r17		# Subtract fraction
	subfe	r16,r18,r16		# Subtract integer

	lwz	r14,Offset(r1)
	stw	r16,0(r14)
	stw	r17,4(r14)

	addi	r20,r20,4
	stw	r20,0(r9)

	postlude
	blr


	.end
