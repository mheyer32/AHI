; $Id$

;    AHI - Hardware independent audio subsystem
;    Copyright (C) 1996-1999 Martin Blom <martin@blom.org>
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

	include	exec/exec.i
	include	lvo/exec_lib.i
	include	devices/ahi.i
	include	utility/hooks.i
	include	lvo/utility_lib.i
	include	ahi_def.i
	include	dsp.i

	XDEF	_InitMixroutine
	XDEF	_CleanUpMixroutine

	XDEF	_calcMasterVolumeTable
	XDEF	_initSignedTable
	XDEF	_calcSignedTable
	XDEF	_initUnsignedTable
	XDEF	_calcUnsignedTable
	XDEF	_SelectAddRoutine
	XDEF	_Mix68k
	XDEF	_CalcSamples

	XREF	_SysBase
	XREF	_UtilityBase
	XREF	_UDivMod64
	XREF	_Fixed2Shift

TABLEMAXVOL	EQU	32
TABLESHIFT	EQU	11	(TABLEMAXVOL<<TABLESHIFT == 0x10000)

	section	.text,code

;-----------------------------------------

;in:
* a2	ptr to AHIAudioCtrl
;out:
* d0	TRUE on success
_InitMixroutine:
	pushm	d1-a6
	move.l	4.w,a6
	move.w	ahiac_Channels(a2),d0
	mulu.w	#AHIChannelData_SIZEOF,d0
	move.l	#MEMF_PUBLIC!MEMF_CLEAR,d1	;may be accessed from interrupts!
	call	AllocVec
	move.l	d0,ahiac_ChannelDatas(a2)
	beq	.error

	clr.l	ahiac_DryList(a2)
	move.l	ahiac_ChannelDatas(a2),a0
	move.l	a0,ahiac_WetList(a2)

*** Update the ChannelData structures (cd_ChannelNo and cd_Succ)
	moveq	#0,d0
.updateCDloop
	move.w	d0,cd_ChannelNo(a0)
	lea	AHIChannelData_SIZEOF(a0),a1
	move.l	a1,cd_Succ(a0)
	add.w	#AHIChannelData_SIZEOF,a0
	addq.w	#1,d0
	cmp.w	ahiac_Channels(a2),d0
	blo	.updateCDloop
	clr.l	-AHIChannelData_SIZEOF+cd_Succ(a0)

	move.w	ahiac_Sounds(a2),d0
	mulu.w	#AHISoundData_SIZEOF,d0
	move.l	#MEMF_PUBLIC!MEMF_CLEAR,d1	;may be accessed from interrupts!
	call	AllocVec
	move.l	d0,ahiac_SoundDatas(a2)
	beq	.error

*** Update the SoundData structure
	move.l	d0,a0
	move.w	ahiac_Sounds(a2),d0
	subq.w	#1,d0
.updateSDloop
	move.l	#AHIST_NOTYPE,sd_Type(a0)
	add.w	#AHISoundData_SIZEOF,a0
	dbf	d0,.updateSDloop

	moveq	#TRUE,d0
.exit	
	popm	d1-a6
	rts
.error
	moveq	#FALSE,d0
	bra	.exit


;in:
* a2	ptr to AHIAudioCtrl
_CleanUpMixroutine:
	rts

;in:
* a2	AHIAudioCtrl
_calcMasterVolumeTable:
	pushm	std
	btst.b	#AHIACB_CLIPPING-24,ahiac_Flags(a2)
	beq	.exit

	btst	#AHIACB_HIFI,ahiac_Flags+3(a2)
	bne	.exit


	move.l	_SysBase,a6
	move.l	ahiac_MasterVolumeTable(a2),d0
	bne	.gottable
	move.l	#65536*2,d0
	moveq	#MEMF_PUBLIC,d1
	call	AllocVec
	move.l	d0,ahiac_MasterVolumeTable(a2)
	beq	.exit

.gottable
	move.l	d0,a0
	moveq	#0,d0
	move.l	ahiac_SetMasterVolume(a2),d1
	lsr.l	#8,d1
.loop
	move.w	d0,d2
	muls.w	d1,d2
	asr.l	#8,d2
	cmp.l	#32767,d2
	ble	.noposclip
	move.w	#32767,d2
.noposclip
	cmp.l	#-32768,d2
	bge	.nonegclip
	move.w	#-32768,d2
.nonegclip
	move.w	d2,(a0)+
	addq.w	#1,d0
	bne	.loop
.exit
	popm	std
	rts

;in:
* a2	ptr to AHIAudioCtrl
;out:
* d0	TRUE on success
_initSignedTable:
	pushm	d1-a6
	
	move.l	ahiac_Flags(a2),d0
	btst.l	#AHIACB_MULTTAB,d0
	beq.b	.notable
	tst.l	ahiac_MultTableS(a2)
	bne.b	.notable			;there is already a table!

	move.l	_SysBase,a6
	move.l	#256*(TABLEMAXVOL+1)*4,d0	;include highest volume, too!
	moveq	#MEMF_PUBLIC,d1			;may be accessed from interrupts
	call	AllocVec
	move.l	d0,ahiac_MultTableS(a2)
	beq.b	.error
	bsr.b	_calcSignedTable
.notable
	moveq	#TRUE,d0
.exit
	popm	d1-a6
	rts
.error
	moveq	#FALSE,d0
	bra	.exit

* Create multiplication table for use with signed bytes/words.
*
* WORD hints:
* // 0<=v<=32 // (h*256+l)*(v*2048)>>16 = ((h*v)<<8+(l*v))>>5
*s	=$fffffcf4
*h	=s>>8		;signed
*l	=s&255		;unsigned
*v	=16
*	PRINTV	h*v,l*v
*	PRINTV	((h*v)<<8+(l*v))>>5
*
* Usage: a0 points to correct line in table
*
*        d1 is signed byte with bits 8-15 cleared:
*        move.w  0(a0,d1.w*4),d2		;d2 is signed!
*
*        d1 is unsigned byte with bits 8-15 cleared:
*        move.w  2(a0,d1.w*4),d2		;d2 is unsigned!
_calcSignedTable:
	pushm	d0-d4/a0
	move.l	ahiac_MultTableS(a2),d0
	beq.b	.notable
	move.l	d0,a0
	add.l	#256*(TABLEMAXVOL+1)*4,a0
	move.w	ahiac_Channels2(a2),d3
	lsl.w	#8,d3
	move.l	ahiac_MasterVolume(a2),d4	; Range: (0 .. 1.0 .. ??) * 65536
	lsr.l	#8,d4
	moveq.l	#(TABLEMAXVOL+1)-1,d0
.10
	move.w	#255,d1
.11
	move.w	d1,d2				; Unsigned
	mulu.w	d0,d2
	mulu.w	d4,d2				; *((Mastervolume*65536)/256)
	lsl.l	#TABLESHIFT-8,d2
	divu.w	d3,d2				; /(Channels*256)
	move.w	d2,-(a0)

	move.b	d1,d2 				; Signed
	ext.w	d2
	muls.w	d0,d2
	muls.w	d4,d2				; *((Mastervolume*65536)/256)
	asl.l	#TABLESHIFT-8,d2
	divs.w	d3,d2				; /(Channels*256)
	move.w	d2,-(a0)

	dbf	d1,.11
	dbf	d0,.10
.notable
	popm	d0-d4/a0
	rts
;in:
* a2	ptr to AHIAudioCtrl
;out:
* d0	TRUE on success
_initUnsignedTable:
	pushm	d1-a6
	move.l	ahiac_Flags(a2),d0

; Unsigned samples unconditionally uses tables!
;	btst.l	#AHIACB_MULTTAB,d0
;	beq.b	.notable

	tst.l	ahiac_MultTableU(a2)
	bne.b	.notable			;there is already a table!

	move.l	_SysBase,a6
	move.l	#256*(TABLEMAXVOL+1)*4,d0	;incude highest volume, too!
						;*4 => Look like the signed table!
	moveq	#MEMF_PUBLIC,d1			;may be accessed from interrupts
	call	AllocVec
	move.l	d0,ahiac_MultTableU(a2)
	beq.b	.error
	bsr.b	_calcUnsignedTable
.notable
	moveq	#TRUE,d0
.exit
	popm	d1-a6
	rts
.error
	moveq	#FALSE,d0
	bra	.exit

* Create multiplication table for use with unsigned bytes
* Note: The table has valid values in every other word. This is
* done in order to be able to use the same mixing routines for
* both signed and unsigned samples.
* Usage: a0 points to correct line in table
*
*        d1 is unsigned byte with bits 8-15 cleared:
*        move.w  0(a0,d1.w*4),d2		;d2 is signed!
_calcUnsignedTable:
	pushm	d0-d4/a0
	move.l	ahiac_MultTableU(a2),d0
	beq.b	.notable
	move.l	d0,a0
	add.l	#256*(TABLEMAXVOL+1)*4,a0
	move.w	ahiac_Channels2(a2),d3
	lsl.w	#8,d3
	move.l	ahiac_MasterVolume(a2),d4	; Range: (0 .. 1.0 .. ??) * 65536
	lsr.l	#8,d4
	moveq.l	#(TABLEMAXVOL+1)-1,d0
.10
	move.w	#255,d1
.11
	move.b	d1,d2				; Unsigned
	sub.b	#$80,d2				; -> Signed
	ext.w	d2
	muls.w	d0,d2
	muls.w	d4,d2				; *((Mastervolume*65536)/256)
	asl.l	#TABLESHIFT-8,d2
	divs.w	d3,d2				; /(Channels*256)
	clr.w	-(a0)				; Insert dummy value
	move.w	d2,-(a0)

	dbf	d1,.11
	dbf	d0,.10
.notable
	popm	d0-d4/a0
	rts


;in:
* d0	VolumeLeft (Fixed)
* d1	VolumeRight (Fixed)
* d2	SampleType
* a0	LONG *ScaleLeft
* a1	LONG *ScaleRight
* a2	AudioCtrl
* a3	void *AddRoutine

RIGHTVOLUME	EQU	1
LEFTVOLUME	EQU	2
FASTMIX		EQU	4
STEREO		EQU	8
HIFI		EQU	16

EIGHTBIT	EQU	0
EIGHT2BIT	EQU	32
SIXTEENBIT	EQU	64
SIXTEEN2BIT	EQU	96

typeconversion:
	dc.l	EIGHTBIT	;AHIST_M8S  (0)
	dc.l	SIXTEENBIT	;AHIST_M16S (1)
	dc.l	EIGHT2BIT	;AHIST_S8S  (2)
	dc.l	SIXTEEN2BIT	;AHIST_S16S (3)
	dc.l	-1		;AHIST_M8U  (4) << OBSOLETE >>
	dc.l	-1
	dc.l	-1
	dc.l	-1
	dc.l	-1		;AHIST_M32S (8)
	dc.l	-1
	dc.l	-1		;AHIST_S32S (10)
	dc.l	-1
	dc.l	-1
	dc.l	-1
	dc.l	-1
	dc.l	-1

_SelectAddRoutine:
	pushm	d2-a6

;	PRINTF	0,"SelectAddRoutine(%ld, %ld, %08lx)",d0, d1, d2

	move.l	a0,a4
	move.l	a1,a5
	move.l	a3,a6

	move.l	d2,d3
	move.l	ahiac_Flags(a2),d4

	and.l	#~(AHIST_BW|AHIST_INPUT),d2

;FIXIT  -- Unsigned samples are obosolete, and should be removed!
	cmp.l	#AHIST_M8U,d2
	beq	sar_unsigned

	lsl.l	#2,d2
	move.l	(typeconversion,pc,d2.l),d2
	bmi	.error

	btst.l	#AHIACB_STEREO,d4
	beq	.not_stereo
	or.l	#STEREO,d2
.not_stereo

	btst.l	#AHIACB_HIFI,d4
	beq	.not_hifi
	or.l	#HIFI,d2
.not_hifi


; Don't use tables for negative volume!

	tst.l	d0
	bmi	.no_tables
	tst.l	d1
	bmi	.no_tables

	move.l	d2,d5
	and.l	#HIFI,d5
	bne	.no_tables		;No tables in HiFi mode

	tst.l	ahiac_MultTableS(a2)
	beq	.no_tables
	or.l	#FASTMIX,d2

; Scale volume for 16 bit /w "tables"
	move.l	d2,d5
	and.l	#SIXTEENBIT,d5		;check for SIXTEENBIT or SIXTEEN2BIT
	bne	.no_tables

	bra	.check_volume
.no_tables

; Scale volume according to the master volume

	push	d2
	moveq	#0,d2
	move.w	ahiac_Channels2(a2),d2
	lsl.w	#8,d2
	move.l	ahiac_MasterVolume(a2),d5
	asr.l	#8,d5

 IFGE	__CPU-68020
	muls.l	d5,d0
	divs.l	d2,d0
	muls.l	d5,d1
	divs.l	d2,d1
 ELSE
	move.l	_UtilityBase,a0
	
	push	d1
	move.l	d5,d1
	jsr	_LVOSMult32(a0)
	move.l	d2,d1
	jsr	_LVOSDivMod32(a0)
	pop	d1

	push	d0
	move.l	d1,d0
	move.l	d5,d1
	jsr	_LVOSMult32(a0)
	move.l	d2,d1
	jsr	_LVOSDivMod32(a0)
	move.l	d0,d1
	pop	d0
 ENDC
	pop	d2

.check_volume
	tst.l	ahiac_MasterVolume(a2)
	beq	.not_volume

	tst.l	d0
	beq	.not_left_volume
	or.l	#LEFTVOLUME,d2
.not_left_volume

	tst.l	d1
	beq	.not_right_volume
	or.l	#RIGHTVOLUME,d2
.not_right_volume

.not_volume
	lsl.l	#2,d2
	move.l	(functionstable,pc,d2.l),a0
;	PRINTF	0,"Func: 0x%08lx", a0
	jsr	(a0)		;returns d0, d1 and a0

	and.l	#AHIST_BW,d3
	beq	.fw
	add.w	#OffsetBackward,a0
.fw

	add.w	(a0),a0
.exit:
	move.l	d0,(a4)
	move.l	d1,(a5)
	move.l	a0,(a6)
;	PRINTF	0, "SAR: %ld, %ld, 0x%08lx",(a4), (a5), (a6)
	popm	d2-a6
	rts
.error
	lea	.dummy(pc),a0
	bra	.exit
.dummy
	rts

;FIXIT -- OBSOLETE!

sar_unsigned:
	pea	.exit(pc)
	tst.l	d0
	bmi	FixVolSilence		;Error
	tst.l	d1
	bmi	FixVolSilence		;Error
	tst.l	ahiac_MultTableU(a2)
	beq	FixVolSilence		;Error

	btst.l	#AHIACB_STEREO,d4
	bne	.stereo
	bra	FixVolUByteMVT
.stereo
	tst.l	d0
	beq	FixVolUByteSVTr
	tst.l	d1
	beq	FixVolUByteSVTl
	bra	FixVolUByteSVPT	

.exit:
	and.l	#AHIST_BW,d3
	beq	.fw
	add.w	#OffsetBackward,a0
.fw

	add.w	(a0),a0

	move.l	d0,(a4)
	move.l	d1,(a5)
	move.l	a0,(a6)
	popm	d2-a6
	rts

