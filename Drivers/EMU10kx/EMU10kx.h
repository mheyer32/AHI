
#ifndef AHI_Drivers_EMU10k1_h
#define AHI_Drivers_EMU10k1_h

#include <exec/types.h>
#include <exec/interrupts.h>

#include "hwaccess.h"

struct EMU10k1
{
    struct Interrupt    interrupt;
    struct emu10k1_card card;
};

#endif /* AHI_Drivers_EMU10k1_h */
