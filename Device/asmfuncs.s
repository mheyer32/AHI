* $Id$
* $Log$
* Revision 1.16  1997/02/12 15:32:45  lcs
* Moved each autodoc header to the file where the function is
*
* Revision 1.15  1997/02/10 02:23:06  lcs
* Infowindow in the requester added.
*
* Revision 1.14  1997/02/09 19:02:01  lcs
* Hertz was misspelled!
*
* Revision 1.13  1997/02/03 16:23:42  lcs
* AHIR_Locale should work now
*
* Revision 1.11  1997/02/02 18:15:04  lcs
* Added protection against CPU overload
*
* Revision 1.10  1997/02/01 23:54:26  lcs
* Rewrote the library open code in C and removed the library bases
* from AHIBase
*
* Revision 1.9  1997/02/01 21:54:53  lcs
* Max freq. for AHI_SetFreq() increased to more than one million! ;)
*
* Revision 1.8  1997/02/01 19:44:18  lcs
* Added stereo samples
*
* Revision 1.7  1997/01/31 19:27:14  lcs
* Added stereo samples to AHI_LoadSound()
* Moved the DSPEcho intialization and deintialization routines
* to a separate file
*
* Revision 1.6  1997/01/15 18:35:07  lcs
* AHIB_Dizzy has a better implementation and definition now.
* (Was BOOL, now pointer to a second tag list)
*
* Revision 1.4  1997/01/04 20:19:56  lcs
* Changed the AHI_DEBUG levels
*
* Revision 1.3  1997/01/04 13:26:41  lcs
* Doc for CMD_WRITE updated
*
* Revision 1.2  1996/12/21 23:06:35  lcs
* Updated doc for CMD_WRITE
*
* Revision 1.1  1996/12/21 13:05:12  lcs
* Initial revision
*

; TODO:
; Konverteringsrutiner

	incdir	include:

	include devices/timer.i
	include	exec/exec.i
	include	dos/dos.i
	include	utility/utility.i
	include	utility/hooks.i
	include	devices/ahi.i
	include	libraries/ahi_sub.i
	include	lvo/exec_lib.i
	include	lvo/timer_lib.i
	include	lvo/utility_lib.i
	include	lvo/ahi_lib.i

	include	ahi_def.i

	XDEF	_SetVol
	XDEF	_SetFreq
	XDEF	_SetSound
	XDEF	_SetEffect
	XDEF	_LoadSound
	XDEF	_UnloadSound

	XDEF	_RecalcBuff
	XDEF	_InitMixroutine
	XDEF	_PreTimer
	XDEF	_PostTimer
	XDEF	_DummyPreTimer
	XDEF	_DummyPostTimer

	XDEF	_DefPlayerHook
	XDEF	_DefRecordHook

	XREF	_DriverVersion
	XREF	_CreateAudioCtrl
	XREF	_UpdateAudioCtrl
	XREF	_LockDatabase
	XREF	_UnlockDatabase
	XREF	_GetDBTagList

	XREF	SelectAddRoutine
	XREF	initUnsignedTable
	XREF	initSignedTable
	XREF	calcSignedTable
	XREF	calcUnsignedTable
	XREF	CalcSamples
	XREF	update_DSPEcho
	XREF	free_DSPEcho

	XREF	_TimerBase
	XREF	_UtilityBase

_DefPlayerHook:
_DefRecordHook:
	dc.l	0,0		;MinNode
	dc.l	.func		;h_Entry
	dc.l	0,0		;h_SubEntry, h_Data
.func
	rts

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
*       * Driver based
*
*       Each supported sound card is controlled by a library-based audio
*       driver. For a 'dumb' sound card, a new driver should be written in
*       a few hours. For a 'smart' sound card, it is possible to utilize an
*       on-board DSP, for example, to maximize performance and sound
*       quality. For sound cards with own DSP but little or no memory, it is
*       possible to use the main CPU to mix channels and do the post-
*       processing with the DSP.
*
*       * Fast, powerful mixing routines (yeah, right... haha)
*
*       The device's mixing routines mix 8- or 16-bit signed samples
*       located in Fast-RAM and outputs 16-bit mono or stereo (with stereo
*       panning if desired) data, using any number of channels (as long as
*       'any' means less than 128...). Tables can be used speed the mixing
*       up (especially when using 8-bit samples). The samples can have any
*       length (including odd) and can have any number of loops. For non-
*       realtime purposes so-called HiFi mixing routines can be used, which
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
*       Audio modes are added to the database with the tool 'AddAudioModes',
*       which either takes IFF-AHIM file(s) as argument or scans
*       'DEVS:AudioModes/' and adds all files found there. The device also
*       features a requester to get an ID code from the user.
*
*       * Future Compatible
*
*       When AmigaOS gets device-independent audio worth it's name, it should
*       not be too difficult to write a driver for AHI, allowing applications
*       using 'ahi.device' to automatically use the new OS interface. At
*       least I hope it wont.
*
*   PROGRAMMING GUIDELINES
*
*        * Follow the rules
*
*        It's really simple. If I tell you to check return values, check
*        sample types when recording, don't trash d2-d7/a2-a6 in Hooks, or
*        don't call AHI_ControlAudio() with the AHIC_Play tag from interrups
*        or Hooks, you do as you are told.
*
*        * The library base
*
*        The AHIBase structure is private, so are the sub libraries' library
*        base structures. Don't try to be clever.
*
*        * The Audio Database
*
*        The implementation of the database is private, and may change any
*        time. 'ahi.device' provides functions access the information in
*        the database (AHI_NextAudioID(), AHI_GetAudioAttrsA() and
*        AHI_BestAudioIDA()).
*
*        * User Hooks
*
*        All user Hooks must follow normal register conventions, which means
*        that d2-d7 and a2-a6 must be presered. They may be called from an
*        interrupt, but you cannot count on that, it can be your own process
*        or another process. Don't assume system is in single-thread mode.
*        Never spend much time in a Hook, get the work done as quick as
*        possible and then return.
*
*        * Function calls from other tasks, interrupts or user Hooks
*
*        The AHIAudioCtrl structure may not be shared with other tasks/
*        threads. The task that called AHI_AllocAudioA() must do all other
*        calls too (except those callable from interrupts).
*
*        Only calls specifically said to be callable from interrups may be
*        called from user Hooks or interrupts. Note that AHI_ControlAudioA()
*        have some tags that must not be present when called from an
*        interrupt.
*
*        * AHI_SetVol(), AHI_SetFreq() and AHI_SetSound() Flags
*
*        These function calls take some flags as arguments. Currently, only
*        one flag is defined, AHISF_IMM, which causes the changes to take
*        effecy immediately. THIS FLAG SHOULD *NEVER* BE SET when you call
*        these routines from a SoundFunc. This is very important. If these
*        functions are called from a PlayerFunc, this flag may be both set
*        or cleared. If they are called from neither a SoundFunc nor a
*        PlayerFunc, the flag MUST BE SET.
*
*        * System requirements
*
*        V37 of the system libraries are required.
*
*        'ahi.device', or rather most audio drivers, need multitasking to be
*        turned on to function properly. Don't turn it off when using the
*        device.
*
*        Some drivers use 'realtime.library', which must be present for those
*        drivers to work. 'realtime.library' needs a free CIA timer. If your
*        application uses a CIA timer, it is strongly recommended that you
*        use the PlayerFunc in 'ahi.device' instead.
*
****************************************************************************
*
*


