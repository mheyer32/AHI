
#ifndef AHI_Drivers_EMU10k1
#define AHI_Drivers_EMU10k1

#include <exec/types.h>


typedef unsigned int   u32;
typedef unsigned short u16;
typedef unsigned char  u8;

typedef signed int     s32;
typedef signed short   s16;
typedef signed char    s8;

struct emu10k1_card
{
    APTR   pci_dev;
    ULONG  iobase;
    ULONG  length;
    UBYTE  irq;
    UBYTE  chiprev;
    UWORD  model;
    BOOL   audigy;
    BOOL   isaps;
};

#endif /* AHI_Drivers_EMU10k1 */
