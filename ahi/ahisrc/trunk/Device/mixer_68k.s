* $Id$
* $Log$
* Revision 1.13  1997/03/24 18:03:10  lcs
* First steps for AHIST_INPUT
*
* Revision 1.11  1997/03/22 18:58:07  lcs
* --background-- updated + some work on dspecho
*
* Revision 1.9  1997/02/18 22:26:49  lcs
* Faster mixing routines for 16 bit samples when using tables.
*
* Revision 1.8  1997/02/01 21:54:53  lcs
* Max freq. for AHI_SetFreq() uncreased to more than one million! ;)
*
* Revision 1.7  1997/02/01 19:44:18  lcs
* Added stereo samples
*
* Revision 1.6  1997/01/27 01:37:17  lcs
* Even more bugs in the 16 bit routines found (68k version this time).
*
* Revision 1.5  1997/01/27 00:27:02  lcs
* Fixed a bug in the 16 bit routines (was lsr instead of asr)
*
* Revision 1.4  1997/01/15 14:59:50  lcs
* Removed most of the unsigned addroutines
*
* Revision 1.3  1997/01/04 20:25:02  lcs
* ...I forgot: AHIET_CHANNELINFO effect added
*
* Revision 1.2  1997/01/04 20:19:56  lcs
* Changed the AHI_DEBUG levels
*
* Revision 1.1  1996/12/21 13:05:12  lcs
* Initial revision
*


 IFD	_PHXASS_

DEBUG	EQU	0	;no debug 
ALIGN	EQU	0	;move code to even 128 bit boundary

 ELSE

DEBUG	EQU	1	;debug when using asmone!
ALIGN	EQU	0	;set to 0 when using the source level debugger, and 1 when timing
 ENDC

	include	exec/exec.i
	include	lvo/exec_lib.i
	include	devices/ahi.i
	include	utility/hooks.i
	include	lvo/utility_lib.i
	include	ahi_def.i
	include	dsp.i

	XDEF	initcode

	XDEF	_initSignedTable
	XDEF	calcSignedTable
	XDEF	_initUnsignedTable
	XDEF	calcUnsignedTable
	XDEF	SelectAddRoutine
	XDEF	_Mix
	XDEF	CalcSamples

	XREF	_UtilityBase
	XREF	_Fixed2Shift
	XREF	_UDivMod64

TABLEMAXVOL	EQU	32
TABLESHIFT	EQU	11	(TABLEMAXVOL<<TABLESHIFT == 0x10000)

;-----------------------------------------

 IFNE	DEBUG
SAMPLES	=30
BUFFER	=(SAMPLES*2+7)&(~8)

 	include	devices/timer.i
 	include	lvo/timer_lib.i
J:
 	base	exec
	bsr	initcode
	call	CacheClearU

	lea	audioctrl(pc),a2
	move.l	#AHIACF_STEREO|AHIACF_HIFI,ahiac_Flags(a2)
	move.w	#1,ahiac_Channels(a2)
	move.w	ahiac_Channels(a2),ahiac_Channels2(a2)
	move.l	#SAMPLES,ahiac_BuffSamples(a2)
	move.l	#BUFFER,ahiac_BuffSize(a2)
	move.l	ahiac_BuffSize(a2),ahiac_BuffSizeNow(a2)
	move.l	#$10000,ahiac_MasterVolume(a2)
	move.l	#channeldatas,ahiac_ChannelDatas(a2)
	move.l	#soundfunc,ahiac_SoundFunc(a2)
	bsr	_initSignedTable
	bsr	_initUnsignedTable

	move.l	ahiac_ChannelDatas(a2),a5

	move.l	cd_AddI(a5),d0
	move.w	cd_AddF(a5),d1
	move.l	cd_Type(a5),d2
	move.l	cd_LastOffsetI(a5),d3
	move.w	cd_LastOffsetF(a5),d4
	move.l	cd_OffsetI(a5),d5
	move.w	cd_OffsetF(a5),d6
	bsr	CalcSamples
	move.l	d0,cd_Samples(a5)

;	lea	buffer(pc),a1
;	bsr	Mix

;	move.l	#$10000,d0
;	move.l	#$10000,d1
;	lea	audioctrl(pc),a2
;;	bsr	FixVolWordSVPH
;;	add.w	(a0),a0
;	move.l	#AHIST_M8S,d2
;	bsr	SelectAddRoutine
;	move.l	d2,a0
;
;	move.l	d1,d2
;	move.l	d0,d1
;	move.l	#SAMPLES,d0
;	moveq	#0,d3
;	moveq	#0,d4
;	moveq	#0,d5
;	move.l	#$4000,d6
;	lea	audioctrl(pc),a2
;	lea	sample(pc),a3
;	lea	buffer(pc),a4
;
;	base	exec
;	call	Disable
;
;	pushm	d0-d1
;	bsr	ReadEClock
;	move.l	d0,eclock1
;	popm	d0-d1
;
;	jsr	(a0)
;
;	bsr	ReadEClock
;	move.l	d0,eclock2
;
;	base	exec
;	call	Enable

exit:
	base	exec

	lea	audioctrl(pc),a2
	move.l	ahiac_MultTableS(a2),a1
	clr.l	ahiac_MultTableS(a2)
	call	FreeVec

	move.l	ahiac_MultTableU(a2),a1
	clr.l	ahiac_MultTableU(a2)
	call	FreeVec
	move.l	eclock1,d0
	sub.l	eclock2,d0
	bpl.b	.ok
	add.l	#65536,d0
.ok
	lsr.l	#4,d0
	rts

audioctrl:
	blk.l	AHIPrivAudioCtrl_SIZEOF,0
eclock1	dc.l	0
eclock2	dc.l	0

ReadEClock:
	moveq	#0,d0
	move.b	$bfe701,d0
	move.b	$bfe601,d1
	cmp.b	$bfe701,d0
	bne.b	ReadEClock
	lsl.w	#8,d0
	move.b	d1,d0
	rts

soundfunc:
	dc.l	0,0
	dc.l	.func
	dc.l	0,0
.func
	rts

BACK=1
* AHIChannelData (private)
channeldatas:
 IFEQ BACK
	dc.w	0		;UWORD	cd_EOS
	dc.b	$ff		;UBYTE	cd_FreqOK
	dc.b	$ff		;UBYTE	cd_SoundOK
	dc.l	0		;ULONG	cd_OffsetI
	dc.w	0		;UWORD	cd_Pad1
	dc.w	0		;UWORD	cd_OffsetF
	dc.l	0		;ULONG	cd_AddI
	dc.w	0		;UWORD	cd_Pad2
	dc.w	$8000		;UWORD	cd_AddF
	dc.l	sample		;APTR	cd_DataStart
	dc.l	sample_len-1	;ULONG	cd_LastOffsetI
	dc.w	0		;UWORD	cd_Pad3
	dc.w	$ffff		;UWORD	cd_LastOffsetF
	dc.l	256		;ULONG	cd_ScaleLeft
	dc.l	0		;ULONG	cd_ScaleRight
	dc.l	AddByteMV	;FPTR	cd_AddRoutine
	dc.l	$10000		;LONG	cd_VolumeLeft
	dc.l	0		;LONG	cd_VolumeRight
	dc.l	AHIST_M8S	;ULONG	cd_Type

	dc.w	0		;UWORD	cd_NextEOS
	dc.b	$ff		;UBYTE	cd_NextFreqOK
	dc.b	$ff		;UBYTE	cd_NextSoundOK
	dc.l	0		;ULONG	cd_NextOffsetI
	dc.w	0		;UWORD	cd_NextPad1
	dc.w	0		;UWORD	cd_NextOffsetF
	dc.l	0		;ULONG	cd_NextAddI
	dc.w	0		;UWORD	cd_NextPad2
	dc.w	$8000		;UWORD	cd_NextAddF
	dc.l	sample		;APTR	cd_NextDataStart
	dc.l	sample_len-1	;ULONG	cd_NextLastOffsetI
	dc.w	0		;UWORD	cd_NextPad3
	dc.w	$ffff		;UWORD	cd_NextLastOffsetF
	dc.l	256		;ULONG	cd_NextScaleLeft
	dc.l	0		;ULONG	cd_NextScaleRight
	dc.l	AddByteMV	;FPTR	cd_NextAddRoutine
	dc.l	$10000		;LONG	cd_NextVolumeLeft
	dc.l	0		;LONG	cd_NextVolumeRight
	dc.l	AHIST_M8S	;ULONG	cd_NextType

	dc.l	0		;ULONG	cd_LCommand
	dc.l	0		;ULONG	cd_LAddress
	dc.l	0		;ULONG	cd_Samples
	dc.l	0		;ULONG	cd_FirstOffsetI
	dc.w	0		;WORD	cd_LastSampleL
	dc.w	0		;WORD	cd_TempLastSampleL
	dc.w	0		;WORD	cd_LastSampleR
	dc.w	0		;WORD	cd_TempLastSampleR
 ELSE
	dc.w	0		;UWORD	cd_EOS
	dc.b	$ff		;UBYTE	cd_FreqOK
	dc.b	$ff		;UBYTE	cd_SoundOK
	dc.l	sample_len-1	;ULONG	cd_OffsetI
	dc.w	0		;UWORD	cd_Pad1
	dc.w	$ffff		;UWORD	cd_OffsetF
	dc.l	0		;ULONG	cd_AddI
	dc.w	0		;UWORD	cd_Pad2
	dc.w	$8000		;UWORD	cd_AddF
	dc.l	sample		;APTR	cd_DataStart
	dc.l	0		;ULONG	cd_LastOffsetI
	dc.w	0		;UWORD	cd_Pad3
	dc.w	0		;UWORD	cd_LastOffsetF
	dc.l	256		;ULONG	cd_ScaleLeft
	dc.l	0		;ULONG	cd_ScaleRight
	dc.l	AddByteBMV	;FPTR	cd_AddRoutine
	dc.l	$10000		;LONG	cd_VolumeLeft
	dc.l	0		;LONG	cd_VolumeRight
	dc.l	AHIST_BW|AHIST_M8S	;ULONG	cd_Type

	dc.w	0		;UWORD	cd_NextEOS
	dc.b	$ff		;UBYTE	cd_NextFreqOK
	dc.b	$ff		;UBYTE	cd_NextSoundOK
	dc.l	sample_len-1	;ULONG	cd_NextOffsetI
	dc.w	0		;UWORD	cd_NextPad1
	dc.w	$ffff		;UWORD	cd_NextOffsetF
	dc.l	0		;ULONG	cd_NextAddI
	dc.w	0		;UWORD	cd_NextPad2
	dc.w	$8000		;UWORD	cd_NextAddF
	dc.l	sample		;APTR	cd_NextDataStart
	dc.l	0		;ULONG	cd_NextLastOffsetI
	dc.w	0		;UWORD	cd_NextPad3
	dc.w	0		;UWORD	cd_NextLastOffsetF
	dc.l	256		;ULONG	cd_NextScaleLeft
	dc.l	0		;ULONG	cd_NextScaleRight
	dc.l	AddByteBMV	;FPTR	cd_NextAddRoutine
	dc.l	$10000		;LONG	cd_NextVolumeLeft
	dc.l	0		;LONG	cd_NextVolumeRight
	dc.l	AHIST_BW|AHIST_M8S	;ULONG	cd_NextType

	dc.l	0		;ULONG	cd_LCommand
	dc.l	0		;ULONG	cd_LAddress
	dc.l	0		;ULONG	cd_Samples
	dc.l	0		;ULONG	cd_FirstOffsetI
	dc.w	0		;WORD	cd_LastSampleL
	dc.w	0		;WORD	cd_TempLastSampleL
	dc.w	0		;WORD	cd_LastSampleR
	dc.w	0		;WORD	cd_TempLastSampleR
 ENDC * BACK 
 ENDC

