* $Id$
* $Log$
* Revision 1.6  1997/01/15 18:35:07  lcs
* AHIB_Dizzy has a better implementation and definition now.
* (Was BOOL, now pointer to a second tag list)
*
* Revision 1.5  1997/01/15 14:59:50  lcs
* Removed most of the unsigned addroutines
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

	include	exec/exec.i
	include	dos/dos.i
	include	utility/utility.i
	include	utility/hooks.i
	include	devices/ahi.i
	include	libraries/ahi_sub.i
	include	lvo/exec_lib.i
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
	XREF	_Mix
	XREF	CalcSamples
	XREF	do_DSPEchoMono16
	XREF	do_DSPEchoStereo16
	XREF	do_DSPEchoMono32
	XREF	do_DSPEchoStereo32

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


****** ahi.device/AHI_AllocAudioA ******************************************
*
*   NAME
*       AHI_AllocAudioA -- allocates and initializes the audio hardware
*       AHI_AllocAudio -- varargs stub for AHI_AllocAudioA()
*
*   SYNOPSIS
*       audioctrl = AHI_AllocAudioA( tags );
*       D0                           A1
*
*       struct AHIAudioCtrl *AHI_AllocAudioA( struct TagItem * );
*
*       audioctrl = AHI_AllocAudio( tag1, ... );
*
*       struct AHIAudioCtrl *AHI_AllocAudio( Tag, ... );
*
*   FUNCTION
*       Allocates and initializes the audio hardware, selects the best
*       mixing routine (if neccessary) according to the supplied tags.
*       To start playing you first need to call AHI_ControlAudioA().
*
*   INPUTS
*       tags - A pointer to a tag list.
*
*   TAGS
*
*       AHIA_AudioID (ULONG) - The audio mode to use (AHI_DEFAULT_ID is the
*           user's default mode. It's a good value to use the first time she
*           starts your application.
*
*       AHIA_MixFreq (ULONG) - Desired mixing frequency. The actual
*           mixing rate may or may not be exactly what you asked for.
*           AHI_DEFAULT_FREQ is the user's prefered frequency.
*
*       AHIA_Channels (UWORD) - Number of channel to use. The actual
*           number of channels used will be equal or grater than the
*           requested. If too many channels were requested, this function
*           will fail. This tag must be supplied.
*
*       AHIA_Sounds (UWORD) - Number of sounds to use. This tag must be
*           supplied.
*
*       AHIA_SoundFunc (struct Hook *) - A function to call each time
*           when a sound has been started. The function receives the
*           following parameters:
*               A0 - (struct Hook *)
*               A2 - (struct AHIAudioCtrl *)
*               A1 - (struct AHISoundMessage *)
*           The hook may be called from an interrupt, so normal interrupt
*           restrictions apply.
*           The called function should follow normal register conventions,
*           which means that d2-d7 and a2-a6 must be preserved.
*
*       AHIA_PlayerFunc (struct Hook *) - A function to be called at regular
*           intervals. By using this hook there is no need for music players
*           to use other timing, such as VBLANK or CIA timers. But the real
*           reason it's present is that it makes it possible to do non-
*           realtime mixing to disk.
*           Using this interrupt source is currently the only supported way
*           to ensure that no mixing occurs between calls to AHI_SetVol(),
*           AHI_SetFreq() or AHI_SetSound().
*           If the sound playback is done without mixing, 'realtime.library'
*           is used to provide timing. The function receives the following
*           parameters:
*               A0 - (struct Hook *)
*               A2 - (struct AHIAudioCtrl *)
*               A1 - Undefined.
*           Do not assume A1 contains any particular value!
*           The hook may be called from an interrupt, so normal interrupt
*           restrictions apply.
*           The called function should follow normal register conventions,
*           which means that d2-d7 and a2-a6 must be preserved.
*
*       AHIA_PlayerFreq (Fixed) - If non-zero, enables timing and specifies
*           how many times per second PlayerFunc will be called. This must
*           be specified if AHIA_PlayerFunc is! Do not use any extreme
*           frequences. The result of MixFreq/PlayerFreq must fit an UWORD,
*           ie it must be less or equal to 65535. It is also suggested that
*           you keep the result over 80. For normal use this should not be a
*           problem. Note that the data type is Fixed, not integer. 50 Hz is
*           50<<16.
*
*       AHIA_MinsPlayerFreq (Fixed) - The minimum frequency (AHIA_PlayerFreq)
*           you will use. You should always supply this if you are using the
*           device's interrupt feature!
*
*       AHIA_MaxPlayerFreq (Fixed) - The maximum frequency (AHIA_PlayerFreq)
*           you will use. You should always supply this if you are using the
*           device's interrupt feature!
*
*       AHIA_RecordFunc (struct Hook *) - This function will be called
*           regulary when sampling is turned on (see AHI_ControlAudioA())
*           with the following parameters:
*               A0 - (struct Hook *)
*               A2 - (struct AHIAudioCtrl *)
*               A1 - (struct AHIRecordMessage *)
*           The message (AHIRecordMessage) is filled as follows:
*               ahirm_Buffer - Pointer to the samples. The buffer is valid
*                   until next time the Hook is called.
*               ahirm_Length - Number of sample FRAMES in buffer.
*                   To get the size in bytes, multiply by 4 if ahiim_Type is
*                   AHIST_S16S.
*               ahirm_Type - Always AHIST_S16S at the moment, but you *must*
*                   check this, since it may change in the future!
*           The hook may be called from an interrupt, so normal interrupt
*           restrictions apply. Signal a process if you wish to save the
*           buffer to disk. The called function should follow normal register
*           conventions, which means that d2-d7 and a2-a6 must be preserved.
*       *** NOTE: The function MUST return NULL (in d0). This was previously
*           not documented. Now you know.
*
*       AHIA_UserData (APTR) - Can be used to initialise the ahiac_UserData
*           field.
*
*   RESULT
*       A pointer to an AHIAudioCtrl structure or NULL if an error occured.
*
*   EXAMPLE
*
*   NOTES
*
*   BUGS
*
*   SEE ALSO
*       AHI_FreeAudio(), AHI_ControlAudioA()
*
****************************************************************************
*
*


****** ahi.device/AHI_FreeAudio *******************************************
*
*   NAME
*       AHI_FreeAudio -- deallocates the audio hardware
*
*   SYNOPSIS
*       AHI_FreeAudio( audioctrl );
*                      A2
*
*       void AHI_FreeAudio( struct AHIAudioCtrl * );
*
*   FUNCTION
*       Deallocates the AHIAudioCtrl structure and any other resources
*       allocated by AHI_AllocAudioA(). After this call it must not be used
*       by any other functions anymore. AHI_UnloadSound() is automatically
*       called for every sound.
*
*   INPUTS
*       audioctrl - A pointer to an AHIAudioCtrl structure obtained from
*           AHI_AllocAudioA(). If NULL, this function does nothing.
*
*   EXAMPLE
*
*   NOTES
*
*   BUGS
*
*   SEE ALSO
*       AHI_AllocAudioA(), AHI_KillAudio(), AHI_UnloadSound()
*
****************************************************************************
*
*


****i* ahi.device/AHI_KillAudio *******************************************
*
*   NAME
*      AHI_KillAudio -- clean up
*
*   SYNOPSIS
*      AHI_KillAudio();
*
*      void AHI_KillAudio( void );
*
*   FUNCTION
*      'ahi.device' keeps track of most of what the user does. This call is
*      used to clean up as much as possible. It must never, ever, be used
*      in an application. It is included for development use only, and can
*      be used to avoid rebooting the computer if your program has allocated
*      the audio hardware and crashed. This call can lead to a system crash,
*      so don't use it if you don't have to.
*
*   INPUTS
*
*   RESULT
*      This function returns nothing. In fact, it may never return.
*
*   EXAMPLE
*
*   NOTES
*
*   BUGS
*
*   SEE ALSO
*      AHI_FreeAudio()
*
****************************************************************************
*
*


****** ahi.device/AHI_ControlAudioA ***************************************
*
*   NAME
*       AHI_ControlAudioA -- change audio attributes
*       AHI_ControlAudio -- varargs stub for AHI_ControlAudioA()
*
*   SYNOPSIS
*       error = AHI_ControlAudioA( audioctrl, tags );
*       D0                         A2         A1
*
*       ULONG AHI_ControlAudioA( struct AHIAudioCtrl *, struct TagItem * );
*
*       error = AHI_ControlAudio( AudioCtrl, tag1, ...);
*
*       ULONG AHI_ControlAudio( struct AHIAudioCtrl *, Tag, ... );
*
*   FUNCTION
*       This function should be used to change attributes for a given
*       AHIAudioCtrl structure. It is also used to start and stop playback,
*       and to control special hardware found on some sound cards.
*
*   INPUTS
*       audioctrl - A pointer to an AHIAudioCtrl structure.
*       tags - A pointer to a tag list.
*
*   TAGS
*       AHIC_Play (BOOL) - Starts (TRUE) and stops (FALSE) playback and
*           PlayerFunc. NOTE: If the audio hardware cannot play at the same
*           time as recording samples, the recording will be stopped.
*
*       AHIC_Record (BOOL) - Starts (TRUE) and stops (FALSE) sampling and
*           RecordFunc. NOTE: If the audio hardware cannot record at the same
*           time as playing samples, the playback will be stopped.
*
*       AHIC_MonitorVolume (Fixed) - Sets the input monitor volume, i.e. how
*           much of the input signal is mixed with the output signal while
*           recording. Use AHI_GetAudioAttrsA() to find the available range.
*
*       AHIC_MonitorVolume_Query (Fixed *) - Get the current input monitor
*           volume. ti_Data is a pointer to a Fixed variable, where the result
*           will be stored.
*
*       AHIC_MixFreq_Query (ULONG *) - Get the current mixing frequency.
*           ti_Data is a pointer to an ULONG variable, where the result will
*           be stored.
*
*       AHIC_InputGain (Fixed) - Set the input gain. Use AHI_GetAudioAttrsA()
*           to find the available range. (V2)
*
*       AHIC_InputGain_Query (Fixed *) - Get current input gain. (V2)
*
*       AHIC_OutputVolume (Fixed) - Set the output volume. Use
*           AHI_GetAudioAttrsA() to find the available range. (V2)
*
*       AHIC_OutputVolume_Query (Fixed *) - Get current output volume. (V2)
*
*       AHIC_Input (ULONG) - Select input source. See AHI_GetAudioAttrsA().
*           (V2)
*
*       AHIC_Input_Query (ULONG *) - Get current input source. (V2)
*
*       AHIC_Output (ULONG) - Select destination for output. See
*           AHI_GetAudioAttrsA(). (V2)
*
*       AHIC_Output_Query (ULONG *) - Get destination for output. (V2)
*
*       The following tags are also recognized by AHI_ControlAudioA(). See
*       AHI_AllocAudioA() for what they do. They may be used from interrupts.
*
*       AHIA_SoundFunc (struct Hook *)
*       AHIA_PlayerFunc (struct Hook *)
*       AHIA_PlayerFreq (Fixed)
*       AHIA_RecordFunc (struct Hook *)
*       AHIA_UserData (APTR)
*
*       Note that AHIA_PlayerFreq must never be outside the limits specified
*       with AHIA_MinPlayerFreq and AHIA_MaxPlayerFreq!
*
*   RESULT
*       An error code, defined in <devices/ahi.h>.
*
*   EXAMPLE
*
*   NOTES
*       The AHIC_Play and AHIC_Record tags *must not* be used from
*       interrupts.
*
*   BUGS
*
*   SEE ALSO
*       AHI_AllocAudioA(), AHI_GetAudioAttrsA(), <devices/ahi.h>
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
;	btst.b	#AHIACB_STEREO,ahiac_Flags+3(a2)
;	bne	.stereo
;	moveq	#0,d2				;pan=0
;.stereo

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
*       freq - The playback frequency in Herz. Can also be AHI_MIXFREQ,
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
*       Freq is limited to 262143 Hz.
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
	lsl.l	#8,d0
	lsl.l	#8-2,d0				;Makes the sample freq limit 262143
	move.l	ahiac_MixFreq(a2),d1
 IFGE	__CPU-68020
	divu.l	d1,d0
 ELSE
	move.l	ahib_UtilityLib(a5),a1
	jsr	_LVOUDivMod32(a1)
 ENDC
.setperiod
	lsl.l	#2,d0
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

***
*** DSPECHO
***
	XDEF	update_DSPEcho
update_DSPEcho:
	pushm	d2-d7/a3-a6
	move.l	a0,a3
	bsr	free_DSPEcho
	move.l	ahiac_BuffType(a2),d0
	moveq	#0,d1
	move.b	.type2shift(pc,d0),d1
	move.l	ahiede_Delay(a3),d0
	move.l	d0,d3
	lsl.l	d1,d3
	add.l	ahiac_MaxBuffSamples(a2),d0
	lsl.l	d1,d0
	move.l	d0,d2
	add.l	#AHIEcho_SIZEOF,d0
	move.l	#MEMF_PUBLIC|MEMF_CLEAR,d1
	move.l	ahib_SysLib(a5),a6
	call	AllocVec
	move.l	d0,a1
	tst.l	d0
	beq	.exit

	lea	ahiecho_Buffer(a1),a0
	move.l	d2,ahiecho_BufferSize(a1)
	add.l	a0,d2
	move.l	d2,ahiecho_EndPtr(a1)
	move.l	a0,ahiecho_SrcPtr(a1)
	add.l	a0,d3
	move.l	d3,ahiecho_DstPtr(a1)

	move.l	ahiac_BuffType(a2),d0
	lea	do_DSPEchoMono16(pc),a0
	cmp.l	#AHIST_M16S,d0
	beq	.save_mono
	lea	do_DSPEchoMono32(pc),a0
	cmp.l	#AHIST_M32S,d0
	beq	.save_mono
	lea	do_DSPEchoStereo16(pc),a0
	cmp.l	#AHIST_S16S,d0
	beq	.save
	lea	do_DSPEchoStereo32(pc),a0
	cmp.l	#AHIST_S32S,d0
	beq	.save
	bra	.exit					;ERROR, unknown buffer!
.save_mono
	clr.l	ahiede_Cross(a3)
.save
	move.l	a0,ahiecho_Code(a1)

* Delay      = ahiede_Delay
	move.l	ahiede_Delay(a3),ahiecho_Delay(a1)
* MixD       = ahiede_Mix
	move.l	ahiede_Mix(a3),d0
	lsr.w	#1,d0
	bpl.b	.0a
	subq.w	#1,d0
.0a
	move.w	d0,ahiecho_MixD(a1)
* MixN       = $10000-ahiede_Mix
	move.l	#$10000,d0
	sub.l	ahiede_Mix(a3),d0
	lsr.w	#1,d0
	bpl.b	.0b
	subq.w	#1,d0
.0b
	move.w	d0,ahiecho_MixN(a1)


* FeedbackDS = ahide_Feedback*($10000-ahide_Cross)
	move.l	ahiede_Feedback(a3),d0
	move.l	#$10000,d1
	sub.l	ahiede_Cross(a3),d1
	mulu.l	d1,d0
	bvc	.1
	moveq	#$ffffffff,d0
.1
	clr.w	d0
	swap.w	d0
	lsr.w	#1,d0
	move.w	d0,ahiecho_FeedbackDS(a1)

* FeedbackDO = ahide_Feedback*ahide_Cross
	move.l	ahiede_Feedback(a3),d0
	mulu.l	ahiede_Cross(a3),d0
	bvc	.2
	moveq	#$ffffffff,d0
.2
	clr.w	d0
	swap.w	d0
	lsr.l	#1,d0
	move.w	d0,ahiecho_FeedbackDO(a1)

* FeedbackNS = ($10000-ahide_Feedback)*($10000-ahide_Cross)
	move.l	#$10000,d0
	sub.l	ahiede_Cross(a3),d0
	move.l	#$10000,d1
	sub.l	ahiede_Feedback(a3),d1
	mulu.l	d1,d0
	bvc	.3
	moveq	#$ffffffff,d0
.3
	clr.w	d0
	swap.w	d0
	lsr.w	#1,d0
	move.w	d0,ahiecho_FeedbackNS(a1)

* FeedbackNO = ($10000-ahide_Feedback)*ahide_Cross
	move.l	ahiede_Cross(a3),d0
	mulu.l	d1,d0
	bvc	.4
	moveq	#$ffffffff,d0
.4
	clr.w	d0
	swap.w	d0
	lsr.l	#1,d0
	move.w	d0,ahiecho_FeedbackNO(a1)

	move.l	a1,ahiac_EffDSPEchoStruct(a2)
.exit
	popm	d2-d7/a3-a6
	rts
.type2shift
	dc.b	0	;AHIST_M8S  (0)
	dc.b	1	;AHIST_M16S (1)
	dc.b	1	;AHIST_S8S  (2)
	dc.b	2	;AHIST_S16S (3)
	dc.b	0	;AHIST_M8U  (4)
	dc.b	0
	dc.b	0
	dc.b	0
	dc.b	2	;AHIST_M32S (8)
	dc.b	0
	dc.b	3	;AHIST_S32S (10)

	even

	XDEF	free_DSPEcho
free_DSPEcho:
	push	a6
	move.l	ahib_SysLib(a5),a6
	move.l	ahiac_EffDSPEchoStruct(a2),a1
	clr.l	ahiac_EffDSPEchoStruct(a2)
	call	FreeVec
	pop	a6
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
*                   AHIST_M16S: Mono, 16 bit signed (WORDs).
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
;	cmp.l	#AHIST_M8U,d1
;	beq	.typeok_unsigned
	moveq	#AHIE_BADSAMPLETYPE,d0
	bra	.exit
.typeok_signed
	move.l	d0,d1
	bsr	initSignedTable
;	tst.l	d0
;	beq	.error_nomem
	bra	.typeok
.typeok_unsigned
	move.l	d0,d1
	bsr	initUnsignedTable
;	tst.l	d0
;	beq	.error_nomem
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


****** ahi.device/AHI_NextAudioID ******************************************
*
*   NAME
*       AHI_NextAudioID -- iterate current audio mode identifiers
*
*   SYNOPSIS
*       next_ID = AHI_NextAudioID( last_ID );
*       D0                         D0
*
*       ULONG AHI_NextAudioID( ULONG );
*
*   FUNCTION
*       This function is used to itereate through all current AudioIDs in
*       the audio database.
*
*   INPUTS
*       last_ID - previous AudioID or AHI_INVALID_ID if beginning iteration.
*
*   RESULT
*       next_ID - subsequent AudioID or AHI_INVALID_ID if no more IDs.
*
*   EXAMPLE
*
*   NOTES
*
*   BUGS
*
*   SEE ALSO
*      AHI_GetAudioAttrsA(), AHI_BestAudioIDA()
*
****************************************************************************
*
*

****** ahi.device/AHI_GetAudioAttrsA ***************************************
*
*   NAME
*       AHI_GetAudioAttrsA -- examine an audio mode via a tag list
*       AHI_GetAudioAttrs -- varargs stub for AHI_GetAudioAttrsA()
*
*   SYNOPSIS
*       success = AHI_GetAudioAttrsA( ID, [audioctrl], tags );
*       D0                            D0  A2           A1
*
*       BOOL AHI_GetAudioAttrsA( ULONG, struct AHIAudioCtrl *,
*                                struct TagItem * );
*
*       success = AHI_GetAudioAttrs( ID, [audioctrl], attr1, &result1, ...);
*
*       BOOL AHI_GetAudioAttrs( ULONG, struct AHIAudioCtrl *, Tag, ... );
*
*   FUNCTION
*       Retrieve information about an audio mode specified by ID or audioctrl
*       according to the tags in the tag list. For each entry in the tag
*       list, ti_Tag identifies the attribute, and ti_Data is mostly a
*       pointer to a LONG (4 bytes) variable where you wish the result to be
*       stored.
*
*   INPUTS
*       ID - An audio mode identifier or AHI_INVALID_ID.
*       audioctrl - A pointer to an AHIAudioCtrl structure, only used if
*           ID equals AHI_INVALID_ID. Set to NULL if not used. If set to
*           NULL when used, this function returns immediately. Always set
*           ID to AHI_INVALID_ID and use audioctrl if you have allocated
*           a valid AHIAudioCtrl structure. Some of the tags return incorrect
*           values otherwise.
*       tags - A pointer to a tag list.
*
*   TAGS
*       AHIDB_Volume (ULONG *) - TRUE if this mode supports volume changes.
*
*       AHIDB_Stereo (ULONG *) - TRUE if output is in stereo. Unless
*           AHIDB_Panning (see below) is TRUE, all even channels are played
*           to the left and all odd to the right.
*
*       AHIDB_Panning (ULONG *) - TRUE if this mode supports stereo panning.
*
*       AHIDB_HiFi (ULONG *) - TRUE if no shortcuts, like predivision, is
*           used by the mixing routines.
*
*       AHIDB_PingPong (ULONG *) - TRUE if this mode can play samples backwards.
*
*       AHIDB_Record (ULONG *) - TRUE if this mode can record samples.
*
*       AHIDB_FullDuplex (ULONG *) - TRUE if this mode can record and play at
*           the same time.
*
*       AHIDB_Realtime (ULONG *) - Modes which return TRUE for this fulfils
*           two criteria:
*           1) Calls to AHI_SetVol(), AHI_SetFreq() or AHI_SetSound() will be
*              preformed within (about) 10 ms if called from a PlayFunc Hook.
*           2) The PlayFunc Hook will be called at the specifed frequency.
*           If you don't use AHI's PlayFunc Hook, you must not use modes that
*           are not realtime. (Criterium 2 is not that obvious if you consider
*           a mode that renders the output to disk as a sample.)
*
*       AHIDB_Bits (ULONG *) - The number of output bits (8, 12, 14, 16 etc).
*
*       AHIDB_MaxChannels (ULONG *) - The maximum number of channels this mode
*           can handle.
*
*       AHIDB_MinMixFreq (ULONG *) - The minimum mixing frequency supported.
*
*       AHIDB_MaxMixFreq (ULONG *) - The maximum mixing frequency supported.
*
*       AHIDB_Frequencies (ULONG *) - The number of different sample rates
*           available.
*
*       AHIDB_FrequencyArg (ULONG) - Specifies which frequency
*           AHIDB_Frequency should return (see below). Range is 0 to
*           AHIDB_Frequencies-1 (including).
*           NOTE: ti_Data is NOT a pointer, but an ULONG.
*
*       AHIDB_Frequency (ULONG *) - Return the frequency associated with the
*           index number specified with AHIDB_FrequencyArg (see above).
*
*       AHIDB_IndexArg (ULONG) - AHIDB_Index will return the index which
*           gives the closest frequency to AHIDB_IndexArg
*           NOTE: ti_Data is NOT a pointer, but an ULONG.
*
*       AHIDB_Index (ULONG *) - Return the index associated with the frequency
*           specified with AHIDB_IndexArg (see above).
*
*       AHIDB_MaxPlaySamples (ULONG *) - Return the lowest number of sample
*           frames that must be present in memory when AHIST_DYNAMICSAMPLE
*           sounds are used. This number must then be scaled by Fs/Fm, where
*           Fs is the frequency of the sound and Fm is the mixing frequency.
*
*       AHIDB_MaxRecordSamples (ULONG *) - Return the number of sample frames
*           you will recieve each time the RecordFunc is called.
*
*       AHIDB_BufferLen (ULONG) - Specifies how many characters will be
*           copyed when requesting text attributes. Default is 0, which
*           means that AHIDB_Driver, AHIDB_Name, AHIDB_Author,
*           AHIDB_Copyright, AHIDB_Version and AHIDB_Annotation,
*           AHIDB_Input and AHIDB_Output will do nothing.
*
*       AHIDB_Driver (STRPTR) - Name of driver (excluding path and
*           extention). 
*           NOTE: ti_Data is a pointer to an UBYTE array where the name
*           will be stored. See AHIDB_BufferLen.
*
*       AHIDB_Name (STRPTR) - Human readable name of this mode.
*           NOTE: ti_Data is a pointer to an UBYTE array where the name
*           will be stored. See AHIDB_BufferLen.
*
*       AHIDB_Author (STRPTR) - Name of driver author.
*           NOTE: ti_Data is a pointer to an UBYTE array where the name
*           will be stored. See AHIDB_BufferLen.
*
*       AHIDB_Copyright (STRPTR) - Driver copyright notice.
*           NOTE: ti_Data is a pointer to an UBYTE array where the name
*           will be stored. See AHIDB_BufferLen
*
*       AHIDB_Version (STRPTR) - Driver version string.
*           NOTE: ti_Data is a pointer to an UBYTE array where the name
*           will be stored. See AHIDB_BufferLen.
*
*       AHIDB_Annotation (STRPTR) - Annotation by driver author.
*           NOTE: ti_Data is a pointer to an UBYTE array where the name
*           will be stored. See AHIDB_BufferLen.
*
*       AHIDB_MinMonitorVolume (Fixed *)
*       AHIDB_MaxMonitorVolume (Fixed *) - Lower/upper limit for input
*           monitor volume, see AHI_ControlAudioA(). If both are 0.0,
*           the sound hardware does not have an input monitor feature.
*           If both are same, but not 0.0, the hardware always sends the
*           recorded sound to the outputs (at the given volume). (V2)
*
*       AHIDB_MinInputGain (Fixed *)
*       AHIDB_MaxInputGain (Fixed *) - Lower/upper limit for input gain,
*           see AHI_ControlAudioA(). If both are same, there is no input
*           gain hardware. (V2)
*
*       AHIDB_MinOutputVolume (Fixed *)
*       AHIDB_MaxOutputVolume (Fixed *) - Lower/upper limit for output
*           volume, see AHI_ControlAudioA(). If both are same, the sound
*           card does not have volume control. (V2)
*
*       AHIDB_Inputs (ULONG *) - The number of inputs the sound card has.
*           (V2)
*
*       AHIDB_InputArg (ULONG) - Specifies what AHIDB_Input should return
*           (see below). Range is 0 to AHIDB_Inputs-1 (including).
*           NOTE: ti_Data is NOT a pointer, but an ULONG. (V2)
*
*       AHIDB_Input (STRPTR) - Gives a human readable string describing the
*           input associated with the index specified with AHIDB_InputArg
*           (see above). See AHI_ControlAudioA() for how to select one.
*           NOTE: ti_Data is a pointer to an UBYTE array where the name
*           will be stored. See AHIDB_BufferLen. (V2)
*
*       AHIDB_Outputs (ULONG *) - The number of outputs the sound card
*           has. (V2)
*
*       AHIDB_OutputArg (ULONG) - Specifies what AHIDB_Output should return
*           (see below). Range is 0 to AHIDB_Outputs-1 (including)
*           NOTE: ti_Data is NOT a pointer, but an ULONG. (V2)
*
*       AHIDB_Output (STRPTR) - Gives a human readable string describing the
*           output associated with the index specified with AHIDB_OutputArg
*           (see above). See AHI_ControlAudioA() for how to select one.
*           NOTE: ti_Data is a pointer to an UBYTE array where the name
*           will be stored. See AHIDB_BufferLen. (V2)
*
*       If the requested information cannot be found, the variable will be not
*       be touched.
*
*   RESULT
*       TRUE if everything went well.
*
*   EXAMPLE
*
*   NOTES
*
*   BUGS
*       In versions earlier than 3, the tags that filled a string buffer would
*       not NULL-terminate the string on buffer overflows.
*
*   SEE ALSO
*      AHI_NextAudioID(), AHI_BestAudioIDA()
*
****************************************************************************
*
*