functionstable:
					; Type	HiFi	Stereo	FastMix	Left	Right
	dc.l	FixVolSilence		; 8	-	-	-	-	-
	dc.l	FixVolByteMV		; 8	-	-	-	-	*
	dc.l	FixVolByteMV		; 8	-	-	-	*	-
	dc.l	FixVolByteMV		; 8	-	-	-	*	*
	dc.l	FixVolSilence		; 8	-	-	*	-	-
	dc.l	FixVolByteMVT		; 8	-	-	*	-	*
	dc.l	FixVolByteMVT		; 8	-	-	*	*	-
	dc.l	FixVolByteMVT		; 8	-	-	*	*	*
	dc.l	FixVolSilence		; 8	-	*	-	-	-
	dc.l	FixVolByteSVr		; 8	-	*	-	-	*
	dc.l	FixVolByteSVl		; 8	-	*	-	*	-
	dc.l	FixVolByteSVP		; 8	-	*	-	*	*
	dc.l	FixVolSilence		; 8	-	*	*	-	-
	dc.l	FixVolByteSVTr		; 8	-	*	*	-	*
	dc.l	FixVolByteSVTl		; 8	-	*	*	*	-
	dc.l	FixVolByteSVPT		; 8	-	*	*	*	*
	dc.l	FixVolSilence		; 8	*	-	-	-	-
	dc.l	FixVolByteMVH		; 8	*	-	-	-	*
	dc.l	FixVolByteMVH		; 8	*	-	-	*	-
	dc.l	FixVolByteMVH		; 8	*	-	-	*	*
	dc.l	FixVolSilence		; 8	*	-	*	-	-
	dc.l	FixVolByteMVH		; 8	*	-	*	-	*
	dc.l	FixVolByteMVH		; 8	*	-	*	*	-
	dc.l	FixVolByteMVH		; 8	*	-	*	*	*
	dc.l	FixVolSilence		; 8	*	*	-	-	-
	dc.l	FixVolByteSVPH		; 8	*	*	-	-	*
	dc.l	FixVolByteSVPH		; 8	*	*	-	*	-
	dc.l	FixVolByteSVPH		; 8	*	*	-	*	*
	dc.l	FixVolSilence		; 8	*	*	*	-	-
	dc.l	FixVolByteSVPH		; 8	*	*	*	-	*
	dc.l	FixVolByteSVPH		; 8	*	*	*	*	-
	dc.l	FixVolByteSVPH		; 8	*	*	*	*	*
	dc.l	FixVolSilence		; 8×2	-	-	-	-	-
	dc.l	FixVolBytesMV		; 8×2	-	-	-	-	*
	dc.l	FixVolBytesMV		; 8×2	-	-	-	*	-
	dc.l	FixVolBytesMV		; 8×2	-	-	-	*	*
	dc.l	FixVolSilence		; 8×2	-	-	*	-	-
	dc.l	FixVolBytesMVT		; 8×2	-	-	*	-	*
	dc.l	FixVolBytesMVT		; 8×2	-	-	*	*	-
	dc.l	FixVolBytesMVT		; 8×2	-	-	*	*	*
	dc.l	FixVolSilence		; 8×2	-	*	-	-	-
	dc.l	FixVolBytesSVr		; 8×2	-	*	-	-	*
	dc.l	FixVolBytesSVl		; 8×2	-	*	-	*	-
	dc.l	FixVolBytesSVP		; 8×2	-	*	-	*	*
	dc.l	FixVolSilence		; 8×2	-	*	*	-	-
	dc.l	FixVolBytesSVTr		; 8×2	-	*	*	-	*
	dc.l	FixVolBytesSVTl		; 8×2	-	*	*	*	-
	dc.l	FixVolBytesSVPT		; 8×2	-	*	*	*	*
	dc.l	FixVolSilence		; 8×2	*	-	-	-	-
	dc.l	FixVolBytesMVH		; 8×2	*	-	-	-	*
	dc.l	FixVolBytesMVH		; 8×2	*	-	-	*	-
	dc.l	FixVolBytesMVH		; 8×2	*	-	-	*	*
	dc.l	FixVolSilence		; 8×2	*	-	*	-	-
	dc.l	FixVolBytesMVH		; 8×2	*	-	*	-	*
	dc.l	FixVolBytesMVH		; 8×2	*	-	*	*	-
	dc.l	FixVolBytesMVH		; 8×2	*	-	*	*	*
	dc.l	FixVolSilence		; 8×2	*	*	-	-	-
	dc.l	FixVolBytesSVPH		; 8×2	*	*	-	-	*
	dc.l	FixVolBytesSVPH		; 8×2	*	*	-	*	-
	dc.l	FixVolBytesSVPH		; 8×2	*	*	-	*	*
	dc.l	FixVolSilence		; 8×2	*	*	*	-	-
	dc.l	FixVolBytesSVPH		; 8×2	*	*	*	-	*
	dc.l	FixVolBytesSVPH		; 8×2	*	*	*	*	-
	dc.l	FixVolBytesSVPH		; 8×2	*	*	*	*	*
	dc.l	FixVolSilence		; 16	-	-	-	-	-
	dc.l	FixVolWordMV		; 16	-	-	-	-	*
	dc.l	FixVolWordMV		; 16	-	-	-	*	-
	dc.l	FixVolWordMV		; 16	-	-	-	*	*
	dc.l	FixVolSilence		; 16	-	-	*	-	-
	dc.l	FixVolWordMVT		; 16	-	-	*	-	*
	dc.l	FixVolWordMVT		; 16	-	-	*	*	-
	dc.l	FixVolWordMVT		; 16	-	-	*	*	*
	dc.l	FixVolSilence		; 16	-	*	-	-	-
	dc.l	FixVolWordSVr		; 16	-	*	-	-	*
	dc.l	FixVolWordSVl		; 16	-	*	-	*	-
	dc.l	FixVolWordSVP		; 16	-	*	-	*	*
	dc.l	FixVolSilence		; 16	-	*	*	-	-
	dc.l	FixVolWordSVTr		; 16	-	*	*	-	*
	dc.l	FixVolWordSVTl		; 16	-	*	*	*	-
	dc.l	FixVolWordSVPT		; 16	-	*	*	*	*
	dc.l	FixVolSilence		; 16	*	-	-	-	-
	dc.l	FixVolWordMVH		; 16	*	-	-	-	*
	dc.l	FixVolWordMVH		; 16	*	-	-	*	-
	dc.l	FixVolWordMVH		; 16	*	-	-	*	*
	dc.l	FixVolSilence		; 16	*	-	*	-	-
	dc.l	FixVolWordMVH		; 16	*	-	*	-	*
	dc.l	FixVolWordMVH		; 16	*	-	*	*	-
	dc.l	FixVolWordMVH		; 16	*	-	*	*	*
	dc.l	FixVolSilence		; 16	*	*	-	-	-
	dc.l	FixVolWordSVPH		; 16	*	*	-	-	*
	dc.l	FixVolWordSVPH		; 16	*	*	-	*	-
	dc.l	FixVolWordSVPH		; 16	*	*	-	*	*
	dc.l	FixVolSilence		; 16	*	*	*	-	-
	dc.l	FixVolWordSVPH		; 16	*	*	*	-	*
	dc.l	FixVolWordSVPH		; 16	*	*	*	*	-
	dc.l	FixVolWordSVPH		; 16	*	*	*	*	*
	dc.l	FixVolSilence		; 16×2	-	-	-	-	-
	dc.l	FixVolWordsMV		; 16×2	-	-	-	-	*
	dc.l	FixVolWordsMV		; 16×2	-	-	-	*	-
	dc.l	FixVolWordsMV		; 16×2	-	-	-	*	*
	dc.l	FixVolSilence		; 16×2	-	-	*	-	-
	dc.l	FixVolWordsMVT		; 16×2	-	-	*	-	*
	dc.l	FixVolWordsMVT		; 16×2	-	-	*	*	-
	dc.l	FixVolWordsMVT		; 16×2	-	-	*	*	*
	dc.l	FixVolSilence		; 16×2	-	*	-	-	-
	dc.l	FixVolWordsSVr		; 16×2	-	*	-	-	*
	dc.l	FixVolWordsSVl		; 16×2	-	*	-	*	-
	dc.l	FixVolWordsSVP		; 16×2	-	*	-	*	*
	dc.l	FixVolSilence		; 16×2	-	*	*	-	-
	dc.l	FixVolWordsSVTr		; 16×2	-	*	*	-	*
	dc.l	FixVolWordsSVTl		; 16×2	-	*	*	*	-
	dc.l	FixVolWordsSVPT		; 16×2	-	*	*	*	*
	dc.l	FixVolSilence		; 16×2	*	-	-	-	-
	dc.l	FixVolWordsMVH		; 16×2	*	-	-	-	*
	dc.l	FixVolWordsMVH		; 16×2	*	-	-	*	-
	dc.l	FixVolWordsMVH		; 16×2	*	-	-	*	*
	dc.l	FixVolSilence		; 16×2	*	-	*	-	-
	dc.l	FixVolWordsMVH		; 16×2	*	-	*	-	*
	dc.l	FixVolWordsMVH		; 16×2	*	-	*	*	-
	dc.l	FixVolWordsMVH		; 16×2	*	-	*	*	*
	dc.l	FixVolSilence		; 16×2	*	*	-	-	-
	dc.l	FixVolWordsSVPH		; 16×2	*	*	-	-	*
	dc.l	FixVolWordsSVPH		; 16×2	*	*	-	*	-
	dc.l	FixVolWordsSVPH		; 16×2	*	*	-	*	*
	dc.l	FixVolSilence		; 16×2	*	*	*	-	-
	dc.l	FixVolWordsSVPH		; 16×2	*	*	*	-	*
	dc.l	FixVolWordsSVPH		; 16×2	*	*	*	*	-
	dc.l	FixVolWordsSVPH		; 16×2	*	*	*	*	*



*
* The mixing routine mixes ahiac_BuffSamples each pass, fitting in
* ahiac_BuffSizeNow bytes. ahiac_BuffSizeNow must be an even multiplier
* of 8.
*

;in:
* a0	Hook
* a1	Mixing buffer (size is 8 byte aligned)
* a2	AHIAudioCtrl
_Mix68k:
	pushm	d0-a6

;	PRINTF	0,"Mix!"
* Clear the buffer
	move.l	ahiac_BuffSizeNow(a2),d0
	lsr.l	#3,d0
	subq.w	#1,d0
	move.l	a1,a0
.clearbuff
	clr.l	(a0)+
	clr.l	(a0)+
	dbf	d0,.clearbuff

* Mix the samples
	clr.b	ahiac_WetOrDry(a2)
	move.l	ahiac_WetList(a2),d0
	move.l	d0,a5
	beq	.do_dry

.nextchannel
	move.l	ahiac_BuffSamples(a2),d0
	move.l	a1,a4			;output buffer

.contchannel
	tst.w	cd_EOS(a5)
	beq	.notEOS

* Call Sound Hook
	move.l	ahiac_SoundFunc(a2),d1
	beq	.noSoundFunc
	pushm	d0/a1
	move.l	d1,a0
	lea	cd_ChannelNo(a5),a1
	move.l	h_Entry(a0),a3
	jsr	(a3)			;a2 ready
	popm	d0/a1
.noSoundFunc
	clr.w	cd_EOS(a5)		;clear EOS flag
.notEOS

	movem.l	(a5),d1/d3/d4/d5/d6/a3	;Flags,Offset,Add,DataStart
	cmp.w	#TRUE<<8 | TRUE,d1	;FreqOK and SoundOK must both be TRUE
	bne	.channel_done		;No sound or freq not set yet.

	cmp.l	cd_Samples(a5),d0
	blo	.wont_reach_end

* How may samples left?
	sub.l	cd_Samples(a5),d0

	push	d0
	move.l	cd_Samples(a5),d0

	movem.l	cd_ScaleLeft(a5),d1/d2/a0
	jsr	(a0)
	move.l	cd_TempLastSampleL(a5),cd_LastSampleL(a5) ;linear interpol. stuff
	move.l	cd_TempLastSampleR(a5),cd_LastSampleR(a5) ;linear interpol. stuff

*** Give AHIST_INPUT special treatment!

	move.l	cd_Type(a5),d1
	and.l	#AHIST_INPUT,d1
	beq	.notinput2

	move.l	cd_NextFlags(a5),cd_Flags(a5)
	movem.l	cd_NextAdd(a5),d0-d7/a0/a3/a6	; Add,DataStart,LastOffset,ScaleLeft,ScaleRight,AddRoutine, VolumeLeft,VolumeRight,Type
	movem.l	d0-d7/a0/a3/a6,cd_Add(a5)

	move.l	ahiac_InputLength(a2),cd_Samples(a5)
	clr.l	cd_Offset+F64_I(a5)
	clr.l	cd_Offset+F64_F(a5)
	clr.l	cd_FirstOffsetI(a5)
	move.l	ahiac_InputBuffer1(a2),cd_DataStart(a5)

	pop	d0
	bra.w	.contchannel		;same channel, new sound

.notinput2

; d3:d4 always points OUTSIDE the sample after this call. Ie, if we read a
; sample at offset d3 now, it does not belong to the sample just played.
; This is true for both backward and forward mixing.
; Oh, btw... OffsetF is unsigned, so -0.5 is expressed as -1:$80000000.

; What we do now is to calculate how much futher we have advanced.
	sub.l	cd_LastOffset+F64_F(a5),d4
	move.l	cd_LastOffset+F64_I(a5),d0
	subx.l	d0,d3

; d3:d4 should now be added to the NEXT OffsetI (and OffsetF which is 0 when
; every sample begin).
; d3:d4 is positive of the sample was mixed forwards, and negative if the sample
; was mixed backwards. There is one catch, however. If the direction is about
; to change now, d3:d4 should instead be SUBTRACTED. Let's check:
	move.l	cd_Type(a5),d0
	move.l	cd_NextType(a5),d1
	eor.l	d0,d1
	and.l	#AHIST_BW,d1			;filter the BW bit
	beq.b	.same_type
	neg.l	d3
	neg.l	d4
.same_type

; Ok, now we add.
	add.l	cd_NextOffset+F64_F(a5),d4
	move.l	cd_NextOffset+F64_I(a5),d0
	addx.l	d0,d3
	move.l	d3,cd_Offset+F64_I(a5)
	move.l	d4,cd_Offset+F64_F(a5)
	move.l	d3,cd_FirstOffsetI(a5)

; But what if the next sample is so short that we just passed it!?
; Here is the nice part. CalcSamples checks this,
; and sets cd_Samples to 0 in that case. And the add routines doesn't
; do anything when asked to mix 0 samples.
; Assume we have passed a sample with 4 samples, and the next one
; is only 3. CalcSamples returns 0. The 'jsr (a0)' call above does
; not do anything at all, OffsetI is still 4. Now we subtract LastOffsetI,
; which is 3. Result: We have passed the sample with 1. And guess what?
; That's in range.

; Now, let's copy the rest of the cd_Next#? stuff...
	move.l	cd_NextFlags(a5),cd_Flags(a5)
	movem.l	cd_NextAdd(a5),d0-d7/a0/a3/a6	; Add,DataStart,LastOffset,ScaleLeft,ScaleRight,AddRoutine, VolumeLeft,VolumeRight,Type
	movem.l	d0-d7/a0/a3/a6,cd_Add(a5)

					;Add,LastOffset ok
	move.l	a6,d2			;Type
	movem.l	cd_Offset(a5),d5/d6	;cd_Offset+F64_I/cd_Offset+F64_F
	bsr	_CalcSamples
	move.l	d0,cd_Samples(a5)

	pop	d0
	move.w	#TRUE,cd_EOS(a5)	;signal End-Of-Sample
	bra.w	.contchannel		;same channel, new sound

.wont_reach_end
	sub.l	d0,cd_Samples(a5)
	movem.l	cd_ScaleLeft(a5),d1/d2/a0
	jsr	(a0)
	movem.l	d3/d4,cd_Offset(a5)	;update Offset

.channel_done
	move.l	cd_Succ(a5),d0		;next channel in list
	move.l	d0,a5
	bne	.nextchannel
	tst.b	ahiac_WetOrDry(a2)
	bne.b	.exit			;Both wet and dry finished
	addq.b	#1,ahiac_WetOrDry(a2)	;Mark dry

	pushm	a0-a6
*** AHIET_DSPECHO
	move.l	ahiac_EffDSPEchoStruct(a2),d0
	beq	.noEffDSPEcho
	move.l	d0,a0
	move.l	ahiecho_Code(a0),a3
	jsr	(a3)
.noEffDSPEcho
	popm	a0-a6

.do_dry
	move.l	ahiac_DryList(a2),d0
	move.l	d0,a5
	beq	.exit
	btst.b	#AHIACB_POSTPROC-24,ahiac_Flags(a2)
	beq	.nextchannel

*** AHIET_MASTERVOLUME
	bsr	DoMasterVolume

; BUG!! FIXIT! If there are no wet channels, and postprocessing is on,
; a4 will be uninitialized!!
	move.l	a4,a1			;New block
	bra	.nextchannel

.exit

*** AHIET_MASTERVOLUME
	bsr	DoMasterVolume

*** AHIET_OUTPUTBUFFER
	move.l	ahiac_EffOutputBufferStruct(a2),d0
	beq	.noEffOutputBuffer
	move.l	a1,a4
	move.l	d0,a1
	move.l	ahieob_Func(a1),a0
	move.l	a4,ahieob_Buffer(a1)
	move.l	ahiac_BuffSamples(a2),ahieob_Length(a1)
	move.l	ahiac_BuffType(a2),ahieob_Type(a1)
	move.l	h_Entry(a0),a3
	jsr	(a3)
.noEffOutputBuffer

*** AHIET_CHANNELINFO
	move.l	ahiac_EffChannelInfoStruct(a2),d0
	beq	.noEffChannelInfo
	move.l	d0,a1
	move.l	ahieci_Func(a1),a0
	move.w	ahieci_Channels(a1),d0
	subq.w	#1,d0
	lea	ahieci_Offset(a1),a3
	move.l	ahiac_ChannelDatas(a2),a4
.ci_loop
	move.l	cd_Offset+F64_I(a4),(a3)+
	add.w	#AHIChannelData_SIZEOF,a4
	dbf	d0,.ci_loop
	move.l	h_Entry(a0),a3
	jsr	(a3)
.noEffChannelInfo


	popm	d0-a6
	rts


;in:
* a1	Buffer
* a2	Audioctrl
DoMasterVolume
	pushm	d0-d2/a0-a1

	btst.b	#AHIACB_CLIPPING-24,ahiac_Flags(a2)
	beq	.noclipping

	move.l	ahiac_BuffSamples(a2),d0
	btst.b	#AHIACB_STEREO,ahiac_Flags+3(a2)
	beq	.notstereo
	lsl.l	#1,d0
.notstereo

	btst.b	#AHIACB_HIFI,ahiac_Flags+3(a2)
	bne	.32bit
	move.l	ahiac_MasterVolumeTable(a2),d1
	move.l	d1,a0
	beq	.16bit

.16bittable
	moveq	#0,d1
.16bittable_loop
 IFGE	__CPU-68020
	move.w	(a1),d1
	move.w	(a0,d1.l*2),(a1)+
 ELSE
	moveq	#0,d1
	move.w	(a1),d1
	add.w	d1,d1
	move.w	(a0,d1.l),(a1)+
 ENDC
	subq.l	#1,d0
	bne	.16bittable_loop
	bra	.exit

.16bit
	move.l	ahiac_SetMasterVolume(a2),d1
	lsr.l	#8,d1
.16bit_loop
	move.w	(a1),d2
	muls.w	d1,d2
	asr.l	#8,d2
	cmp.l	#32767,d2
	ble	.16bit_noposclip
	move.w	#32767,d2
.16bit_noposclip
	cmp.l	#-32768,d2
	bge	.16bit_nonegclip
	move.w	#-32768,d2
.16bit_nonegclip
	move.w	d2,(a1)+
	subq.l	#1,d0
	bne	.16bit_loop
	bra	.exit

.32bit
 IFGE	__CPU-68020
	move.l	ahiac_SetMasterVolume(a2),d1
	lsr.l	#8,d1
.32bit_loop
	move.l	(a1),d2
	asr.l	#8,d2
	muls.l	d1,d2
	bvc	.32bit_store
	bpl	.32bit_negclip
	move.l	#$7fffffff,d2		; MAXINT
	bra	.32bit_store
.32bit_negclip
	move.l	#$80000000,d2		; MININT
.32bit_store
	move.l	d2,(a1)+
	subq.l	#1,d0
	bne	.32bit_loop
	bra	.exit
 ENDIF
.exit

.noclipping
	popm	d0-d2/a0-a1
	rts

;in:
* d0	AddI
* d1	AddF
* d2	Type
* d3	LastOffsetI
* d4	LastOffsetF
* d5	OffsetI
* d6	OffsetF
;ut:
* d0	Samples
_CalcSamples:
* Calc how many loops the addroutines should run (Times=Length/Rate)

; Quick fix when changed fraction to 32 bits...
	swap.w	d1
	swap.w	d4
	swap.w	d6

;	and.l	#AHIST_BW|AHIST_INPUT,d2
;	beq	.forwards
	and.l	#AHIST_BW,d2
	bne	.backwards
;*** AHIST_INPUT here
;	sub.l	d5,d3
;	move.l	d3,d0
;	rts
;.forwards
	sub.w	d6,d4
	subx.l	d5,d3
	bra	.1
.backwards
	sub.w	d4,d6
	subx.l	d3,d5
	move.w	d6,d4
	move.l	d5,d3
.1
	bmi	.error
  IFLT	__CPU-68060
	swap.w	d4
	move.w	d3,d4
	swap.w	d4
	clr.w	d3
	swap.w	d3
; d3:d4 is now (positive) length <<16

	swap	d0
	move.w	d1,d0
	tst.l	d0
	beq	.error
; d0 is now rate<<16

  IFGE	__CPU-68020
	divu.l	d0,d3:d4
	move.l	d4,d0
  ELSE
	move.l	d3,d1
	move.l	d4,d2
	jsr	_UDivMod64		;d0 = (d1:d2)/d0
  ENDC * 68020
	addq.l	#1,d0

 ELSE

*
* BAHH! It doesn't work!
*
;	fmove.l	d3,fp0
;	fmove.l	#65536,fp2
;	fmul	fp2,fp0
;	fmove.l	d0,fp1
;	fadd.w	d4,fp0
;	fmul	fp2,fp1
;	fadd.w	d1,fp1
;	fbeq	.error
;	fdiv	fp1,fp0
;	fmove.l	fp0,d0

	swap.w	d4
	move.w	d3,d4
	swap.w	d4
	clr.w	d3
	swap.w	d3
	swap	d0
	move.w	d1,d0
	tst.l	d0
	beq	.error
	move.l	d3,d1
	move.l	d4,d2
	jsr	_UDivMod64		;d0 = (d1:d2)/d0
	addq.l	#1,d0
 ENDC * 68060

	rts
.error
	moveq	#0,d0
	rts


* ALL FIXVOL RUTINES:
;in:
* d0	VolumeLeft (Fixed)
* d1	VolumeRight (Fixed)
* d3	Sample type (or:ed with AHIST_BW if backward play)
* a2	AudioCtrl
;out:
* d0	ScaleLeft
* d1	ScaleRight (if needed)
* a0	Pointer to correct Offs#? label


;******************************************************************************

;FIXIT -- OBSOLETE!

FixVolUByteMVT:
	add.l	d1,d0
	move.l	ahiac_MultTableU(a2),a0
	lsr.l	#TABLESHIFT-(8+2),d0
	and.l	#(TABLEMAXVOL*2-1)<<(8+2),d0
	add.l	a0,d0
	lea	OffsByteMVT(pc),a0		;Reuse!
	rts

FixVolUByteSVTl:
	move.l	ahiac_MultTableU(a2),a0
	lsr.l	#TABLESHIFT-(8+2),d0
	and.l	#(TABLEMAXVOL*2-1)<<(8+2),d0
	add.l	a0,d0
	lea	OffsByteSVTl(pc),a0		;Reuse!
	rts