;-----------------------------------------

;in:
* a6	ExecBase
initcode:
 IFNE	ALIGN
* Align Add#? routines to even 16-byte address
	lea	AlignStart(pc),a0
	move.l	a0,d0
	and.b	#$f0,d0
	move.l	d0,a1
	move.l	#AlignEnd-AlignStart-1,d0
.11
	move.b	(a0)+,(a1)+
	dbf	d0,.11
* Update relative pointers
	lea	AlignStart(pc),a0
	move.l	a0,d0
	and.w	#$0f,d0
	lea	OffsetTable(pc),a0
.12
	tst.w	(a0)
	bmi.b	.13
	sub.w	d0,(a0)+
	bra.b	.12
.13
	call	CacheClearU
 ENDC
	rts


;in:
* a2	ptr to AHIAudioCtrl
* a5	ptr to AHIBase
;out:
* d0	TRUE on success
_initSignedTable:
	pushm	d1-a6
	
	move.l	ahiac_Flags(a2),d0
	btst.l	#AHIACB_MULTTAB,d0
	beq.b	.notable
	tst.l	ahiac_MultTableS(a2)
	bne.b	.notable			;there is already a table!

 IFEQ	DEBUG
	move.l	ahib_SysLib(a5),a6
 ELSE
 	base	exec
 ENDC
	move.l	#256*(TABLEMAXVOL+1)*4,d0	;include highest volume, too!
	moveq	#MEMF_PUBLIC,d1			;may be accessed from interrupts
	call	AllocVec
	move.l	d0,ahiac_MultTableS(a2)
	beq.b	.error
	bsr.b	calcSignedTable
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
calcSignedTable:
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
* a5	ptr to AHIBase
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

 IFEQ	DEBUG
	move.l	ahib_SysLib(a5),a6
 ELSE
 	base	exec
 ENDC

	move.l	#256*(TABLEMAXVOL+1)*4,d0	;incude highest volume, too!
						;*4 => Look like the signed table!
	moveq	#MEMF_PUBLIC,d1			;may be accessed from interrupts
	call	AllocVec
	move.l	d0,ahiac_MultTableU(a2)
	beq.b	.error
	bsr.b	calcUnsignedTable
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
calcUnsignedTable:
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
* a2	AudioCtrl
;out:
* d0	ScaleLeft
* d1	ScaleRight
* d2	AddRoutine


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

SelectAddRoutine:
	pushm	d3-a6

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
	move.l	_UtilityBase(pc),a0
	
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
	jsr	(a0)		;returns d0, d1 and a0

	and.l	#AHIST_BW,d3
	beq	.fw
	add.w	#OffsetBackward,a0
.fw

	add.w	(a0),a0
.exit:
	move.l	a0,d2
	popm	d3-a6
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


	move.l	a0,d2
	popm	d3-a6
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

printit:
	PRINTF	2,"AHIST_INPUT on channel %ld",cd_ChannelNo(a5)
	rts
;in:
* a0	Hook
* a1	Mixing buffer (size is 8 byte aligned)
* a2	AHIAudioCtrl
_Mix:
	pushm	d0-a6

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

	move.l	cd_Type(a5),d1
	and.l	#AHIST_INPUT,d1
	beq	.notinput
	bsr	printit
	bra	.noSoundFunc
.notinput

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

	movem.l	(a5),d1/d3/d4/d5/d6/a3	;Flags,OffsI,OffsF,AddI,AddF,DataStart
	not.w	d1			;FreqOK and SoundOK must both be $FF
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

; d3:d4 always points OUTSIDE the sample after this call. Ie, if we read a
; sample at offset d3 now, it does not belong to the sample just played.
; This is true for both backward and forward mixing.
; Oh, btw... OffsetF is unsigned, so -0.5 is expressed as -1:$8000.

; What we do now is to calculate how much futher we have advanced.
	sub.w	cd_LastOffsetF(a5),d4
	move.l	cd_LastOffsetI(a5),d0
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
	neg.w	d4
.same_type

; Ok, now we add.
	add.w	cd_NextOffsetF(a5),d4
	move.l	cd_NextOffsetI(a5),d0
	addx.l	d0,d3
	move.l	d3,cd_OffsetI(a5)
	move.w	d4,cd_OffsetF(a5)
	move.l	d3,cd_FirstOffsetI(a5)

; But what if the next sample is so short that we just passed it!?
; Here is the nice part. CalcSamples checks this,
; and set cd_Samples to 0 in that case. And the add routines doesn't
; do anything when asked to mix 0 samples.
; Assume we have passed a sample with 4 samples, and the next one
; is only 3. CalcSamples returns 0. The 'jsr (a0)' call above does
; not do anything at all, OffsetI is still 4. Now we subtract LastOffsetI,
; which is 3. Result: We have passed the sample with 1. And guess what?
; That's in range.

; Now, let's copy the rest of the cd_Next#? stuff...
	move.l	cd_NextFlags(a5),cd_Flags(a5)
	movem.l	cd_NextAddI(a5),d0-d7/a0/a3/a6	; AddI,AddF,DataStart,LastOffsetI,LastOffsetF,ScaleLeft,ScaleRight,AddRoutine, VolumeLeft,VolumeRight,Type
	movem.l	d0-d7/a0/a3/a6,cd_AddI(a5)

					;AddI/AddF,LastOffsetI/LastOffsetF ok
	move.l	a6,d2			;Type
	movem.l	cd_OffsetI(a5),d5/d6	;cd_OffsetI/cd_OffsetF
	bsr	CalcSamples
	move.l	d0,cd_Samples(a5)

	pop	d0
	move.w	#$ffff,cd_EOS(a5)	;signal End-Of-Sample
	bra.w	.contchannel		;same channel, new sound

.wont_reach_end
	sub.l	d0,cd_Samples(a5)
	movem.l	cd_ScaleLeft(a5),d1/d2/a0
	jsr	(a0)
	movem.l	d3/d4,cd_OffsetI(a5)		;update Offset

.channel_done
	move.l	cd_Succ(a5),d0		;next channel in list
	move.l	d0,a5
	bne.w	.nextchannel
	tst.b	ahiac_WetOrDry(a2)
	bne.b	.exit			;Both wet and dry finished
	addq.b	#1,ahiac_WetOrDry(a2)	;Mark dry

*** AHIET_DSPECHO
	move.l	ahiac_EffDSPEchoStruct(a2),d0
	beq	.noEffDSPEcho
	move.l	d0,a0
	move.l	ahiecho_Code(a0),a0
	jsr	(a0)
.noEffDSPEcho

.do_dry
	move.l	ahiac_DryList(a2),d0
	move.l	d0,a5
	beq	.exit
	btst.b	#AHIACB_POSTPROC-24,ahiac_Flags(a2)
	beq	.nextchannel
	move.l	a4,a1			;New block
	bra	.nextchannel

