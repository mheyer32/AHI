
#ifndef EMU10kx_EMU10kx_h
#define EMU10kx_EMU10kx_h

#include <exec/types.h>
#include <exec/interrupts.h>

#include "hwaccess.h"

#define RECORD_BUFFER_SAMPLES 4096

struct EMU10kx
{
    struct Interrupt    interrupt;
    BOOL                interrupt_added;

    struct emu10k1_card card;
};

#endif /* EMU10kx_EMU10kx_h */
