/*
     emu10kx.audio - AHI driver for SoundBlaster Live! series
     Copyright (C) 2002-2003 Martin Blom <martin@blom.org>

     This program is free software; you can redistribute it and/or
     modify it under the terms of the GNU General Public License
     as published by the Free Software Foundation; either version 2
     of the License, or (at your option) any later version.

     This program is distributed in the hope that it will be useful,
     but WITHOUT ANY WARRANTY; without even the implied warranty of
     MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
     GNU General Public License for more details.

     You should have received a copy of the GNU General Public License
     along with this program; if not, write to the Free Software
     Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.
*/

/******************************************************************************
** WARNING  *******************************************************************
*******************************************************************************

Note that you CANNOT base proprietary drivers on this particular
driver! Anything that is based on this driver has to be GPL:ed.

*******************************************************************************
** WARNING  *******************************************************************
******************************************************************************/


#include <config.h>

#ifdef __AMIGAOS4__
#include <proto/expansion.h>
#else
#include <libraries/openpci.h>
#include <proto/openpci.h>
#include <clib/alib_protos.h>
#endif

#include <devices/ahi.h>
#include <exec/memory.h>
#include <libraries/ahi_sub.h>

#include <proto/ahi_sub.h>
#include <proto/exec.h>
#include <proto/utility.h>

#include <string.h>

#include "library.h"
#include "8010.h"
#include "emu10kx-misc.h"


/******************************************************************************
** Globals ********************************************************************
******************************************************************************/

#define FREQUENCIES 8

static const ULONG Frequencies[ FREQUENCIES ] =
{
  8000,     // ?- and A-Law
  11025,    // CD/4
  16000,    // DAT/3
  22050,    // CD/2
  24000,    // DAT/2
  32000,    // DAT/1.5
  44100,    // CD
  48000     // DAT
};

#define INPUTS 8

static const STRPTR Inputs[ INPUTS ] =
{
  "Mixer",
  "Line in",
  "Mic",
  "CD",
  "Aux",
  "Phone",
  "Video",
  "Mixer (mono)"
};

/* Not static since it's used in emu10kx-misc.c too */
const UWORD InputBits[ INPUTS ] =
{
  AC97_RECMUX_STEREO_MIX,
  AC97_RECMUX_LINE,
  AC97_RECMUX_MIC,
  AC97_RECMUX_CD,
  AC97_RECMUX_AUX,
  AC97_RECMUX_PHONE,
  AC97_RECMUX_VIDEO,
  AC97_RECMUX_MONO_MIX
};


#define OUTPUTS 2

static const STRPTR Outputs[ OUTPUTS ] =
{
  "Front",
  "Front & Rear"
};


/******************************************************************************
** AHIsub_AllocAudio **********************************************************
******************************************************************************/

ULONG
_AHIsub_AllocAudio( struct TagItem*         taglist,
		    struct AHIAudioCtrlDrv* AudioCtrl,
		    struct DriverBase*      AHIsubBase )
{
  struct EMU10kxBase* EMU10kxBase = (struct EMU10kxBase*) AHIsubBase;

  int   card_num;
  ULONG ret;
  int   i;

  card_num = ( GetTagData( AHIDB_AudioID, 0, taglist) & 0x0000f000 ) >> 12;

  if( card_num >= EMU10kxBase->cards_found ||
      EMU10kxBase->driverdatas[ card_num ] == NULL )
  {
    Req( "No EMU10kxData for card %ld.", card_num );
    return AHISF_ERROR;
  }
  else
  {
    struct EMU10kxData* dd = EMU10kxBase->driverdatas[ card_num ];
    BOOL                in_use;

    AudioCtrl->ahiac_DriverData = dd;

    ObtainSemaphore( &EMU10kxBase->semaphore );
    in_use = ( dd->audioctrl != NULL );
    if( !in_use )
    {
      dd->audioctrl = AudioCtrl;
    }
    ReleaseSemaphore( &EMU10kxBase->semaphore );

    if( in_use )
    {
      return AHISF_ERROR;
    }
    
    /* Since the EMU10kx chips can play a voice at any sample rate, we
       do not have to examine/modify AudioCtrl->ahiac_MixFreq here.

       Had this not been the case, AudioCtrl->ahiac_MixFreq should be
       set to the frequency we will use.

       However, recording can only be performed at the fixed sampling
       rates.
    */
  }