FixVolUByteSVTr:
	move.l	ahiac_MultTableU(a2),a0
	lsr.l	#TABLESHIFT-(8+2),d1
	and.l	#(TABLEMAXVOL*2-1)<<(8+2),d1
	add.l	a0,d1
	lea	OffsByteSVTr(pc),a0		;Reuse!
	rts

FixVolUByteSVPT:
	move.l	ahiac_MultTableU(a2),a0
	lsr.l	#TABLESHIFT-(8+2),d0
	and.l	#(TABLEMAXVOL*2-1)<<(8+2),d0
	add.l	a0,d0
	lsr.l	#TABLESHIFT-(8+2),d1
	and.l	#(TABLEMAXVOL*2-1)<<(8+2),d1
	add.l	a0,d1
	lea	OffsByteSVPT(pc),a0		;Reuse!
	rts


;******************************************************************************

FixVolSilence:
	lea	OffsSilence(pc),a0
	rts

;******************************************************************************

FixVolByteMV:
	add.l	d1,d0
	asr.l	#8,d0
	lea	OffsByteMV(pc),a0
	rts

FixVolByteMVT:
	add.l	d1,d0
	move.l	ahiac_MultTableS(a2),a0
	lsr.l	#TABLESHIFT-(8+2),d0
	and.l	#(TABLEMAXVOL*2-1)<<(8+2),d0
	add.l	a0,d0
	lea	OffsByteMVT(pc),a0
	rts

FixVolByteSVl:
	asr.l	#8,d0
	lea	OffsByteSVl(pc),a0
	rts

FixVolByteSVr:
	asr.l	#8,d1
	lea	OffsByteSVr(pc),a0
	rts

FixVolByteSVP:
	asr.l	#8,d0
	asr.l	#8,d1
	lea	OffsByteSVP(pc),a0
	rts

FixVolByteSVTl:
	move.l	ahiac_MultTableS(a2),a0
	lsr.l	#TABLESHIFT-(8+2),d0
	and.l	#(TABLEMAXVOL*2-1)<<(8+2),d0
	add.l	a0,d0
	lea	OffsByteSVTl(pc),a0
	rts

FixVolByteSVTr:
	move.l	ahiac_MultTableS(a2),a0
	lsr.l	#TABLESHIFT-(8+2),d1
	and.l	#(TABLEMAXVOL*2-1)<<(8+2),d1
	add.l	a0,d1
	lea	OffsByteSVTr(pc),a0
	rts

FixVolByteSVPT:
	move.l	ahiac_MultTableS(a2),a0
	lsr.l	#TABLESHIFT-(8+2),d0
	and.l	#(TABLEMAXVOL*2-1)<<(8+2),d0
	add.l	a0,d0
	lsr.l	#TABLESHIFT-(8+2),d1
	and.l	#(TABLEMAXVOL*2-1)<<(8+2),d1
	add.l	a0,d1
	lea	OffsByteSVPT(pc),a0
	rts

FixVolByteMVH:
	add.l	d1,d0
	lea	OffsByteMVH(pc),a0
	rts

FixVolByteSVPH:
	lea	OffsByteSVPH(pc),a0
	rts

;******************************************************************************

FixVolBytesMV:
	asr.l	#8,d0
	asr.l	#8,d1
	lea	OffsBytesMV(pc),a0
	rts

FixVolBytesMVT:
	move.l	ahiac_MultTableS(a2),a0
	lsr.l	#TABLESHIFT-(8+2),d0
	and.l	#(TABLEMAXVOL*2-1)<<(8+2),d0
	add.l	a0,d0
	lsr.l	#TABLESHIFT-(8+2),d1
	and.l	#(TABLEMAXVOL*2-1)<<(8+2),d1
	add.l	a0,d1
	lea	OffsBytesMVT(pc),a0
	rts

FixVolBytesSVl:
	asr.l	#8,d0
	lea	OffsBytesSVl(pc),a0
	rts

FixVolBytesSVr:
	asr.l	#8,d1
	lea	OffsBytesSVr(pc),a0
	rts

FixVolBytesSVP:
	asr.l	#8,d0
	asr.l	#8,d1
	lea	OffsBytesSVP(pc),a0
	rts

FixVolBytesSVTl:
	move.l	ahiac_MultTableS(a2),a0
	lsr.l	#TABLESHIFT-(8+2),d0
	and.l	#(TABLEMAXVOL*2-1)<<(8+2),d0
	add.l	a0,d0
	lea	OffsBytesSVTl(pc),a0
	rts

FixVolBytesSVTr:
	move.l	ahiac_MultTableS(a2),a0
	lsr.l	#TABLESHIFT-(8+2),d1
	and.l	#(TABLEMAXVOL*2-1)<<(8+2),d1
	add.l	a0,d1
	lea	OffsBytesSVTr(pc),a0
	rts

FixVolBytesSVPT:
	move.l	ahiac_MultTableS(a2),a0
	lsr.l	#TABLESHIFT-(8+2),d0
	and.l	#(TABLEMAXVOL*2-1)<<(8+2),d0
	add.l	a0,d0
	lsr.l	#TABLESHIFT-(8+2),d1
	and.l	#(TABLEMAXVOL*2-1)<<(8+2),d1
	add.l	a0,d1
	lea	OffsBytesSVPT(pc),a0
	rts

FixVolBytesMVH:
	lea	OffsBytesMVH(pc),a0
	rts

FixVolBytesSVPH:
	lea	OffsBytesSVPH(pc),a0
	rts

;******************************************************************************

FixVolWordMV:
	add.l	d1,d0
 IFGE	__CPU-68020
	cmp.l	#$10000,d0		; Fix: 32768 doesn't fit a WORD!
	bne	.1
	move.l	#$ffff,d0
.1
	asr.l	#1,d0
 ELSE
	asr.l	#8,d0
 ENDC
	lea	OffsWordMV(pc),a0
	rts

FixVolWordMVT:
	add.l	d1,d0
	jsr	_Fixed2Shift
	lea	OffsWordMVT(pc),a0
	rts

FixVolWordSVl:
 IFGE	__CPU-68020
	cmp.l	#$10000,d0		; Fix: 32768 doesn't fit a WORD!
	bne	.1
	move.l	#$ffff,d0
.1
	asr.l	#1,d0
 ELSE
	asr.l	#8,d0
 ENDC
	lea	OffsWordSVl(pc),a0
	rts

FixVolWordSVr:
 IFGE	__CPU-68020
	cmp.l	#$10000,d1		; Fix: 32768 doesn't fit a WORD!
	bne	.1
	move.l	#$ffff,d1
.1
	asr.l	#1,d1
 ELSE
	asr.l	#8,d1
 ENDC
	lea	OffsWordSVr(pc),a0
	rts

FixVolWordSVP:
 IFGE	__CPU-68020
	cmp.l	#$10000,d0		; Fix: 32768 doesn't fit a WORD!
	bne	.1
	move.l	#$ffff,d0
.1
	cmp.l	#$10000,d1		; Fix: 32768 doesn't fit a WORD!
	bne	.2
	move.l	#$ffff,d1
.2
 	asr.l	#1,d0
 	asr.l	#1,d1
 ELSE
	asr.l	#8,d0
	asr.l	#8,d1
 ENDC
	lea	OffsWordSVP(pc),a0
	rts

FixVolWordSVTl:
	jsr	_Fixed2Shift
	lea	OffsWordSVTl(pc),a0
	rts

FixVolWordSVTr:
	move.l	d1,d0
	jsr	_Fixed2Shift
	move.l	d0,d1
	lea	OffsWordSVTr(pc),a0
	rts

FixVolWordSVPT:
	push	d1
	jsr	_Fixed2Shift
	pop	d1
	push	d0
	move.l	d1,d0
	jsr	_Fixed2Shift
	move.l	d0,d1
	pop	d0
	lea	OffsWordSVPT(pc),a0
	rts

FixVolWordMVH:
	add.l	d1,d0
	lea	OffsWordMVH(pc),a0
	rts

FixVolWordSVPH:
	lea	OffsWordSVPH(pc),a0
	rts

;******************************************************************************

FixVolWordsMV:
	cmp.l	#$10000,d0		; Fix: 32768 doesn't fit a WORD!
	bne	.1
	move.l	#$ffff,d0
.1
	cmp.l	#$10000,d1		; Fix: 32768 doesn't fit a WORD!
	bne	.2
	move.l	#$ffff,d1
.2
	asr.l	#1,d0
	asr.l	#1,d1
	lea	OffsWordsMV(pc),a0
	rts

FixVolWordsMVT:
	push	d1
	jsr	_Fixed2Shift
	pop	d1
	push	d0
	move.l	d1,d0
	jsr	_Fixed2Shift
	move.l	d0,d1
	pop	d0
	lea	OffsWordsMVT(pc),a0
	rts

FixVolWordsSVl:
	cmp.l	#$10000,d0		; Fix: 32768 doesn't fit a WORD!
	bne	.1
	move.l	#$ffff,d0
.1
	asr.l	#1,d0
	lea	OffsWordsSVl(pc),a0
	rts

FixVolWordsSVr:
	cmp.l	#$10000,d1		; Fix: 32768 doesn't fit a WORD!
	bne	.1
	move.l	#$ffff,d1
.1
	asr.l	#1,d1
	lea	OffsWordsSVr(pc),a0
	rts

FixVolWordsSVP:
	cmp.l	#$10000,d0		; Fix: 32768 doesn't fit a WORD!
	bne	.1
	move.l	#$ffff,d0
.1
	cmp.l	#$10000,d1		; Fix: 32768 doesn't fit a WORD!
	bne	.2
	move.l	#$ffff,d1
.2
 	asr.l	#1,d0
 	asr.l	#1,d1
	lea	OffsWordsSVP(pc),a0
	rts

FixVolWordsSVTl:
	jsr	_Fixed2Shift
	lea	OffsWordsSVTl(pc),a0
	rts

FixVolWordsSVTr:
	move.l	d1,d0
	jsr	_Fixed2Shift
	move.l	d0,d1
	lea	OffsWordsSVTr(pc),a0
	rts

FixVolWordsSVPT:
	push	d1
	jsr	_Fixed2Shift
	pop	d1
	push	d0
	move.l	d1,d0
	jsr	_Fixed2Shift
	move.l	d0,d1
	pop	d0
	lea	OffsWordsSVPT(pc),a0
	rts

FixVolWordsMVH:
	lea	OffsWordsMVH(pc),a0
	rts

FixVolWordsSVPH:
	lea	OffsWordsSVPH(pc),a0
	rts

;******************************************************************************



;------------------------------------------------------------------------------
* Overview: - M=Mono S=Stereo V=Volume P=Panning T=Table H=HiFi
*             l=Left r=Right
*
* Routine	Speed¹	Memory	Volume	Panning
*
* AddSilence	0	None	=0	-
*
* AddByteMV	403	None	±8 bit	-
* AddByteMVH		None	±15 bit	-
* AddByteMVT	250	²	5 bit ²	-
* AddByteSVl	455	None	±8 bit	No (Left)
* AddByteSVr	474	None	±8 bit	No (Right)
* AddByteSVP	704	None	±8 bit	Yes
* AddByteSVPH		None	±15 bit	Yes
* AddByteSVTl	327	²	5 bit ²	No (Left)
* AddByteSVTr	324	²	5 bit ²	No (Right)
* AddByteSVPT	384	²	5 bit ²	Yes
*
* AddWordMV	438	None	±15 bit	-
* AddWordMVH		None	±16 bit	Yes
* AddWordMVT	374	²	5 bit ²	-
* AddWordSVl	523	None	±15 bit	No (Left)
* AddWordSVr	523	None	±15 bit	No (Right)
* AddWordSVP	837	None	±15 bit	Yes
* AddWordSVPH		None	±16 bit	Yes
* AddWordSVTl	452	²	5 bit ²	No (Left)
* AddWordSVTr	452	²	5 bit ²	No (Right)
* AddWordSVPT	685	²	5 bit ²	Yes
*
*
* ¹) Number of CIAA timer B ticks (PAL) it takes to process
*    10000 samples, divided by 16. Lower is faster. NOTE: The samples
*    are all the same, which makes the table routines look a little
*    better than they would in real life, where the cache cannot be
*    fully used.
*    System data:
*       A4000/040 25 Mhz, all caches on.
*       Code, samples and mixing buffer in Fast RAM.
*       All interrupts off - no multitasking.
* ²) Multiplication table, using 256*(TABLEMAXVOL+1)*4 bytes 
*    (includes 0 and TABLEMAXVOL itself!).
* ³) Multiplication table, using 256*(TABLEMAXVOL+1)*2 bytes 
*    (includes 0 and TABLEMAXVOL itself!).
*
;------------------------------------------------------------------------------
* ALL ADD ROUTINES:
;in:
* d0.l	Samples (scratch)
* d3	Offset Integer
* d4	Offset Fraction (upper word cleared!)
* d5	Add Integer (s)
* d6	Add Fraction (s) (upper word cleared!)
* d7	Scratch
* a0	Scratch
* a2	AHIAudioCtrl (only used by AddSilence routines)
* a3	Sample data (s)
* a4	Output buffer
* a5	ChannelDatas (only used by HiFi routines)
* a6	Scratch
;out:
* d3	Updated Offset Integer
* d4	Updated Offset Fraction
* a4	Updated buffer pointer (where to store next sample)
;------------------------------------------------------------------------------

OffsetTable:
OffsSilence:	dc.w	AddSilence-*

OffsByteMV:	dc.w	AddByteMV-*
OffsByteMVT:	dc.w	AddByteMVT-*
OffsByteSVl:	dc.w	AddByteSVl-*
OffsByteSVr:	dc.w	AddByteSVr-*
OffsByteSVP:	dc.w	AddByteSVP-*
OffsByteSVTl:	dc.w	AddByteSVTl-*
OffsByteSVTr:	dc.w	AddByteSVTr-*
OffsByteSVPT:	dc.w	AddByteSVPT-*
OffsByteMVH:	dc.w	AddByteMVH-*
OffsByteSVPH:	dc.w	AddByteSVPH-*

OffsBytesMV:	dc.w	AddBytesMV-*
OffsBytesMVT:	dc.w	AddBytesMVT-*
OffsBytesSVl:	dc.w	AddBytesSVl-*
OffsBytesSVr:	dc.w	AddBytesSVr-*
OffsBytesSVP:	dc.w	AddBytesSVP-*
OffsBytesSVTl:	dc.w	AddBytesSVTl-*
OffsBytesSVTr:	dc.w	AddBytesSVTr-*
OffsBytesSVPT:	dc.w	AddBytesSVPT-*
OffsBytesMVH:	dc.w	AddBytesMVH-*
OffsBytesSVPH:	dc.w	AddBytesSVPH-*

OffsWordMV:	dc.w	AddWordMV-*
OffsWordMVT:	dc.w	AddWordMVT-*
OffsWordSVl:	dc.w	AddWordSVl-*
OffsWordSVr:	dc.w	AddWordSVr-*
OffsWordSVP:	dc.w	AddWordSVP-*
OffsWordSVTl:	dc.w	AddWordSVTl-*
OffsWordSVTr:	dc.w	AddWordSVTr-*
OffsWordSVPT:	dc.w	AddWordSVPT-*
OffsWordMVH:	dc.w	AddWordMVH-*
OffsWordSVPH:	dc.w	AddWordSVPH-*

OffsWordsMV:	dc.w	AddWordsMV-*
OffsWordsMVT:	dc.w	AddWordsMVT-*
OffsWordsSVl:	dc.w	AddWordsSVl-*
OffsWordsSVr:	dc.w	AddWordsSVr-*
OffsWordsSVP:	dc.w	AddWordsSVP-*
OffsWordsSVTl:	dc.w	AddWordsSVTl-*
OffsWordsSVTr:	dc.w	AddWordsSVTr-*
OffsWordsSVPT:	dc.w	AddWordsSVPT-*
OffsWordsMVH:	dc.w	AddWordsMVH-*
OffsWordsSVPH:	dc.w	AddWordsSVPH-*

OffsetBackward	EQU	*-OffsetTable

OffsSilenceB:	dc.w	AddSilenceB-*

OffsByteMVB:	dc.w	AddByteMVB-*
OffsByteMVTB:	dc.w	AddByteMVTB-*
OffsByteSVlB:	dc.w	AddByteSVlB-*
OffsByteSVrB:	dc.w	AddByteSVrB-*
OffsByteSVPB:	dc.w	AddByteSVPB-*
OffsByteSVTlB:	dc.w	AddByteSVTlB-*
OffsByteSVTrB:	dc.w	AddByteSVTrB-*
OffsByteSVPTB:	dc.w	AddByteSVPTB-*
OffsByteMVHB:	dc.w	AddByteMVHB-*
OffsByteSVPHB:	dc.w	AddByteSVPHB-*

OffsBytesMVB:	dc.w	AddBytesMVB-*
OffsBytesMVTB:	dc.w	AddBytesMVTB-*
OffsBytesSVlB:	dc.w	AddBytesSVlB-*
OffsBytesSVrB:	dc.w	AddBytesSVrB-*
OffsBytesSVPB:	dc.w	AddBytesSVPB-*
OffsBytesSVTlB:	dc.w	AddBytesSVTlB-*
OffsBytesSVTrB:	dc.w	AddBytesSVTrB-*
OffsBytesSVPTB:	dc.w	AddBytesSVPTB-*
OffsBytesMVHB:	dc.w	AddBytesMVHB-*
OffsBytesSVPHB:	dc.w	AddBytesSVPHB-*

OffsWordMVB:	dc.w	AddWordMVB-*
OffsWordMVTB:	dc.w	AddWordMVTB-*
OffsWordSVlB:	dc.w	AddWordSVlB-*
OffsWordSVrB:	dc.w	AddWordSVrB-*
OffsWordSVPB:	dc.w	AddWordSVPB-*
OffsWordSVTlB:	dc.w	AddWordSVTlB-*
OffsWordSVTrB:	dc.w	AddWordSVTrB-*
OffsWordSVPTB:	dc.w	AddWordSVPTB-*
OffsWordMVHB:	dc.w	AddWordMVHB-*
OffsWordSVPHB:	dc.w	AddWordSVPHB-*

OffsWordsMVB:	dc.w	AddWordsMVB-*
OffsWordsMVTB:	dc.w	AddWordsMVTB-*
OffsWordsSVlB:	dc.w	AddWordsSVlB-*
OffsWordsSVrB:	dc.w	AddWordsSVrB-*
OffsWordsSVPB:	dc.w	AddWordsSVPB-*
OffsWordsSVTlB:	dc.w	AddWordsSVTlB-*
OffsWordsSVTrB:	dc.w	AddWordsSVTrB-*
OffsWordsSVPTB:	dc.w	AddWordsSVPTB-*
OffsWordsMVHB:	dc.w	AddWordsMVHB-*
OffsWordsSVPHB:	dc.w	AddWordsSVPHB-*
		dc.w	-1



	cnop	0,16
	ds.b	16

; To make the backward-mixing routines, do this:
; 1) Copy all mixing routines.
; 2) Replace all occurences of ':' with 'B:' (all labels)
; 3) Replace all occurences of 'add.l	d6,d4' with 'sub.l	d6,d4'
; 4) Replace all occurences of 'addx.l	d5,d3' with 'subx.l	d5,d3'
; 5) AddSilence uses different source registers for add/addx.
; 6) The HiFi routines are different.


*******************************************************************************
; HIFI routines (unoptimized, only 020+!)
;------------------------------------------------------------------------------
	cnop	0,16

;in
* d1.l	-65536..65536
AddByteMVH:
 IFGE	__CPU-68020
	clr.w	d4		; 16 bit fraction
	clr.w	d6
	swap.w	d4
	swap.w	d6
	subq.w	#1,d0
	bmi.b	.exit
.nextsample
	cmp.l	cd_FirstOffsetI(a5),d3	;Is this the first sample?
	bne	.not_first
	move.l	cd_LastSampleL(a5),a0
	bra	.got_sample