****** ahi.device/AHI_SetVol ***********************************************
*
*   NAME
*       AHI_SetVol -- set volume and stereo panning for a channel
*
*   SYNOPSIS
*       AHI_SetVol( channel, volume, pan, audioctrl, flags );
*                   D0:16    D1      D2   A2         D3
*
*       void AHI_SetVol( UWORD, Fixed, sposition, struct AHIAudioCtrl *,
*                        ULONG );
*
*   FUNCTION
*       Changes the volume and stereo panning for a channel.
*
*   INPUTS
*       channel - The channel to set volume for.
*       volume - The desired volume. Fixed is a LONG fixed-point value with
*           16 bits to the left of the point and 16 to the right
*           (typedef LONG Fixed; from IFF-8SVX docs).
*           Maximum volume is 1.0 (0x10000L) and 0.0 (0x0L) will turn off
*           this channel. Note: The sound will continue to play, but you
*           wont hear it. To stop a sound completely, use AHI_SetSound().
*           Starting with V3 volume can also be negative, which tells AHI
*           to invert the samples before playing. Note that all drivers
*           may not be able to handle negative volume. In that case the
*           absolute volume will be used.
*       pan - The desired panning. sposition is the same as Fixed
*           (typedef Fixed sposition; from IFF-8SVX.PAN docs).
*           1.0 (0x10000L) means that the sound is panned all the way to
*           the right, 0.5 (0x8000L) means the sound is centered and 0.0
*           (0x0L) means that the sound is panned all the way to the left.
*           Try to set Pan to the 'correct' value even if you know it has no
*           effect. For example, if you know you use a mono mode, set pan to
*           0.5 even if it does not matter.
*           Starting with V3 pan can also be negative, which tells AHI to
*           use the surround speaker for this channel. Note that all drivers
*           may not be able to handle negative pan. In that case the absolute
*           pan will be used.
*       audioctrl - A pointer to an AHIAudioCtrl structure.
*       flags - Only one flag is defined
*           AHISF_IMM - Set this flag if this command should take effect
*               immediately. If this bit is not set, the command will not
*               take effect until the current sound is finished. MUST NOT
*               be set if called from a SoundFunc. See the programming
*               guidlines for more information about this flag.
*
*   RESULT
*
*   EXAMPLE
*
*   NOTES
*       It is safe to call this function from an interrupt.
*
*       Negative volume or negative pan may use more CPU time than positive.
*
*       Using both negative volume and negative pan will play the inverted
*       sound on the surround speaker.
*
*   BUGS
*
*   SEE ALSO
*       AHI_SetEffect(), AHI_SetFreq(), AHI_SetSound(), AHI_LoadSound()
*       
*
****************************************************************************
*
*

_SetVol:
	cmp.b	#AHI_DEBUG_ALL,ahib_DebugLevel(a6)
	blo	SetVol_nodebug
	and.l	#$ffff,d0
	PRINTF	2,"AHI_SetVol(%ld, 0x%08lx, 0x%08lx, 0x%08lx, 0x%08lx)",d0,d1,d2,a2,d3
SetVol_nodebug

	pushm	d1/a0-a1/a6

*** Disable surround sound?
	btst.b	#AHIBB_NOSURROUND,ahib_Flags(a6)
	beq	.surround_ok
	tst.l	d1
	bpl	.vol_pos
	neg.l	d1
.vol_pos
	tst.l	d2
	bpl	.vol_pan
	neg.l	d2
.vol_pan
.surround_ok

	push	d0
	move.l	ahiac_SubLib(a2),a6
	call	AHIsub_SetVol
	btst.b	#AHIACB_NOMIXING-24,ahiac_Flags(a2)
	bne	.1
	cmp.l	#AHIS_UNKNOWN,d0
	beq	.2
.1
	moveq	#0,d0
	addq.l	#4,sp				;skip d0
	popm	d1/a0-a1/a6
	rts
.2
	pop	d0
	popm	d1/a0-a1/a6

	pushm	d2-d7/a2-a6
	move.l	ahiac_SubLib(a2),a6
	call	AHIsub_Disable

	btst.b	#AHIACB_VOL,ahiac_Flags+3(a2)
	bne	.volume
	clr.w	d1				;|vol|=0 or $10000
.volume
	btst.b	#AHIACB_PAN,ahiac_Flags+3(a2)
	bne	.pan
	move.w	d0,d2
	and.l	#1,d2
	swap.w	d2				;pan=0 or $10000
.pan
	mulu.w	#AHIChannelData_SIZEOF,d0
	move.l	ahiac_ChannelDatas(a2),a0
	add.l	d0,a0