  ret = AHISF_KNOWHIFI | AHISF_KNOWSTEREO | AHISF_MIXING | AHISF_TIMING;

  for( i = 0; i < FREQUENCIES; ++i )
  {
    if( AudioCtrl->ahiac_MixFreq == Frequencies[ i ] )
    {
      ret |= AHISF_CANRECORD;
      break;
    }
  }

  return ret;
}



/******************************************************************************
** AHIsub_FreeAudio ***********************************************************
******************************************************************************/

void
_AHIsub_FreeAudio( struct AHIAudioCtrlDrv* AudioCtrl,
		   struct DriverBase*      AHIsubBase )
{
  struct EMU10kxBase* EMU10kxBase = (struct EMU10kxBase*) AHIsubBase;
  struct EMU10kxData* dd = (struct EMU10kxData*) AudioCtrl->ahiac_DriverData;

  if( dd != NULL )
  {
    ObtainSemaphore( &EMU10kxBase->semaphore );
    if( dd->audioctrl == AudioCtrl )
    {
      // Release it if we own it.
      dd->audioctrl = NULL;
    }
    ReleaseSemaphore( &EMU10kxBase->semaphore );

    AudioCtrl->ahiac_DriverData = NULL;
  }
}


/******************************************************************************
** AHIsub_Disable *************************************************************
******************************************************************************/

void
_AHIsub_Disable( struct AHIAudioCtrlDrv* AudioCtrl,
		 struct DriverBase*      AHIsubBase )
{
  struct EMU10kxBase* EMU10kxBase = (struct EMU10kxBase*) AHIsubBase;

  // V6 drivers do not have to preserve all registers

  Disable();
}


/******************************************************************************
** AHIsub_Enable **************************************************************
******************************************************************************/

void
_AHIsub_Enable( struct AHIAudioCtrlDrv* AudioCtrl,
		struct DriverBase*      AHIsubBase )
{
  struct EMU10kxBase* EMU10kxBase = (struct EMU10kxBase*) AHIsubBase;

  // V6 drivers do not have to preserve all registers

  Enable();
}


/******************************************************************************
** AHIsub_Start ***************************************************************
******************************************************************************/

ULONG
_AHIsub_Start( ULONG                   flags,
	       struct AHIAudioCtrlDrv* AudioCtrl,
	       struct DriverBase*      AHIsubBase )
{
  struct EMU10kxBase* EMU10kxBase = (struct EMU10kxBase*) AHIsubBase;
  struct EMU10kxData* dd = (struct EMU10kxData*) AudioCtrl->ahiac_DriverData;

  /* Stop playback/recording, free old buffers (if any) */
  AHIsub_Stop( flags, AudioCtrl );