.not_first
	move.b	-1(a3,d3.l),d7
	lsl.w	#8,d7
;	ext.l	d7
;	move.l	d7,a0
	move.w	d7,a0
.got_sample
	move.b	0(a3,d3.l),d7
	lsl.w	#8,d7
	ext.l	d7
	move.l	d7,cd_TempLastSampleL(a5)
	sub.l	a0,d7
	asr.l	#1,d7
	muls.l	d4,d7
	asr.l	#7,d7
	asr.l	#8,d7
	add.l	d7,a0

	move.l	a0,d7
	muls.l	d1,d7
	add.l	d7,(a4)+

	add.w	d6,d4
	addx.l	d5,d3
	dbf	d0,.nextsample
.exit
	swap.w	d4		; 32 bit fraction
	swap.w	d6
 ENDC
	rts

;in
* d1.l	-65536..65536
* d2.l	-65536..65536
AddByteSVPH:
 IFGE	__CPU-68020
	clr.w	d4		; 16 bit fraction
	clr.w	d6
	swap.w	d4
	swap.w	d6
	subq.w	#1,d0
	bmi.b	.exit
.nextsample
	cmp.l	cd_FirstOffsetI(a5),d3	;Is this the first sample?
	bne	.not_first
	move.l	cd_LastSampleL(a5),a0
	bra	.got_sample
.not_first
	move.b	-1(a3,d3.l),d7
	lsl.w	#8,d7
;	ext.l	d7
;	move.l	d7,a0
	move.w	d7,a0
.got_sample
	move.b	0(a3,d3.l),d7
	lsl.w	#8,d7
	ext.l	d7
	move.l	d7,cd_TempLastSampleL(a5)
	sub.l	a0,d7
	asr.l	#1,d7
	muls.l	d4,d7
	asr.l	#7,d7
	asr.l	#8,d7
	add.l	d7,a0

	move.l	a0,d7
	muls.l	d1,d7
	add.l	d7,(a4)+

	move.l	a0,d7
	muls.l	d2,d7
	add.l	d7,(a4)+

	add.w	d6,d4
	addx.l	d5,d3
	dbf	d0,.nextsample
.exit
	swap.w	d4		; 32 bit fraction
	swap.w	d6
 ENDC
	rts

;in
* d1.l	-65536..65536
* d2.l	-65536..65536
AddBytesMVH:
 IFGE	__CPU-68020
	clr.w	d4		; 16 bit fraction
	clr.w	d6
	swap.w	d4
	swap.w	d6
	subq.w	#1,d0
	bmi.b	.exit
.nextsample
	cmp.l	cd_FirstOffsetI(a5),d3	;Is this the first sample?
	bne	.not_firstL
	move.l	cd_LastSampleL(a5),a0
	bra	.got_sampleL
.not_firstL
	move.b	-2(a3,d3.l*2),d7
	lsl.w	#8,d7
;	ext.l	d7
;	move.l	d7,a0
	move.w	d7,a0
.got_sampleL
	move.b	0(a3,d3.l*2),d7
	lsl.w	#8,d7
	ext.l	d7
	move.l	d7,cd_TempLastSampleL(a5)
	sub.l	a0,d7
	asr.l	#1,d7
	muls.l	d4,d7
	asr.l	#7,d7
	asr.l	#8,d7
	add.l	d7,a0

	move.l	a0,d7
	muls.l	d1,d7
	add.l	d7,(a4)

	cmp.l	cd_FirstOffsetI(a5),d3	;Is this the first sample?
	bne	.not_firstR
	move.l	cd_LastSampleR(a5),a0
	bra	.got_sampleR
.not_firstR
	move.b	-1(a3,d3.l*2),d7
	lsl.w	#8,d7
;	ext.l	d7
;	move.l	d7,a0
	move.w	d7,a0
.got_sampleR
	move.b	1(a3,d3.l*2),d7
	lsl.w	#8,d7
	ext.l	d7
	move.l	d7,cd_TempLastSampleR(a5)
	sub.l	a0,d7
	asr.l	#1,d7
	muls.l	d4,d7
	asr.l	#7,d7
	asr.l	#8,d7
	add.l	d7,a0

	move.l	a0,d7
	muls.l	d2,d7
	add.l	d7,(a4)+

	add.w	d6,d4
	addx.l	d5,d3
	dbf	d0,.nextsample
.exit
	swap.w	d4		; 32 bit fraction
	swap.w	d6
 ENDC
	rts

;in
* d1.l	-65536..65536
* d2.l	-65536..65536
AddBytesSVPH:
 IFGE	__CPU-68020
	clr.w	d4		; 16 bit fraction
	swap.w	d4
	clr.w	d6
	swap.w	d6
	subq.w	#1,d0
	bmi.b	.exit
.nextsample
	cmp.l	cd_FirstOffsetI(a5),d3	;Is this the first sample?
	bne	.not_firstL
	move.l	cd_LastSampleL(a5),a0
	bra	.got_sampleL
.not_firstL
	move.b	-2(a3,d3.l*2),d7
	lsl.w	#8,d7
;	ext.l	d7
;	move.l	d7,a0
	move.w	d7,a0
.got_sampleL
	move.b	0(a3,d3.l*2),d7
	lsl.w	#8,d7
	ext.l	d7
	move.l	d7,cd_TempLastSampleL(a5)
	sub.l	a0,d7
	asr.l	#1,d7
	muls.l	d4,d7
	asr.l	#7,d7
	asr.l	#8,d7
	add.l	d7,a0

	move.l	a0,d7
	muls.l	d1,d7
	add.l	d7,(a4)+

	cmp.l	cd_FirstOffsetI(a5),d3	;Is this the first sample?
	bne	.not_firstR
	move.l	cd_LastSampleR(a5),a0
	bra	.got_sampleR
.not_firstR
	move.b	-1(a3,d3.l*2),d7
	lsl.w	#8,d7
;	ext.l	d7
;	move.l	d7,a0
	move.w	d7,a0
.got_sampleR
	move.b	1(a3,d3.l*2),d7
	lsl.w	#8,d7
	ext.l	d7
	move.l	d7,cd_TempLastSampleR(a5)
	sub.l	a0,d7
	asr.l	#1,d7
	muls.l	d4,d7
	asr.l	#7,d7
	asr.l	#8,d7
	add.l	d7,a0

	move.l	a0,d7
	muls.l	d2,d7
	add.l	d7,(a4)+

	add.w	d6,d4
	addx.l	d5,d3
	dbf	d0,.nextsample
.exit
	swap.w	d4		; 32 bit fraction
	swap.w	d6
 ENDC
	rts

;in
* d1.l	-65536..65536
AddWordMVH:
 IFGE	__CPU-68020
	clr.w	d4		; 16 bit fraction
	clr.w	d6
	swap.w	d4
	swap.w	d6
	subq.w	#1,d0
	bmi.b	.exit
.nextsample
	cmp.l	cd_FirstOffsetI(a5),d3	;Is this the first sample?
	bne	.not_first
	move.l	cd_LastSampleL(a5),a0
	bra	.got_sample
.not_first
	move.w	-2(a3,d3.l*2),a0
;	ext.l	d7
;	move.l	d7,a0
.got_sample
	move.w	0(a3,d3.l*2),d7
	ext.l	d7
	move.l	d7,cd_TempLastSampleL(a5)
	sub.l	a0,d7
	asr.l	#1,d7
	muls.l	d4,d7
	asr.l	#7,d7
	asr.l	#8,d7
	add.l	d7,a0

	move.l	a0,d7
	muls.l	d1,d7
	add.l	d7,(a4)+

	add.w	d6,d4
	addx.l	d5,d3
	dbf	d0,.nextsample
.exit
	swap.w	d4		; 32 bit fraction
	swap.w	d6
 ENDC
	rts

;in
* d1.l	-65536..65536
* d2.l	-65536..65536
AddWordSVPH:
 IFGE	__CPU-68020
	clr.w	d4		; 16 bit fraction
	clr.w	d6
	swap.w	d4
	swap.w	d6
	subq.w	#1,d0
	bmi.b	.exit
.nextsample
	cmp.l	cd_FirstOffsetI(a5),d3	;Is this the first sample?
	bne	.not_first
	move.l	cd_LastSampleL(a5),a0
	bra	.got_sample
.not_first
	move.w	-2(a3,d3.l*2),a0
;	ext.l	d7
;	move.l	d7,a0
.got_sample
	move.w	0(a3,d3.l*2),d7
	ext.l	d7
	move.l	d7,cd_TempLastSampleL(a5)
	sub.l	a0,d7
	asr.l	#1,d7
	muls.l	d4,d7
	asr.l	#7,d7
	asr.l	#8,d7
	add.l	d7,a0

	move.l	a0,d7
	muls.l	d1,d7
	add.l	d7,(a4)+

	move.l	a0,d7
	muls.l	d2,d7
	add.l	d7,(a4)+

	add.w	d6,d4
	addx.l	d5,d3
	dbf	d0,.nextsample
.exit
	swap.w	d4		; 32 bit fraction
	swap.w	d6
 ENDC
	rts

;in
* d1.l	-65536..65536
* d2.l	-65536..65536
AddWordsMVH:
 IFGE	__CPU-68020
	clr.w	d4		; 16 bit fraction
	clr.w	d6
	swap.w	d4
	swap.w	d6
	subq.w	#1,d0
	bmi.b	.exit
.nextsample
	cmp.l	cd_FirstOffsetI(a5),d3	;Is this the first sample?
	bne	.not_firstL
	move.l	cd_LastSampleL(a5),a0
	bra	.got_sampleL
.not_firstL
	move.w	-4(a3,d3.l*4),a0
;	ext.l	d7
;	move.l	d7,a0
.got_sampleL
	move.w	0(a3,d3.l*4),d7
	ext.l	d7
	move.l	d7,cd_TempLastSampleL(a5)
	sub.l	a0,d7
	asr.l	#1,d7
	muls.l	d4,d7
	asr.l	#7,d7
	asr.l	#8,d7
	add.l	d7,a0

	move.l	a0,d7
	muls.l	d1,d7
	add.l	d7,(a4)

	cmp.l	cd_FirstOffsetI(a5),d3	;Is this the first sample?
	bne	.not_firstR
	move.l	cd_LastSampleR(a5),a0
	bra	.got_sampleR
.not_firstR
	move.w	-2(a3,d3.l*4),a0
;	ext.l	d7
;	move.l	d7,a0
.got_sampleR
	move.w	2(a3,d3.l*4),d7
	ext.l	d7
	move.l	d7,cd_TempLastSampleR(a5)
	sub.l	a0,d7
	asr.l	#1,d7
	muls.l	d4,d7
	asr.l	#7,d7
	asr.l	#8,d7
	add.l	d7,a0

	move.l	a0,d7
	muls.l	d2,d7
	add.l	d7,(a4)+

	add.w	d6,d4
	addx.l	d5,d3
	dbf	d0,.nextsample
.exit
	swap.w	d4		; 32 bit fraction
	swap.w	d6
 ENDC
	rts

;in
* d1.l	-65536..65536
* d2.l	-65536..65536
AddWordsSVPH:
 IFGE	__CPU-68020
	clr.w	d4		; 16 bit fraction
	clr.w	d6
	swap.w	d4
	swap.w	d6
	subq.w	#1,d0
	bmi.b	.exit
.nextsample
	cmp.l	cd_FirstOffsetI(a5),d3	;Is this the first sample?
	bne	.not_firstL
	move.l	cd_LastSampleL(a5),a0
	bra	.got_sampleL
.not_firstL
	move.w	-4(a3,d3.l*4),a0
;	ext.l	d7
;	move.l	d7,a0
.got_sampleL
	move.w	0(a3,d3.l*4),d7
	ext.l	d7
	move.l	d7,cd_TempLastSampleL(a5)
	sub.l	a0,d7
	asr.l	#1,d7
	muls.l	d4,d7
	asr.l	#7,d7
	asr.l	#8,d7
	add.l	d7,a0

	move.l	a0,d7
	muls.l	d1,d7
	add.l	d7,(a4)+

	cmp.l	cd_FirstOffsetI(a5),d3	;Is this the first sample?
	bne	.not_firstR
	move.l	cd_LastSampleR(a5),a0
	bra	.got_sampleR
.not_firstR
	move.w	-2(a3,d3.l*4),a0
;	ext.l	d7
;	move.l	d7,a0
.got_sampleR
	move.w	2(a3,d3.l*4),d7
	ext.l	d7
	move.l	d7,cd_TempLastSampleR(a5)
	sub.l	a0,d7
	asr.l	#1,d7
	muls.l	d4,d7
	asr.l	#7,d7
	asr.l	#8,d7
	add.l	d7,a0

	move.l	a0,d7
	muls.l	d2,d7
	add.l	d7,(a4)+

	add.w	d6,d4
	addx.l	d5,d3
	dbf	d0,.nextsample
.exit
	swap.w	d4		; 32 bit fraction
	swap.w	d6
 ENDC
	rts


;------------------------------------------------------------------------------

AddByteMVHB:
 IFGE	__CPU-68020
	clr.w	d4		; 16 bit fraction
	clr.w	d6
	swap.w	d4
	swap.w	d6
	neg.w	d4
	subq.w	#1,d0
	bmi.b	.exit
.nextsample
	cmp.l	cd_FirstOffsetI(a5),d3	;Is this the first sample?
	bne	.not_first
	move.l	cd_LastSampleL(a5),a0
	bra	.got_sample
.not_first
	move.b	1(a3,d3.l),d7
	lsl.w	#8,d7
;	ext.l	d7
;	move.l	d7,a0
	move.w	d7,a0
.got_sample
	move.b	0(a3,d3.l),d7
	lsl.w	#8,d7
	ext.l	d7
	move.l	d7,cd_TempLastSampleL(a5)
	sub.l	a0,d7
	asr.l	#1,d7
	muls.l	d4,d7
	asr.l	#7,d7
	asr.l	#8,d7
	add.l	d7,a0

	move.l	a0,d7
	muls.l	d1,d7
	add.l	d7,(a4)+

	add.w	d6,d4
	subx.l	d5,d3
	dbf	d0,.nextsample
.exit
	neg.w	d4
	swap.w	d4		; 32 bit fraction
	swap.w	d6
 ENDC
	rts

AddByteSVPHB:
 IFGE	__CPU-68020
	clr.w	d4		; 16 bit fraction
	clr.w	d6
	swap.w	d4
	swap.w	d6
	neg.w	d4
	subq.w	#1,d0
	bmi.b	.exit
.nextsample
	cmp.l	cd_FirstOffsetI(a5),d3	;Is this the first sample?
	bne	.not_first
	move.l	cd_LastSampleL(a5),a0
	bra	.got_sample
.not_first
	move.b	1(a3,d3.l),d7
	lsl.w	#8,d7
;	ext.l	d7
;	move.l	d7,a0
	move.w	d7,a0
.got_sample
	move.b	0(a3,d3.l),d7
	lsl.w	#8,d7
	ext.l	d7
	move.l	d7,cd_TempLastSampleL(a5)
	sub.l	a0,d7
	asr.l	#1,d7
	muls.l	d4,d7
	asr.l	#7,d7
	asr.l	#8,d7
	add.l	d7,a0

	move.l	a0,d7
	muls.l	d1,d7
	add.l	d7,(a4)+

	move.l	a0,d7
	muls.l	d2,d7
	add.l	d7,(a4)+

	add.w	d6,d4
	subx.l	d5,d3
	dbf	d0,.nextsample
.exit
	neg.w	d4
	swap.w	d4		; 32 bit fraction
	swap.w	d6
 ENDC
	rts

AddBytesMVHB:
 IFGE	__CPU-68020
	clr.w	d4		; 16 bit fraction
	clr.w	d6
	swap.w	d4
	swap.w	d6
	neg.w	d4
	subq.w	#1,d0
	bmi.b	.exit
.nextsample
	cmp.l	cd_FirstOffsetI(a5),d3	;Is this the first sample?
	bne	.not_firstL
	move.l	cd_LastSampleL(a5),a0
	bra	.got_sampleL
.not_firstL
	move.b	2(a3,d3.l*2),d7
	lsl.w	#8,d7
;	ext.l	d7
;	move.l	d7,a0
	move.w	d7,a0
.got_sampleL
	move.b	0(a3,d3.l*2),d7
	lsl.w	#8,d7
	ext.l	d7
	move.l	d7,cd_TempLastSampleL(a5)
	sub.l	a0,d7
	asr.l	#1,d7
	muls.l	d4,d7
	asr.l	#7,d7
	asr.l	#8,d7
	add.l	d7,a0

	move.l	a0,d7
	muls.l	d1,d7
	add.l	d7,(a4)

	cmp.l	cd_FirstOffsetI(a5),d3	;Is this the first sample?
	bne	.not_firstR
	move.l	cd_LastSampleR(a5),a0
	bra	.got_sampleR
.not_firstR
	move.b	3(a3,d3.l*2),d7
	lsl.w	#8,d7
;	ext.l	d7
;	move.l	d7,a0
	move.w	d7,a0
.got_sampleR
	move.b	1(a3,d3.l*2),d7
	lsl.w	#8,d7
	ext.l	d7
	move.l	d7,cd_TempLastSampleR(a5)
	sub.l	a0,d7
	asr.l	#1,d7
	muls.l	d4,d7
	asr.l	#7,d7
	asr.l	#8,d7
	add.l	d7,a0

	move.l	a0,d7
	muls.l	d2,d7
	add.l	d7,(a4)+

	add.w	d6,d4
	subx.l	d5,d3
	dbf	d0,.nextsample
.exit
	neg.w	d4
	swap.w	d4		; 32 bit fraction
	swap.w	d6
 ENDC
	rts

AddBytesSVPHB:
 IFGE	__CPU-68020
	clr.w	d4		; 16 bit fraction
	clr.w	d6
	swap.w	d4
	swap.w	d6
	neg.w	d4
	subq.w	#1,d0
	bmi.b	.exit
.nextsample
	cmp.l	cd_FirstOffsetI(a5),d3	;Is this the first sample?
	bne	.not_firstL
	move.l	cd_LastSampleL(a5),a0
	bra	.got_sampleL
.not_firstL
	move.b	2(a3,d3.l*2),d7
	lsl.w	#8,d7
;	ext.l	d7
;	move.l	d7,a0
	move.w	d7,a0
.got_sampleL
	move.b	0(a3,d3.l*2),d7
	lsl.w	#8,d7
	ext.l	d7
	move.l	d7,cd_TempLastSampleL(a5)
	sub.l	a0,d7
	asr.l	#1,d7
	muls.l	d4,d7
	asr.l	#7,d7
	asr.l	#8,d7
	add.l	d7,a0

	move.l	a0,d7
	muls.l	d1,d7
	add.l	d7,(a4)+

	cmp.l	cd_FirstOffsetI(a5),d3	;Is this the first sample?
	bne	.not_firstR
	move.l	cd_LastSampleR(a5),a0
	bra	.got_sampleR
.not_firstR
	move.b	3(a3,d3.l*2),d7
	lsl.w	#8,d7
;	ext.l	d7
;	move.l	d7,a0
	move.w	d7,a0
.got_sampleR
	move.b	1(a3,d3.l*2),d7
	lsl.w	#8,d7
	ext.l	d7
	move.l	d7,cd_TempLastSampleR(a5)
	sub.l	a0,d7
	asr.l	#1,d7
	muls.l	d4,d7
	asr.l	#7,d7
	asr.l	#8,d7
	add.l	d7,a0

	move.l	a0,d7
	muls.l	d2,d7
	add.l	d7,(a4)+

	add.w	d6,d4
	subx.l	d5,d3
	dbf	d0,.nextsample