.exit

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
	move.l	cd_OffsetI(a4),(a3)+
	add.w	#AHIChannelData_SIZEOF,a4
	dbf	d0,.ci_loop
	move.l	h_Entry(a0),a3
	jsr	(a3)
.noEffChannelInfo


	popm	d0-a6
	rts



;in:
* d0	AddI
* d1.w	AddF
* d2	Type
* d3	LastOffsetI
* d4.w	LastOffsetF
* d5	OffsetI
* d6.w	OffsetF
;ut:
* d0	Samples
CalcSamples:
* Calc how many samples it will take when mixed (Samples=Length×Rate)

	and.l	#AHIST_BW,d2
	bne.b	.backwards
	sub.w	d6,d4
	subx.l	d5,d3
	bra.b	.1
.backwards
	sub.w	d4,d6
	subx.l	d3,d5
	move.w	d6,d4
	move.l	d5,d3
.1
	bmi.b	.error
	swap.w	d4
	move.w	d3,d4
	swap.w	d4
	clr.w	d3
	swap.w	d3
; d3:d4 is now (positive) length <<16

	swap	d0
	move.w	d1,d0
	tst.l	d0
	beq.b	.error
; d0 is now rate<<16

 IFGE	__CPU-68020
	divu.l	d0,d3:d4
	move.l	d4,d0
 ELSE
	move.l	d3,d1
	move.l	d4,d2
	bsr	UDivMod64		;d0 = (d1:d2)/d0
 ENDC
	addq.l	#1,d0
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
	asr.l	#1,d0
 ELSE
	asr.l	#8,d0
 ENDC
	lea	OffsWordMV(pc),a0
	rts

FixVolWordMVT:
	add.l	d1,d0
	bsr	_Fixed2Shift
	lea	OffsWordMVT(pc),a0
	rts

FixVolWordSVl:
 IFGE	__CPU-68020
	asr.l	#1,d0
 ELSE
	asr.l	#8,d0
 ENDC
	lea	OffsWordSVl(pc),a0
	rts

FixVolWordSVr:
 IFGE	__CPU-68020
	asr.l	#1,d1
 ELSE
	asr.l	#8,d1
 ENDC
	lea	OffsWordSVr(pc),a0
	rts

FixVolWordSVP:
 IFGE	__CPU-68020
 	asr.l	#1,d0
 	asr.l	#1,d1
 ELSE
	asr.l	#8,d0
	asr.l	#8,d1
 ENDC
	lea	OffsWordSVP(pc),a0
	rts

FixVolWordSVTl:
	bsr	_Fixed2Shift
	lea	OffsWordSVTl(pc),a0
	rts

FixVolWordSVTr:
	move.l	d1,d0
	bsr	_Fixed2Shift
	move.l	d0,d1
	lea	OffsWordSVTr(pc),a0
	rts

FixVolWordSVPT:
	bsr	_Fixed2Shift
	push	d0
	move.l	d1,d0
	bsr	_Fixed2Shift
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
	asr.l	#1,d0
	asr.l	#1,d1
	lea	OffsWordsMV(pc),a0
	rts

FixVolWordsMVT:
	bsr	_Fixed2Shift
	push	d0
	move.l	d1,d0
	bsr	_Fixed2Shift
	move.l	d0,d1
	pop	d0
	lea	OffsWordsMVT(pc),a0
	rts

FixVolWordsSVl:
	asr.l	#1,d0
	lea	OffsWordsSVl(pc),a0
	rts

FixVolWordsSVr:
	asr.l	#1,d1
	lea	OffsWordsSVr(pc),a0
	rts

FixVolWordsSVP:
 	asr.l	#1,d0
 	asr.l	#1,d1
	lea	OffsWordsSVP(pc),a0
	rts

FixVolWordsSVTl:
	bsr	_Fixed2Shift
	lea	OffsWordsSVTl(pc),a0
	rts

FixVolWordsSVTr:
	move.l	d1,d0
	bsr	_Fixed2Shift
	move.l	d0,d1
	lea	OffsWordsSVTr(pc),a0
	rts

FixVolWordsSVPT:
	bsr	_Fixed2Shift
	push	d0
	move.l	d1,d0
	bsr	_Fixed2Shift
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
* d4.w	Offset Fraction (upper word cleared!)
* d5	Add Integer (s)
* d6.w	Add Fraction (s) (upper word cleared!)
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
AlignStart:

; To make the backward-mixing routines, do this:
; 1) Copy all mixing routines.
; 2) Replace all occurences of ':' with 'B:' (all labels)
; 3) Replace all occurences of 'add.w	d6,d4' with 'sub.w	d6,d4'
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
	subq.w	#1,d0
	bmi.b	.exit
.nextsample
	cmp.l	cd_FirstOffsetI(a5),d3	;Is this the first sample?
	bne	.not_first
	move.l	cd_LastSampleL(a5),d7
	bra	.got_sample
.not_first
	move.b	-1(a3,d3.l),d7
	lsl.w	#8,d7
	ext.l	d7
.got_sample
	neg.w	d4			;(65536-offset fraction)
	beq.b	.null
	muls.l	d4,d7
	neg.w	d4
	move.l	d7,a0
	move.b	0(a3,d3.l),d7
	lsl.w	#8,d7
	ext.l	d7
	move.l	d7,cd_TempLastSampleL(a5)
	muls.l	d4,d7
	add.l	a0,d7
	asr.l	#8,d7
	asr.l	#8,d7
.null
	muls.l	d1,d7
	add.l	d7,(a4)+

	add.w	d6,d4
	addx.l	d5,d3
	dbf	d0,.nextsample
.exit
 ENDC
	rts

;in
* d1.l	-65536..65536
* d2.l	-65536..65536
AddByteSVPH:
 IFGE	__CPU-68020
	subq.w	#1,d0
	bmi.b	.exit
.nextsample
	cmp.l	cd_FirstOffsetI(a5),d3	;Is this the first sample?
	bne	.not_first
	move.l	cd_LastSampleL(a5),d7
	bra	.got_sample
.not_first
	move.b	-1(a3,d3.l),d7
	lsl.w	#8,d7
	ext.l	d7
.got_sample
	neg.w	d4			;(65536-offset fraction)
	beq.b	.null
	muls.l	d4,d7
	neg.w	d4
	move.l	d7,a0
	move.b	0(a3,d3.l),d7
	lsl.w	#8,d7
	ext.l	d7
	move.l	d7,cd_TempLastSampleL(a5)
	muls.l	d4,d7
	add.l	a0,d7
	asr.l	#8,d7
	asr.l	#8,d7
.null
	move.l	d7,a0
	muls.l	d1,d7
	add.l	d7,(a4)+

	move.l	a0,d7
	muls.l	d2,d7
	add.l	d7,(a4)+

	add.w	d6,d4
	addx.l	d5,d3
	dbf	d0,.nextsample
.exit
 ENDC
	rts

;in
* d1.l	-65536..65536
* d2.l	-65536..65536
AddBytesMVH:
 IFGE	__CPU-68020
	subq.w	#1,d0
	bmi.b	.exit
.nextsample
	cmp.l	cd_FirstOffsetI(a5),d3	;Is this the first sample?
	bne	.not_firstL
	move.l	cd_LastSampleL(a5),d7
	bra	.got_sampleL
.not_firstL
	move.b	-2(a3,d3.l*2),d7
	lsl.w	#8,d7
	ext.l	d7
.got_sampleL
	neg.w	d4			;(65536-offset fraction)
	beq.b	.nullL
	muls.l	d4,d7
	neg.w	d4
	move.l	d7,a0
	move.b	0(a3,d3.l*2),d7
	lsl.w	#8,d7
	ext.l	d7
	move.l	d7,cd_TempLastSampleL(a5)
	muls.l	d4,d7
	add.l	a0,d7
	asr.l	#8,d7
	asr.l	#8,d7
.nullL
	muls.l	d1,d7
	add.l	d7,(a4)

	cmp.l	cd_FirstOffsetI(a5),d3	;Is this the first sample?
	bne	.not_firstR
	move.l	cd_LastSampleR(a5),d7
	bra	.got_sampleR
.not_firstR
	move.b	-1(a3,d3.l*2),d7
	lsl.w	#8,d7
	ext.l	d7
.got_sampleR
	neg.w	d4			;(65536-offset fraction)
	beq.b	.nullR
	muls.l	d4,d7
	neg.w	d4
	move.l	d7,a0
	move.b	1(a3,d3.l*2),d7
	lsl.w	#8,d7
	ext.l	d7
	move.l	d7,cd_TempLastSampleR(a5)
	muls.l	d4,d7
	add.l	a0,d7
	asr.l	#8,d7
	asr.l	#8,d7
.nullR
	muls.l	d2,d7
	add.l	d7,(a4)+

	add.w	d6,d4
	addx.l	d5,d3
	dbf	d0,.nextsample
.exit
 ENDC
	rts

;in
* d1.l	-65536..65536
* d2.l	-65536..65536
AddBytesSVPH:
 IFGE	__CPU-68020
	subq.w	#1,d0
	bmi.b	.exit
.nextsample
	cmp.l	cd_FirstOffsetI(a5),d3	;Is this the first sample?
	bne	.not_firstL
	move.l	cd_LastSampleL(a5),d7
	bra	.got_sampleL
.not_firstL
	move.b	-2(a3,d3.l*2),d7
	lsl.w	#8,d7
	ext.l	d7