  if( flags & AHISF_PLAY )
  {
    ULONG dma_buffer_size;
    ULONG dma_sample_frame_size;

    /* Update cached/syncronized variables */

    AHIsub_Update( AHISF_PLAY, AudioCtrl );

    /* Allocate a new mixing buffer. Note: The buffer must be cleared, since
       it might not be filled by the mixer software interrupt because of
       pretimer/posttimer! */

    dd->mix_buffer = AllocVec( AudioCtrl->ahiac_BuffSize,
			       MEMF_ANY | MEMF_PUBLIC | MEMF_CLEAR );

    if( dd->mix_buffer == NULL )
    {
      Req( "Unable to allocate %ld bytes for mixing buffer.",
	   AudioCtrl->ahiac_BuffSize );
      return AHIE_NOMEM;
    }

    /* Allocate a voice buffer large enough for 16-bit double-buffered
       playback (mono or stereo) */


    if( AudioCtrl->ahiac_Flags & AHIACF_STEREO )
    {
      dma_sample_frame_size = 4;
      dma_buffer_size = AudioCtrl->ahiac_MaxBuffSamples * dma_sample_frame_size;
    }
    else
    {
      dma_sample_frame_size = 2;
      dma_buffer_size = AudioCtrl->ahiac_MaxBuffSamples * dma_sample_frame_size;
    }

    if( emu10k1_voice_alloc_buffer( &dd->card,
				    &dd->voice.mem,
				    ( dma_buffer_size * 2 + PAGE_SIZE - 1 )
				    / PAGE_SIZE ) < 0 )
    {
      Req( "Unable to allocate voice buffer." );
      return AHIE_NOMEM;
    }

    memset( dd->voice.mem.addr, 0, dma_buffer_size * 2 );

    dd->voice_buffer_allocated = TRUE;


    dd->voice.usage = VOICE_USAGE_PLAYBACK;

    if( AudioCtrl->ahiac_Flags & AHIACF_STEREO )
    {
      dd->voice.flags = VOICE_FLAGS_STEREO | VOICE_FLAGS_16BIT;
    }
    else
    {
      dd->voice.flags = VOICE_FLAGS_16BIT;
    }

    if( emu10k1_voice_alloc( &dd->card, &dd->voice ) < 0 )
    {
      Req( "Unable to allocate voice." );
      return AHIE_UNKNOWN;
    }

    dd->voice_allocated = TRUE;

    dd->voice.initial_pitch = (u16) ( srToPitch( AudioCtrl->ahiac_MixFreq ) >> 8 );
    dd->voice.pitch_target  = SamplerateToLinearPitch( AudioCtrl->ahiac_MixFreq );

    DPD(2, "Initial pitch --> %#x\n", dd->voice.initial_pitch);

    /* start, startloop and endloop is unit sample frames, not bytes */

    dd->voice.start     = ( ( dd->voice.mem.emupageindex << 12 )
			    / dma_sample_frame_size );
    dd->voice.endloop   = dd->voice.start + AudioCtrl->ahiac_MaxBuffSamples * 2;
    dd->voice.startloop = dd->voice.start;


    /* Make interrupt routine start at the correct location */

    dd->current_position = dd->current_length;
    dd->current_buffer   = ( dd->voice.mem.addr +
			     dd->current_position * dma_sample_frame_size );

    dd->voice.params[0].volume_target = 0xffff;
    dd->voice.params[0].initial_fc = 0xff;
    dd->voice.params[0].initial_attn = 0x00;
    dd->voice.params[0].byampl_env_sustain = 0x7f;
    dd->voice.params[0].byampl_env_decay = 0x7f;
    
    if( dd->voice.flags & VOICE_FLAGS_STEREO )
    {
      if( dd->card.is_audigy )
      {
	dd->voice.params[0].send_dcba = 0x00ff00ff;
	dd->voice.params[0].send_hgfe = 0x00007f7f;
	dd->voice.params[1].send_dcba = 0xff00ff00;
	dd->voice.params[1].send_hgfe = 0x00007f7f;

	dd->voice.params[0].send_routing  = dd->voice.params[1].send_routing  = 0x03020100;
	dd->voice.params[0].send_routing2 = dd->voice.params[1].send_routing2 = 0x07060504;
      }
      else
      {
	dd->voice.params[0].send_dcba = 0x000000ff;
	dd->voice.params[0].send_hgfe = 0;
	dd->voice.params[1].send_dcba = 0x0000ff00;
	dd->voice.params[1].send_hgfe = 0;

	dd->voice.params[0].send_routing  = dd->voice.params[1].send_routing  = 0x3210;
	dd->voice.params[0].send_routing2 = dd->voice.params[1].send_routing2 = 0;
      }

      dd->voice.params[1].volume_target = 0xffff;
      dd->voice.params[1].initial_fc = 0xff;
      dd->voice.params[1].initial_attn = 0x00;
      dd->voice.params[1].byampl_env_sustain = 0x7f;
      dd->voice.params[1].byampl_env_decay = 0x7f;
    }
    else
    {
      if( dd->card.is_audigy )
      {
	dd->voice.params[0].send_dcba = 0xffffffff;
	dd->voice.params[0].send_hgfe = 0x0000ffff;
 
	dd->voice.params[0].send_routing  = 0x03020100;
	dd->voice.params[0].send_routing2 = 0x07060504;
     }
      else
      {
	dd->voice.params[0].send_dcba = 0x000ffff;
	dd->voice.params[0].send_hgfe = 0;

	dd->voice.params[0].send_routing  = 0x3210;
	dd->voice.params[0].send_routing2 = 0;
      }
    }

    DPD(2, "voice: startloop=%x, endloop=%x\n",
	dd->voice.startloop, dd->voice.endloop);

    emu10k1_voice_playback_setup( &dd->voice );

    dd->playback_interrupt_enabled = TRUE;

    /* Enable timer interrupts (TIMER_INTERRUPT_FREQUENCY Hz) */

    emu10k1_timer_set( &dd->card, 48000 / TIMER_INTERRUPT_FREQUENCY );
    emu10k1_irq_enable( &dd->card, INTE_INTERVALTIMERENB );

    emu10k1_voices_start( &dd->voice, 1, 0 );

    dd->voice_started = TRUE;

    dd->is_playing = TRUE;
  }