* Calculate left and right volume

 IFGE	__CPU-68020
	asr.l	#1,d1
	asr.l	#1,d2
	move.l	d2,d0
	sub.l	#$10000>>1,d2
	neg.l	d2

	muls.l	d1,d0
	asr.l	#8,d0
	asr.l	#8-(1*2),d0
	move.l	d0,cd_NextVolumeRight(a0)
	muls.l	d1,d2
	asr.l	#8,d2
	asr.l	#8-(1*2),d2
	move.l	d2,cd_NextVolumeLeft(a0)
 ELSE
	asr.l	#2,d1
	asr.l	#2,d2
	move.l	d2,d0
	sub.l	#$10000>>2,d2
	neg.l	d2

	muls.w	d1,d0
	asr.l	#8,d0
	asr.l	#8-(2*2),d0
	move.l	d0,cd_NextVolumeRight(a0)
	muls.w	d1,d2
	asr.l	#8,d2
	asr.l	#8-(2*2),d2
	move.l	d2,cd_NextVolumeLeft(a0)
 ENDC

	btst.l	#AHISB_IMM,d3
	beq	.notnow
	move.l	cd_NextVolumeLeft(a0),cd_VolumeLeft(a0)
	move.l	cd_NextVolumeRight(a0),cd_VolumeRight(a0)

	move.l	cd_VolumeLeft(a0),d0
	move.l	cd_VolumeRight(a0),d1
	move.l	cd_Type(a0),d2
	bsr.w	SelectAddRoutine
	move.l	d0,cd_ScaleLeft(a0)
	move.l	d1,cd_ScaleRight(a0)
	move.l	d2,cd_AddRoutine(a0)
.notnow
	move.l	cd_NextVolumeLeft(a0),d0
	move.l	cd_NextVolumeRight(a0),d1
	move.l	cd_NextType(a0),d2
	bsr.w	SelectAddRoutine
	move.l	d0,cd_NextScaleLeft(a0)
	move.l	d1,cd_NextScaleRight(a0)
	move.l	d2,cd_NextAddRoutine(a0)

	call	AHIsub_Enable			;a2 ok

	moveq	#0,d0
	popm	d2-d7/a2-a6
	rts


****** ahi.device/AHI_SetFreq **********************************************
*
*   NAME
*       AHI_SetFreq -- set frequency for a channel
*
*   SYNOPSIS
*       AHI_SetFreq( channel, freq, audioctrl, flags );
*                    D0:16    D1    A2         D2
*
*       void AHI_SetFreq( UWORD, ULONG, struct AHIAudioCtrl *, ULONG );
*
*   FUNCTION
*       Sets the playback frequency for a channel.
*
*   INPUTS
*       channel - The channel to set playback frequency for.
*       freq - The playback frequency in Hertz. Can also be AHI_MIXFREQ,
*           is the current mixing frequency (only usable with AHIST_INPUT
*           sounds), or 0 to temporary stop the sound (it will restart at
*           the same point when its frequency changed).
*       audioctrl - A pointer to an AHIAudioCtrl structure.
*       flags - Only one flag is defined
*           AHISF_IMM - Set this flag if this command should take effect
*               immediately. If this bit is not set, the command will not
*               take effect until the current sound is finished. MUST NOT
*               be set if called from a SoundFunc. See the programming
*               guidlines for more information about this flag.
*
*   RESULT
*
*   EXAMPLE
*
*   NOTES
*       It is safe to call this function from an interrupt.
*
*   BUGS
*
*   SEE ALSO
*       AHI_SetEffect(),  AHI_SetSound(), AHI_SetVol(), AHI_LoadSound()
*
****************************************************************************
*
*

_SetFreq:
	cmp.b	#AHI_DEBUG_ALL,ahib_DebugLevel(a6)
	blo	SetFreq_nodebug
	and.l	#$ffff,d0
	PRINTF	2,"AHI_SetFreq(%ld, %ld, 0x%08lx, 0x%08lx)",d0,d1,a2,d2
SetFreq_nodebug

	pushm	d1/a0-a1/a6
	push	d0
	move.l	ahiac_SubLib(a2),a6
	call	AHIsub_SetFreq
	btst.b	#AHIACB_NOMIXING-24,ahiac_Flags(a2)
	bne	.1
	cmp.l	#AHIS_UNKNOWN,d0
	beq	.2
.1
	moveq	#0,d0
	addq.l	#4,sp				;skip d0
	popm	d1/a0-a1/a6
	rts
.2
	pop	d0
	popm	d1/a0-a1/a6

	pushm	d2-d7/a2-a6
	move.l	a6,a5
	move.l	ahiac_SubLib(a2),a6
	call	AHIsub_Disable

	mulu.w	#AHIChannelData_SIZEOF,d0
	move.l	ahiac_ChannelDatas(a2),a0
	add.l	d0,a0

	tst.l	d1
	bne	.normalfreq
	clr.b	cd_NextFreqOK(a0)
	moveq	#0,d0				;If cd_NextFreqOK is NOT checked by the
	bra	.setperiod			;mixer, this could trigger a Div-by-0!
.normalfreq
	move.b	#$ff,cd_NextFreqOK(a0)

	cmp.l	#AHI_MIXFREQ,d1
	bne	.not_mixfreq
	move.l	ahiac_MixFreq(a2),d1
.not_mixfreq
	move.l	d1,d0

* Calculate period
	cmp.l	#65536,d0
	bhs	.gt64k
	moveq	#0,d3
	bra	.shiftfreq
.gt64k
	cmp.l	#262144,d0
	bhs	.gt256k
	moveq	#2,d3
	bra	.shiftfreq
.gt256k
	moveq	#4,d3
.shiftfreq
	moveq	#16,d1
	sub.l	d3,d1
	lsl.l	d1,d0
	move.l	ahiac_MixFreq(a2),d1
 IFGE	__CPU-68020
	divu.l	d1,d0
 ELSE
	move.l	_UtilityBase(pc),a1
	jsr	_LVOUDivMod32(a1)
 ENDC
