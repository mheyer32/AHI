
#ifndef AHI_Drivers_EMU10k1
#define AHI_Drivers_EMU10k1

struct EMU10k1
{
    APTR   m_PCIDev;
    UBYTE* m_IOBase;
    ULONG  m_IOLength;
    UBYTE  m_IRQ;
    UBYTE  m_ChipRev;
    UWORD  m_Model;
};

#endif /* AHI_Drivers_EMU10k1 */