.got_sampleL
	neg.w	d4			;(65536-offset fraction)
	beq.b	.nullL
	muls.l	d4,d7
	neg.w	d4
	move.l	d7,a0
	move.b	0(a3,d3.l*2),d7
	lsl.w	#8,d7
	ext.l	d7
	move.l	d7,cd_TempLastSampleL(a5)
	muls.l	d4,d7
	add.l	a0,d7
	asr.l	#8,d7
	asr.l	#8,d7
.nullL
	muls.l	d1,d7
	add.l	d7,(a4)+

	cmp.l	cd_FirstOffsetI(a5),d3	;Is this the first sample?
	bne	.not_firstR
	move.l	cd_LastSampleR(a5),d7
	bra	.got_sampleR
.not_firstR
	move.b	-1(a3,d3.l*2),d7
	lsl.w	#8,d7
	ext.l	d7
.got_sampleR
	neg.w	d4			;(65536-offset fraction)
	beq.b	.nullR
	muls.l	d4,d7
	neg.w	d4
	move.l	d7,a0
	move.b	1(a3,d3.l*2),d7
	lsl.w	#8,d7
	ext.l	d7
	move.l	d7,cd_TempLastSampleR(a5)
	muls.l	d4,d7
	add.l	a0,d7
	asr.l	#8,d7
	asr.l	#8,d7
.nullR
	muls.l	d2,d7
	add.l	d7,(a4)+

	add.w	d6,d4
	addx.l	d5,d3
	dbf	d0,.nextsample
.exit
 ENDC
	rts

;in
* d1.l	-65536..65536
* d2.l	-65536..65536
AddWordMVH:
 IFGE	__CPU-68020
	subq.w	#1,d0
	bmi.b	.exit
.nextsample
	cmp.l	cd_FirstOffsetI(a5),d3	;Is this the first sample?
	bne	.not_first
	move.l	cd_LastSampleL(a5),d7
	bra	.got_sample
.not_first
	move.w	-2(a3,d3.l*2),d7
	ext.l	d7
.got_sample
	neg.w	d4			;(65536-offset fraction)
	beq.b	.null
	muls.l	d4,d7
	neg.w	d4
	move.l	d7,a0
	move.w	0(a3,d3.l*2),d7
	ext.l	d7
	move.l	d7,cd_TempLastSampleL(a5)
	muls.l	d4,d7
	add.l	a0,d7
	asr.l	#8,d7
	asr.l	#8,d7
.null
	muls.l	d1,d7
	add.l	d7,(a4)+

	add.w	d6,d4
	addx.l	d5,d3
	dbf	d0,.nextsample
.exit
 ENDC
	rts

;in
* d1.l	-65536..65536
* d2.l	-65536..65536
AddWordSVPH:
 IFGE	__CPU-68020
	subq.w	#1,d0
	bmi.b	.exit
.nextsample
	cmp.l	cd_FirstOffsetI(a5),d3	;Is this the first sample?
	bne	.not_first
	move.l	cd_LastSampleL(a5),d7
	bra	.got_sample
.not_first
	move.w	-2(a3,d3.l*2),d7
	ext.l	d7
.got_sample
	neg.w	d4			;(65536-offset fraction)
	beq.b	.null
	muls.l	d4,d7
	neg.w	d4
	move.l	d7,a0
	move.w	0(a3,d3.l*2),d7
	ext.l	d7
	move.l	d7,cd_TempLastSampleL(a5)
	muls.l	d4,d7
	add.l	a0,d7
	asr.l	#8,d7
	asr.l	#8,d7
.null
	move.l	d7,a0
	muls.l	d1,d7
	add.l	d7,(a4)+

	move.l	a0,d7
	muls.l	d2,d7
	add.l	d7,(a4)+

	add.w	d6,d4
	addx.l	d5,d3
	dbf	d0,.nextsample
.exit
 ENDC
	rts

;in
* d1.l	-65536..65536
* d2.l	-65536..65536
AddWordsMVH:
 IFGE	__CPU-68020
	subq.w	#1,d0
	bmi.b	.exit
.nextsample
	cmp.l	cd_FirstOffsetI(a5),d3	;Is this the first sample?
	bne	.not_firstL
	move.l	cd_LastSampleL(a5),d7
	bra	.got_sampleL
.not_firstL
	move.w	-4(a3,d3.l*4),d7
	ext.l	d7
.got_sampleL
	neg.w	d4			;(65536-offset fraction)
	beq.b	.nullL
	muls.l	d4,d7
	neg.w	d4
	move.l	d7,a0
	move.w	0(a3,d3.l*4),d7
	ext.l	d7
	move.l	d7,cd_TempLastSampleL(a5)
	muls.l	d4,d7
	add.l	a0,d7
	asr.l	#8,d7
	asr.l	#8,d7
.nullL
	muls.l	d1,d7
	add.l	d7,(a4)

	cmp.l	cd_FirstOffsetI(a5),d3	;Is this the first sample?
	bne	.not_firstR
	move.l	cd_LastSampleR(a5),d7
	bra	.got_sampleR
.not_firstR
	move.w	-2(a3,d3.l*4),d7
	ext.l	d7
.got_sampleR
	neg.w	d4			;(65536-offset fraction)
	beq.b	.nullR
	muls.l	d4,d7
	neg.w	d4
	move.l	d7,a0
	move.w	2(a3,d3.l*4),d7
	ext.l	d7
	move.l	d7,cd_TempLastSampleR(a5)
	muls.l	d4,d7
	add.l	a0,d7
	asr.l	#8,d7
	asr.l	#8,d7
.nullR
	muls.l	d2,d7
	add.l	d7,(a4)+

	add.w	d6,d4
	addx.l	d5,d3
	dbf	d0,.nextsample
.exit
 ENDC
	rts

;in
* d1.l	-65536..65536
* d2.l	-65536..65536
AddWordsSVPH:
 IFGE	__CPU-68020
	subq.w	#1,d0
	bmi.b	.exit
.nextsample
	cmp.l	cd_FirstOffsetI(a5),d3	;Is this the first sample?
	bne	.not_firstL
	move.l	cd_LastSampleL(a5),d7
	bra	.got_sampleL
.not_firstL
	move.w	-4(a3,d3.l*4),d7
	ext.l	d7
.got_sampleL
	neg.w	d4			;(65536-offset fraction)
	beq.b	.nullL
	muls.l	d4,d7
	neg.w	d4
	move.l	d7,a0
	move.w	0(a3,d3.l*4),d7
	ext.l	d7
	move.l	d7,cd_TempLastSampleL(a5)
	muls.l	d4,d7
	add.l	a0,d7
	asr.l	#8,d7
	asr.l	#8,d7
.nullL
	muls.l	d1,d7
	add.l	d7,(a4)+

	cmp.l	cd_FirstOffsetI(a5),d3	;Is this the first sample?
	bne	.not_firstR
	move.l	cd_LastSampleR(a5),d7
	bra	.got_sampleR
.not_firstR
	move.w	-2(a3,d3.l*4),d7
	ext.l	d7
.got_sampleR
	neg.w	d4			;(65536-offset fraction)
	beq.b	.nullR
	muls.l	d4,d7
	neg.w	d4
	move.l	d7,a0
	move.w	2(a3,d3.l*4),d7
	ext.l	d7
	move.l	d7,cd_TempLastSampleR(a5)
	muls.l	d4,d7
	add.l	a0,d7
	asr.l	#8,d7
	asr.l	#8,d7
.nullR
	muls.l	d2,d7
	add.l	d7,(a4)+

	add.w	d6,d4
	addx.l	d5,d3
	dbf	d0,.nextsample
.exit
 ENDC
	rts


;------------------------------------------------------------------------------

AddByteMVHB:
 IFGE	__CPU-68020
	subq.w	#1,d0
	bmi.b	.exit
.nextsample
	cmp.l	cd_FirstOffsetI(a5),d3	;Is this the first sample?
	bne	.not_first
	move.l	cd_LastSampleL(a5),d7
	bra	.got_sample
.not_first
	move.b	1(a3,d3.l),d7
	lsl.w	#8,d7
	ext.l	d7
.got_sample
	muls.l	d4,d7
	move.l	d7,a0
	move.b	0(a3,d3.l),d7
	lsl.w	#8,d7
	ext.l	d7
	move.l	d7,cd_TempLastSampleL(a5)
	neg.w	d4			;(65536-offset fraction)
	beq.b	.null
	muls.l	d4,d7
	neg.w	d4
	add.l	a0,d7
	asr.l	#8,d7
	asr.l	#8,d7
.null
	muls.l	d1,d7
	add.l	d7,(a4)+

	sub.w	d6,d4
	subx.l	d5,d3
	dbf	d0,.nextsample
.exit
 ENDC
	rts

AddByteSVPHB:
 IFGE	__CPU-68020
	subq.w	#1,d0
	bmi.b	.exit
.nextsample
	cmp.l	cd_FirstOffsetI(a5),d3	;Is this the first sample?
	bne	.not_first
	move.l	cd_LastSampleL(a5),d7
	bra	.got_sample
.not_first
	move.b	1(a3,d3.l),d7
	lsl.w	#8,d7
	ext.l	d7
.got_sample
	muls.l	d4,d7
	move.l	d7,a0
	move.b	0(a3,d3.l*2),d7
	lsl.w	#8,d7
	ext.l	d7
	move.l	d7,cd_TempLastSampleL(a5)
	neg.w	d4			;(65536-offset fraction)
	beq.b	.null
	muls.l	d4,d7
	neg.w	d4
	add.l	a0,d7
	asr.l	#8,d7
	asr.l	#8,d7