.setperiod
	lsl.l	d3,d0


	move.w	d0,cd_NextAddF(a0)
	clr.w	d0
	swap.w	d0
	move.l	d0,cd_NextAddI(a0)

	btst.l	#AHISB_IMM,d2
	beq	.notnow

	move.l	cd_NextAddI(a0),cd_AddI(a0)
	move.w	cd_NextAddF(a0),cd_AddF(a0)
	move.b	cd_NextFreqOK(a0),cd_FreqOK(a0)

	move.l	cd_AddI(a0),d0
	move.w	cd_AddF(a0),d1
	move.l	cd_Type(a0),d2
	move.l	cd_LastOffsetI(a0),d3
	move.w	cd_LastOffsetF(a0),d4
	move.l	cd_OffsetI(a0),d5
	move.w	cd_OffsetF(a0),d6
	bsr	CalcSamples
	move.l	d0,cd_Samples(a0)

.notnow
	call	AHIsub_Enable			;a2 ok

	moveq	#0,d0
	popm	d2-d7/a2-a6
	rts


****** ahi.device/AHI_SetSound *********************************************
*
*   NAME
*       AHI_SetSound -- set what sound to play for a channel
*
*   SYNOPSIS
*       AHI_SetSound( channel, sound, offset, length, audioctrl, flags );
*                      D0:16   D1:16   D2      D3      A2         D4
*
*       void AHI_SetSound( UWORD, UWORD, ULONG, LONG,
*                          struct AHIAudioCtrl *, ULONG );
*
*   FUNCTION
*       Sets a sound to be played on a channel.
*
*   INPUTS
*       channel - The channel to set sound for.
*       sound - Sound to be played, or AHI_NOSOUND to turn the channel off.
*       offset - Only available if the sound type is AHIST_SAMPLE or
*           AHIST_DYNAMICSAMPLE. Must be 0 otherwise.
*           Specifies an offset (in samples) where the playback will begin.
*           If you wish to play the whole sound, set offset to 0.
*       length - Only available if the sound type is AHIST_SAMPLE or
*           AHIST_DYNAMICSAMPLE. Must be 0 otherwise.
*           Specifies how many samples that should be played. If you
*           wish to play the whole sound forwards, set offset to 0 and length
*           to either 0 or the length of the sample array. You may not set
*           length to 0 if offset is not 0! To play a sound backwards, just
*           set length to a negative number.
*       audioctrl - A pointer to an AHIAudioCtrl structure.
*       flags - Only one flag is defined
*           AHISF_IMM - Set this flag if this command should take effect
*               immediately. If this bit is not set, the command will not
*               take effect until the current sound is finished. MUST NOT
*               be set if called from a SoundFunc. See the programming
*               guidlines for more information about this flag.
*
*   RESULT
*
*   EXAMPLE
*
*   NOTES
*       It is safe to call this function from an interrupt.
*
*       If offset or length is not zero, make sure you do not exceed the
*       sample limits.
*
*   BUGS
*
*   SEE ALSO
*       AHI_SetEffect(),  AHI_SetFreq(), AHI_SetVol(), AHI_LoadSound()
*
****************************************************************************
*
*

_SetSound:
	cmp.b	#AHI_DEBUG_ALL,ahib_DebugLevel(a6)
	blo	SetSound_nodebug
	and.l	#$ffff,d0
	and.l	#$ffff,d1
	PRINTF	2,"AHI_SetSound(%ld, %ld, 0x%08lx, 0x%08lx, 0x%08lx, 0x%08lx)",d0,d1,d2,d3,a2,d4
SetSound_nodebug

	pushm	d1/a0-a1/a6
	push	d0
	move.l	ahiac_SubLib(a2),a6
	call	AHIsub_SetSound
	btst.b	#AHIACB_NOMIXING-24,ahiac_Flags(a2)
	bne	.1
	cmp.l	#AHIS_UNKNOWN,d0
	beq	.2
.1
	moveq	#0,d0
	addq.l	#4,sp				;skip d0
	popm	d1/a0-a1/a6
	rts
.2
	pop	d0
	popm	d1/a0-a1/a6

	pushm	d2-d7/a2-a6
	move.l	ahiac_SubLib(a2),a6
	call	AHIsub_Disable

	mulu.w	#AHIChannelData_SIZEOF,d0
	move.l	ahiac_ChannelDatas(a2),a0
	add.l	d0,a0

	cmp.w	#AHI_NOSOUND,d1
	bne	.not_off
	clr.b	cd_NextSoundOK(a0)
	btst.l	#AHISB_IMM,d4
	beq.w	.exit
	clr.b	cd_SoundOK(a0)
	move.w	#$ffff,cd_EOS(a0)		;Signal End-Of-Sample
	bra.w	.exit
.not_off

	mulu.w	#AHISoundData_SIZEOF,d1
	move.l	ahiac_SoundDatas(a2),a1
	add.l	d1,a1

	move.l	sd_Type(a1),d0
	cmp.l	#AHIST_NOTYPE,d0
	beq.w	.exit
	tst.l	d3
	bne	.10
	move.l	sd_Length(a1),d3
.10
	move.l	sd_Addr(a1),cd_NextDataStart(a0)
	move.l	sd_Type(a1),cd_NextType(a0)
.11

	move.l	d2,cd_NextOffsetI(a0)
	add.l	d3,d2

* Set AHIST_BW flag if negative length
	tst.l	d3
	bpl	.poslength
	or.l	#AHIST_BW,cd_NextType(a0)

	clr.w	cd_NextLastOffsetF(a0)
	move.w	#$ffff,cd_NextOffsetF(a0)
	addq.l	#1,d2
	move.l	d2,cd_NextLastOffsetI(a0)
	bra	.20
.poslength
	move.w	#$ffff,cd_NextLastOffsetF(a0)
	clr.w	cd_NextOffsetF(a0)
	subq.l	#1,d2
	move.l	d2,cd_NextLastOffsetI(a0)