.exit
	neg.w	d4
	swap.w	d4		; 32 bit fraction
	swap.w	d6
 ENDC
	rts

AddWordMVHB:
 IFGE	__CPU-68020
	clr.w	d4		; 16 bit fraction
	clr.w	d6
	swap.w	d4
	swap.w	d6
	neg.w	d4
	subq.w	#1,d0
	bmi.b	.exit
.nextsample
	cmp.l	cd_FirstOffsetI(a5),d3	;Is this the first sample?
	bne	.not_first
	move.l	cd_LastSampleL(a5),a0
	bra	.got_sample
.not_first
	move.w	2(a3,d3.l*2),a0
;	ext.l	d7
;	move.l	d7,a0
.got_sample
	move.w	0(a3,d3.l*2),d7
	ext.l	d7
	move.l	d7,cd_TempLastSampleL(a5)
	sub.l	a0,d7
	asr.l	#1,d7
	muls.l	d4,d7
	asr.l	#7,d7
	asr.l	#8,d7
	add.l	d7,a0

	move.l	a0,d7
	muls.l	d1,d7
	add.l	d7,(a4)+

	add.w	d6,d4
	subx.l	d5,d3
	dbf	d0,.nextsample
.exit
	neg.w	d4
	swap.w	d4		; 32 bit fraction
	swap.w	d6
 ENDC
	rts

AddWordSVPHB:
 IFGE	__CPU-68020
	clr.w	d4		; 16 bit fraction
	clr.w	d6
	swap.w	d4
	swap.w	d6
	neg.w	d4
	subq.w	#1,d0
	bmi.b	.exit
.nextsample
	cmp.l	cd_FirstOffsetI(a5),d3	;Is this the first sample?
	bne	.not_first
	move.l	cd_LastSampleL(a5),a0
	bra	.got_sample
.not_first
	move.w	2(a3,d3.l*2),a0
;	ext.l	d7
;	move.l	d7,a0
.got_sample
	move.w	0(a3,d3.l*2),d7
	ext.l	d7
	move.l	d7,cd_TempLastSampleL(a5)
	sub.l	a0,d7
	asr.l	#1,d7
	muls.l	d4,d7
	asr.l	#7,d7
	asr.l	#8,d7
	add.l	d7,a0

	move.l	a0,d7
	muls.l	d1,d7
	add.l	d7,(a4)+

	move.l	a0,d7
	muls.l	d2,d7
	add.l	d7,(a4)+

	add.w	d6,d4
	subx.l	d5,d3
	dbf	d0,.nextsample
.exit
	neg.w	d4
	swap.w	d4		; 32 bit fraction
	swap.w	d6
 ENDC
	rts


AddWordsMVHB:
 IFGE	__CPU-68020
	clr.w	d4		; 16 bit fraction
	clr.w	d6
	swap.w	d4
	swap.w	d6
	neg.w	d4
	subq.w	#1,d0
	bmi.b	.exit
.nextsample
	cmp.l	cd_FirstOffsetI(a5),d3	;Is this the first sample?
	bne	.not_firstL
	move.l	cd_LastSampleL(a5),a0
	bra	.got_sampleL
.not_firstL
	move.w	4(a3,d3.l*4),a0
;	ext.l	d7
;	move.l	d7,a0
.got_sampleL
	move.w	0(a3,d3.l*4),d7
	ext.l	d7
	move.l	d7,cd_TempLastSampleL(a5)
	sub.l	a0,d7
	asr.l	#1,d7
	muls.l	d4,d7
	asr.l	#7,d7
	asr.l	#8,d7
	add.l	d7,a0

	move.l	a0,d7
	muls.l	d1,d7
	add.l	d7,(a4)

	cmp.l	cd_FirstOffsetI(a5),d3	;Is this the first sample?
	bne	.not_firstR
	move.l	cd_LastSampleR(a5),a0
	bra	.got_sampleR
.not_firstR
	move.w	6(a3,d3.l*4),a0
;	ext.l	d7
;	move.l	d7,a0
.got_sampleR
	move.w	2(a3,d3.l*4),d7
	ext.l	d7
	move.l	d7,cd_TempLastSampleR(a5)
	sub.l	a0,d7
	asr.l	#1,d7
	muls.l	d4,d7
	asr.l	#7,d7
	asr.l	#8,d7
	add.l	d7,a0

	move.l	a0,d7
	muls.l	d2,d7
	add.l	d7,(a4)+

	add.w	d6,d4
	subx.l	d5,d3
	dbf	d0,.nextsample
.exit
	neg.w	d4
	swap.w	d4		; 32 bit fraction
	swap.w	d6
 ENDC
	rts

AddWordsSVPHB:
 IFGE	__CPU-68020
	clr.w	d4		; 16 bit fraction
	clr.w	d6
	swap.w	d4
	swap.w	d6
	neg.w	d4
	subq.w	#1,d0
	bmi.b	.exit
.nextsample
	cmp.l	cd_FirstOffsetI(a5),d3	;Is this the first sample?
	bne	.not_firstL
	move.l	cd_LastSampleL(a5),a0
	bra	.got_sampleL
.not_firstL
	move.w	4(a3,d3.l*4),a0
;	ext.l	d7
;	move.l	d7,a0
.got_sampleL
	move.w	0(a3,d3.l*4),d7
	ext.l	d7
	move.l	d7,cd_TempLastSampleL(a5)
	sub.l	a0,d7
	asr.l	#1,d7
	muls.l	d4,d7
	asr.l	#7,d7
	asr.l	#8,d7
	add.l	d7,a0

	move.l	a0,d7
	muls.l	d1,d7
	add.l	d7,(a4)+

	cmp.l	cd_FirstOffsetI(a5),d3	;Is this the first sample?
	bne	.not_firstR
	move.l	cd_LastSampleR(a5),a0
	bra	.got_sampleR
.not_firstR
	move.w	6(a3,d3.l*4),a0
;	ext.l	d7
;	move.l	d7,a0
.got_sampleR
	move.w	2(a3,d3.l*4),d7
	ext.l	d7
	move.l	d7,cd_TempLastSampleR(a5)
	sub.l	a0,d7
	asr.l	#1,d7
	muls.l	d4,d7
	asr.l	#7,d7
	asr.l	#8,d7
	add.l	d7,a0

	move.l	a0,d7
	muls.l	d2,d7
	add.l	d7,(a4)+

	add.w	d6,d4
	subx.l	d5,d3
	dbf	d0,.nextsample
.exit
	neg.w	d4
	swap.w	d4		; 32 bit fraction
	swap.w	d6
 ENDC
	rts

;******************************************************************************
	cnop	0,16

AddSilence:
	clr.w	d4			; 16 bit fraction
	clr.w	d6
	swap.w	d4
	swap.w	d6
 IFGE	__CPU-68020
  IFLT	__CPU-68060
	move.l	d5,d1			;AddI<65535
	swap.w	d1
	move.w	d6,d1			;d1=Add<<16
	mulu.l	d0,d2:d1
	add.w	d0,d4			;New OffsetF (X)
	move.w	d1,d0
	move.w	d2,d1
	swap.w	d1			;d1=d2:d1>>16
	addx.l	d1,d3			;New OffsetI
  ELSE
	move.l	d5,d1
	mulu.w	d0,d1			;OffsI*BuffSamples
	move.l	d6,d2
	add.l	d1,d3			;New OffsetI (1)
	mulu.w	d0,d2			;OffsF*BuffSamples...
	add.w	d2,d4			;New OffsetF (X)
	clr.w	d2
	swap.w	d2			;...>>16
	addx.l	d2,d3			;New OffsetI (2)
  ENDC
 ELSE
	move.l	d5,d1
	mulu.w	d0,d1			;OffsI*BuffSamples
	move.l	d6,d2
	add.l	d1,d3			;New OffsetI (1)
	mulu.w	d0,d2			;OffsF*BuffSamples...
	add.w	d2,d4			;New OffsetF (X)
	clr.w	d2
	swap.w	d2			;...>>16
	addx.l	d2,d3			;New OffsetI (2)
 ENDC
	btst.b	#AHIACB_STEREO,ahiac_Flags+3(a2)
	beq.b	.nostereo
	lsl.l	#1,d0
.nostereo
	btst.b	#AHIACB_HIFI,ahiac_Flags+3(a2)
	beq.b	.nohifi
	lsl.l	#1,d0
.nohifi
	lsl.l	#1,d0
	add.l	d0,a4			;New buffer pointer
	swap.w	d4			; 32 bit fraction
	swap.w	d6
	rts

;------------------------------------------------------------------------------
	cnop	0,16

AddSilenceB:
	clr.w	d4			; 16 bit fraction
	clr.w	d6
	swap.w	d4
	swap.w	d6
 IFGE	__CPU-68020
  IFLT	__CPU-68060
	move.l	d5,d1			;AddI<65535
	swap.w	d1
	move.w	d6,d1			;d1=Add<<16
	mulu.l	d0,d2:d1
	sub.w	d0,d4			;New OffsetF (X)
	move.w	d1,d0
	move.w	d2,d1
	swap.w	d1			;d1=d2:d1>>16
	subx.l	d1,d3			;New OffsetI
  ELSE
	move.l	d5,d1
	mulu.w	d0,d1			;OffsI*BuffSamples
	move.l	d6,d2
	add.l	d1,d3			;New OffsetI (1)
	mulu.w	d0,d2			;OffsF*BuffSamples...
	sub.w	d2,d4			;New OffsetF (X)
	clr.w	d2
	swap.w	d2			;...>>16
	subx.l	d2,d3			;New OffsetI (2)
  ENDC
 ELSE
	move.l	d5,d1
	mulu.w	d0,d1			;OffsI*BuffSamples
	move.l	d6,d2
	add.l	d1,d3			;New OffsetI (1)
	mulu.w	d0,d2			;OffsF*BuffSamples...
	sub.w	d2,d4			;New OffsetF (X)
	clr.w	d2
	swap.w	d2			;...>>16
	subx.l	d2,d3			;New OffsetI (2)
 ENDC
	btst.b	#AHIACB_STEREO,ahiac_Flags+3(a2)
	beq.b	.nostereo
	lsl.l	#1,d0
.nostereo
	btst.b	#AHIACB_HIFI,ahiac_Flags+3(a2)
	beq.b	.nohifi
	lsl.l	#1,d0
.nohifi
	lsl.l	#1,d0
	add.l	d0,a4			;New buffer pointer
	swap.w	d4			; 32 bit fraction
	swap.w	d6
	rts

;******************************************************************************
	cnop	0,16

;in
* d1.w	-256..256
AddByteMV:
	lsr.w	#1,d0
	bcs.b	.1
	subq.w	#1,d0
	bmi.b	.exit
.nextsample
	move.b	(a3,d3.l),d7
	ext.w	d7
	muls.w	d1,d7
	add.w	d7,(a4)+
	add.l	d6,d4
	addx.l	d5,d3
.1
	move.b	(a3,d3.l),d7
	ext.w	d7
	muls.w	d1,d7
	add.w	d7,(a4)+
	add.l	d6,d4
	addx.l	d5,d3
	dbf	d0,.nextsample
.exit
	rts

;------------------------------------------------------------------------------
	cnop	0,16

;in
* d1.l	Pointer in multiplication table
AddByteMVT:
	move.l	d1,a0
	moveq	#0,d1
	lsr.w	#1,d0
	bcs.b	.1
	subq.w	#1,d0
	bmi.b	.exit
.nextsample
	move.b	(a3,d3.l),d1
 IFGE	__CPU-68020
	move.w	0(a0,d1.w*4),d2		;signed multiplication
 ELSE
	add.w	d1,d1
	add.w	d1,d1
	move.w	0(a0,d1.w),d2		;signed multiplication
	moveq	#0,d1
 ENDC
	add.w	d2,(a4)+
	add.l	d6,d4
	addx.l	d5,d3
.1
	move.b	(a3,d3.l),d1
 IFGE	__CPU-68020
	move.w	0(a0,d1.w*4),d2		;signed multiplication
 ELSE
	add.w	d1,d1
	add.w	d1,d1
	move.w	0(a0,d1.w),d2		;signed multiplication
	moveq	#0,d1
 ENDC
	add.w	d2,(a4)+
	add.l	d6,d4
	addx.l	d5,d3

	dbf	d0,.nextsample
.exit
	rts

;------------------------------------------------------------------------------
	cnop	0,16

;in
* d1.w	-256..256
AddByteSVl:
	lsr.w	#1,d0
	bcs.b	.1
	subq.w	#1,d0
	bmi.b	.exit
.nextsample
	move.b	(a3,d3.l),d7
	ext.w	d7
	muls.w	d1,d7
	add.w	d7,(a4)+
	add.l	d6,d4
	addx.l	d5,d3
	addq.l	#2,a4
.1
	move.b	(a3,d3.l),d7
	ext.w	d7
	muls.w	d1,d7
	add.w	d7,(a4)+
	add.l	d6,d4
	addx.l	d5,d3
	addq.l	#2,a4
	dbf	d0,.nextsample
.exit
	rts

;------------------------------------------------------------------------------
	cnop	0,16

;in
* d2.w	-256..256
AddByteSVr:
	moveq	#0,d7
	lsr.w	#1,d0
	bcs.b	.1
	subq.w	#1,d0
	bmi.b	.exit
.nextsample
	move.b	(a3,d3.l),d7
	addq.l	#2,a4
	ext.w	d7
	muls.w	d2,d7
	add.w	d7,(a4)+
	add.l	d6,d4
	addx.l	d5,d3
.1
	move.b	(a3,d3.l),d7
	addq.l	#2,a4
	ext.w	d7
	muls.w	d2,d7
	add.w	d7,(a4)+
	add.l	d6,d4
	addx.l	d5,d3
	dbf	d0,.nextsample
.exit
	rts

;------------------------------------------------------------------------------
	cnop	0,16

;in
* d1.w	-256..256
* d2.w	-256..256
AddByteSVP:
	lsr.w	#1,d0
	bcs.b	.1
	subq.w	#1,d0
	bmi.b	.exit
.nextsample
	move.b	(a3,d3.l),d7
	ext.w	d7
	move.w	d7,a0
	muls.w	d1,d7
	add.w	d7,(a4)+
	move.w	a0,d7
	muls.w	d2,d7
	add.w	d7,(a4)+
	add.l	d6,d4
	addx.l	d5,d3
.1
	move.b	(a3,d3.l),d7
	ext.w	d7
	move.w	d7,a0
	muls.w	d1,d7
	add.w	d7,(a4)+
	move.w	a0,d7
	muls.w	d2,d7
	add.w	d7,(a4)+
	add.l	d6,d4
	addx.l	d5,d3
	dbf	d0,.nextsample
.exit
	rts

;------------------------------------------------------------------------------
	cnop	0,16

;in
* d1.l	Pointer in multiplication table
AddByteSVTl:
	move.l	d1,a0
	moveq	#0,d1
	lsr.w	#1,d0
	bcs.b	.1
	subq.w	#1,d0
	bmi.b	.exit
.nextsample
	move.b	(a3,d3.l),d1
 IFGE	__CPU-68020
	move.w	0(a0,d1.w*4),d2		;signed multiplication
 ELSE
	add.w	d1,d1
	add.w	d1,d1
	move.w	0(a0,d1.w),d2		;signed multiplication
	moveq	#0,d1
 ENDC
	add.w	d2,(a4)+
	add.l	d6,d4
	addx.l	d5,d3
	addq.l	#2,a4
.1
	move.b	(a3,d3.l),d1
 IFGE	__CPU-68020
	move.w	0(a0,d1.w*4),d2		;signed multiplication
 ELSE
	add.w	d1,d1
	add.w	d1,d1
	move.w	0(a0,d1.w),d2		;signed multiplication
	moveq	#0,d1
 ENDC
	add.w	d2,(a4)+
	add.l	d6,d4
	addx.l	d5,d3
	addq.l	#2,a4
	dbf	d0,.nextsample
.exit
	rts
;------------------------------------------------------------------------------
	cnop	0,16

;in
* d2.l	Pointer in multiplication table
AddByteSVTr:
	move.l	d2,a0
	moveq	#0,d1
	moveq	#0,d2
	lsr.w	#1,d0
	bcs.b	.1
	subq.w	#1,d0
	bmi.b	.exit
.nextsample
	move.b	(a3,d3.l),d1
 IFGE	__CPU-68020
	move.w	0(a0,d1.w*4),d2		;signed multiplication
 ELSE
	add.w	d1,d1
	add.w	d1,d1
	move.w	0(a0,d1.w),d2		;signed multiplication
	moveq	#0,d1
 ENDC
	addq.l	#2,a4
	add.w	d2,(a4)+
	add.l	d6,d4
	addx.l	d5,d3
.1
	move.b	(a3,d3.l),d1
 IFGE	__CPU-68020
	move.w	0(a0,d1.w*4),d2		;signed multiplication
 ELSE
	add.w	d1,d1
	add.w	d1,d1
	move.w	0(a0,d1.w),d2		;signed multiplication
	moveq	#0,d1
 ENDC
	addq.l	#2,a4
	add.w	d2,(a4)+
	add.l	d6,d4
	addx.l	d5,d3
	dbf	d0,.nextsample
.exit
	rts

;------------------------------------------------------------------------------
	cnop	0,16

;in
* d1.l	Pointer in multiplication table
* d2.l	Pointer in multiplication table
AddByteSVPT:
	move.l	a1,d7			;save a1
	move.l	d1,a0
	move.l	d2,a1
	moveq	#0,d1
	lsr.w	#1,d0
	bcs.b	.1
	subq.w	#1,d0
	bmi.b	.exit
.nextsample
	move.b	(a3,d3.l),d1
 IFGE	__CPU-68020
	move.w	0(a0,d1.w*4),d2
 ELSE
	add.w	d1,d1
	add.w	d1,d1
	move.w	0(a0,d1.w),d2
 ENDC
	add.w	d2,(a4)+

 IFGE	__CPU-68020
	move.w	0(a1,d1.w*4),d2
 ELSE
	move.w	0(a1,d1.w),d2
	moveq	#0,d1
 ENDC
	add.w	d2,(a4)+
	add.l	d6,d4
	addx.l	d5,d3
.1
	move.b	(a3,d3.l),d1
 IFGE	__CPU-68020
	move.w	0(a0,d1.w*4),d2
 ELSE
	add.w	d1,d1
	add.w	d1,d1
	move.w	0(a0,d1.w),d2
 ENDC
	add.w	d2,(a4)+

 IFGE	__CPU-68020
	move.w	0(a1,d1.w*4),d2
 ELSE
	move.w	0(a1,d1.w),d2
	moveq	#0,d1
 ENDC
	add.w	d2,(a4)+
	add.l	d6,d4
	addx.l	d5,d3
	dbf	d0,.nextsample
.exit
	move.l	d7,a1			;restore a1
	rts

;******************************************************************************
	cnop	0,16

;in
* d1.w	-256..256
* d2.w	-256..256
AddBytesMV:
 IFGE	__CPU-68020
	lsr.w	#1,d0
	bcs.b	.1
	subq.w	#1,d0
	bmi.b	.exit
.nextsample
	move.b	0(a3,d3.l*2),d7
	ext.w	d7
	muls.w	d1,d7
	add.w	d7,(a4)
	move.b	1(a3,d3.l*2),d7
	ext.w	d7
	muls.w	d2,d7
	add.w	d7,(a4)+
	add.l	d6,d4
	addx.l	d5,d3
.1
	move.b	0(a3,d3.l*2),d7
	ext.w	d7
	muls.w	d1,d7
	add.w	d7,(a4)
	move.b	1(a3,d3.l*2),d7
	ext.w	d7
	muls.w	d2,d7
	add.w	d7,(a4)+
	add.l	d6,d4
	addx.l	d5,d3
	dbf	d0,.nextsample
.exit
 ENDC
	rts