.null
	move.l	d7,a0
	muls.l	d1,d7
	add.l	d7,(a4)+

	move.l	a0,d7
	muls.l	d2,d7
	add.l	d7,(a4)+

	sub.w	d6,d4
	subx.l	d5,d3
	dbf	d0,.nextsample
.exit
 ENDC
	rts

AddBytesMVHB:
 IFGE	__CPU-68020
	subq.w	#1,d0
	bmi.b	.exit
.nextsample
	cmp.l	cd_FirstOffsetI(a5),d3	;Is this the first sample?
	bne	.not_firstL
	move.l	cd_LastSampleL(a5),d7
	bra	.got_sampleL
.not_firstL
	move.b	2(a3,d3.l*2),d7
	lsl.w	#8,d7
	ext.l	d7
.got_sampleL
	muls.l	d4,d7
	move.l	d7,a0
	move.b	0(a3,d3.l*2),d7
	lsl.w	#8,d7
	ext.l	d7
	move.l	d7,cd_TempLastSampleL(a5)
	neg.w	d4			;(65536-offset fraction)
	beq.b	.nullL
	muls.l	d4,d7
	neg.w	d4
	add.l	a0,d7
	asr.l	#8,d7
	asr.l	#8,d7
.nullL
	muls.l	d1,d7
	add.l	d7,(a4)

	cmp.l	cd_FirstOffsetI(a5),d3	;Is this the first sample?
	bne	.not_firstR
	move.l	cd_LastSampleR(a5),d7
	bra	.got_sampleR
.not_firstR
	move.b	3(a3,d3.l*2),d7
	lsl.w	#8,d7
	ext.l	d7
.got_sampleR
	muls.l	d4,d7
	move.l	d7,a0
	move.b	1(a3,d3.l*2),d7
	lsl.w	#8,d7
	ext.l	d7
	move.l	d7,cd_TempLastSampleR(a5)
	neg.w	d4			;(65536-offset fraction)
	beq.b	.nullR
	muls.l	d4,d7
	neg.w	d4
	add.l	a0,d7
	asr.l	#8,d7
	asr.l	#8,d7
.nullR
	muls.l	d2,d7
	add.l	d7,(a4)+

	sub.w	d6,d4
	subx.l	d5,d3
	dbf	d0,.nextsample
.exit
 ENDC
	rts

AddBytesSVPHB:
 IFGE	__CPU-68020
	subq.w	#1,d0
	bmi.b	.exit
.nextsample
	cmp.l	cd_FirstOffsetI(a5),d3	;Is this the first sample?
	bne	.not_firstL
	move.l	cd_LastSampleL(a5),d7
	bra	.got_sampleL
.not_firstL
	move.b	2(a3,d3.l*2),d7
	lsl.w	#8,d7
	ext.l	d7
.got_sampleL
	muls.l	d4,d7
	move.l	d7,a0
	move.b	0(a3,d3.l*2),d7
	lsl.w	#8,d7
	ext.l	d7
	move.l	d7,cd_TempLastSampleL(a5)
	neg.w	d4			;(65536-offset fraction)
	beq.b	.nullL
	muls.l	d4,d7
	neg.w	d4
	add.l	a0,d7
	asr.l	#8,d7
	asr.l	#8,d7
.nullL
	muls.l	d1,d7
	add.l	d7,(a4)+

	cmp.l	cd_FirstOffsetI(a5),d3	;Is this the first sample?
	bne	.not_firstR
	move.l	cd_LastSampleR(a5),d7
	bra	.got_sampleR
.not_firstR
	move.b	3(a3,d3.l*2),d7
	lsl.w	#8,d7
	ext.l	d7
.got_sampleR
	muls.l	d4,d7
	move.l	d7,a0
	move.b	1(a3,d3.l*2),d7
	lsl.w	#8,d7
	ext.l	d7
	move.l	d7,cd_TempLastSampleR(a5)
	neg.w	d4			;(65536-offset fraction)
	beq.b	.nullR
	muls.l	d4,d7
	neg.w	d4
	add.l	a0,d7
	asr.l	#8,d7
	asr.l	#8,d7
.nullR
	muls.l	d2,d7
	add.l	d7,(a4)+

	sub.w	d6,d4
	subx.l	d5,d3
	dbf	d0,.nextsample
.exit
 ENDC
	rts

AddWordMVHB:
 IFGE	__CPU-68020
	subq.w	#1,d0
	bmi.b	.exit
.nextsample
	cmp.l	cd_FirstOffsetI(a5),d3	;Is this the first sample?
	bne	.not_first
	move.l	cd_LastSampleL(a5),d7
	bra	.got_sample
.not_first
	move.w	2(a3,d3.l*2),d7
	ext.l	d7
.got_sample
	muls.l	d4,d7
	move.l	d7,a0
	move.w	0(a3,d3.l*2),d7
	ext.l	d7
	move.l	d7,cd_TempLastSampleL(a5)
	neg.w	d4			;(65536-offset fraction)
	beq.b	.null
	muls.l	d4,d7
	neg.w	d4
	add.l	a0,d7
	asr.l	#8,d7
	asr.l	#8,d7
.null
	muls.l	d1,d7
	add.l	d7,(a4)+

	sub.w	d6,d4
	subx.l	d5,d3
	dbf	d0,.nextsample
.exit
 ENDC
	rts

AddWordSVPHB:
 IFGE	__CPU-68020
	subq.w	#1,d0
	bmi.b	.exit
.nextsample
	cmp.l	cd_FirstOffsetI(a5),d3	;Is this the first sample?
	bne	.not_first
	move.l	cd_LastSampleL(a5),d7
	bra	.got_sample
.not_first
	move.w	2(a3,d3.l*2),d7
	ext.l	d7
.got_sample
	muls.l	d4,d7
	move.l	d7,a0
	move.w	0(a3,d3.l*2),d7
	ext.l	d7
	move.l	d7,cd_TempLastSampleL(a5)
	neg.w	d4			;(65536-offset fraction)
	beq.b	.null
	muls.l	d4,d7
	neg.w	d4
	add.l	a0,d7
	asr.l	#8,d7
	asr.l	#8,d7
.null
	move.l	d7,a0
	muls.l	d1,d7
	add.l	d7,(a4)+

	move.l	a0,d7
	muls.l	d2,d7
	add.l	d7,(a4)+

	sub.w	d6,d4
	subx.l	d5,d3
	dbf	d0,.nextsample
.exit
 ENDC
	rts


AddWordsMVHB:
 IFGE	__CPU-68020
	subq.w	#1,d0
	bmi.b	.exit
.nextsample
	cmp.l	cd_FirstOffsetI(a5),d3	;Is this the first sample?
	bne	.not_firstL
	move.l	cd_LastSampleL(a5),d7
	bra	.got_sampleL
.not_firstL
	move.w	4(a3,d3.l*4),d7
	ext.l	d7
.got_sampleL
	muls.l	d4,d7
	move.l	d7,a0
	move.w	0(a3,d3.l*4),d7
	ext.l	d7
	move.l	d7,cd_TempLastSampleL(a5)
	neg.w	d4			;(65536-offset fraction)
	beq.b	.nullL
	muls.l	d4,d7
	neg.w	d4
	add.l	a0,d7
	asr.l	#8,d7
	asr.l	#8,d7
.nullL
	muls.l	d1,d7
	add.l	d7,(a4)

	cmp.l	cd_FirstOffsetI(a5),d3	;Is this the first sample?
	bne	.not_firstR
	move.l	cd_LastSampleR(a5),d7
	bra	.got_sampleR
.not_firstR
	move.w	6(a3,d3.l*4),d7
	ext.l	d7
.got_sampleR
	muls.l	d4,d7
	move.l	d7,a0
	move.w	2(a3,d3.l*4),d7
	ext.l	d7
	move.l	d7,cd_TempLastSampleR(a5)
	neg.w	d4			;(65536-offset fraction)
	beq.b	.nullR
	muls.l	d4,d7
	neg.w	d4
	add.l	a0,d7
	asr.l	#8,d7
	asr.l	#8,d7
.nullR
	muls.l	d2,d7
	add.l	d7,(a4)+

	sub.w	d6,d4
	subx.l	d5,d3
	dbf	d0,.nextsample
.exit
 ENDC
	rts

AddWordsSVPHB:
 IFGE	__CPU-68020
	subq.w	#1,d0
	bmi.b	.exit
.nextsample
	cmp.l	cd_FirstOffsetI(a5),d3	;Is this the first sample?
	bne	.not_firstL
	move.l	cd_LastSampleL(a5),d7
	bra	.got_sampleL
.not_firstL
	move.w	4(a3,d3.l*4),d7
	ext.l	d7
.got_sampleL
	muls.l	d4,d7
	move.l	d7,a0
	move.w	0(a3,d3.l*4),d7
	ext.l	d7
	move.l	d7,cd_TempLastSampleL(a5)
	neg.w	d4			;(65536-offset fraction)
	beq.b	.nullL
	muls.l	d4,d7
	neg.w	d4
	add.l	a0,d7
	asr.l	#8,d7
	asr.l	#8,d7