.20
	move.b	#$ff,cd_NextSoundOK(a0)
	btst.l	#AHISB_IMM,d4
	beq	.notnow

	move.l	cd_NextOffsetI(a0),cd_OffsetI(a0)
	move.l	cd_NextOffsetI(a0),cd_FirstOffsetI(a0)		;for linear interpol.
	move.w	cd_NextOffsetF(a0),cd_OffsetF(a0)
	move.l	cd_NextLastOffsetI(a0),cd_LastOffsetI(a0)
	move.w	cd_NextLastOffsetF(a0),cd_LastOffsetF(a0)
	move.l	cd_NextDataStart(a0),cd_DataStart(a0)
	move.l	cd_NextType(a0),cd_Type(a0)
	move.b	cd_NextSoundOK(a0),cd_SoundOK(a0)

	move.l	cd_VolumeLeft(a0),d0
	move.l	cd_VolumeRight(a0),d1
	move.l	cd_Type(a0),d2
	bsr.w	SelectAddRoutine
	move.l	d0,cd_ScaleLeft(a0)
	move.l	d1,cd_ScaleRight(a0)
	move.l	d2,cd_AddRoutine(a0)


	move.l	cd_AddI(a0),d0
	move.w	cd_AddF(a0),d1
	move.l	cd_Type(a0),d2
	move.l	cd_LastOffsetI(a0),d3
	move.w	cd_LastOffsetF(a0),d4
	move.l	cd_OffsetI(a0),d5
	move.w	cd_OffsetF(a0),d6
	bsr	CalcSamples
	move.l	d0,cd_Samples(a0)

	move.w	#$ffff,cd_EOS(a0)		;Signal End-Of-Sample
.notnow
	move.l	cd_NextVolumeLeft(a0),d0
	move.l	cd_NextVolumeRight(a0),d1
	move.l	cd_NextType(a0),d2
	bsr.w	SelectAddRoutine
	move.l	d0,cd_NextScaleLeft(a0)
	move.l	d1,cd_NextScaleRight(a0)
	move.l	d2,cd_NextAddRoutine(a0)
.exit
	call	AHIsub_Enable			;a2 ok
	moveq	#0,d0
	popm	d2-d7/a2-a6
	rts


****** ahi.device/AHI_SetEffect ********************************************
*
*   NAME
*       AHI_SetEffect -- set effect
*
*   SYNOPSIS
*       error = AHI_SetEffect( effect, audioctrl );
*       d0                     A0      A2
*
*       ULONG AHI_SetEffect( APTR, struct AHIAudioCtrl * );
*
*   FUNCTION
*       Selects an effect to be used, described by a structure.
*
*   INPUTS
*       effect - A pointer to an effect data structure, as defined in
*           <devices/ahi.h>. The following effects are defined:
*           AHIET_MASTERVOLUME - Changes the volume for all channels. Can
*               also be used to boost volume over 100%.
*           AHIET_OUTPUTBUFFER - Gives READ-ONLY access to the mixed output.
*               Can be used to show nice scopes and VU-meters.
*           AHIET_DSPMASK - Select which channels will be affected by the
*               DSP effects. (V3)
*           AHIET_DSPECHO - A DSP effects that adds (cross-)echo and delay.
*               (V3)
*       audioctrl - A pointer to an AHIAudioCtrl structure.
*
*   EFFECTS
*       AHIET_MASTERVOLUME - Effect is a struct AHIEffMasterVolume, with
*           ahiemv_Volume set to the desired volume. The range is 0 to
*           (channels/hardware channel). Assume you have 4 channels in
*           mono mode. The range is then 0.0 to 4.0. The range is the same
*           if the mode is stereo with panning. However, assume you have 4
*           channels with a stereo mode *without* panning. Then you have two
*           channels to the left and two to the right => range is 0.0 - 2.0.
*           Setting the volume outside the range will give an unpredictable
*           result!
*
*       AHIET_OUTPUTBUFFER - Effect is a struct AHIEffOutputBuffer, with
*           ahieob_Func pointing to a hook that will be called with the
*           following parameters:
*               A0 - (struct Hook *)
*               A2 - (struct AHIAudioCtrl *)
*               A1 - (struct AHIEffOutputBuffer *)
*           The information you are looking for then is in ahieob_Type,
*           ahieob_Buffer and ahieob_Length. Always check ahieob_Type!
*           ahieob_Length is neither in bytes nor samples, but sample frames.
*
*       AHIET_DSPMASK - Effect is a struct AHIEffDSPMask, where ahiddm_Mask
*           is an array with ahiedm_Channels elements. Each UBYTE in the
*           array can either make the channel 'wet' (affected by the DSP
*           effects), by using the AHIEDM_WET constant or 'dry' (not
*           affected by the DSP effects) by using the AHIEDM_DRY constant.
*           The default is all channels wet. If ahiedm_Channels does not
*           equal the current number of channels allocated, the result of
*           this call is undefined (crash warning!). (V3)
*
*       AHIET_DSPECHO - Effect is a struct AHIEffDSPEcho.
*           ahiede_Delay is the delay in samples (and thus depends on the
*           mixing rate).
*
*           ahiede_Feedback is a Fixed value between 0 and 1.0, and defines
*           how much of the delayed signal should be feed back to the delay
*           stage. Setting this to 0 gives a delay effect, otherwise echo.
*
*           ahiede_Mix tells how much of the delayed signal should be mixed
*           with the normal signal. Setting this to 0 disables delay/echo,
*           and setting it to 1.0 outputs only the delay/echo signal.
*
*           ahiede_Cross only has effect of the current playback mode is
*           stereo. It tells how the delayed signal should be panned to
*           the other channel. 0 means no cross echo, 1.0 means full
*           cross echo.
*
*           If the user has enabled "Fast Echo", AHI may take several short-
*           cuts to increase the performance. This could include rounding the
*           parameters to a power of two, or even to the extremes. Without
*           "Fast Echo", this effect will suck some major CPU cycles on most
*           sound hardware. (V3)
*
*       AHIET_CHANNELINFO - Effect is a struct AHIEffChannelInfo, where
*           ahieci_Func is pointing to a hook that will be called with the
*           following parameters:
*               A0 - (struct Hook *)
*               A2 - (struct AHIAudioCtrl *)
*               A1 - (struct AHIEffChannelInfo *)
*           ahieci_Channels must equal the current number of channels used.
*           ahieci_Offset is an array of ULONGs, which will be filled by
*           AHI before the hook is called (the offset is specifed in sample
*           frames). The array must have at least ahieci_Channels elements.
*
*           This "effect" can be used to find out how far each channel has
*           played. You must probably keep track of the other parameters
*           yourself (like which sound is playing, it's volume, balance and
*           frequency etc) in order have meaningful usage of the information.
*           (V3)
*
*
*       NOTE! To turn off an effect, call again with ahie_Effect OR:ed
*       with AHIET_CANCEL. For example, it is NOT correct to disable
*       the AHIET_MASTERVOLUME effect by setting ahiemv_Volume to 1.0!
*
*       It is important that you always turn off effects before you
*       deallocate the audio hardware. Otherwise memory may be lost.
*       It is safe to turn off an effect that has never been turned on
*       in the first place.
*
*       Never count on that an effect will be available. For example,
*       AHIET_OUTPUTBUFFER is impossible to implement with some sound
*       cards.
*
*   RESULT
*       An error code, defined in <devices/ahi.h>.
*
*   EXAMPLE
*
*   NOTES
*       Unlike the other functions whose names begin with "AHI_Set", this
*       function may NOT be called from an interrupt (or AHI Hook).
*
*       Previous to V3, this call always returned AHIE_OK.
*
*   BUGS
*
*   SEE ALSO
*       AHI_SetFreq(), AHI_SetSound(), AHI_SetVol(), AHI_LoadSound(),
*       <devices/ahi.h>
*
****************************************************************************
*
*