  if( flags & AHISF_RECORD )
  {
    int adcctl = 0;
    /* Find out the recording frequency */

    switch( AudioCtrl->ahiac_MixFreq )
    {
      case 48000:
	adcctl = ADCCR_SAMPLERATE_48;
	break;

      case 44100:
	adcctl = ADCCR_SAMPLERATE_44;
	break;

      case 32000:
	adcctl = ADCCR_SAMPLERATE_32;
	break;

      case 24000:
	adcctl = ADCCR_SAMPLERATE_24;
	break;

      case 22050:
	adcctl = ADCCR_SAMPLERATE_22;
	break;

      case 16000:
	adcctl = ADCCR_SAMPLERATE_16;
	break;

      case 11025:
	adcctl = ADCCR_SAMPLERATE_11;
	break;

      case 8000:
	adcctl = ADCCR_SAMPLERATE_8;
	break;

      default:
	return AHIE_UNKNOWN;
    }

    adcctl |= ADCCR_LCHANENABLE | ADCCR_RCHANENABLE;

    /* Allocate a new recording buffer (page aligned!) */
    dd->record_buffer = pci_alloc_consistent( dd->card.pci_dev,
					      RECORD_BUFFER_SAMPLES * 4,
					      &dd->record_dma_handle );

    if( dd->record_buffer == NULL )
    {
      Req( "Unable to allocate %ld bytes for the recording buffer.",
	   RECORD_BUFFER_SAMPLES * 4 );
      return AHIE_NOMEM;
    }

    SaveMixerState( dd );
    UpdateMonitorMixer( dd );

    sblive_writeptr( &dd->card, ADCBA, 0, dd->record_dma_handle );
    sblive_writeptr( &dd->card, ADCBS, 0, RECORD_BUFFER_SIZE_VALUE );
    sblive_writeptr( &dd->card, ADCCR, 0, adcctl );

    dd->record_interrupt_enabled = TRUE;

    /* Enable ADC interrupts  */

    emu10k1_irq_enable( &dd->card, INTE_ADCBUFENABLE );

    dd->is_recording = TRUE;
  }

  return AHIE_OK;
}


/******************************************************************************
** AHIsub_Update **************************************************************
******************************************************************************/

void
_AHIsub_Update( ULONG                   flags,
		struct AHIAudioCtrlDrv* AudioCtrl,
		struct DriverBase*      AHIsubBase )
{
  struct EMU10kxBase* EMU10kxBase = (struct EMU10kxBase*) AHIsubBase;
  struct EMU10kxData* dd = (struct EMU10kxData*) AudioCtrl->ahiac_DriverData;

  dd->current_length = AudioCtrl->ahiac_BuffSamples;