;------------------------------------------------------------------------------
	cnop	0,16

;in
* d1.l	Pointer in multiplication table
* d2.l	Pointer in multiplication table
AddBytesMVT:
 IFGE	__CPU-68020
	push	a1
	move.l	d1,a0
	move.l	d2,a1
	moveq	#0,d1
	lsr.w	#1,d0
	bcs.b	.1
	subq.w	#1,d0
	bmi.b	.exit
.nextsample
	move.b	0(a3,d3.l*2),d1
	move.w	0(a0,d1.w*4),d2		;signed multiplication
	move.b	1(a3,d3.l*2),d1
	add.w	0(a1,d1.w*4),d2		;signed multiplication
	add.w	d2,(a4)+
	add.l	d6,d4
	addx.l	d5,d3
.1
	move.b	0(a3,d3.l*2),d1
	move.w	0(a0,d1.w*4),d2		;signed multiplication
	move.b	1(a3,d3.l*2),d1
	add.w	0(a1,d1.w*4),d2		;signed multiplication
	add.w	d2,(a4)+
	add.l	d6,d4
	addx.l	d5,d3

	dbf	d0,.nextsample
.exit
	pop	a1
 ENDC
	rts

;------------------------------------------------------------------------------
	cnop	0,16

;in
* d1.w	-256..256
AddBytesSVl:
 IFGE	__CPU-68020
	lsr.w	#1,d0
	bcs.b	.1
	subq.w	#1,d0
	bmi.b	.exit
.nextsample
	move.b	0(a3,d3.l*2),d7
	ext.w	d7
	muls.w	d1,d7
	add.w	d7,(a4)+
	add.l	d6,d4
	addx.l	d5,d3
	addq.l	#2,a4
.1
	move.b	0(a3,d3.l*2),d7
	ext.w	d7
	muls.w	d1,d7
	add.w	d7,(a4)+
	add.l	d6,d4
	addx.l	d5,d3
	addq.l	#2,a4
	dbf	d0,.nextsample
.exit
 ENDC
	rts

;------------------------------------------------------------------------------
	cnop	0,16

;in
* d2.w	-256..256
AddBytesSVr:
 IFGE	__CPU-68020
	moveq	#0,d7
	lsr.w	#1,d0
	bcs.b	.1
	subq.w	#1,d0
	bmi.b	.exit
.nextsample
	move.b	1(a3,d3.l*2),d7
	addq.l	#2,a4
	ext.w	d7
	muls.w	d2,d7
	add.w	d7,(a4)+
	add.l	d6,d4
	addx.l	d5,d3
.1
	move.b	1(a3,d3.l*2),d7
	addq.l	#2,a4
	ext.w	d7
	muls.w	d2,d7
	add.w	d7,(a4)+
	add.l	d6,d4
	addx.l	d5,d3
	dbf	d0,.nextsample
.exit
 ENDC
	rts

;------------------------------------------------------------------------------
	cnop	0,16

;in
* d1.w	-256..256
* d2.w	-256..256
AddBytesSVP:
 IFGE	__CPU-68020
	lsr.w	#1,d0
	bcs.b	.1
	subq.w	#1,d0
	bmi.b	.exit
.nextsample
	move.b	0(a3,d3.l*2),d7
	ext.w	d7
	muls.w	d1,d7
	add.w	d7,(a4)+
	move.b	1(a3,d3.l*2),d7
	ext.w	d7
	muls.w	d2,d7
	add.w	d7,(a4)+
	add.l	d6,d4
	addx.l	d5,d3
.1
	move.b	0(a3,d3.l*2),d7
	ext.w	d7
	muls.w	d1,d7
	add.w	d7,(a4)+
	move.b	1(a3,d3.l*2),d7
	ext.w	d7
	muls.w	d2,d7
	add.w	d7,(a4)+
	add.l	d6,d4
	addx.l	d5,d3
	dbf	d0,.nextsample
.exit
 ENDC
	rts

;------------------------------------------------------------------------------
	cnop	0,16

;in
* d1.l	Pointer in multiplication table
AddBytesSVTl:
 IFGE	__CPU-68020
	move.l	d1,a0
	moveq	#0,d1
	lsr.w	#1,d0
	bcs.b	.1
	subq.w	#1,d0
	bmi.b	.exit
.nextsample
	move.b	0(a3,d3.l*2),d1
	move.w	0(a0,d1.w*4),d2		;signed multiplication
	add.w	d2,(a4)+
	add.l	d6,d4
	addx.l	d5,d3
	addq.l	#2,a4
.1
	move.b	0(a3,d3.l*2),d1
	move.w	0(a0,d1.w*4),d2		;signed multiplication
	add.w	d2,(a4)+
	add.l	d6,d4
	addx.l	d5,d3
	addq.l	#2,a4
	dbf	d0,.nextsample
.exit
 ENDC
	rts
;------------------------------------------------------------------------------
	cnop	0,16

;in
* d2.l	Pointer in multiplication table
AddBytesSVTr:
 IFGE	__CPU-68020
	move.l	d2,a0
	moveq	#0,d1
	moveq	#0,d2
	lsr.w	#1,d0
	bcs.b	.1
	subq.w	#1,d0
	bmi.b	.exit
.nextsample
	move.b	1(a3,d3.l*2),d1
	move.w	0(a0,d1.w*4),d2		;signed multiplication
	addq.l	#2,a4
	add.w	d2,(a4)+
	add.l	d6,d4
	addx.l	d5,d3
.1
	move.b	1(a3,d3.l*2),d1
	move.w	0(a0,d1.w*4),d2		;signed multiplication
	addq.l	#2,a4
	add.w	d2,(a4)+
	add.l	d6,d4
	addx.l	d5,d3
	dbf	d0,.nextsample
.exit
 ENDC
	rts

;------------------------------------------------------------------------------
	cnop	0,16

;in
* d1.l	Pointer in multiplication table
* d2.l	Pointer in multiplication table
AddBytesSVPT:
 IFGE	__CPU-68020
	move.l	a1,d7			;save a1
	move.l	d1,a0
	move.l	d2,a1
	moveq	#0,d1
	lsr.w	#1,d0
	bcs.b	.1
	subq.w	#1,d0
	bmi.b	.exit
.nextsample
	move.b	0(a3,d3.l*2),d1
	move.w	0(a0,d1.w*4),d2
	add.w	d2,(a4)+
	move.b	1(a3,d3.l*2),d1
	move.w	0(a1,d1.w*4),d2
	add.w	d2,(a4)+
	add.l	d6,d4
	addx.l	d5,d3
.1
	move.b	0(a3,d3.l*2),d1
	move.w	0(a0,d1.w*4),d2
	add.w	d2,(a4)+
	move.b	1(a3,d3.l*2),d1
	move.w	0(a1,d1.w*4),d2
	add.w	d2,(a4)+
	add.l	d6,d4
	addx.l	d5,d3
	dbf	d0,.nextsample
.exit
	move.l	d7,a1			;restore a1
 ENDC
	rts

;******************************************************************************
	cnop	0,16

