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

#include <config.h>

#include <libraries/ahi_sub.h>
#include <libraries/openpci.h>

#include <proto/exec.h>
#include <proto/openpci.h>
#include <proto/utility.h>

#include "library.h"
#include "8010.h"

#define min(a,b) ((a)<(b)?(a):(b))

/******************************************************************************
** Hardware interrupt handler *************************************************
******************************************************************************/

ULONG
EMU10kxInterrupt( struct EMU10kxData* dd )
{
  struct AHIAudioCtrlDrv* AudioCtrl = dd->audioctrl;
  struct DriverBase*  AHIsubBase = (struct DriverBase*) dd->ahisubbase;
  struct EMU10kxBase* EMU10kxBase = (struct EMU10kxBase*) AHIsubBase;

  ULONG intreq;
  BOOL  handled = FALSE;

  while( ( intreq = SWAPLONG( pci_inl( dd->card.iobase + IPR ) ) ) != 0 )
  {
    if( intreq & IPR_INTERVALTIMER &&
	AudioCtrl != NULL )
    {
      int hw   = sblive_readptr( &dd->card, CCCA_CURRADDR, dd->voice.num );
      int diff = dd->current_position - ( hw - dd->voice.start );

      if( diff < 0 )
      {
	diff += AudioCtrl->ahiac_MaxBuffSamples * 2;
      }

//      KPrintF( ">>> hw_pos = %08lx; current_pos = %08lx; diff=%ld <<<\n",
//	       hw, dd->current_position, diff );

      if( (ULONG) diff < dd->current_length )
      {
	if( dd->playback_interrupt_enabled )
	{
	  /* Invoke softint to fetch new sample data */

	  dd->playback_interrupt_enabled = FALSE;
	  Cause( &dd->playback_interrupt );
	}
      }
    }

    if( intreq & ( IPR_ADCBUFHALFFULL | IPR_ADCBUFFULL ) &&
	AudioCtrl != NULL )
    {
      if( intreq & IPR_ADCBUFHALFFULL )
      {
	dd->current_record_buffer = dd->record_buffer;
      }
      else
      {
	dd->current_record_buffer = ( dd->record_buffer +
				      RECORD_BUFFER_SAMPLES * 4 / 2 );
      }

      if( dd->record_interrupt_enabled )
      {
	/* Invoke softint to convert and feed AHI with the new sample data */

	dd->record_interrupt_enabled = FALSE;
	Cause( &dd->record_interrupt );
      }
    }

    /* Clear interrupt pending bit(s) */
    pci_outl( SWAPLONG( intreq ), dd->card.iobase + IPR );

    handled = TRUE;
  }

  return handled;
}


/******************************************************************************
** Playback interrupt handler *************************************************
******************************************************************************/

void
PlaybackInterrupt( struct EMU10kxData* dd )
{
  struct AHIAudioCtrlDrv* AudioCtrl = dd->audioctrl;
  struct DriverBase*  AHIsubBase = (struct DriverBase*) dd->ahisubbase;
  struct EMU10kxBase* EMU10kxBase = (struct EMU10kxBase*) AHIsubBase;

  if( dd->mix_buffer != NULL && dd->current_buffer != NULL )
  {
    BOOL   skip_mix;

    WORD*  src;
    WORD*  dst;
    size_t skip;
    size_t samples;
    int    i;

    skip_mix = CallHookPkt( AudioCtrl->ahiac_PreTimerFunc, AudioCtrl, 0 );

    CallHookPkt( AudioCtrl->ahiac_PlayerFunc, (Object*) AudioCtrl, NULL );

    if( ! skip_mix )
    {
      CallHookPkt( AudioCtrl->ahiac_MixerFunc, (Object*) AudioCtrl, dd->mix_buffer );
    }

    /* Now translate and transfer to the DMA buffer */

    skip    = ( AudioCtrl->ahiac_Flags & AHIACF_HIFI ) ? 2 : 1;
    samples = dd->current_length;

    src     = dd->mix_buffer;
    dst     = dd->current_buffer;

    i = min( samples,
	     AudioCtrl->ahiac_MaxBuffSamples * 2 - dd->current_position );

    /* Update 'current_position' and 'samples' before destroying 'i' */

    dd->current_position += i;

    samples -= i;

    if( AudioCtrl->ahiac_Flags & AHIACF_STEREO )
    {
      i *= 2;
    }

    while( i > 0 )
    {
      *dst = ( ( *src & 0xff ) << 8 ) | ( ( *src & 0xff00 ) >> 8 );

      src += skip;
      dst += 1;

      --i;
    }

    if( dd->current_position == AudioCtrl->ahiac_MaxBuffSamples * 2 )
    {
      dst = dd->voice.mem.addr;
      dd->current_position = 0;
    }

    if( samples > 0 )
    {
      /* Update 'current_position' before destroying 'samples' */

      dd->current_position += samples;

      if( AudioCtrl->ahiac_Flags & AHIACF_STEREO )
      {
	samples *= 2;
      }

      while( samples > 0 )
      {
	*dst = ( ( *src & 0xff ) << 8 ) | ( ( *src & 0xff00 ) >> 8 );

	src += skip;
	dst += 1;

	--samples;
      }
    }

    /* Update 'current_buffer' */

    dd->current_buffer = dst;

    CallHookPkt( AudioCtrl->ahiac_PostTimerFunc, AudioCtrl, 0 );
  }

  dd->playback_interrupt_enabled = TRUE;
}


/******************************************************************************
** Record interrupt handler ***************************************************
******************************************************************************/

void
RecordInterrupt( struct EMU10kxData* dd )
{
  struct AHIAudioCtrlDrv* AudioCtrl = dd->audioctrl;
  struct DriverBase*  AHIsubBase = (struct DriverBase*) dd->ahisubbase;
  struct EMU10kxBase* EMU10kxBase = (struct EMU10kxBase*) AHIsubBase;

  struct AHIRecordMessage rm =
  {
    AHIST_S16S,
    dd->current_record_buffer,
    RECORD_BUFFER_SAMPLES / 2
  };

  int   i   = 0;
  WORD* ptr = dd->current_record_buffer;

  while( i < RECORD_BUFFER_SAMPLES / 2 * 2 )
  {
    *ptr = ( ( *ptr & 0xff ) << 8 ) | ( ( *ptr & 0xff00 ) >> 8 );

    ++i;
    ++ptr;
  }

  CallHookPkt( AudioCtrl->ahiac_SamplerFunc, (Object*) AudioCtrl, &rm );

  dd->record_interrupt_enabled = TRUE;
}