  if( AudioCtrl->ahiac_Flags & AHIACF_STEREO )
  {
    dd->current_size = dd->current_length * 4;
  }
  else
  {
    dd->current_size = dd->current_length * 2;
  }
}


/******************************************************************************
** AHIsub_Stop ****************************************************************
******************************************************************************/

void
_AHIsub_Stop( ULONG                   flags,
	      struct AHIAudioCtrlDrv* AudioCtrl,
	      struct DriverBase*      AHIsubBase )
{
  struct EMU10kxBase* EMU10kxBase = (struct EMU10kxBase*) AHIsubBase;
  struct EMU10kxData* dd = (struct EMU10kxData*) AudioCtrl->ahiac_DriverData;

  if( flags & AHISF_PLAY )
  {
    dd->is_playing= FALSE;

    if( dd->voice_started )
    {
      emu10k1_irq_disable( &dd->card, INTE_INTERVALTIMERENB );

//      sblive_writeptr( &dd->card, CLIEL, dd->voice.num, 0 );
      emu10k1_voices_stop( &dd->voice, 1 );
      dd->voice_started = FALSE;
    }

    if( dd->voice_allocated )
    {
      emu10k1_voice_free( &dd->voice );
      dd->voice_allocated = FALSE;
    }

    if( dd->voice_buffer_allocated )
    {
      emu10k1_voice_free_buffer( &dd->card, &dd->voice.mem );
      dd->voice_buffer_allocated = FALSE;
    }

    memset( &dd->voice, 0, sizeof( dd->voice ) );

    dd->current_length   = 0;
    dd->current_size     = 0;
    dd->current_buffer   = NULL;
    dd->current_position = 0;

    FreeVec( dd->mix_buffer );
    dd->mix_buffer = NULL;
  }

  if( flags & AHISF_RECORD )
  {
    emu10k1_irq_disable( &dd->card, INTE_ADCBUFENABLE );

    sblive_writeptr( &dd->card, ADCCR, 0, 0 );
    sblive_writeptr( &dd->card, ADCBS, 0, ADCBS_BUFSIZE_NONE );

    if( dd->is_recording )
    {
      // Do not restore mixer unless they have been saved
      RestoreMixerState( dd );
    }

    if( dd->record_buffer != NULL )
    {
      pci_free_consistent( dd->card.pci_dev,
			   RECORD_BUFFER_SAMPLES * 4,
			   dd->record_buffer,
			   dd->record_dma_handle );
    }

    dd->record_buffer = NULL;
    dd->record_dma_handle = 0;

    dd->is_recording = FALSE;
  }
}


/******************************************************************************
** AHIsub_GetAttr *************************************************************
******************************************************************************/

LONG
_AHIsub_GetAttr( ULONG                   attribute,
		 LONG                    argument,
		 LONG                    def,
		 struct TagItem*         taglist,
		 struct AHIAudioCtrlDrv* AudioCtrl,
		 struct DriverBase*      AHIsubBase )
{
  struct EMU10kxBase* EMU10kxBase = (struct EMU10kxBase*) AHIsubBase;
  int i;