_SetEffect:
	cmp.b	#AHI_DEBUG_ALL,ahib_DebugLevel(a6)
	blo	SetEffect_nodebug
	PRINTF	2,"AHI_SetEffect(x%08lx, 0x%08lx)",a0,a2
SetEffect_nodebug

	pushm	d1/a0-a1/a6
	push	d0
	move.l	ahiac_SubLib(a2),a6
	call	AHIsub_SetEffect
	btst.b	#AHIACB_NOMIXING-24,ahiac_Flags(a2)
	bne	.1
	cmp.l	#AHIS_UNKNOWN,d0
	beq	.2
.1
	moveq	#0,d0
	addq.l	#4,sp				;skip d0
	popm	d1/a0-a1/a6
	rts
.2
	pop	d0
	popm	d1/a0-a1/a6

	pushm	d2-d7/a2-a6
	move.l	a6,a5

*
* MASTERVOLUME
*
	cmp.l	#AHIET_MASTERVOLUME,ahie_Effect(a0)
	bne	.no_mastervolume
	move.l	ahiemv_Volume(a0),d0
	cmp.l	ahiac_MasterVolume(a2),d0
	beq	.exit				;same value as before!
	move.l	ahiemv_Volume(a0),ahiac_MasterVolume(a2)
	bsr	update_MasterVolume
	bra	.exit
.no_mastervolume

	cmp.l	#AHIET_CANCEL|AHIET_MASTERVOLUME,ahie_Effect(a0)
	bne	.no_mastervolumeOFF
	cmp.l	#$10000,ahiac_MasterVolume(a2)
	beq	.exit
	move.l	#$10000,ahiac_MasterVolume(a2)
	bsr	update_MasterVolume
	bra	.exit
.no_mastervolumeOFF

*
* OUTPUTBUFFER
*
	cmp.l	#AHIET_OUTPUTBUFFER,ahie_Effect(a0)
	bne	.no_outputbuffer
	move.l	a0,ahiac_EffOutputBufferStruct(a2)
	bra	.exit
.no_outputbuffer

	cmp.l	#AHIET_CANCEL|AHIET_OUTPUTBUFFER,ahie_Effect(a0)
	bne	.no_outputbufferOFF
	clr.l	ahiac_EffOutputBufferStruct(a2)
	bra	.exit
.no_outputbufferOFF

 IFGE	__CPU-68020

*
* DSPMASK
*
	cmp.l	#AHIET_DSPMASK,ahie_Effect(a0)
	bne	.no_dspmask
	bsr	update_DSPMask
	bra	.exit
.no_dspmask

	cmp.l	#AHIET_CANCEL|AHIET_DSPMASK,ahie_Effect(a0)
	bne	.no_dspmaskOFF
	bsr	clear_DSPMask
	bra	.exit
.no_dspmaskOFF

*
* DSPECHO
*
	cmp.l	#AHIET_DSPECHO,ahie_Effect(a0)
	bne	.no_dspecho
	btst.b	#AHIBB_NOECHO,ahib_Flags(a5)		; Disable echo?
	bne	.no_dspecho
	bsr	update_DSPEcho
	bra	.exit
.no_dspecho

	cmp.l	#AHIET_CANCEL|AHIET_DSPECHO,ahie_Effect(a0)
	bne	.no_dspechoOFF
	bsr	free_DSPEcho
	bra	.exit
.no_dspechoOFF

 ENDC * MC020

*
* CHANNELINFO
*
	cmp.l	#AHIET_CHANNELINFO,ahie_Effect(a0)
	bne	.no_channelinfo
	move.l	a0,ahiac_EffChannelInfoStruct(a2)
	bra	.exit
.no_channelinfo

	cmp.l	#AHIET_CANCEL|AHIET_CHANNELINFO,ahie_Effect(a0)
	bne	.no_channelinfoOFF
	clr.l	ahiac_EffChannelInfoStruct(a2)
	bra	.exit
.no_channelinfoOFF



.exit
	popm	d2-d7/a2-a6
	moveq	#0,d0
	rts

***
*** MASTERVOLUME
***

update_MasterVolume:
; Update tables
	bsr	calcUnsignedTable
	bsr	calcSignedTable

; Update volume for channels

	move.l	ahiac_SubLib(a2),a6
	call	AHIsub_Disable			;a2 ok

	move.l	ahiac_ChannelDatas(a2),a0
	move.w	ahiac_Channels(a2),d3
	subq.w	#1,d3
	bmi	.exit