****** ahi.device/AHI_BestAudioIDA *****************************************
*
*   NAME
*       AHI_BestAudioIDA -- calculate the best ModeID with given parameters
*       AHI_BestAudioID -- varargs stub for AHI_BestAudioIDA()
*
*   SYNOPSIS
*       ID = AHI_BestAudioIDA( tags );
*       D0                     A1
*
*       ULONG AHI_BestAudioIDA( struct TagItem * );
*
*       ID = AHI_BestAudioID( tag1, ... );
*
*       ULONG AHI_BestAudioID( Tag, ... );
*
*   FUNCTION
*       Determines the best AudioID to fit the parameters set in the tag
*       list.
*
*   INPUTS
*       tags - A pointer to a tag list. Only the tags present matter.
*
*   TAGS
*       Many combinations are probably stupid to ask for, like not supporting
*       panning or recording.
*
*       AHIDB_AudioID (ULONG) - The mode must use the same audio hardware
*           as this mode does.
*
*       AHIDB_Volume (BOOL) - If TRUE: mode must support volume changes.
*           If FALSE: mode must not support volume changes.
*
*       AHIDB_Stereo (BOOL) - If TRUE: mode must have stereo output.
*           If FALSE: mode must not have stereo output (=mono).
*
*       AHIDB_Panning (BOOL) - If TRUE: mode must support volume panning.
*           If FALSE: mode must not support volume panning. 
*
*       AHIDB_HiFi (BOOL) - If TRUE: mode must have HiFi output.
*           If FALSE: mode must not have HiFi output.
*
*       AHIDB_PingPong (BOOL) - If TRUE: mode must support playing samples
*           backwards. If FALSE: mode must not support playing samples
*           backwards.
*
*       AHIDB_Record (BOOL) - If TRUE: mode must support recording. If FALSE:
*           mode must not support recording.
*
*       AHIDB_Realtime (BOOL) - If TRUE: mode must be realtime. If FALSE:
*           take a wild guess.
*
*       AHIDB_FullDuplex (BOOL) - If TRUE: mode must be able to record and
*           play at the same time.
*
*       AHIDB_Bits (UBYTE) - Mode must have greater or equal number of bits.
*
*       AHIDB_MaxChannels (UWORD) - Mode must have greater or equal number
*           of channels.
*
*       AHIDB_MinMixFreq (ULONG) - Lowest mixing frequency supported must be
*           less or equal.
*
*       AHIDB_MaxMixFreq (ULONG) - Highest mixing frequency must be greater
*           or equal.
*
*       AHIB_Dizzy (struct TagItem *) - This tag points to a second tag list.
*           After all other tags has been tested, the mode that matches these
*           tags best is returned, i.e. the one that has most of the features
*           you ask for, and least of the ones you don't want. Without this
*           second tag list, this function hardly does what its name
*           suggests. (V3)
*
*   RESULT
*       ID - The best AudioID to use or AHI_INVALID_ID if none of the modes
*           in the audio database could meet the requirements.
*
*   EXAMPLE
*
*   NOTES
*
*   BUGS
*
*   SEE ALSO
*      AHI_NextAudioID(), AHI_GetAudioAttrsA()
*
****************************************************************************
*
*