  switch( attribute )
  {
    case AHIDB_Bits:
      return 16;

    case AHIDB_Frequencies:
      return FREQUENCIES;

    case AHIDB_Frequency: // Index->Frequency
      return (LONG) Frequencies[ argument ];

    case AHIDB_Index: // Frequency->Index
      if( argument <= (LONG) Frequencies[ 0 ] )
      {
        return 0;
      }

      if( argument >= (LONG) Frequencies[ FREQUENCIES - 1 ] )
      {
        return FREQUENCIES-1;
      }

      for( i = 1; i < FREQUENCIES; i++ )
      {
        if( (LONG) Frequencies[ i ] > argument )
        {
          if( ( argument - (LONG) Frequencies[ i - 1 ] )
	      < ( (LONG) Frequencies[ i ] - argument ) )
          {
            return i-1;
          }
          else
          {
            return i;
          }
        }
      }

      return 0;  // Will not happen

    case AHIDB_Author:
      return (LONG) "Martin 'Leviticus' Blom, Bertrand Lee et al.";

    case AHIDB_Copyright:
      return (LONG) "GNU GPL";

    case AHIDB_Version:
      return (LONG) LibIDString;

    case AHIDB_Annotation:
      return (LONG)
	"Funded by Hyperion Entertainment. Based on the Linux driver.";

    case AHIDB_Record:
      return TRUE;

    case AHIDB_FullDuplex:
      return TRUE;

    case AHIDB_Realtime:
      return TRUE;

    case AHIDB_MaxRecordSamples:
      return RECORD_BUFFER_SAMPLES;

    case AHIDB_MinMonitorVolume:
      return 0x00000;

    case AHIDB_MaxMonitorVolume:
      return 0x40000;

    case AHIDB_MinInputGain:
      return 0x10000;

    case AHIDB_MaxInputGain:
      return 0xD7450;

    case AHIDB_MinOutputVolume:
      return 0x00000;

    case AHIDB_MaxOutputVolume:
      return 0x40000;

    case AHIDB_Inputs:
      return INPUTS;

    case AHIDB_Input:
      return (LONG) Inputs[ argument ];

    case AHIDB_Outputs:
      return OUTPUTS;

    case AHIDB_Output:
      return (LONG) Outputs[ argument ];

    default:
      return def;
  }
}


/******************************************************************************
** AHIsub_HardwareControl *****************************************************
******************************************************************************/

ULONG
_AHIsub_HardwareControl( ULONG                   attribute,
			 LONG                    argument,
			 struct AHIAudioCtrlDrv* AudioCtrl,
			 struct DriverBase*      AHIsubBase )
{
  struct EMU10kxBase* EMU10kxBase = (struct EMU10kxBase*) AHIsubBase;
  struct EMU10kxData* dd = (struct EMU10kxData*) AudioCtrl->ahiac_DriverData;

  switch( attribute )
  {
    case AHIC_MonitorVolume:
      dd->monitor_volume = Linear2MixerGain( (Fixed) argument,
					     &dd->monitor_volume_bits );
      if( dd->is_recording )
      {
	UpdateMonitorMixer( dd );
      }
      return TRUE;

    case AHIC_MonitorVolume_Query:
      return dd->monitor_volume;

    case AHIC_InputGain:
      dd->input_gain = Linear2RecordGain( (Fixed) argument,
					  &dd->input_gain_bits );
      emu10k1_writeac97( &dd->card, AC97_RECORD_GAIN, dd->input_gain_bits );
      return TRUE;

    case AHIC_InputGain_Query:
      return dd->input_gain;

    case AHIC_OutputVolume:
      dd->output_volume = Linear2MixerGain( (Fixed) argument,
					    &dd->output_volume_bits );
      emu10k1_writeac97( &dd->card, AC97_PCMOUT_VOL, dd->output_volume_bits );
      return TRUE;

    case AHIC_OutputVolume_Query:
      return dd->output_volume;

    case AHIC_Input:
      dd->input = argument;
      emu10k1_writeac97( &dd->card, AC97_RECORD_SELECT, InputBits[ dd->input ] );

      if( dd->is_recording )
      {
	UpdateMonitorMixer( dd );
      }

      return TRUE;

    case AHIC_Input_Query:
      return dd->input;

    case AHIC_Output:
      dd->output = argument;

      if( dd->output == 0 )
      {
	emu10k1_set_volume_gpr( &dd->card, 0x19, 0, VOL_5BIT);
	emu10k1_set_volume_gpr( &dd->card, 0x1a, 0, VOL_5BIT);
      }
      else
      {
	emu10k1_set_volume_gpr( &dd->card, 0x19, 80, VOL_5BIT);
	emu10k1_set_volume_gpr( &dd->card, 0x1a, 80, VOL_5BIT);
      }
      return TRUE;

    case AHIC_Output_Query:
      return dd->output;

    default:
      return FALSE;
  }
}