.nullL
	muls.l	d1,d7
	add.l	d7,(a4)+

	cmp.l	cd_FirstOffsetI(a5),d3	;Is this the first sample?
	bne	.not_firstR
	move.l	cd_LastSampleR(a5),d7
	bra	.got_sampleR
.not_firstR
	move.w	6(a3,d3.l*4),d7
	ext.l	d7
.got_sampleR
	muls.l	d4,d7
	move.l	d7,a0
	move.w	2(a3,d3.l*4),d7
	ext.l	d7
	move.l	d7,cd_TempLastSampleR(a5)
	neg.w	d4			;(65536-offset fraction)
	beq.b	.nullR
	muls.l	d4,d7
	neg.w	d4
	add.l	a0,d7
	asr.l	#8,d7
	asr.l	#8,d7
.nullR
	muls.l	d2,d7
	add.l	d7,(a4)+

	sub.w	d6,d4
	subx.l	d5,d3
	dbf	d0,.nextsample
.exit
 ENDC
	rts

;******************************************************************************
	cnop	0,16

AddSilence:
 IFGE	__CPU-68020
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
	add.l	d1,d3			;New OffsetI (1)
	move.l	d6,d2
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
	rts

;------------------------------------------------------------------------------
	cnop	0,16

AddSilenceB:
 IFGE	__CPU-68020
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
	add.l	d1,d3			;New OffsetI (1)
	move.l	d6,d2
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
	add.w	d6,d4
	addx.l	d5,d3
.1
	move.b	(a3,d3.l),d7
	ext.w	d7
	muls.w	d1,d7
	add.w	d7,(a4)+
	add.w	d6,d4
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
	add.w	d6,d4
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
	add.w	d6,d4
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
	add.w	d6,d4
	addx.l	d5,d3
	addq.l	#2,a4
.1
	move.b	(a3,d3.l),d7
	ext.w	d7
	muls.w	d1,d7
	add.w	d7,(a4)+
	add.w	d6,d4
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
	add.w	d6,d4
	addx.l	d5,d3
.1
	move.b	(a3,d3.l),d7
	addq.l	#2,a4
	ext.w	d7
	muls.w	d2,d7
	add.w	d7,(a4)+
	add.w	d6,d4
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
	add.w	d6,d4
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
	add.w	d6,d4
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
	add.w	d6,d4
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
	add.w	d6,d4
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
	add.w	d6,d4
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
	add.w	d6,d4
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
	add.w	d6,d4
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
	add.w	d6,d4
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
	add.w	d6,d4
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
	add.w	d6,d4
	addx.l	d5,d3
	dbf	d0,.nextsample
.exit
	rts

;------------------------------------------------------------------------------
	cnop	0,16

;in
* d1.l	Pointer in multiplication table
* d2.l	Pointer in multiplication table
AddBytesMVT:
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
	add.w	d6,d4
	addx.l	d5,d3
.1
	move.b	0(a3,d3.l*2),d1
	move.w	0(a0,d1.w*4),d2		;signed multiplication
	move.b	1(a3,d3.l*2),d1
	add.w	0(a1,d1.w*4),d2		;signed multiplication
	add.w	d2,(a4)+
	add.w	d6,d4
	addx.l	d5,d3

	dbf	d0,.nextsample
.exit
	pop	a1
	rts

;------------------------------------------------------------------------------
	cnop	0,16

;in
* d1.w	-256..256
AddBytesSVl:
	lsr.w	#1,d0
	bcs.b	.1
	subq.w	#1,d0
	bmi.b	.exit
.nextsample
	move.b	0(a3,d3.l*2),d7
	ext.w	d7
	muls.w	d1,d7
	add.w	d7,(a4)+
	add.w	d6,d4
	addx.l	d5,d3
	addq.l	#2,a4
.1
	move.b	0(a3,d3.l*2),d7
	ext.w	d7
	muls.w	d1,d7
	add.w	d7,(a4)+
	add.w	d6,d4
	addx.l	d5,d3
	addq.l	#2,a4
	dbf	d0,.nextsample
.exit
	rts

;------------------------------------------------------------------------------
	cnop	0,16

;in
* d2.w	-256..256
AddBytesSVr:
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
	add.w	d6,d4
	addx.l	d5,d3
.1
	move.b	1(a3,d3.l*2),d7
	addq.l	#2,a4
	ext.w	d7
	muls.w	d2,d7
	add.w	d7,(a4)+
	add.w	d6,d4
	addx.l	d5,d3
	dbf	d0,.nextsample
.exit
	rts

;------------------------------------------------------------------------------
	cnop	0,16

;in
* d1.w	-256..256
* d2.w	-256..256
AddBytesSVP:
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
	add.w	d6,d4
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
	add.w	d6,d4
	addx.l	d5,d3
	dbf	d0,.nextsample
.exit
	rts

;------------------------------------------------------------------------------
	cnop	0,16

;in
* d1.l	Pointer in multiplication table
AddBytesSVTl:
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
	add.w	d6,d4
	addx.l	d5,d3
	addq.l	#2,a4
.1
	move.b	0(a3,d3.l*2),d1
	move.w	0(a0,d1.w*4),d2		;signed multiplication
	add.w	d2,(a4)+
	add.w	d6,d4
	addx.l	d5,d3
	addq.l	#2,a4
	dbf	d0,.nextsample
.exit
	rts
;------------------------------------------------------------------------------
	cnop	0,16

;in
* d2.l	Pointer in multiplication table
AddBytesSVTr:
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
	add.w	d6,d4
	addx.l	d5,d3
.1
	move.b	1(a3,d3.l*2),d1
	move.w	0(a0,d1.w*4),d2		;signed multiplication
	addq.l	#2,a4
	add.w	d2,(a4)+
	add.w	d6,d4
	addx.l	d5,d3
	dbf	d0,.nextsample
.exit
	rts

;------------------------------------------------------------------------------
	cnop	0,16

;in
* d1.l	Pointer in multiplication table
* d2.l	Pointer in multiplication table
AddBytesSVPT:
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
	add.w	d6,d4
	addx.l	d5,d3
.1
	move.b	0(a3,d3.l*2),d1
	move.w	0(a0,d1.w*4),d2
	add.w	d2,(a4)+
	move.b	1(a3,d3.l*2),d1
	move.w	0(a1,d1.w*4),d2
	add.w	d2,(a4)+
	add.w	d6,d4
	addx.l	d5,d3
	dbf	d0,.nextsample
.exit
	move.l	d7,a1			;restore a1
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
	add.w	d6,d4
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
	add.w	d6,d4
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
	add.w	d6,d4
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
	add.w	d6,d4
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
	add.w	d6,d4
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
	add.w	d6,d4
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
	add.w	d6,d4
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
	add.w	d6,d4
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
	add.w	d6,d4
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
	add.w	d6,d4
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
	add.w	d6,d4
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
	add.w	d6,d4
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
	add.w	d6,d4
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
	add.w	d6,d4
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
	add.w	d6,d4
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
	add.w	d6,d4
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
	move.w	1(a3,d3.l*4),d7
	muls.w	d2,d7
	asr.l	#8,d7
	asr.l	#7,d7
	add.w	d7,(a4)+
	add.w	d6,d4
	addx.l	d5,d3
.1
	move.w	0(a3,d3.l*4),d7
	muls.w	d1,d7
	asr.l	#8,d7
	asr.l	#7,d7
	add.w	d7,(a4)
	move.w	1(a3,d3.l*4),d7
	muls.w	d2,d7
	asr.l	#8,d7
	asr.l	#7,d7
	add.w	d7,(a4)+
	add.w	d6,d4
	addx.l	d5,d3
.exit
	rts

;------------------------------------------------------------------------------
	cnop	0,16

;in
* d1.l	Shift Count
* d2.l	Shift Count
AddWordsMVT:
	lsr.w	#1,d0
	bcs.b	.1
	subq.w	#1,d0
	bmi.b	.exit
.nextsample
 IFGE	__CPU-68020
	move.w	(a3,d3.l*4),d7
	asr.w	d1,d7
	add.w	d7,(a4)
	move.w	2(a3,d3.l*4),d7
	asr.w	d2,d7
	add.w	d7,(a4)+
 ELSE
	move.l	d3,d7
	add.l	d7,d7
	add.l	d7,d7
	lea	(a3,d7.l),a0
	move.w	(a0)+,d7
	asr.w	d1,d7
	add.w	d7,(a4)
	move.w	(a0),d7
	asr.w	d2,d7
	add.w	d7,(a4)+
 ENDC
	add.w	d6,d4
	addx.l	d5,d3
.1
 IFGE	__CPU-68020
	move.w	(a3,d3.l*4),d7
	asr.w	d1,d7
	add.w	d7,(a4)
	move.w	2(a3,d3.l*4),d7
	asr.w	d2,d7
	add.w	d7,(a4)+
 ELSE
	move.l	d3,d7
	add.l	d7,d7
	add.l	d7,d7
	lea	(a3,d7.l),a0
	move.w	(a0)+,d7
	asr.w	d1,d7
	add.w	d7,(a4)
	move.w	(a0),d7
	asr.w	d2,d7
	add.w	d7,(a4)+
 ENDC
	add.w	d6,d4
	addx.l	d5,d3

	dbf	d0,.nextsample
.exit
	rts

;------------------------------------------------------------------------------
	cnop	0,16

;in
* d1.l	-32768..32767
AddWordsSVl:
* 16 bit signed input
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
	add.w	d6,d4
	addx.l	d5,d3
	addq.l	#2,a4
