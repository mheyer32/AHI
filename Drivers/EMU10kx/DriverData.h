/*
     emu10kx.audio - AHI driver for SoundBlaster Live! series
     Copyright (C) 2002 Martin Blom <martin@blom.org>
     
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

#ifndef EMU10kx_DriverData_h
#define EMU10kx_DriverData_h

#include <exec/types.h>
#include <exec/interrupts.h>

#include "hwaccess.h"
#include "voicemgr.h"

#define RECORD_BUFFER_SAMPLES 4096

struct DriverData
{
    BOOL                requested;
    BOOL                enabled;
    BOOL                master_enabled;
    BOOL                emu10k1_initialized;
    
    struct Interrupt    interrupt;
    BOOL                interrupt_added;

    struct Interrupt    software_interrupt;
    UWORD               pad;
    
    struct emu10k1_card card;
    struct emu_voice    voice;
    
    APTR                mixbuffer;
    BOOL                voice_buffer_allocated;
    BOOL                voice_allocated;
    BOOL                voice_started;

    BOOL                mixing_enabled;

    ULONG               current_length;
    ULONG               current_size;
    APTR                current_buffer;
    ULONG               current_position;

    UWORD               input;
    UWORD               output;
    Fixed               monitor_volume;
    Fixed               input_gain;
    Fixed               output_volume;
};

#endif /* EMU10kx_DriverData_h */
