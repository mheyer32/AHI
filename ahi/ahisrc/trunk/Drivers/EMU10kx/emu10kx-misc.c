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

#include "DriverData.h"

extern const UWORD InputBits[];

/******************************************************************************
** Misc. **********************************************************************
******************************************************************************/

void
SaveMixerState( struct EMU10kxData* dd )
{
  dd->ac97_mic    = emu10k1_readac97( &dd->card, AC97_MIC_VOL );
  dd->ac97_cd     = emu10k1_readac97( &dd->card, AC97_CD_VOL );
  dd->ac97_video  = emu10k1_readac97( &dd->card, AC97_VIDEO_VOL );
  dd->ac97_aux    = emu10k1_readac97( &dd->card, AC97_AUX_VOL );
  dd->ac97_linein = emu10k1_readac97( &dd->card, AC97_LINEIN_VOL );
  dd->ac97_phone  = emu10k1_readac97( &dd->card, AC97_PHONE_VOL );
}


void
RestoreMixerState( struct EMU10kxData* dd )
{
  emu10k1_writeac97( &dd->card, AC97_MIC_VOL,    dd->ac97_mic );
  emu10k1_writeac97( &dd->card, AC97_CD_VOL,     dd->ac97_cd );
  emu10k1_writeac97( &dd->card, AC97_VIDEO_VOL,  dd->ac97_video );
  emu10k1_writeac97( &dd->card, AC97_AUX_VOL,    dd->ac97_aux );
  emu10k1_writeac97( &dd->card, AC97_LINEIN_VOL, dd->ac97_linein );
  emu10k1_writeac97( &dd->card, AC97_PHONE_VOL,  dd->ac97_phone );
}

void
UpdateMonitorMixer( struct EMU10kxData* dd )
{
  int   i  = InputBits[ dd->input ];
  UWORD m  = dd->monitor_volume_bits & 0x801f;
  UWORD s  = dd->monitor_volume_bits;
  UWORD mm = AC97_MUTE | 0x0008;
  UWORD sm = AC97_MUTE | 0x0808;

  if( i == AC97_RECMUX_STEREO_MIX ||
      i == AC97_RECMUX_MONO_MIX )
  {
    // Use the original mixer settings
    RestoreMixerState( dd );
  }
  else
  {
    emu10k1_writeac97( &dd->card, AC97_MIC_VOL,
		       i == AC97_RECMUX_MIC ? m : mm );

    emu10k1_writeac97( &dd->card, AC97_CD_VOL,
		       i == AC97_RECMUX_CD ? s : sm );

    emu10k1_writeac97( &dd->card, AC97_VIDEO_VOL,
		       i == AC97_RECMUX_VIDEO ? s : sm );

    emu10k1_writeac97( &dd->card, AC97_AUX_VOL,
		       i == AC97_RECMUX_AUX ? s : sm );

    emu10k1_writeac97( &dd->card, AC97_LINEIN_VOL,
		       i == AC97_RECMUX_LINE ? s : sm );

    emu10k1_writeac97( &dd->card, AC97_PHONE_VOL,
		       i == AC97_RECMUX_PHONE ? m : mm );
  }
}


Fixed
Linear2MixerGain( Fixed  linear,
		  UWORD* bits )
{
  static const Fixed gain[ 33 ] =
  {
    260904, // +12.0 dB
    219523, // +10.5 dB
    184706, //  +9.0 dB
    155410, //  +7.5 dB
    130762, //  +6.0 dB
    110022, //  +4.5 dB
    92572,  //  +3.0 dB
    77890,  //  +1.5 dB
    65536,  //  ±0.0 dB
    55142,  //  -1.5 dB
    46396,  //  -3.0 dB
    39037,  //  -4.5 dB
    32846,  //  -6.0 dB
    27636,  //  -7.5 dB
    23253,  //  -9.0 dB
    19565,  // -10.5 dB
    16462,  // -12.0 dB
    13851,  // -13.5 dB
    11654,  // -15.0 dB
    9806,   // -16.5 dB
    8250,   // -18.0 dB
    6942,   // -19.5 dB
    5841,   // -21.0 dB
    4915,   // -22.5 dB
    4135,   // -24.0 dB
    3479,   // -25.5 dB
    2927,   // -27.0 dB
    2463,   // -28.5 dB
    2072,   // -30.0 dB
    1744,   // -31.5 dB
    1467,   // -33.0 dB
    1234,   // -34.5 dB
    0       //   -oo dB
  };

  int v = 0;

  while( linear < gain[ v ] )
  {
    ++v;
  }

  if( v == 32 )
  {
    *bits = 0x8000; // Mute
  }
  else
  {
    *bits = ( v << 8 ) | v;
  }

//  KPrintF( "l2mg %08lx -> %08lx (%04lx)\n", linear, gain[ v ], *bits );
  return gain[ v ];
}

Fixed
Linear2RecordGain( Fixed  linear,
		   UWORD* bits )
{
  static const Fixed gain[ 17 ] =
  {
    873937, // +22.5 dB
    735326, // +21.0 dB
    618700, // +19.5 dB
    520571, // +18.0 dB
    438006, // +16.5 dB
    368536, // +15.0 dB
    310084, // +13.5 dB
    260904, // +12.0 dB
    219523, // +10.5 dB
    184706, //  +9.0 dB
    155410, //  +7.5 dB
    130762, //  +6.0 dB
    110022, //  +4.5 dB
    92572,  //  +3.0 dB
    77890,  //  +1.5 dB
    65536,  //  ±0.0 dB
    0       //  -oo dB
  };

  int v = 0;

  while( linear < gain[ v ] )
  {
    ++v;
  }

  if( v == 16 )
  {
    *bits = 0x8000; // Mute
  }
  else
  {
    *bits = ( ( 15 - v ) << 8 ) | ( 15 - v );
  }

  return gain[ v ];
}


ULONG
SamplerateToLinearPitch( ULONG samplingrate )
{
  samplingrate = (samplingrate << 8) / 375;
  return (samplingrate >> 1) + (samplingrate & 1);
}