.1
	move.w	0(a3,d3.l*4),d7
	muls.w	d1,d7
	asr.l	d2,d7
	add.w	d7,(a4)+
	add.w	d6,d4
	addx.l	d5,d3
	addq.l	#2,a4
	dbf	d0,.nextsample
.exit
	rts

;------------------------------------------------------------------------------
	cnop	0,16

;in
* d2.l	-32768..32767/
AddWordsSVr:
* 16 bit signed input
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
	add.w	d6,d4
	addx.l	d5,d3
.1
	move.w	2(a3,d3.l*4),d7
	muls.w	d2,d7
	asr.l	d1,d7
	addq.l	#2,a4
	add.w	d7,(a4)+
	add.w	d6,d4
	addx.l	d5,d3
	dbf	d0,.nextsample
.exit
	rts

;------------------------------------------------------------------------------
	cnop	0,16

;in
* d1.l	-32768..32767/-256..256
* d2.l	-32768..32767/-256..256
AddWordsSVP:
* 16/8 bit signed input (8 for '000 version)
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
	add.w	d6,d4
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
	add.w	d6,d4
	addx.l	d5,d3
	dbf	d0,.nextsample
.exit
	rts

;------------------------------------------------------------------------------
	cnop	0,16

;in
* d1.l	Shift count
AddWordsSVTl:
	lsr.w	#1,d0
	bcs.b	.1
	subq.w	#1,d0
	bmi.b	.exit
.nextsample
 IFGE	__CPU-68020
	move.w	(a3,d3.l*4),d7
 ELSE
	move.l	d3,d7
	add.l	d7,d7
	add.l	d7,d7
	move.w	(a3,d7.l),d7
 ENDC
	asr.w	d1,d7
	add.w	d7,(a4)+
	add.w	d6,d4
	addx.l	d5,d3
	addq.l	#2,a4
.1
 IFGE	__CPU-68020
	move.w	(a3,d3.l*4),d7
 ELSE
	move.l	d3,d7
	add.l	d7,d7
	add.l	d7,d7
	move.w	(a3,d7.l),d7
 ENDC
	asr.w	d1,d7
	add.w	d7,(a4)+
	add.w	d6,d4
	addx.l	d5,d3
	addq.l	#2,a4

	dbf	d0,.nextsample
.exit
	rts

;------------------------------------------------------------------------------
	cnop	0,16

;in
* d2.l	Shift count
AddWordsSVTr:
	lsr.w	#1,d0
	bcs.b	.1
	subq.w	#1,d0
	bmi.b	.exit
.nextsample
 IFGE	__CPU-68020
	move.w	2(a3,d3.l*4),d7
 ELSE
	move.l	d3,d7
	add.l	d7,d7
	add.l	d7,d7
	move.w	2(a3,d7.l),d7
 ENDC
	asr.w	d2,d7
	addq.l	#2,a4
	add.w	d7,(a4)+
	add.w	d6,d4
	addx.l	d5,d3
.1
 IFGE	__CPU-68020
	move.w	2(a3,d3.l*4),d7
 ELSE
	move.l	d3,d7
	add.l	d7,d7
	add.l	d7,d7
	move.w	2(a3,d7.l),d7
 ENDC
	asr.w	d2,d7
	addq.l	#2,a4
	add.w	d7,(a4)+
	add.w	d6,d4
	addx.l	d5,d3

	dbf	d0,.nextsample
.exit
	rts

;------------------------------------------------------------------------------
	cnop	0,16

;in
* d1.l	Shift count
* d2.l	Shift count
AddWordsSVPT:
	lsr.w	#1,d0
	bcs.b	.1
	subq.w	#1,d0
	bmi.b	.exit
.nextsample
 IFGE	__CPU-68020
	move.w	(a3,d3.l*4),d7
	asr.w	d1,d7
	add.w	d7,(a4)+
	move.w	2(a3,d3.l*4),d7
	asr.w	d2,d7
	add.w	d7,(a4)+
 ELSE
	move.l	d3,d7
	add.l	d7,d7
	add.l	d7,d7
	lea	(a3,d7.l),a0
	move.w	(a0)+,d7
	asr.w	d1,d7
	add.w	d7,(a4)+
	move.w	(a0),d7
	asr.w	d2,d7
	add.w	d7,(a4)+
 ENDC
	add.w	d6,d4
	addx.l	d5,d3
.1
 IFGE	__CPU-68020
	move.w	(a3,d3.l*4),d7
	asr.w	d1,d7
	add.w	d7,(a4)+
	move.w	2(a3,d3.l*4),d7
	asr.w	d2,d7
	add.w	d7,(a4)+
 ELSE
	move.l	d3,d7
	add.l	d7,d7
	add.l	d7,d7
	lea	(a3,d7.l),a0
	move.w	(a0)+,d7
	asr.w	d1,d7
	add.w	d7,(a4)+
	move.w	(a0),d7
	asr.w	d2,d7
	add.w	d7,(a4)+
 ENDC
	add.w	d6,d4
	addx.l	d5,d3

	dbf	d0,.nextsample
.exit
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
	sub.w	d6,d4
	subx.l	d5,d3
.1
	move.b	(a3,d3.l),d7
	ext.w	d7
	muls.w	d1,d7
	add.w	d7,(a4)+
	sub.w	d6,d4
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
	sub.w	d6,d4
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
	sub.w	d6,d4
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
	sub.w	d6,d4
	subx.l	d5,d3
	addq.l	#2,a4
.1
	move.b	(a3,d3.l),d7
	ext.w	d7
	muls.w	d1,d7
	add.w	d7,(a4)+
	sub.w	d6,d4
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
	sub.w	d6,d4
	subx.l	d5,d3
.1
	move.b	(a3,d3.l),d7
	addq.l	#2,a4
	ext.w	d7
	muls.w	d2,d7
	add.w	d7,(a4)+
	sub.w	d6,d4
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
	sub.w	d6,d4
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
	sub.w	d6,d4
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
	sub.w	d6,d4
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
	sub.w	d6,d4
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
	sub.w	d6,d4
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
	sub.w	d6,d4
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
	sub.w	d6,d4
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
	sub.w	d6,d4
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
	sub.w	d6,d4
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
	sub.w	d6,d4
	subx.l	d5,d3
	dbf	d0,.nextsample
.exit
	rts

;------------------------------------------------------------------------------
	cnop	0,16

;in
* d1.l	Pointer in multiplication table
* d2.l	Pointer in multiplication table
AddBytesMVTB:
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
	sub.w	d6,d4
	subx.l	d5,d3
.1
	move.b	0(a3,d3.l*2),d1
	move.w	0(a0,d1.w*4),d2		;signed multiplication
	move.b	1(a3,d3.l*2),d1
	add.w	0(a1,d1.w*4),d2		;signed multiplication
	add.w	d2,(a4)+
	sub.w	d6,d4
	subx.l	d5,d3

	dbf	d0,.nextsample
.exit
	pop	a1
	rts

;------------------------------------------------------------------------------
	cnop	0,16

;in
* d1.w	-256..256
AddBytesSVlB:
	lsr.w	#1,d0
	bcs.b	.1
	subq.w	#1,d0
	bmi.b	.exit
.nextsample
	move.b	0(a3,d3.l*2),d7
	ext.w	d7
	muls.w	d1,d7
	add.w	d7,(a4)+
	sub.w	d6,d4
	subx.l	d5,d3
	addq.l	#2,a4
.1
	move.b	0(a3,d3.l*2),d7
	ext.w	d7
	muls.w	d1,d7
	add.w	d7,(a4)+
	sub.w	d6,d4
	subx.l	d5,d3
	addq.l	#2,a4
	dbf	d0,.nextsample
.exit
	rts

;------------------------------------------------------------------------------
	cnop	0,16

;in
* d2.w	-256..256
AddBytesSVrB:
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
	sub.w	d6,d4
	subx.l	d5,d3
.1
	move.b	1(a3,d3.l*2),d7
	addq.l	#2,a4
	ext.w	d7
	muls.w	d2,d7
	add.w	d7,(a4)+
	sub.w	d6,d4
	subx.l	d5,d3
	dbf	d0,.nextsample
.exit
	rts

;------------------------------------------------------------------------------
	cnop	0,16

;in
* d1.w	-256..256
* d2.w	-256..256
AddBytesSVPB:
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
	sub.w	d6,d4
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
	sub.w	d6,d4
	subx.l	d5,d3
	dbf	d0,.nextsample
.exit
	rts

;------------------------------------------------------------------------------
	cnop	0,16

;in
* d1.l	Pointer in multiplication table
AddBytesSVTlB:
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
	sub.w	d6,d4
	subx.l	d5,d3
	addq.l	#2,a4
.1
	move.b	0(a3,d3.l*2),d1
	move.w	0(a0,d1.w*4),d2		;signed multiplication
	add.w	d2,(a4)+
	sub.w	d6,d4
	subx.l	d5,d3
	addq.l	#2,a4
	dbf	d0,.nextsample
.exit
	rts
;------------------------------------------------------------------------------
	cnop	0,16

;in
* d2.l	Pointer in multiplication table
AddBytesSVTrB:
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
	sub.w	d6,d4
	subx.l	d5,d3
.1
	move.b	1(a3,d3.l*2),d1
	move.w	0(a0,d1.w*4),d2		;signed multiplication
	addq.l	#2,a4
	add.w	d2,(a4)+
	sub.w	d6,d4
	subx.l	d5,d3
	dbf	d0,.nextsample
.exit
	rts

;------------------------------------------------------------------------------
	cnop	0,16

;in
* d1.l	Pointer in multiplication table
* d2.l	Pointer in multiplication table
AddBytesSVPTB:
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
	sub.w	d6,d4
	subx.l	d5,d3
.1
	move.b	0(a3,d3.l*2),d1
	move.w	0(a0,d1.w*4),d2
	add.w	d2,(a4)+
	move.b	1(a3,d3.l*2),d1
	move.w	0(a1,d1.w*4),d2
	add.w	d2,(a4)+
	sub.w	d6,d4
	subx.l	d5,d3
	dbf	d0,.nextsample
.exit
	move.l	d7,a1			;restore a1
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
	sub.w	d6,d4
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
	sub.w	d6,d4
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
	sub.w	d6,d4
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
	sub.w	d6,d4
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
	sub.w	d6,d4
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
	sub.w	d6,d4
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
	sub.w	d6,d4
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
	sub.w	d6,d4
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
	sub.w	d6,d4
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
	sub.w	d6,d4
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
	sub.w	d6,d4
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
	sub.w	d6,d4
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
	sub.w	d6,d4
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
	sub.w	d6,d4
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
	sub.w	d6,d4
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
	sub.w	d6,d4
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
	move.w	1(a3,d3.l*4),d7
	muls.w	d2,d7
	asr.l	#8,d7
	asr.l	#7,d7
	add.w	d7,(a4)+
	sub.w	d6,d4
	subx.l	d5,d3
.1
	move.w	0(a3,d3.l*4),d7
	muls.w	d1,d7
	asr.l	#8,d7
	asr.l	#7,d7
	add.w	d7,(a4)
	move.w	1(a3,d3.l*4),d7
	muls.w	d2,d7
	asr.l	#8,d7
	asr.l	#7,d7
	add.w	d7,(a4)+
	sub.w	d6,d4
	subx.l	d5,d3
.exit
	rts

;------------------------------------------------------------------------------
	cnop	0,16

;in
* d1.l	Shift Count
* d2.l	Shift Count
AddWordsMVTB:
	lsr.w	#1,d0
	bcs.b	.1
	subq.w	#1,d0
	bmi.b	.exit
.nextsample
 IFGE	__CPU-68020
	move.w	(a3,d3.l*4),d7
	asr.w	d1,d7
	add.w	d7,(a4)
	move.w	2(a3,d3.l*4),d7
	asr.w	d2,d7
	add.w	d7,(a4)+
 ELSE
	move.l	d3,d7
	add.l	d7,d7
	add.l	d7,d7
	lea	(a3,d7.l),a0
	move.w	(a0)+,d7
	asr.w	d1,d7
	add.w	d7,(a4)
	move.w	(a0),d7
	asr.w	d2,d7
	add.w	d7,(a4)+
 ENDC
	sub.w	d6,d4
	subx.l	d5,d3
.1
 IFGE	__CPU-68020
	move.w	(a3,d3.l*4),d7
	asr.w	d1,d7
	add.w	d7,(a4)
	move.w	2(a3,d3.l*4),d7
	asr.w	d2,d7
	add.w	d7,(a4)+
 ELSE
	move.l	d3,d7
	add.l	d7,d7
	add.l	d7,d7
	lea	(a3,d7.l),a0
	move.w	(a0)+,d7
	asr.w	d1,d7
	add.w	d7,(a4)
	move.w	(a0),d7
	asr.w	d2,d7
	add.w	d7,(a4)+
 ENDC
	sub.w	d6,d4
	subx.l	d5,d3

	dbf	d0,.nextsample
.exit
	rts

;------------------------------------------------------------------------------
	cnop	0,16

;in
* d1.l	-32768..32767
AddWordsSVlB:
* 16 bit signed input
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
	sub.w	d6,d4
	subx.l	d5,d3
	addq.l	#2,a4
.1
	move.w	0(a3,d3.l*4),d7
	muls.w	d1,d7
	asr.l	d2,d7
	add.w	d7,(a4)+
	sub.w	d6,d4
	subx.l	d5,d3
	addq.l	#2,a4
	dbf	d0,.nextsample
.exit
	rts

;------------------------------------------------------------------------------
	cnop	0,16

;in
* d2.l	-32768..32767/
AddWordsSVrB:
* 16 bit signed input
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
	sub.w	d6,d4
	subx.l	d5,d3
.1
	move.w	2(a3,d3.l*4),d7
	muls.w	d2,d7
	asr.l	d1,d7
	addq.l	#2,a4
	add.w	d7,(a4)+
	sub.w	d6,d4
	subx.l	d5,d3
	dbf	d0,.nextsample
.exit
	rts

;------------------------------------------------------------------------------
	cnop	0,16

;in
* d1.l	-32768..32767/-256..256
* d2.l	-32768..32767/-256..256
AddWordsSVPB:
* 16/8 bit signed input (8 for '000 version)
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
	sub.w	d6,d4
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
	sub.w	d6,d4
	subx.l	d5,d3
	dbf	d0,.nextsample
.exit
	rts

;------------------------------------------------------------------------------
	cnop	0,16

;in
* d1.l	Shift count
AddWordsSVTlB:
	lsr.w	#1,d0
	bcs.b	.1
	subq.w	#1,d0
	bmi.b	.exit
.nextsample
 IFGE	__CPU-68020
	move.w	(a3,d3.l*4),d7
 ELSE
	move.l	d3,d7
	add.l	d7,d7
	add.l	d7,d7
	move.w	(a3,d7.l),d7
 ENDC
	asr.w	d1,d7
	add.w	d7,(a4)+
	sub.w	d6,d4
	subx.l	d5,d3
	addq.l	#2,a4
.1
 IFGE	__CPU-68020
	move.w	(a3,d3.l*4),d7
 ELSE
	move.l	d3,d7
	add.l	d7,d7
	add.l	d7,d7
	move.w	(a3,d7.l),d7
 ENDC
	asr.w	d1,d7
	add.w	d7,(a4)+
	sub.w	d6,d4
	subx.l	d5,d3
	addq.l	#2,a4

	dbf	d0,.nextsample
.exit
	rts

;------------------------------------------------------------------------------
	cnop	0,16

;in
* d2.l	Shift count
AddWordsSVTrB:
	lsr.w	#1,d0
	bcs.b	.1
	subq.w	#1,d0
	bmi.b	.exit
.nextsample
 IFGE	__CPU-68020
	move.w	2(a3,d3.l*4),d7
 ELSE
	move.l	d3,d7
	add.l	d7,d7
	add.l	d7,d7
	move.w	2(a3,d7.l),d7
 ENDC
	asr.w	d2,d7
	addq.l	#2,a4
	add.w	d7,(a4)+
	sub.w	d6,d4
	subx.l	d5,d3
.1
 IFGE	__CPU-68020
	move.w	2(a3,d3.l*4),d7
 ELSE
	move.l	d3,d7
	add.l	d7,d7
	add.l	d7,d7
	move.w	2(a3,d7.l),d7
 ENDC
	asr.w	d2,d7
	addq.l	#2,a4
	add.w	d7,(a4)+
	sub.w	d6,d4
	subx.l	d5,d3

	dbf	d0,.nextsample
.exit
	rts

;------------------------------------------------------------------------------
	cnop	0,16

;in
* d1.l	Shift count
* d2.l	Shift count
AddWordsSVPTB:
	lsr.w	#1,d0
	bcs.b	.1
	subq.w	#1,d0
	bmi.b	.exit
.nextsample
 IFGE	__CPU-68020
	move.w	(a3,d3.l*4),d7
	asr.w	d1,d7
	add.w	d7,(a4)+
	move.w	2(a3,d3.l*4),d7
	asr.w	d2,d7
	add.w	d7,(a4)+
 ELSE
	move.l	d3,d7
	add.l	d7,d7
	add.l	d7,d7
	lea	(a3,d7.l),a0
	move.w	(a0)+,d7
	asr.w	d1,d7
	add.w	d7,(a4)+
	move.w	(a0),d7
	asr.w	d2,d7
	add.w	d7,(a4)+
 ENDC
	sub.w	d6,d4
	subx.l	d5,d3
.1
 IFGE	__CPU-68020
	move.w	(a3,d3.l*4),d7
	asr.w	d1,d7
	add.w	d7,(a4)+
	move.w	2(a3,d3.l*4),d7
	asr.w	d2,d7
	add.w	d7,(a4)+
 ELSE
	move.l	d3,d7
	add.l	d7,d7
	add.l	d7,d7
	lea	(a3,d7.l),a0
	move.w	(a0)+,d7
	asr.w	d1,d7
	add.w	d7,(a4)+
	move.w	(a0),d7
	asr.w	d2,d7
	add.w	d7,(a4)+
 ENDC
	sub.w	d6,d4
	subx.l	d5,d3

	dbf	d0,.nextsample
.exit
	rts

;------------------------------------------------------------------------------
AlignEnd

 IFNE	DEBUG
sample
;	dc.b	$5,$10,$5,0,-$5,-$10,-$5,0
	dc.b	$AA,$AA,$AA,$AA,$AA,$AA,$AA,$AA
sample_len=*-sample

	blk.w	SAMPLES,$5555
buffer		blk.b	BUFFER,'*'
end
 ENDC