;in
* d1.l	-32768..32767/-256..256
AddWordMV:
* 16/8 bit signed input (8 for '000 version)
	moveq	#15,d2

	lsr.w	#1,d0
	bcs.b	.1
	subq.w	#1,d0
	bmi.b	.exit
.nextsample
 IFGE	__CPU-68020
	move.w	(a3,d3.l*2),d7
	muls.w	d1,d7
	asr.l	d2,d7
 ELSE
	move.l	d3,d7
	add.l	d7,d7
	move.b	0(a3,d7.l),d7		;high byte
	ext.w	d7
	muls.w	d1,d7
 ENDC
	add.w	d7,(a4)+
	add.l	d6,d4
	addx.l	d5,d3
.1
 IFGE	__CPU-68020
	move.w	(a3,d3.l*2),d7
	muls.w	d1,d7
	asr.l	d2,d7
 ELSE
	move.l	d3,d7
	add.l	d7,d7
	move.b	0(a3,d7.l),d7		;high byte
	ext.w	d7
	muls.w	d1,d7
 ENDC
	add.w	d7,(a4)+
	add.l	d6,d4
	addx.l	d5,d3
	dbf	d0,.nextsample
.exit
	rts

;------------------------------------------------------------------------------
	cnop	0,16

;in
* d1.l	Shift count
AddWordMVT:
	lsr.w	#1,d0
	bcs.b	.1
	subq.w	#1,d0
	bmi.b	.exit
.nextsample
 IFGE	__CPU-68020
	move.w	(a3,d3.l*2),d7
 ELSE
	move.l	d3,d7
 	add.l	d7,d7
	move.w	(a3,d7.l),d7
 ENDC
	asr.w	d1,d7
	add.w	d7,(a4)+
	add.l	d6,d4
	addx.l	d5,d3
.1
 IFGE	__CPU-68020
	move.w	(a3,d3.l*2),d7
 ELSE
	move.l	d3,d7
 	add.l	d7,d7
	move.w	(a3,d7.l),d7
 ENDC
	asr.w	d1,d7
	add.w	d7,(a4)+
	add.l	d6,d4
	addx.l	d5,d3
	dbf	d0,.nextsample
.exit
	rts

;------------------------------------------------------------------------------
	cnop	0,16

;in
* d1.l	-32768..32767/-256..256
AddWordSVl:
* 16/8 bit signed input (8 for '000 version)
	moveq	#15,d2
	lsr.w	#1,d0
	bcs.b	.1
	subq.w	#1,d0
	bmi.b	.exit
.nextsample
 IFGE	__CPU-68020
	move.w	(a3,d3.l*2),d7
	muls.w	d1,d7
	asr.l	d2,d7
 ELSE
	move.l	d3,d7
	add.l	d7,d7
	move.b	0(a3,d7.l),d7		;high byte
	ext.w	d7
	muls.w	d1,d7
 ENDC
	add.w	d7,(a4)+
	add.l	d6,d4
	addx.l	d5,d3
	addq.l	#2,a4
.1
 IFGE	__CPU-68020
	move.w	(a3,d3.l*2),d7
	muls.w	d1,d7
	asr.l	d2,d7
 ELSE
	move.l	d3,d7
	add.l	d7,d7
	move.b	0(a3,d7.l),d7		;high byte
	ext.w	d7
	muls.w	d1,d7
 ENDC
	add.w	d7,(a4)+
	add.l	d6,d4
	addx.l	d5,d3
	addq.l	#2,a4
	dbf	d0,.nextsample
.exit
	rts

;------------------------------------------------------------------------------
	cnop	0,16

;in
* d2.l	-32768..32767/-256..256
AddWordSVr:
* 16/8 bit signed input (8 for '000 version)
	moveq	#15,d1

	lsr.w	#1,d0
	bcs.b	.1
	subq.w	#1,d0
	bmi.b	.exit
.nextsample
 IFGE	__CPU-68020
	move.w	(a3,d3.l*2),d7
	muls.w	d2,d7
	asr.l	d1,d7
 ELSE
	move.l	d3,d7
	add.l	d7,d7
	move.b	0(a3,d7.l),d7		;high byte
	ext.w	d7
	muls.w	d2,d7
 ENDC
	addq.l	#2,a4
	add.w	d7,(a4)+
	add.l	d6,d4
	addx.l	d5,d3
.1
 IFGE	__CPU-68020
	move.w	(a3,d3.l*2),d7
	muls.w	d2,d7
	asr.l	d1,d7
 ELSE
	move.l	d3,d7
	add.l	d7,d7
	move.b	0(a3,d7.l),d7		;high byte
	ext.w	d7
	muls.w	d2,d7
 ENDC
	addq.l	#2,a4
	add.w	d7,(a4)+
	add.l	d6,d4
	addx.l	d5,d3
	dbf	d0,.nextsample
.exit
	rts

;------------------------------------------------------------------------------
	cnop	0,16

;in
* d1.l	-32768..32767/-256..256
* d2.l	-32768..32767/-256..256
AddWordSVP:
* 16/8 bit signed input (8 for '000 version)
	lsr.w	#1,d0
	bcs.b	.1
	subq.w	#1,d0
	bmi.b	.exit
.nextsample
 IFGE	__CPU-68020
	move.w	(a3,d3.l*2),d7
	move.l	d7,a0
	muls.w	d1,d7
	asr.l	#8,d7
	asr.l	#7,d7
	add.w	d7,(a4)+

	move.l	a0,d7
	muls.w	d2,d7
	asr.l	#8,d7
	asr.l	#7,d7
	add.w	d7,(a4)+
 ELSE
	move.l	d3,d7
	add.l	d7,d7
	move.b	0(a3,d7.l),d7		;high byte
	ext.w	d7
	move.l	d7,a0
	muls.w	d1,d7
	add.w	d7,(a4)+

	move.l	a0,d7
	muls.w	d2,d7
	add.w	d7,(a4)+
 ENDC
	add.l	d6,d4
	addx.l	d5,d3
.1
 IFGE	__CPU-68020
	move.w	(a3,d3.l*2),d7
	move.l	d7,a0
	muls.w	d1,d7
	asr.l	#8,d7
	asr.l	#7,d7
	add.w	d7,(a4)+

	move.l	a0,d7
	muls.w	d2,d7
	asr.l	#8,d7
	asr.l	#7,d7
	add.w	d7,(a4)+
 ELSE
	move.l	d3,d7
	add.l	d7,d7
	move.b	0(a3,d7.l),d7		;high byte
	ext.w	d7
	move.l	d7,a0
	muls.w	d1,d7
	add.w	d7,(a4)+

	move.l	a0,d7
	muls.w	d2,d7
	add.w	d7,(a4)+
 ENDC
	add.l	d6,d4
	addx.l	d5,d3
	dbf	d0,.nextsample
.exit
	rts

;------------------------------------------------------------------------------
	cnop	0,16

;in
* d1.l	Shift count
AddWordSVTl:
	lsr.w	#1,d0
	bcs.b	.1
	subq.w	#1,d0
	bmi.b	.exit
.nextsample
 IFGE	__CPU-68020
	move.w	(a3,d3.l*2),d7
 ELSE
	move.l	d3,d7
	add.l	d7,d7
	move.w	(a3,d7.l),d7
 ENDC
	asr.w	d1,d7
	add.w	d7,(a4)+
	add.l	d6,d4
	addx.l	d5,d3
	addq.l	#2,a4
.1
 IFGE	__CPU-68020
	move.w	(a3,d3.l*2),d7
 ELSE
	move.l	d3,d7
	add.l	d7,d7
	move.w	(a3,d7.l),d7
 ENDC
	asr.w	d1,d7
	add.w	d7,(a4)+
	add.l	d6,d4
	addx.l	d5,d3
	addq.l	#2,a4

	dbf	d0,.nextsample
.exit
	rts

;------------------------------------------------------------------------------
	cnop	0,16

;in
* d2.l	Shift count
AddWordSVTr:
	lsr.w	#1,d0
	bcs.b	.1
	subq.w	#1,d0
	bmi.b	.exit
.nextsample
 IFGE	__CPU-68020
	move.w	(a3,d3.l*2),d7
 ELSE
	move.l	d3,d7
	add.l	d7,d7
	move.w	(a3,d7.l),d7
 ENDC
	asr.w	d2,d7
	addq.l	#2,a4
	add.w	d7,(a4)+
	add.l	d6,d4
	addx.l	d5,d3
.1
 IFGE	__CPU-68020
	move.w	(a3,d3.l*2),d7
 ELSE
	move.l	d3,d7
	add.l	d7,d7
	move.w	(a3,d7.l),d7
 ENDC
	asr.w	d2,d7
	addq.l	#2,a4
	add.w	d7,(a4)+
	add.l	d6,d4
	addx.l	d5,d3
	dbf	d0,.nextsample
.exit
	rts

;------------------------------------------------------------------------------
	cnop	0,16

;in
* d1.l	Shift count
* d2.l	Shift count
AddWordSVPT:
	lsr.w	#1,d0
	bcs.b	.1
	subq.w	#1,d0
	bmi.b	.exit
.nextsample
 IFGE	__CPU-68020
	move.w	(a3,d3.l*2),d7
	asr.w	d1,d7
	add.w	d7,(a4)+
	move.w	(a3,d3.l*2),d7
	asr.w	d2,d7
	add.w	d7,(a4)+
 ELSE
	move.l	d3,d7
	add.l	d7,d7
	move.w	(a3,d7.l),d7
	move.l	d7,a0
	asr.w	d1,d7
	add.w	d7,(a4)+
	move.l	a0,d7
	asr.w	d2,d7
	add.w	d7,(a4)+
 ENDC
	add.l	d6,d4
	addx.l	d5,d3
.1
 IFGE	__CPU-68020
	move.w	(a3,d3.l*2),d7
	asr.w	d1,d7
	add.w	d7,(a4)+
	move.w	(a3,d3.l*2),d7
	asr.w	d2,d7
	add.w	d7,(a4)+
 ELSE
	move.l	d3,d7
	add.l	d7,d7
	move.w	(a3,d7.l),d7
	move.l	d7,a0
	asr.w	d1,d7
	add.w	d7,(a4)+
	move.l	a0,d7
	asr.w	d2,d7
	add.w	d7,(a4)+
 ENDC
	add.l	d6,d4
	addx.l	d5,d3

	dbf	d0,.nextsample
.exit
	rts


;******************************************************************************
	cnop	0,16

;in
* d1.l	-32768..32767
* d2.l	-32768..32767
AddWordsMV:
* 16 bit signed input
 IFGE	__CPU-68020
	lsr.w	#1,d0
	bcs.b	.1
	subq.w	#1,d0
	bmi.b	.exit
.nextsample
	move.w	0(a3,d3.l*4),d7
	muls.w	d1,d7
	asr.l	#8,d7
	asr.l	#7,d7
	add.w	d7,(a4)
	move.w	2(a3,d3.l*4),d7
	muls.w	d2,d7
	asr.l	#8,d7
	asr.l	#7,d7
	add.w	d7,(a4)+
	add.l	d6,d4
	addx.l	d5,d3
.1
	move.w	0(a3,d3.l*4),d7
	muls.w	d1,d7
	asr.l	#8,d7
	asr.l	#7,d7
	add.w	d7,(a4)
	move.w	2(a3,d3.l*4),d7
	muls.w	d2,d7
	asr.l	#8,d7
	asr.l	#7,d7
	add.w	d7,(a4)+
	add.l	d6,d4
	addx.l	d5,d3
	dbf	d0,.nextsample
.exit
 ENDC
	rts

;------------------------------------------------------------------------------
	cnop	0,16

;in
* d1.l	Shift Count
* d2.l	Shift Count
AddWordsMVT:
 IFGE	__CPU-68020
	lsr.w	#1,d0
	bcs.b	.1
	subq.w	#1,d0
	bmi.b	.exit
.nextsample
	move.w	(a3,d3.l*4),d7
	asr.w	d1,d7
	add.w	d7,(a4)
	move.w	2(a3,d3.l*4),d7
	asr.w	d2,d7
	add.w	d7,(a4)+
	add.l	d6,d4
	addx.l	d5,d3
.1
	move.w	(a3,d3.l*4),d7
	asr.w	d1,d7
	add.w	d7,(a4)
	move.w	2(a3,d3.l*4),d7
	asr.w	d2,d7
	add.w	d7,(a4)+
	add.l	d6,d4
	addx.l	d5,d3

	dbf	d0,.nextsample
.exit
 ENDC
	rts

;------------------------------------------------------------------------------
	cnop	0,16

;in
* d1.l	-32768..32767
AddWordsSVl:
* 16 bit signed input
 IFGE	__CPU-68020
	moveq	#15,d2
	lsr.w	#1,d0
	bcs.b	.1
	subq.w	#1,d0
	bmi.b	.exit
.nextsample
	move.w	0(a3,d3.l*4),d7
	muls.w	d1,d7
	asr.l	d2,d7
	add.w	d7,(a4)+
	add.l	d6,d4
	addx.l	d5,d3
	addq.l	#2,a4
.1
	move.w	0(a3,d3.l*4),d7
	muls.w	d1,d7
	asr.l	d2,d7
	add.w	d7,(a4)+
	add.l	d6,d4
	addx.l	d5,d3
	addq.l	#2,a4
	dbf	d0,.nextsample
.exit
 ENDC
	rts

;------------------------------------------------------------------------------
	cnop	0,16

;in
* d2.l	-32768..32767/
AddWordsSVr:
* 16 bit signed input
 IFGE	__CPU-68020
	moveq	#15,d1

	lsr.w	#1,d0
	bcs.b	.1
	subq.w	#1,d0
	bmi.b	.exit
.nextsample
	move.w	2(a3,d3.l*4),d7
	muls.w	d2,d7
	asr.l	d1,d7
	addq.l	#2,a4
	add.w	d7,(a4)+
	add.l	d6,d4
	addx.l	d5,d3
.1
	move.w	2(a3,d3.l*4),d7
	muls.w	d2,d7
	asr.l	d1,d7
	addq.l	#2,a4
	add.w	d7,(a4)+
	add.l	d6,d4
	addx.l	d5,d3
	dbf	d0,.nextsample
.exit
 ENDC
	rts

;------------------------------------------------------------------------------
	cnop	0,16

;in
* d1.l	-32768..32767/-256..256
* d2.l	-32768..32767/-256..256
AddWordsSVP:
* 16/8 bit signed input (8 for '000 version)
 IFGE	__CPU-68020
	lsr.w	#1,d0
	bcs.b	.1
	subq.w	#1,d0
	bmi.b	.exit
.nextsample
	move.w	0(a3,d3.l*4),d7
	muls.w	d1,d7
	asr.l	#8,d7
	asr.l	#7,d7
	add.w	d7,(a4)+

	move.w	2(a3,d3.l*4),d7
	muls.w	d2,d7
	asr.l	#8,d7
	asr.l	#7,d7
	add.w	d7,(a4)+
	add.l	d6,d4
	addx.l	d5,d3
.1
	move.w	0(a3,d3.l*4),d7
	muls.w	d1,d7
	asr.l	#8,d7
	asr.l	#7,d7
	add.w	d7,(a4)+

	move.w	2(a3,d3.l*4),d7
	muls.w	d2,d7
	asr.l	#8,d7
	asr.l	#7,d7
	add.w	d7,(a4)+
	add.l	d6,d4
	addx.l	d5,d3
	dbf	d0,.nextsample
.exit
 ENDC
	rts

;------------------------------------------------------------------------------
	cnop	0,16

;in
* d1.l	Shift count
AddWordsSVTl:
 IFGE	__CPU-68020
	lsr.w	#1,d0
	bcs.b	.1
	subq.w	#1,d0
	bmi.b	.exit
.nextsample
	move.w	(a3,d3.l*4),d7
	asr.w	d1,d7
	add.w	d7,(a4)+
	add.l	d6,d4
	addx.l	d5,d3
	addq.l	#2,a4
.1
	move.w	(a3,d3.l*4),d7
	asr.w	d1,d7
	add.w	d7,(a4)+
	add.l	d6,d4
	addx.l	d5,d3
	addq.l	#2,a4

	dbf	d0,.nextsample
.exit
 ENDC
	rts

;------------------------------------------------------------------------------
	cnop	0,16

;in
* d2.l	Shift count
AddWordsSVTr:
 IFGE	__CPU-68020
	lsr.w	#1,d0
	bcs.b	.1
	subq.w	#1,d0
	bmi.b	.exit
.nextsample
	move.w	2(a3,d3.l*4),d7
	asr.w	d2,d7
	addq.l	#2,a4
	add.w	d7,(a4)+
	add.l	d6,d4
	addx.l	d5,d3
.1
	move.w	2(a3,d3.l*4),d7
	asr.w	d2,d7
	addq.l	#2,a4
	add.w	d7,(a4)+
	add.l	d6,d4
	addx.l	d5,d3

	dbf	d0,.nextsample
.exit
 ENDC
	rts

;------------------------------------------------------------------------------
	cnop	0,16

;in
* d1.l	Shift count
* d2.l	Shift count
AddWordsSVPT:
 IFGE	__CPU-68020
	lsr.w	#1,d0
	bcs.b	.1
	subq.w	#1,d0
	bmi.b	.exit
.nextsample
	move.w	(a3,d3.l*4),d7
	asr.w	d1,d7
	add.w	d7,(a4)+
	move.w	2(a3,d3.l*4),d7
	asr.w	d2,d7
	add.w	d7,(a4)+
	add.l	d6,d4
	addx.l	d5,d3
.1
	move.w	(a3,d3.l*4),d7
	asr.w	d1,d7
	add.w	d7,(a4)+
	move.w	2(a3,d3.l*4),d7
	asr.w	d2,d7
	add.w	d7,(a4)+
	add.l	d6,d4
	addx.l	d5,d3

	dbf	d0,.nextsample
.exit
 ENDC
	rts

;******************************************************************************
;**** Backwards ***************************************************************
;******************************************************************************

	cnop	0,16

;in
* d1.w	-256..256
AddByteMVB:
	lsr.w	#1,d0
	bcs.b	.1
	subq.w	#1,d0
	bmi.b	.exit
.nextsample
	move.b	(a3,d3.l),d7
	ext.w	d7
	muls.w	d1,d7
	add.w	d7,(a4)+
	sub.l	d6,d4
	subx.l	d5,d3
.1
	move.b	(a3,d3.l),d7
	ext.w	d7
	muls.w	d1,d7
	add.w	d7,(a4)+
	sub.l	d6,d4
	subx.l	d5,d3
	dbf	d0,.nextsample
.exit
	rts

;------------------------------------------------------------------------------
	cnop	0,16

;in
* d1.l	Pointer in multiplication table
AddByteMVTB:
	move.l	d1,a0
	moveq	#0,d1
	lsr.w	#1,d0
	bcs.b	.1
	subq.w	#1,d0
	bmi.b	.exit
.nextsample
	move.b	(a3,d3.l),d1
 IFGE	__CPU-68020
	move.w	0(a0,d1.w*4),d2		;signed multiplication
 ELSE
	add.w	d1,d1
	add.w	d1,d1
	move.w	0(a0,d1.w),d2		;signed multiplication
	moveq	#0,d1
 ENDC
	add.w	d2,(a4)+
	sub.l	d6,d4
	subx.l	d5,d3
.1
	move.b	(a3,d3.l),d1
 IFGE	__CPU-68020
	move.w	0(a0,d1.w*4),d2		;signed multiplication
 ELSE
	add.w	d1,d1
	add.w	d1,d1
	move.w	0(a0,d1.w),d2		;signed multiplication
	moveq	#0,d1
 ENDC
	add.w	d2,(a4)+
	sub.l	d6,d4
	subx.l	d5,d3

	dbf	d0,.nextsample
.exit
	rts

;------------------------------------------------------------------------------
	cnop	0,16

;in
* d1.w	-256..256
AddByteSVlB:
	lsr.w	#1,d0
	bcs.b	.1
	subq.w	#1,d0
	bmi.b	.exit
.nextsample
	move.b	(a3,d3.l),d7
	ext.w	d7
	muls.w	d1,d7
	add.w	d7,(a4)+
	sub.l	d6,d4
	subx.l	d5,d3
	addq.l	#2,a4
.1
	move.b	(a3,d3.l),d7
	ext.w	d7
	muls.w	d1,d7
	add.w	d7,(a4)+
	sub.l	d6,d4
	subx.l	d5,d3
	addq.l	#2,a4
	dbf	d0,.nextsample
.exit
	rts

;------------------------------------------------------------------------------
	cnop	0,16

;in
* d2.w	-256..256
AddByteSVrB:
	moveq	#0,d7
	lsr.w	#1,d0
	bcs.b	.1
	subq.w	#1,d0
	bmi.b	.exit
.nextsample
	move.b	(a3,d3.l),d7
	addq.l	#2,a4
	ext.w	d7
	muls.w	d2,d7
	add.w	d7,(a4)+
	sub.l	d6,d4
	subx.l	d5,d3
.1
	move.b	(a3,d3.l),d7
	addq.l	#2,a4
	ext.w	d7
	muls.w	d2,d7
	add.w	d7,(a4)+
	sub.l	d6,d4
	subx.l	d5,d3
	dbf	d0,.nextsample
.exit
	rts

;------------------------------------------------------------------------------
	cnop	0,16

;in
* d1.w	-256..256
* d2.w	-256..256
AddByteSVPB:
	lsr.w	#1,d0
	bcs.b	.1
	subq.w	#1,d0
	bmi.b	.exit
.nextsample
	move.b	(a3,d3.l),d7
	ext.w	d7
	move.w	d7,a0
	muls.w	d1,d7
	add.w	d7,(a4)+
	move.w	a0,d7
	muls.w	d2,d7
	add.w	d7,(a4)+
	sub.l	d6,d4
	subx.l	d5,d3
.1
	move.b	(a3,d3.l),d7
	ext.w	d7
	move.w	d7,a0
	muls.w	d1,d7
	add.w	d7,(a4)+
	move.w	a0,d7
	muls.w	d2,d7
	add.w	d7,(a4)+
	sub.l	d6,d4
	subx.l	d5,d3
	dbf	d0,.nextsample
.exit
	rts

;------------------------------------------------------------------------------
	cnop	0,16

;in
* d1.l	Pointer in multiplication table
AddByteSVTlB:
	move.l	d1,a0
	moveq	#0,d1
	lsr.w	#1,d0
	bcs.b	.1
	subq.w	#1,d0
	bmi.b	.exit
.nextsample
	move.b	(a3,d3.l),d1
 IFGE	__CPU-68020
	move.w	0(a0,d1.w*4),d2		;signed multiplication
 ELSE
	add.w	d1,d1
	add.w	d1,d1
	move.w	0(a0,d1.w),d2		;signed multiplication
	moveq	#0,d1
 ENDC
	add.w	d2,(a4)+
	sub.l	d6,d4
	subx.l	d5,d3
	addq.l	#2,a4
.1
	move.b	(a3,d3.l),d1
 IFGE	__CPU-68020
	move.w	0(a0,d1.w*4),d2		;signed multiplication
 ELSE
	add.w	d1,d1
	add.w	d1,d1
	move.w	0(a0,d1.w),d2		;signed multiplication
	moveq	#0,d1
 ENDC
	add.w	d2,(a4)+
	sub.l	d6,d4
	subx.l	d5,d3
	addq.l	#2,a4
	dbf	d0,.nextsample
.exit
	rts
;------------------------------------------------------------------------------
	cnop	0,16

;in
* d2.l	Pointer in multiplication table
AddByteSVTrB:
	move.l	d2,a0
	moveq	#0,d1
	moveq	#0,d2
	lsr.w	#1,d0
	bcs.b	.1
	subq.w	#1,d0
	bmi.b	.exit
.nextsample
	move.b	(a3,d3.l),d1
 IFGE	__CPU-68020
	move.w	0(a0,d1.w*4),d2		;signed multiplication
 ELSE
	add.w	d1,d1
	add.w	d1,d1
	move.w	0(a0,d1.w),d2		;signed multiplication
	moveq	#0,d1
 ENDC
	addq.l	#2,a4
	add.w	d2,(a4)+
	sub.l	d6,d4
	subx.l	d5,d3
.1
	move.b	(a3,d3.l),d1
 IFGE	__CPU-68020
	move.w	0(a0,d1.w*4),d2		;signed multiplication
 ELSE
	add.w	d1,d1
	add.w	d1,d1
	move.w	0(a0,d1.w),d2		;signed multiplication
	moveq	#0,d1
 ENDC
	addq.l	#2,a4
	add.w	d2,(a4)+
	sub.l	d6,d4
	subx.l	d5,d3
	dbf	d0,.nextsample
.exit
	rts

;------------------------------------------------------------------------------
	cnop	0,16

;in
* d1.l	Pointer in multiplication table
* d2.l	Pointer in multiplication table
AddByteSVPTB:
	move.l	a1,d7			;save a1
	move.l	d1,a0
	move.l	d2,a1
	moveq	#0,d1
	lsr.w	#1,d0
	bcs.b	.1
	subq.w	#1,d0
	bmi.b	.exit
.nextsample
	move.b	(a3,d3.l),d1
 IFGE	__CPU-68020
	move.w	0(a0,d1.w*4),d2
 ELSE
	add.w	d1,d1
	add.w	d1,d1
	move.w	0(a0,d1.w),d2
 ENDC
	add.w	d2,(a4)+

 IFGE	__CPU-68020
	move.w	0(a1,d1.w*4),d2
 ELSE
	move.w	0(a1,d1.w),d2
	moveq	#0,d1
 ENDC
	add.w	d2,(a4)+
	sub.l	d6,d4
	subx.l	d5,d3
.1
	move.b	(a3,d3.l),d1
 IFGE	__CPU-68020
	move.w	0(a0,d1.w*4),d2
 ELSE
	add.w	d1,d1
	add.w	d1,d1
	move.w	0(a0,d1.w),d2
 ENDC
	add.w	d2,(a4)+

 IFGE	__CPU-68020
	move.w	0(a1,d1.w*4),d2
 ELSE
	move.w	0(a1,d1.w),d2
	moveq	#0,d1
 ENDC
	add.w	d2,(a4)+
	sub.l	d6,d4
	subx.l	d5,d3
	dbf	d0,.nextsample
.exit
	move.l	d7,a1			;restore a1
	rts

;******************************************************************************
	cnop	0,16

;in
* d1.w	-256..256
* d2.w	-256..256
AddBytesMVB:
 IFGE	__CPU-68020
	lsr.w	#1,d0
	bcs.b	.1
	subq.w	#1,d0
	bmi.b	.exit
.nextsample
	move.b	0(a3,d3.l*2),d7
	ext.w	d7
	muls.w	d1,d7
	add.w	d7,(a4)
	move.b	1(a3,d3.l*2),d7
	ext.w	d7
	muls.w	d2,d7
	add.w	d7,(a4)+
	sub.l	d6,d4
	subx.l	d5,d3
.1
	move.b	0(a3,d3.l*2),d7
	ext.w	d7
	muls.w	d1,d7
	add.w	d7,(a4)
	move.b	1(a3,d3.l*2),d7
	ext.w	d7
	muls.w	d2,d7
	add.w	d7,(a4)+
	sub.l	d6,d4
	subx.l	d5,d3
	dbf	d0,.nextsample
.exit
 ENDC
	rts

;------------------------------------------------------------------------------
	cnop	0,16

;in
* d1.l	Pointer in multiplication table
* d2.l	Pointer in multiplication table
AddBytesMVTB:
 IFGE	__CPU-68020
	push	a1
	move.l	d1,a0
	move.l	d2,a1
	moveq	#0,d1
	lsr.w	#1,d0
	bcs.b	.1
	subq.w	#1,d0
	bmi.b	.exit
.nextsample
	move.b	0(a3,d3.l*2),d1
	move.w	0(a0,d1.w*4),d2		;signed multiplication
	move.b	1(a3,d3.l*2),d1
	add.w	0(a1,d1.w*4),d2		;signed multiplication
	add.w	d2,(a4)+
	sub.l	d6,d4
	subx.l	d5,d3
.1
	move.b	0(a3,d3.l*2),d1
	move.w	0(a0,d1.w*4),d2		;signed multiplication
	move.b	1(a3,d3.l*2),d1
	add.w	0(a1,d1.w*4),d2		;signed multiplication
	add.w	d2,(a4)+
	sub.l	d6,d4
	subx.l	d5,d3

	dbf	d0,.nextsample
.exit
	pop	a1
 ENDC
	rts

;------------------------------------------------------------------------------
	cnop	0,16

;in
* d1.w	-256..256
AddBytesSVlB:
 IFGE	__CPU-68020
	lsr.w	#1,d0
	bcs.b	.1
	subq.w	#1,d0
	bmi.b	.exit
.nextsample
	move.b	0(a3,d3.l*2),d7
	ext.w	d7
	muls.w	d1,d7
	add.w	d7,(a4)+
	sub.l	d6,d4
	subx.l	d5,d3
	addq.l	#2,a4
.1
	move.b	0(a3,d3.l*2),d7
	ext.w	d7
	muls.w	d1,d7
	add.w	d7,(a4)+
	sub.l	d6,d4
	subx.l	d5,d3
	addq.l	#2,a4
	dbf	d0,.nextsample
.exit
 ENDC
	rts

;------------------------------------------------------------------------------
	cnop	0,16

;in
* d2.w	-256..256
AddBytesSVrB:
 IFGE	__CPU-68020
	moveq	#0,d7
	lsr.w	#1,d0
	bcs.b	.1
	subq.w	#1,d0
	bmi.b	.exit
.nextsample
	move.b	1(a3,d3.l*2),d7
	addq.l	#2,a4
	ext.w	d7
	muls.w	d2,d7
	add.w	d7,(a4)+
	sub.l	d6,d4
	subx.l	d5,d3
.1
	move.b	1(a3,d3.l*2),d7
	addq.l	#2,a4
	ext.w	d7
	muls.w	d2,d7
	add.w	d7,(a4)+
	sub.l	d6,d4
	subx.l	d5,d3
	dbf	d0,.nextsample
.exit
 ENDC
	rts

;------------------------------------------------------------------------------
	cnop	0,16

;in
* d1.w	-256..256
* d2.w	-256..256
AddBytesSVPB:
 IFGE	__CPU-68020
	lsr.w	#1,d0
	bcs.b	.1
	subq.w	#1,d0
	bmi.b	.exit
.nextsample
	move.b	0(a3,d3.l*2),d7
	ext.w	d7
	muls.w	d1,d7
	add.w	d7,(a4)+
	move.b	1(a3,d3.l*2),d7
	ext.w	d7
	muls.w	d2,d7
	add.w	d7,(a4)+
	sub.l	d6,d4
	subx.l	d5,d3
.1
	move.b	0(a3,d3.l*2),d7
	ext.w	d7
	muls.w	d1,d7
	add.w	d7,(a4)+
	move.b	1(a3,d3.l*2),d7
	ext.w	d7
	muls.w	d2,d7
	add.w	d7,(a4)+
	sub.l	d6,d4
	subx.l	d5,d3
	dbf	d0,.nextsample
.exit
 ENDC
	rts

;------------------------------------------------------------------------------
	cnop	0,16

;in
* d1.l	Pointer in multiplication table
AddBytesSVTlB:
 IFGE	__CPU-68020
	move.l	d1,a0
	moveq	#0,d1
	lsr.w	#1,d0
	bcs.b	.1
	subq.w	#1,d0
	bmi.b	.exit
.nextsample
	move.b	0(a3,d3.l*2),d1
	move.w	0(a0,d1.w*4),d2		;signed multiplication
	add.w	d2,(a4)+
	sub.l	d6,d4
	subx.l	d5,d3
	addq.l	#2,a4
.1
	move.b	0(a3,d3.l*2),d1
	move.w	0(a0,d1.w*4),d2		;signed multiplication
	add.w	d2,(a4)+
	sub.l	d6,d4
	subx.l	d5,d3
	addq.l	#2,a4
	dbf	d0,.nextsample
.exit
 ENDC
	rts
;------------------------------------------------------------------------------
	cnop	0,16

;in
* d2.l	Pointer in multiplication table
AddBytesSVTrB:
 IFGE	__CPU-68020
	move.l	d2,a0
	moveq	#0,d1
	moveq	#0,d2
	lsr.w	#1,d0
	bcs.b	.1
	subq.w	#1,d0
	bmi.b	.exit
.nextsample
	move.b	1(a3,d3.l*2),d1
	move.w	0(a0,d1.w*4),d2		;signed multiplication
	addq.l	#2,a4
	add.w	d2,(a4)+
	sub.l	d6,d4
	subx.l	d5,d3
.1
	move.b	1(a3,d3.l*2),d1
	move.w	0(a0,d1.w*4),d2		;signed multiplication
	addq.l	#2,a4
	add.w	d2,(a4)+
	sub.l	d6,d4
	subx.l	d5,d3
	dbf	d0,.nextsample
.exit
 ENDC
	rts

;------------------------------------------------------------------------------
	cnop	0,16

;in
* d1.l	Pointer in multiplication table
* d2.l	Pointer in multiplication table
AddBytesSVPTB:
 IFGE	__CPU-68020
	move.l	a1,d7			;save a1
	move.l	d1,a0
	move.l	d2,a1
	moveq	#0,d1
	lsr.w	#1,d0
	bcs.b	.1
	subq.w	#1,d0
	bmi.b	.exit
.nextsample
	move.b	0(a3,d3.l*2),d1
	move.w	0(a0,d1.w*4),d2
	add.w	d2,(a4)+
	move.b	1(a3,d3.l*2),d1
	move.w	0(a1,d1.w*4),d2
	add.w	d2,(a4)+
	sub.l	d6,d4
	subx.l	d5,d3
.1
	move.b	0(a3,d3.l*2),d1
	move.w	0(a0,d1.w*4),d2
	add.w	d2,(a4)+
	move.b	1(a3,d3.l*2),d1
	move.w	0(a1,d1.w*4),d2
	add.w	d2,(a4)+
	sub.l	d6,d4
	subx.l	d5,d3
	dbf	d0,.nextsample
.exit
	move.l	d7,a1			;restore a1
 ENDC
	rts

;******************************************************************************
	cnop	0,16

;in
* d1.l	-32768..32767/-256..256
AddWordMVB:
* 16/8 bit signed input (8 for '000 version)
	moveq	#15,d2

	lsr.w	#1,d0
	bcs.b	.1
	subq.w	#1,d0
	bmi.b	.exit
.nextsample
 IFGE	__CPU-68020
	move.w	(a3,d3.l*2),d7
	muls.w	d1,d7
	asr.l	d2,d7
 ELSE
	move.l	d3,d7
	add.l	d7,d7
	move.b	0(a3,d7.l),d7		;high byte
	ext.w	d7
	muls.w	d1,d7
 ENDC
	add.w	d7,(a4)+
	sub.l	d6,d4
	subx.l	d5,d3
.1
 IFGE	__CPU-68020
	move.w	(a3,d3.l*2),d7
	muls.w	d1,d7
	asr.l	d2,d7
 ELSE
	move.l	d3,d7
	add.l	d7,d7
	move.b	0(a3,d7.l),d7		;high byte
	ext.w	d7
	muls.w	d1,d7
 ENDC
	add.w	d7,(a4)+
	sub.l	d6,d4
	subx.l	d5,d3
	dbf	d0,.nextsample
.exit
	rts

;------------------------------------------------------------------------------
	cnop	0,16

;in
* d1.l	Shift count
AddWordMVTB:
	lsr.w	#1,d0
	bcs.b	.1
	subq.w	#1,d0
	bmi.b	.exit
.nextsample
 IFGE	__CPU-68020
	move.w	(a3,d3.l*2),d7
 ELSE
	move.l	d3,d7
 	add.l	d7,d7
	move.w	(a3,d7.l),d7
 ENDC
	asr.w	d1,d7
	add.w	d7,(a4)+
	sub.l	d6,d4
	subx.l	d5,d3
.1
 IFGE	__CPU-68020
	move.w	(a3,d3.l*2),d7
 ELSE
	move.l	d3,d7
 	add.l	d7,d7
	move.w	(a3,d7.l),d7
 ENDC
	asr.w	d1,d7
	add.w	d7,(a4)+
	sub.l	d6,d4
	subx.l	d5,d3
	dbf	d0,.nextsample
.exit
	rts

;------------------------------------------------------------------------------
	cnop	0,16

;in
* d1.l	-32768..32767/-256..256
AddWordSVlB:
* 16/8 bit signed input (8 for '000 version)
	moveq	#15,d2
	lsr.w	#1,d0
	bcs.b	.1
	subq.w	#1,d0
	bmi.b	.exit
.nextsample
 IFGE	__CPU-68020
	move.w	(a3,d3.l*2),d7
	muls.w	d1,d7
	asr.l	d2,d7
 ELSE
	move.l	d3,d7
	add.l	d7,d7
	move.b	0(a3,d7.l),d7		;high byte
	ext.w	d7
	muls.w	d1,d7
 ENDC
	add.w	d7,(a4)+
	sub.l	d6,d4
	subx.l	d5,d3
	addq.l	#2,a4
.1
 IFGE	__CPU-68020
	move.w	(a3,d3.l*2),d7
	muls.w	d1,d7
	asr.l	d2,d7
 ELSE
	move.l	d3,d7
	add.l	d7,d7
	move.b	0(a3,d7.l),d7		;high byte
	ext.w	d7
	muls.w	d1,d7
 ENDC
	add.w	d7,(a4)+
	sub.l	d6,d4
	subx.l	d5,d3
	addq.l	#2,a4
	dbf	d0,.nextsample
.exit
	rts

;------------------------------------------------------------------------------
	cnop	0,16

;in
* d2.l	-32768..32767/-256..256
AddWordSVrB:
* 16/8 bit signed input (8 for '000 version)
	moveq	#15,d1

	lsr.w	#1,d0
	bcs.b	.1
	subq.w	#1,d0
	bmi.b	.exit
.nextsample
 IFGE	__CPU-68020
	move.w	(a3,d3.l*2),d7
	muls.w	d2,d7
	asr.l	d1,d7
 ELSE
	move.l	d3,d7
	add.l	d7,d7
	move.b	0(a3,d7.l),d7		;high byte
	ext.w	d7
	muls.w	d2,d7
 ENDC
	addq.l	#2,a4
	add.w	d7,(a4)+
	sub.l	d6,d4
	subx.l	d5,d3
.1
 IFGE	__CPU-68020
	move.w	(a3,d3.l*2),d7
	muls.w	d2,d7
	asr.l	d1,d7
 ELSE
	move.l	d3,d7
	add.l	d7,d7
	move.b	0(a3,d7.l),d7		;high byte
	ext.w	d7
	muls.w	d2,d7
 ENDC
	addq.l	#2,a4
	add.w	d7,(a4)+
	sub.l	d6,d4
	subx.l	d5,d3
	dbf	d0,.nextsample
.exit
	rts

;------------------------------------------------------------------------------
	cnop	0,16

;in
* d1.l	-32768..32767/-256..256
* d2.l	-32768..32767/-256..256
AddWordSVPB:
* 16/8 bit signed input (8 for '000 version)
	lsr.w	#1,d0
	bcs.b	.1
	subq.w	#1,d0
	bmi.b	.exit
.nextsample
 IFGE	__CPU-68020
	move.w	(a3,d3.l*2),d7
	move.l	d7,a0
	muls.w	d1,d7
	asr.l	#8,d7
	asr.l	#7,d7
	add.w	d7,(a4)+

	move.l	a0,d7
	muls.w	d2,d7
	asr.l	#8,d7
	asr.l	#7,d7
	add.w	d7,(a4)+
 ELSE
	move.l	d3,d7
	add.l	d7,d7
	move.b	0(a3,d7.l),d7		;high byte
	ext.w	d7
	move.l	d7,a0
	muls.w	d1,d7
	add.w	d7,(a4)+

	move.l	a0,d7
	muls.w	d2,d7
	add.w	d7,(a4)+
 ENDC
	sub.l	d6,d4
	subx.l	d5,d3
.1
 IFGE	__CPU-68020
	move.w	(a3,d3.l*2),d7
	move.l	d7,a0
	muls.w	d1,d7
	asr.l	#8,d7
	asr.l	#7,d7
	add.w	d7,(a4)+

	move.l	a0,d7
	muls.w	d2,d7
	asr.l	#8,d7
	asr.l	#7,d7
	add.w	d7,(a4)+
 ELSE
	move.l	d3,d7
	add.l	d7,d7
	move.b	0(a3,d7.l),d7		;high byte
	ext.w	d7
	move.l	d7,a0
	muls.w	d1,d7
	add.w	d7,(a4)+

	move.l	a0,d7
	muls.w	d2,d7
	add.w	d7,(a4)+
 ENDC
	sub.l	d6,d4
	subx.l	d5,d3
	dbf	d0,.nextsample
.exit
	rts

;------------------------------------------------------------------------------
	cnop	0,16

;in
* d1.l	Shift count
AddWordSVTlB:
	lsr.w	#1,d0
	bcs.b	.1
	subq.w	#1,d0
	bmi.b	.exit
.nextsample
 IFGE	__CPU-68020
	move.w	(a3,d3.l*2),d7
 ELSE
	move.l	d3,d7
	add.l	d7,d7
	move.w	(a3,d7.l),d7
 ENDC
	asr.w	d1,d7
	add.w	d7,(a4)+
	sub.l	d6,d4
	subx.l	d5,d3
	addq.l	#2,a4
.1
 IFGE	__CPU-68020
	move.w	(a3,d3.l*2),d7
 ELSE
	move.l	d3,d7
	add.l	d7,d7
	move.w	(a3,d7.l),d7
 ENDC
	asr.w	d1,d7
	add.w	d7,(a4)+
	sub.l	d6,d4
	subx.l	d5,d3
	addq.l	#2,a4

	dbf	d0,.nextsample
.exit
	rts

;------------------------------------------------------------------------------
	cnop	0,16

;in
* d2.l	Shift count
AddWordSVTrB:
	moveq	#0,d1
	lsr.w	#1,d0
	bcs.b	.1
	subq.w	#1,d0
	bmi.b	.exit
.nextsample
 IFGE	__CPU-68020
	move.w	(a3,d3.l*2),d7
 ELSE
	move.l	d3,d7
	add.l	d7,d7
	move.w	(a3,d7.l),d7
 ENDC
	asr.w	d2,d7
	addq.l	#2,a4
	add.w	d7,(a4)+
	sub.l	d6,d4
	subx.l	d5,d3
.1
 IFGE	__CPU-68020
	move.w	(a3,d3.l*2),d7
 ELSE
	move.l	d3,d7
	add.l	d7,d7
	move.w	(a3,d7.l),d7
 ENDC
	asr.w	d2,d7
	addq.l	#2,a4
	add.w	d7,(a4)+
	sub.l	d6,d4
	subx.l	d5,d3
	dbf	d0,.nextsample
.exit
	rts

;------------------------------------------------------------------------------
	cnop	0,16

;in
* d1.l	Shift count
* d2.l	Shift count
AddWordSVPTB:
	lsr.w	#1,d0
	bcs.b	.1
	subq.w	#1,d0
	bmi.b	.exit
.nextsample
 IFGE	__CPU-68020
	move.w	(a3,d3.l*2),d7
	asr.w	d1,d7
	add.w	d7,(a4)+
	move.w	(a3,d3.l*2),d7
	asr.w	d2,d7
	add.w	d7,(a4)+
 ELSE
	move.l	d3,d7
	add.l	d7,d7
	move.w	(a3,d7.l),d7
	move.l	d7,a0
	asr.w	d1,d7
	add.w	d7,(a4)+
	move.l	a0,d7
	asr.w	d2,d7
	add.w	d7,(a4)+
 ENDC
	sub.l	d6,d4
	subx.l	d5,d3
.1
 IFGE	__CPU-68020
	move.w	(a3,d3.l*2),d7
	asr.w	d1,d7
	add.w	d7,(a4)+
	move.w	(a3,d3.l*2),d7
	asr.w	d2,d7
	add.w	d7,(a4)+
 ELSE
	move.l	d3,d7
	add.l	d7,d7
	move.w	(a3,d7.l),d7
	move.l	d7,a0
	asr.w	d1,d7
	add.w	d7,(a4)+
	move.l	a0,d7
	asr.w	d2,d7
	add.w	d7,(a4)+
 ENDC
	sub.l	d6,d4
	subx.l	d5,d3

	dbf	d0,.nextsample
.exit
	rts


;******************************************************************************
	cnop	0,16

;in
* d1.l	-32768..32767
* d2.l	-32768..32767
AddWordsMVB:
* 16 bit signed input
 IFGE	__CPU-68020
	lsr.w	#1,d0
	bcs.b	.1
	subq.w	#1,d0
	bmi.b	.exit
.nextsample
	move.w	0(a3,d3.l*4),d7
	muls.w	d1,d7
	asr.l	#8,d7
	asr.l	#7,d7
	add.w	d7,(a4)
	move.w	2(a3,d3.l*4),d7
	muls.w	d2,d7
	asr.l	#8,d7
	asr.l	#7,d7
	add.w	d7,(a4)+
	sub.l	d6,d4
	subx.l	d5,d3
.1
	move.w	0(a3,d3.l*4),d7
	muls.w	d1,d7
	asr.l	#8,d7
	asr.l	#7,d7
	add.w	d7,(a4)
	move.w	2(a3,d3.l*4),d7
	muls.w	d2,d7
	asr.l	#8,d7
	asr.l	#7,d7
	add.w	d7,(a4)+
	sub.l	d6,d4
	subx.l	d5,d3
	dbf	d0,.nextsample
.exit
 ENDC
	rts

;------------------------------------------------------------------------------
	cnop	0,16

;in
* d1.l	Shift Count
* d2.l	Shift Count
AddWordsMVTB:
 IFGE	__CPU-68020
	lsr.w	#1,d0
	bcs.b	.1
	subq.w	#1,d0
	bmi.b	.exit
.nextsample
	move.w	(a3,d3.l*4),d7
	asr.w	d1,d7
	add.w	d7,(a4)
	move.w	2(a3,d3.l*4),d7
	asr.w	d2,d7
	add.w	d7,(a4)+
	sub.l	d6,d4
	subx.l	d5,d3
.1
	move.w	(a3,d3.l*4),d7
	asr.w	d1,d7
	add.w	d7,(a4)
	move.w	2(a3,d3.l*4),d7
	asr.w	d2,d7
	add.w	d7,(a4)+
	sub.l	d6,d4
	subx.l	d5,d3

	dbf	d0,.nextsample
.exit
 ENDC
	rts

;------------------------------------------------------------------------------
	cnop	0,16

;in
* d1.l	-32768..32767
AddWordsSVlB:
* 16 bit signed input
 IFGE	__CPU-68020
	moveq	#15,d2
	lsr.w	#1,d0
	bcs.b	.1
	subq.w	#1,d0
	bmi.b	.exit
.nextsample
	move.w	0(a3,d3.l*4),d7
	muls.w	d1,d7
	asr.l	d2,d7
	add.w	d7,(a4)+
	sub.l	d6,d4
	subx.l	d5,d3
	addq.l	#2,a4
.1
	move.w	0(a3,d3.l*4),d7
	muls.w	d1,d7
	asr.l	d2,d7
	add.w	d7,(a4)+
	sub.l	d6,d4
	subx.l	d5,d3
	addq.l	#2,a4
	dbf	d0,.nextsample
.exit
 ENDC
	rts

;------------------------------------------------------------------------------
	cnop	0,16

;in
* d2.l	-32768..32767/
AddWordsSVrB:
* 16 bit signed input
 IFGE	__CPU-68020
	moveq	#15,d1

	lsr.w	#1,d0
	bcs.b	.1
	subq.w	#1,d0
	bmi.b	.exit
.nextsample
	move.w	2(a3,d3.l*4),d7
	muls.w	d2,d7
	asr.l	d1,d7
	addq.l	#2,a4
	add.w	d7,(a4)+
	sub.l	d6,d4
	subx.l	d5,d3
.1
	move.w	2(a3,d3.l*4),d7
	muls.w	d2,d7
	asr.l	d1,d7
	addq.l	#2,a4
	add.w	d7,(a4)+
	sub.l	d6,d4
	subx.l	d5,d3
	dbf	d0,.nextsample
.exit
 ENDC
	rts

;------------------------------------------------------------------------------
	cnop	0,16

;in
* d1.l	-32768..32767/-256..256
* d2.l	-32768..32767/-256..256
AddWordsSVPB:
* 16/8 bit signed input (8 for '000 version)
 IFGE	__CPU-68020
	lsr.w	#1,d0
	bcs.b	.1
	subq.w	#1,d0
	bmi.b	.exit
.nextsample
	move.w	0(a3,d3.l*4),d7
	muls.w	d1,d7
	asr.l	#8,d7
	asr.l	#7,d7
	add.w	d7,(a4)+

	move.w	2(a3,d3.l*4),d7
	muls.w	d2,d7
	asr.l	#8,d7
	asr.l	#7,d7
	add.w	d7,(a4)+
	sub.l	d6,d4
	subx.l	d5,d3
.1
	move.w	0(a3,d3.l*4),d7
	muls.w	d1,d7
	asr.l	#8,d7
	asr.l	#7,d7
	add.w	d7,(a4)+

	move.w	2(a3,d3.l*4),d7
	muls.w	d2,d7
	asr.l	#8,d7
	asr.l	#7,d7
	add.w	d7,(a4)+
	sub.l	d6,d4
	subx.l	d5,d3
	dbf	d0,.nextsample
.exit
 ENDC
	rts

;------------------------------------------------------------------------------
	cnop	0,16

;in
* d1.l	Shift count
AddWordsSVTlB:
 IFGE	__CPU-68020
	lsr.w	#1,d0
	bcs.b	.1
	subq.w	#1,d0
	bmi.b	.exit
.nextsample
	move.w	(a3,d3.l*4),d7
	asr.w	d1,d7
	add.w	d7,(a4)+
	sub.l	d6,d4
	subx.l	d5,d3
	addq.l	#2,a4
.1
	move.w	(a3,d3.l*4),d7
	asr.w	d1,d7
	add.w	d7,(a4)+
	sub.l	d6,d4
	subx.l	d5,d3
	addq.l	#2,a4

	dbf	d0,.nextsample
.exit
 ENDC
	rts

;------------------------------------------------------------------------------
	cnop	0,16

;in
* d2.l	Shift count
AddWordsSVTrB:
 IFGE	__CPU-68020
	lsr.w	#1,d0
	bcs.b	.1
	subq.w	#1,d0
	bmi.b	.exit
.nextsample
	move.w	2(a3,d3.l*4),d7
	asr.w	d2,d7
	addq.l	#2,a4
	add.w	d7,(a4)+
	sub.l	d6,d4
	subx.l	d5,d3
.1
	move.w	2(a3,d3.l*4),d7
	asr.w	d2,d7
	addq.l	#2,a4
	add.w	d7,(a4)+
	sub.l	d6,d4
	subx.l	d5,d3

	dbf	d0,.nextsample
.exit
 ENDC
	rts

;------------------------------------------------------------------------------
	cnop	0,16

;in
* d1.l	Shift count
* d2.l	Shift count
AddWordsSVPTB:
 IFGE	__CPU-68020
	lsr.w	#1,d0
	bcs.b	.1
	subq.w	#1,d0
	bmi.b	.exit
.nextsample
	move.w	(a3,d3.l*4),d7
	asr.w	d1,d7
	add.w	d7,(a4)+
	move.w	2(a3,d3.l*4),d7
	asr.w	d2,d7
	add.w	d7,(a4)+
	sub.l	d6,d4
	subx.l	d5,d3
.1
	move.w	(a3,d3.l*4),d7
	asr.w	d1,d7
	add.w	d7,(a4)+
	move.w	2(a3,d3.l*4),d7
	asr.w	d2,d7
	add.w	d7,(a4)+
	sub.l	d6,d4
	subx.l	d5,d3

	dbf	d0,.nextsample
.exit
 ENDC
	rts

;------------------------------------------------------------------------------