****** ahi.device/AHI_AllocAudioRequestA ***********************************
*
*   NAME
*       AHI_AllocAudioRequestA -- allocate an audio mode requester.
*       AHI_AllocAudioRequest -- varargs stub for AHI_AllocAudioRequestA()
*
*   SYNOPSIS
*       requester = AHI_AllocAudioRequestA( tags );
*       D0                                  A0
*
*       struct AHIAudioModeRequester *AHI_AllocAudioRequestA(
*           struct TagItem * );
*
*       requester = AHI_AllocAudioRequest( tag1, ... );
*
*       struct AHIAudioModeRequester *AHI_AllocAudioRequest( Tag, ... );
*
*   FUNCTION
*       Allocates an audio mode requester data structure.
*
*   INPUTS
*       tags - A pointer to an optional tag list specifying how to initialize
*           the data structure returned by this function. See the
*           documentation for AHI_AudioRequestA() for an explanation of how
*           to use the currently defined tags.
*
*   RESULT
*       requester - An initialized requester data structure, or NULL on
*           failure. 
*
*   EXAMPLE
*
*   NOTES
*       The requester data structure is READ-ONLY and can only be modified
*       by using tags!
*
*   BUGS
*
*   SEE ALSO
*      AHI_AudioRequestA(), AHI_FreeAudioRequest()
*
****************************************************************************
*
*

