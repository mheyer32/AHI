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

#define RECORD_BUFFER_SAMPLES     4096
#define RECORD_BUFFER_SIZE_VALUE  ADCBS_BUFSIZE_16384
#define TIMER_INTERRUPT_FREQUENCY 1000

struct DriverData
{
    /*** PCI/EMU10kx initialization progress **********************************/

    /** TRUE if I/O and interrupt resources claimed */
    BOOL                pci_requested;

    /** TRUE if PCI device is enabled */
    BOOL                pci_enabled;

    /** TRUE if bus mastering is activated */
    BOOL                pci_master_enabled;

    /** TRUE if the EMU10kx chip has been initialized */
    BOOL                emu10k1_initialized;

    
    /*** Playback/recording interrupts ****************************************/
    
    /** TRUE when playback is enabled */
    BOOL                is_playing;

    /** TRUE when recording is enabled */
    BOOL                is_recording;

    /** The main (hardware) interrupt */
    struct Interrupt    interrupt;

    /** TRUE if the hardware interrupt has been added to the PCI subsystem */
    BOOL                interrupt_added;

    /** The playback software interrupt */
    struct Interrupt    playback_interrupt;

    /** TRUE if the hardware interrupt may Cause() playback_interrupt */
    BOOL                playback_interrupt_enabled;

    /** The recording software interrupt */
    struct Interrupt    record_interrupt;

    /** TRUE if the hardware interrupt may Cause() playback_interrupt */
    BOOL                record_interrupt_enabled;

    
    /*** EMU10kx structures ***************************************************/
    
    struct emu10k1_card card;
    struct emu_voice    voice;

    BOOL                voice_buffer_allocated;
    BOOL                voice_allocated;
    BOOL                voice_started;
    UWORD               pad;

    
    /*** Playback interrupt variables *****************************************/

    /** The mixing buffer (a cyclic buffer filled by AHI) */
    APTR                mix_buffer;

    /** The length of each playback buffer in sample frames */
    ULONG               current_length;
    
    /** The length of each playback buffer in sample bytes */
    ULONG               current_size;

    /** Where (inside the cyclic buffer) we're currently writing */
    APTR                current_buffer;

    /** The offset (inside the cyclic buffer) we're currently writing */
    ULONG               current_position;


    /*** Recording interrupt variables ****************************************/

    /** The recording buffer (simple double buffering is used */
    APTR                record_buffer;

    /** The DMA handle for the record buffer */
    dma_addr_t          record_dma_handle;

    /** Were (inside the recording buffer) the current data is */
    APTR                current_record_buffer;
    
    /** Analog mixer variables ************************************************/

    /** The currently selected input */
    UWORD               input;

    /** The currently selected output */
    UWORD               output;

    /** The current (recording) monitor volume */
    Fixed               monitor_volume;

    /** The current (recording) input gain */
    Fixed               input_gain;

    /** The current (playback) output volume */
    Fixed               output_volume;

    /** The hardware register value corresponding to monitor_volume */
    UWORD               monitor_volume_bits;

    /** The hardware register value corresponding to input_gain */
    UWORD               input_gain_bits;

    /** The hardware register value corresponding to output_volume */
    UWORD               output_volume_bits;
};

#endif /* EMU10kx_DriverData_h */