.loop
	move.l	cd_VolumeLeft(a0),d0
	move.l	cd_VolumeRight(a0),d1
	move.l	cd_Type(a0),d2
	bsr.w	SelectAddRoutine
	move.l	d0,cd_ScaleLeft(a0)
	move.l	d1,cd_ScaleRight(a0)
	move.l	d2,cd_AddRoutine(a0)

	move.l	cd_NextVolumeLeft(a0),d0
	move.l	cd_NextVolumeRight(a0),d1
	move.l	cd_NextType(a0),d2
	bsr.w	SelectAddRoutine
	move.l	d0,cd_NextScaleLeft(a0)
	move.l	d1,cd_NextScaleRight(a0)
	move.l	d2,cd_NextAddRoutine(a0)

	add.w	#AHIChannelData_SIZEOF,a0
	dbf	d3,.loop
.exit
	call	AHIsub_Enable			;a2 ok
	rts

 IFGE	__CPU-68020

***
*** DSPMASK
***

update_DSPMask:
	move.l	ahiac_SubLib(a2),a6
	call	AHIsub_Disable			;a2 ok

	move.w	ahiedm_Channels(a0),d1
	cmp.w	ahiac_Channels(a2),d1		;sanity checks...
	bne	.exit
	subq.w	#1,d1
	bmi	.exit

; Yes, Sir, this IS a lame algoritm. So?

	clr.l	ahiac_WetList(a2)
	clr.l	ahiac_DryList(a2)
	lea	ahiedm_Mask(a0),a1
	move.l	ahiac_ChannelDatas(a2),a3
.loop
	tst.b	(a1)+
	bne	.dry
	lea	ahiac_WetList-cd_Succ(a2),a0
	skipl
.dry
	lea	ahiac_DryList-cd_Succ(a2),a0

;Add struct a3 to list a0
	move.l	a0,d0
.scanlist
	move.l	d0,a0
	move.l	cd_Succ(a0),d0
	bne	.scanlist
	move.l	a3,-AHIChannelData_SIZEOF+cd_Succ(a0)
	clr.l	cd_Succ(a3)

	add.w	#AHIChannelData_SIZEOF,a3
	dbf	d1,.loop
.exit
	call	AHIsub_Enable			;a2 ok
	rts

clear_DSPMask:
	move.l	ahiac_SubLib(a2),a6
	call	AHIsub_Disable			;a2 ok

* Make all channels wet
	clr.l	ahiac_DryList(a2)
	move.l	ahiac_ChannelDatas(a2),a0
	move.l	a0,ahiac_WetList(a2)

	move.w	ahiac_Channels(a2),d0
	subq.w	#1,d0
	bmi	.exit
.loop
	lea	AHIChannelData_SIZEOF(a0),a1
	move.l	a1,cd_Succ(a0)
	add.w	#AHIChannelData_SIZEOF,a0
	dbf	d0,.loop
	clr.l	-AHIChannelData_SIZEOF+cd_Succ(a0)
.exit
	call	AHIsub_Enable			;a2 ok
	rts

 ENDC * MC020



****** ahi.device/AHI_LoadSound ********************************************
*
*   NAME
*       AHI_LoadSound -- prepare a sound for playback
*
*   SYNOPSIS
*       error = AHI_LoadSound( sound, type, info, audioctrl );
*       D0                     D0:16  D1    A0    A2
*
*       ULONG AHI_LoadSound( UWORD, ULONG, APTR, struct AHIAudioCtrl * );
*
*   FUNCTION
*       Defines an ID number for the sound and prepares it for playback.
*
*   INPUTS
*       sound - The numeric ID to be used as a reference to this sound.
*           The ID is a number greater or equal to 0 and less than what you
*           specified with AHIA_Sounds when you called AHI_AllocAudioA().
*       type - The type of the sound. Currently four types are supported:
*           AHIST_SAMPLE - array of 8 or 16 bit samples. Note that the
*               portion of memory where the sample is stored must NOT be
*               altered until AHI_UnloadSound() has been called! This is
*               because some audio drivers may wish to upload the samples
*               to local RAM. It is ok to read, though.
*
*           AHIST_DYNAMICSAMPLE - array of 8 or 16 bit samples, which can be
*               updated dynamically. Typically used to play data that is
*               loaded from disk or calculated realtime.
*               Avoid using this sound type as much as possible; it will
*               use much more CPU power than AHIST_SAMPLE on a DMA/DSP
*               sound card.
*
*           AHIST_INPUT - The input from your sampler (not fully functional
*               yet).
*
*       info - Depends on type:
*           AHIST_SAMPLE - A pointer to a struct AHISampleInfo, filled with:
*               ahisi_Type - Format of samples (only two supported).
*                   AHIST_M8S: Mono, 8 bit signed (BYTEs).
*                   AHIST_S8S: Stereo, 8 bit signed (2×BYTEs) (V3). 
*                   AHIST_M16S: Mono, 16 bit signed (WORDs).
*                   AHIST_S16S: Stereo, 16 bit signed (2×WORDs) (V3).
*               ahisi_Address - Address to the sample array.
*               ahisi_Length - The size of the array, in samples.
*               Don't even think of setting ahisi_Address to 0 and
*               ahisi_Length to 0xffffffff as you can do with
*               AHIST_DYNAMICSAMPLE! Very few DMA/DSP cards has 4 GB onboard
*               RAM...
*
*           AHIST_DYNAMICSAMPLE A pointer to a struct AHISampleInfo, filled
*               as described above (AHIST_SAMPLE).
*               If ahisi_Address is 0 and ahisi_Length is 0xffffffff
*               AHI_SetSound() can take the real address of an 8 bit sample
*               to be played as offset argument. Unfortunately, this does not
*               work for 16 bit samples.
*
*           AHIST_INPUT - Allways set info to NULL.
*               Note that AHI_SetFreq() may only be called with AHI_MIXFREQ
*               for this sample type.
*
*       audioctrl - A pointer to an AHIAudioCtrl structure.
*
*   RESULT
*       An error code, defined in <devices/ahi.h>.
*
*   EXAMPLE
*
*   NOTES
*       There is no need to place a sample array in Chip memory, but it
*       MUST NOT be swapped out! Allocate your sample memory with the
*       MEMF_PUBLIC flag set. 
*
*       SoundFunc will be called in the same manner as Paula interrups
*       occur; when the device has updated its internal variables and can
*       accept new commands.
*
*   BUGS
*       AHIST_INPUT does not fully work yet.
*
*   SEE ALSO
*       AHI_UnloadSound(), AHI_SetEffect(), AHI_SetFreq(), AHI_SetSound(),
*       AHI_SetVol(), <devices/ahi.h>
*
****************************************************************************
*
*