****** ahi.device/AHI_AudioRequestA ****************************************
*
*   NAME
*       AHI_AudioRequestA -- get an audio mode from user using an requester.
*       AHI_AudioRequest -- varargs stub for AHI_AudioRequestA()
*
*   SYNOPSIS
*       success = AHI_AudioRequestA( requester, tags );
*       D0                           A0         A1
*
*       BOOL AHI_AudioRequestA( struct AHIAudioModeRequester *,
*           struct TagItem * );
*
*       result = AHI_AudioRequest( requester, tag1, ... );
*
*       BOOL AHI_AudioRequest( struct AHIAudioModeRequester *, Tag, ... );
*
*   FUNCTION
*       Prompts the user for an audio mode, based on the modifying tags.
*       If the user cancels or the system aborts the request, FALSE is
*       returned, otherwise the requester's data structure relects the
*       user input.
*
*       Note that tag values stay in effect for each use of the requester
*       until they are cleared or modified by passing the same tag with a
*       new value.
*
*   INPUTS
*       requester - Requester structure allocated with
*           AHI_AllocAudioRequestA(). If this parameter is NULL, this
*           function will always return FALSE with a dos.library/IoErr()
*           result of ERROR_NO_FREE_STORE.
*       tags - Pointer to an optional tag list which may be used to control
*           features of the requester.
*
*   TAGS
*       Tags used for the requester (they look remarkable similar to the
*       screen mode requester in asl, don't they? ;-) )
*
*       AHIR_Window (struct Window *) - Parent window of requester. If no
*           AHIR_Screen tag is specified, the window structure is used to
*           determine on which screen to open the requesting window.
*
*       AHIR_PubScreenName (STRPTR) - Name of a public screen to open on.
*           This overrides the screen used by AHIR_Window.
*
*       AHIR_Screen (struct Screen *) - Screen on which to open the
*           requester. This overrides the screen used by AHIR_Window or by
*           AHIR_PubScreenName.
*
*       AHIR_PrivateIDCMP (BOOL) - When set to TRUE, this tells AHI to
*           allocate a new IDCMP port for the requesting window. If not
*           specified or set to FALSE, and if AHIR_Window is provided, the
*           requesting window will share AHIR_Window's IDCMP port.
*
*       AHIR_IntuiMsgFunc (struct Hook *) - A function to call whenever an
*           unknown Intuition message arrives at the message port being used
*           by the requesting window. The function receives the following
*           parameters:
*               A0 - (struct Hook *)
*               A1 - (struct IntuiMessage *)
*               A2 - (struct AHIAudioModeRequester *)
*
*       AHIR_SleepWindow (BOOL) - When set to TRUE, this tag will cause the
*           window specified by AHIR_Window to be "put to sleep". That is, a
*           busy pointer will be displayed in the parent window, and no
*           gadget or menu activity will be allowed. This is done by opening
*           an invisible Intuition Requester in the parent window.
*
*       AHIR_UserData (APTR) - A 32-bit value that is simply copied in the
*           ahiam_UserData field of the requester structure.
*
*       AHIR_TextAttr (struct TextAttr *) - Font to be used for the
*           requesting window's gadgets and menus. If this tag is not
*           provided or its value is NULL, the default font of the screen
*           on which the requesting window opens will be used. This font
*           must already be in memory as AHI calls OpenFont() and not
*           OpenDiskFont().
*
*       AHIR_Locale (struct Locale *) - Locale to use for the requesting
*           window. This determines the language used for the requester's
*           gadgets and menus. If this tag is not provided or its value is
*           NULL, the system's current default locale will be used.
*           This tag does not work yet.
*
*       AHIR_TitleText (STRPTR) - Title to use for the requesting window.
*           Default is no title.
*
*       AHIR_PositiveText (STRPTR) - Label of the positive gadget in the
*           requester. English default is "OK".
*
*       AHIR_NegativeText (STRPTR) - Label of the negative gadget in the
*           requester. English default is "Cancel".
*
*       AHIR_InitialLeftEdge (WORD) - Suggested left edge of requesting
*           window.
*
*       AHIR_InitialTopEdge (WORD) - Suggested top edge of requesting
*           window.
*
*       AHIR_InitialWidth (WORD) - Suggested width of requesting window.
*
*       AHIR_InitialHeight (WORD) - Suggested height of requesting window.
*
*       AHIR_InitialAudioID (ULONG) - Initial setting of the Mode list view
*           gadget (ahiam_AudioID). Default is ~0 (AHI_INVALID_ID), which
*           means that no mode will be selected.
*
*       AHIR_InitialMixFreq (ULONG) - Initial setting of the frequency
*           slider. Default is the lowest frequency supported.
*
*       AHIR_InitialInfoOpened (BOOL) - Whether to open the property
*           information window automatically. Default is FALSE.
*           This tag does not work yet.
*
*       AHIR_InitialInfoLeftEdge (WORD) - Initial left edge of information
*           window.
*           This tag does not work yet.
*
*       AHIR_InitialInfoTopEdge (WORD) - Initial top edge of information
*           window.
*           This tag does not work yet.
*
*       AHIR_DoMixFreq (BOOL) - Set this tag to TRUE to cause the requester
*           to display the frequency slider gadget. Default is FALSE.
*
*       AHIR_FilterFunc (struct Hook *) - A function to call for each mode
*           encountered. If the function returns TRUE, the mode is included
*           in the file list, otherwise it is rejected and not displayed. The
*           function receives the following parameters:
*               A0 - (struct Hook *)
*               A1 - (ULONG) mode id
*               A2 - (struct AHIAudioModeRequester *)
*
*       AHIR_FilterTags (struct TagItem *) - A pointer to a tag list used to
*           filter modes away, like AHIR_FilterFunc does. The tags are the
*           same as AHI_BestAudioIDA() takes as arguments. See that function
*           for an explanation of each tag.
*
*   RESULT
*       result - FALSE if the user cancelled the requester or if something
*           prevented the requester from opening. If TRUE, values in the
*           requester structure will be set.
*
*           If the return value is FALSE, you can look at the result from the
*           dos.library/IoErr() function to determine whether the requester
*           was cancelled or simply failed to open. If dos.library/IoErr()
*           returns 0, then the requester was cancelled, any other value
*           indicates a failure to open. Current possible failure codes are
*           ERROR_NO_FREE_STORE which indicates there was not enough memory,
*           and ERROR_NO_MORE_ENTRIES which indicates no modes were available
*           (usually because the application filter hook filtered them all
*           away).
*
*   EXAMPLE
*
*   NOTES
*       The requester data structure is READ-ONLY and can only be modified
*       by using tags!
*
*       The mixing/recording frequencies that are presented to the user
*       may not be the only ones a driver supports, but just a selection.
*
*   BUGS
*       AHIR_Locale and the information window is not inplemented.
*
*   SEE ALSO
*      AHI_AllocAudioRequestA(), AHI_FreeAudioRequest()
*
****************************************************************************
*
*

****** ahi.device/AHI_FreeAudioRequest *************************************
*
*   NAME
*       AHI_FreeAudioRequest -- frees requester resources 
*
*   SYNOPSIS
*       AHI_FreeAudioRequest( requester );
*                             A0
*
*       void AHI_FreeAudioRequest( struct AHIAudioModeRequester * );
*
*   FUNCTION
*       Frees any resources allocated by AHI_AllocAudioRequestA(). Once a
*       requester has been freed, it can no longer be used with other calls to
*       AHI_AudioRequestA().
*
*   INPUTS
*       requester - Requester obtained from AHI_AllocAudioRequestA(), or NULL
*       in which case this function does nothing.
*
*   RESULT
*
*   EXAMPLE
*
*   NOTES
*
*   BUGS
*
*   SEE ALSO
*      AHI_AllocAudioRequestA()
*
****************************************************************************
*
*


****** ahi.device/AHI_PlayA ************************************************
*
*   NAME
*       AHI_PlayA -- Start multiple sounds in one call (V3)
*       AHI_Play -- varargs stub for AHI_PlayA()
*
*   SYNOPSIS
*       AHI_PlayA( audioctrl, tags );
*                  A2         A1
*
*       void AHI_PlayA( struct AHIAudioCtrl *, struct TagItem * );
*
*       AHI_Play( AudioCtrl, tag1, ...);
*
*       void AHI_Play( struct AHIAudioCtrl *, Tag, ... );
*
*   FUNCTION
*       This function performs the same actions as multiple calls to
*       AHI_SetFreq(), AHI_SetSound() and AHI_SetVol(). The advantages
*       of using only one call is that simple loops can be set without
*       using a SoundFunc (see AHI_AllocAudioA(), tag AHIA_SoundFunc) and
*       that sounds on different channels can be syncronized even when the
*       sounds are not started from a PlayerFunc (see AHI_AllocAudioA(), tag
*       AHIA_PlayerFunc). The disadvantage is that this call has more
*       overhead than AHI_SetFreq(), AHI_SetSound() and AHI_SetVol(). It is
*       therefore recommended that you only use this call if you are not
*       calling from a SoundFunc or PlayerFunc.
*
*       The supplied tag list works like a 'program'. This means that
*       the order of tags matter.
*
*   INPUTS
*       audioctrl - A pointer to an AHIAudioCtrl structure.
*       tags - A pointer to a tag list.
*
*   TAGS
*       AHIP_BeginChannel (UWORD) - Before you start setting attributes
*           for a sound to play, you have to use this tag to chose a
*           channel to operate on. If AHIP_BeginChannel is omitted, the
*           resullt is undefined.
*
*       AHIP_EndChannel (ULONG) - Signals the end of attributes for
*           the current channel. If AHIP_EndChannel is omitted, the result
*           is undefined. ti_Data MUST BE NULL!
*
*       AHIP_Freq (ULONG) - The playback frequency in Herz or AHI_MIXFREQ.
*
*       AHIP_Vol (Fixed) - The desired volume. If omitted, but AHIP_Pan is
*           present, AHIP_Vol defaults to 0.
*
*       AHIP_Pan (sposition) - The desired panning. If omitted, but AHIP_Vol
*           is present, AHIP_Pan defaults to 0 (extreme left).
*
*       AHIP_Sound (UWORD) - Sound to be played, or AHI_NOSOUND.
*
*       AHIP_Offset (ULONG) - Specifies an offset (in samples) into the
*           sound. If this tag is present, AHIP_Length MUST be present too!
*
*       AHIP_Length (LONG) - Specifies how many samples that should be
*           player.
*
*       AHIP_LoopFreq (ULONG)
*       AHIP_LoopVol (Fixed)
*       AHIP_LoopPan (sposition)
*       AHIP_LoopSound (UWORD)
*       AHIP_LoopOffset (ULONG)
*       AHIP_LoopLength (LONG) - These tags can be used to set simple loop
*          attributes. They default to their sisters. These tags must be
*          after the other tags.
*
*   RESULT
*
*   EXAMPLE
*
*   NOTES
*
*   BUGS
*
*   SEE ALSO
*       AHI_SetFreq(), AHI_SetSound(), AHI_SetVol()
*
****************************************************************************
*
*

****** ahi.device/AHI_SampleFrameSize **************************************
*
*   NAME
*       AHI_SampleFrameSize -- get the size of a sample frame (V3)
*
*   SYNOPSIS
*       size = AHI_SampleFrameSize( sampletype );
*       D0                          D0
*
*       ULONG AHI_SampleFrameSize( ULONG );
*
*   FUNCTION
*       Returns the size in bytes of a sample frame for a given sample type.
*
*   INPUTS
*       sampletype - The sample type to examine. See <devices/ahi.h> for
*           possible types.
*
*   RESULT
*
*   EXAMPLE
*
*   NOTES
*
*   BUGS
*
*   SEE ALSO
*      <devices/ahi.h>
*
****************************************************************************
*
*

****i* ahi.device/AHI_AddAudioMode *****************************************
*
*   NAME
*       AHI_AddAudioMode -- add a audio mode to the database (V3)
*
*   SYNOPSIS
*       success = AHI_AddAudioMode( DBtags );
*       D0                          A0
*
*       ULONG AHI_AddAudioMode( struct TagItem *, UBYTE *, UBYTE *);
*
*   FUNCTION
*       Adds the audio mode described by a taglist to the audio mode
*       database. If the database does not exists, it will be created.
*
*   INPUTS
*       DBtags - Tags describing the properties of this mode.
*
*   RESULT
*       success - FALSE if the mode could not be added.
*
*   EXAMPLE
*
*   NOTES
*
*   BUGS
*
*   SEE ALSO
*
****************************************************************************
*
*

****i* ahi.device/AHI_RemoveAudioMode **************************************
*
*   NAME
*       AHI_RemoveAudioMode -- remove a audio mode to the database (V3)
*
*   SYNOPSIS
*       success = AHI_RemoveAudioMode( ID );
*       D0                             D0
*
*       ULONG AHI_RemoveAudioMode( ULONG );
*
*   FUNCTION
*       Removes the audio mode from the audio mode database.
*
*   INPUTS
*       ID - The audio ID of the mode to be removed.
*
*   RESULT
*       success - FALSE if the mode could not be removed.
*
*   EXAMPLE
*
*   NOTES
*
*   BUGS
*
*   SEE ALSO
*
****************************************************************************
*
*

****i* ahi.device/AHI_LoadModeFile *****************************************
*
*   NAME
*       AHI_LoadModeFile -- Add all modes in a mode file to the database (V3)
*
*   SYNOPSIS
*       success = AHI_LoadModeFile( name );
*       D0                          A0
*
*       ULONG AHI_LoadModeFile( STRPTR );
*
*   FUNCTION
*       This function takes the name of a file or a directory and either
*       adds all modes in the file or the modes of all files in the
*       directory to the audio mode database. Directories inside the
*       given directory will not be recursed. The file format is IFF-AHIM.
*
*   INPUTS
*       name - A pointer to the name of a file or directory.
*
*   RESULT
*       success - FALSE on error. Check dos.library/IOErr() for more
*           information.
*
*   EXAMPLE
*
*   NOTES
*
*   BUGS
*
*   SEE ALSO
*
****************************************************************************
*
*

****** ahi.device/NSCMD_DEVICEQUERY  ***************************************
*
*   NAME
*       NSCMD_DEVICEQUERY -- Query the device for its capabilities (V3)
*
*   FUNCTION
*       Fills an initialized NSDeviceQueryResult structure with
*       information about the device.
*
*   IO REQUEST INPUT
*       io_Device       Preset by the call to OpenDevice().
*       io_Unit         Preset by the call to OpenDevice().
*       io_Command      NSCMD_DEVICEQUERY
*       io_Data         Pointer to the NSDeviceQueryResult structure,
*                       initialized as follows:
*                           DevQueryFormat - Set to 0
*                           SizeAvailable  - Must be cleared.
*                       It is probably good manners to clear all other
*                       fields as well.
*       io_Length       Size of the NSDeviceQueryResult structure.
*
*   IO REQUEST RESULT
*       io_Error        0 for success, or an error code as defined in
*                       <ahi/devices.h> and <exec/errors.h>.
*       io_Actual       If io_Error is 0, the value in
*                       NSDeviceQueryResult.SizeAvailable.
*
*       The NSDeviceQueryResult structure now contains valid information.
*
*       The other fields, except io_Device, io_Unit and io_Command, are
*       trashed.
*
*   EXAMPLE
*
*   NOTES
*
*   BUGS
*
*   SEE ALSO
*       <ahi/devices.h>, <exec/errors.h>
*
****************************************************************************
*
*

****** ahi.device/CMD_RESET ************************************************
*
*   NAME
*       CMD_RESET -- Restore device to a known state (V3)
*
*   FUNCTION
*       Aborts all current requestes, even other programs requests
*       (CMD_FLUSH), rereads the configuration file and resets the hardware
*       to its initial state
*       
*
*   IO REQUEST INPUT
*       io_Device       Preset by the call to OpenDevice().
*       io_Unit         Preset by the call to OpenDevice().
*       io_Command      CMD_RESET
*
*   IO REQUEST RESULT
*       io_Error        0 for success, or an error code as defined in
*                       <ahi/devices.h> and <exec/errors.h>.
*
*       The other fields, except io_Device, io_Unit and io_Command, are
*       trashed.
*
*   EXAMPLE
*
*   NOTES
*       This command should only be used in very rare cases, like AHI
*       system utilities. Never use this command in an application.
*
*   BUGS
*
*   SEE ALSO
*       CMD_FLUSH, <ahi/devices.h>, <exec/errors.h>
*
****************************************************************************
*
*

****** ahi.device/CMD_READ *************************************************
*
*   NAME
*       CMD_READ -- Read raw samples from audio input (V3)
*
*   FUNCTION
*       Reads samples from the users prefered input to memory. The sample
*       format and frequency will be converted on the fly. 
*
*   IO REQUEST INPUT
*       io_Device       Preset by the call to OpenDevice().
*       io_Unit         Preset by the call to OpenDevice().
*       io_Command      CMD_READ
*       io_Data         Pointer to the buffer where the data should be put.
*       io_Length       Number of bytes to read, must be a multiple of the
*                       sample frame size (see ahir_Type).
*       io_Offset       Set to 0 when you use for the first time or after
*                       a delay.
*       ahir_Type       The desired sample format, see <ahi/devices.h>.
*       ahir_Frequency  The desired sample frequency in Herz.
*
*   IO REQUEST RESULT
*       io_Error        0 for success, or an error code as defined in
*                       <ahi/devices.h> and <exec/errors.h>.
*       io_Actual       If io_Error is 0, number of bytes actually
*                       transferred.
*       io_Offset       Updated to be used as input next time.
*
*       The other fields, except io_Device, io_Unit and io_Command, are
*       trashed.
*
*   EXAMPLE
*
*   NOTES
*
*   BUGS
*
*   SEE ALSO
*       <ahi/devices.h>, <exec/errors.h>
*
****************************************************************************
*
*

****** ahi.device/CMD_WRITE ************************************************
*
*   NAME
*       CMD_WRITE -- Write raw samples to audio output (V3)
*
*   FUNCTION
*       Plays the samples to the users prefered audio output.
*
*   IO REQUEST INPUT
*       io_Device       Preset by the call to OpenDevice().
*       io_Unit         Preset by the call to OpenDevice().
*       io_Command      CMD_WRITE
*       io_Data         Pointer to the buffer of samples to be played.
*       io_Length       Number of bytes to play, must be a multiple of the
*                       sample frame size (see ahir_Type).
*       io_Offset       Must be 0.
*       ahir_Type       The desired sample format, see <ahi/devices.h>.
*       ahir_Frequency  The desired sample frequency in Herz.
*       ahir_Volume     The desired volume. The range is 0 to 0x10000, where
*                       0 means muted and 0x10000 (== 1.0) means full volume.
*       ahir_Position   Defines the stereo balance. 0 is far left, 0x8000 is
*                       center and 0x10000 is far right.
*       ahir_Link       If non-zero, pointer to a previously sent AHIRequest
*                       which this AHIRequest will be linked to. This
*                       request will be delayed until the old one is
*                       finished (used for double buffering). Must be set
*                       to NULL if not used.
*
*   IO REQUEST RESULT
*       io_Error        0 for success, or an error code as defined in
*                       <ahi/devices.h> and <exec/errors.h>.
*       io_Actual       If io_Error is 0, number of bytes actually
*                       played.
*
*       The other fields, except io_Device, io_Unit and io_Command, are
*       trashed.
*
*   EXAMPLE
*
*   NOTES
*
*   BUGS
*
*   SEE ALSO
*       <ahi/devices.h>, <exec/errors.h>
*
****************************************************************************
*
*

****** ahi.device/CMD_FLUSH ************************************************
*
*   NAME
*       CMD_FLUSH -- Cancel all I/O requests (V3)
*
*   FUNCTION
*       Aborts ALL current requestes, both active and waiting, even
*       other programs requests!
*
*   IO REQUEST INPUT
*       io_Device       Preset by the call to OpenDevice().
*       io_Unit         Preset by the call to OpenDevice().
*       io_Command      CMD_FLUSH
*
*   IO REQUEST RESULT
*       io_Error        0 for success, or an error code as defined in
*                       <ahi/devices.h> and <exec/errors.h>.
*       io_Actual       If io_Error is 0, number of requests actually
*                       flushed.
*
*       The other fields, except io_Device, io_Unit and io_Command, are
*       trashed.
*
*   EXAMPLE
*
*   NOTES
*       This command should only be used in very rare cases, like AHI
*       system utilities. Never use this command in an application.
*
*   BUGS
*
*   SEE ALSO
*       CMD_RESET, <ahi/devices.h>, <exec/errors.h>
*
****************************************************************************
*
*

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