_LoadSound:
	cmp.b	#AHI_DEBUG_LOW,ahib_DebugLevel(a6)
	blo	LoadSound_nodebug1
	and.l	#$ffff,d0
	PRINTF	1,"AHI_LoadSound(%ld, %ld, 0x%08lx, 0x%08lx)",d0,d1,a0,a2
LoadSound_nodebug1

	pushm	d1/a0-a1/a6
	push	d0
	move.l	ahiac_SubLib(a2),a6
	call	AHIsub_LoadSound
	btst.b	#AHIACB_NOMIXING-24,ahiac_Flags(a2)
	bne	.1
	cmp.l	#AHIS_UNKNOWN,d0
	beq	.2
.1
	moveq	#0,d0
	addq.l	#4,sp				;skip d0
	popm	d1/a0-a1/a6
	rts
.2
	pop	d0
	popm	d1/a0-a1/a6

	pushm	d2-d7/a2-a6
	move.l	a6,a5
	move.l	ahib_SysLib(a5),a6

	mulu.w	#AHISoundData_SIZEOF,d0
	move.l	ahiac_SoundDatas(a2),a3

*** AHIST_(DYNAMIC)SAMPLE
	cmp.l	#AHIST_DYNAMICSAMPLE,d1
	beq	.sample
	cmp.l	#AHIST_SAMPLE,d1
	bne	.nosample
.sample
	move.l	ahisi_Type(a0),d1
	cmp.l	#AHIST_M8S,d1
	beq	.typeok_signed
	cmp.l	#AHIST_M16S,d1
	beq	.typeok_signed
 IFGE	__CPU-68020
	cmp.l	#AHIST_S8S,d1
	beq	.typeok_signed
	cmp.l	#AHIST_S16S,d1
	beq	.typeok_signed
 ENDC
	cmp.l	#AHIST_M8U,d1
	beq	.typeok_unsigned
	moveq	#AHIE_BADSAMPLETYPE,d0
	bra	.exit
.typeok_signed
	move.l	d0,d1
	bsr	initSignedTable
	tst.l	d0
	beq	.error_nomem
	bra	.typeok
.typeok_unsigned
	move.l	d0,d1
	bsr	initUnsignedTable
	tst.l	d0
	beq	.error_nomem
.typeok
	move.l	ahisi_Type(a0),sd_Type(a3,d1.l)
	move.l	ahisi_Address(a0),sd_Addr(a3,d1.l)
	move.l	ahisi_Length(a0),sd_Length(a3,d1.l)
	moveq	#AHIE_OK,d0
	bra	.exit
.nosample
*** Unknown type
	moveq	#AHIE_BADSOUNDTYPE,d0
	bra	.exit
.error_nomem
	moveq	#AHIE_NOMEM,d0
.exit
	popm	d2-d7/a2-a6

	cmp.b	#AHI_DEBUG_LOW,ahib_DebugLevel(a6)
	blo	LoadSound_nodebug2
	PRINTF	2,"=>%ld",d0
LoadSound_nodebug2
	rts


****** ahi.device/AHI_UnloadSound ******************************************
*
*   NAME
*       AHI_UnloadSound -- discard a sound
*
*   SYNOPSIS
*       AHI_UnloadSound( sound, audioctrl );
*                        D0:16  A2
*
*       void AHI_UnloadSound( UWORD, struct AHIAudioCtrl * );
*
*   FUNCTION
*       Tells 'ahi.device' that this sound will not be used anymore.
*
*   INPUTS
*       sound - The ID of the sound to unload.
*       audioctrl - A pointer to an AHIAudioCtrl structure.
*
*   RESULT
*
*   EXAMPLE
*
*   NOTES
*       This call will not break a Forbid() state.
*
*   BUGS
*
*   SEE ALSO
*       AHI_LoadSound()
*
****************************************************************************
*
*

_UnloadSound:
	cmp.b	#AHI_DEBUG_LOW,ahib_DebugLevel(a6)
	blo	UnloadSound_nodebug
	and.l	#$ffff,d0
	PRINTF	2,"AHI_UnloadSound(%ld, 0x%08lx)",d0,a2
UnloadSound_nodebug

	pushm	d1/a0-a1/a6
	push	d0
	move.l	ahiac_SubLib(a2),a6
	call	AHIsub_UnloadSound
	btst.b	#AHIACB_NOMIXING-24,ahiac_Flags(a2)
	bne	.1
	cmp.l	#AHIS_UNKNOWN,d0
	beq	.2
.1
	moveq	#0,d0
	addq.l	#4,sp				;skip d0
	popm	d1/a0-a1/a6
	rts
.2
	pop	d0
	popm	d1/a0-a1/a6

	pushm	d2-d7/a2-a6
	move.l	a6,a5
	move.l	ahib_SysLib(a5),a6
	mulu.w	#AHISoundData_SIZEOF,d0
	move.l	ahiac_SoundDatas(a2),a3
	move.l	sd_Type(a3,d0.l),d1
	cmp.l	#AHIST_NOTYPE,d1
	beq	.exit
	move.l	#AHIST_NOTYPE,sd_Type(a3,d0.l)
.exit
	moveq	#0,d0
	popm	d2-d7/a2-a6
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
	moveq	#TRUE,d0
.exit	
	popm	d1-a6
	rts
.error
	moveq	#FALSE,d0
	bra	.exit

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
	divu.l	d2,d1
	cmp.b	ahiac_MaxCPU(a2),d1
	bls	.ok
	moveq	#TRUE,d0
	bra	.exit
.ok
	moveq	#FALSE,d0
.exit
	tst.l	d0
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
