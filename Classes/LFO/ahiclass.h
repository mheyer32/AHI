#ifndef AHI_Classes_LFO_ahiclass_h
#define AHI_Classes_LFO_ahiclass_h

#include <classes/ahi_types.h>
#include <exec/lists.h>

#include "boopsi.h"

struct AHIClassBase {
    struct CommonBase common;
};

struct AHIClassData {
    double frequency;		// The LFO frequency in Hz
    double phase_offset;	// Phase offset (0.0 <= phase_offset < 1.0)
    ULONG  amplitude;		// The LFO amplitude
    LONG   bias;		// The LFO's DC bias
    ULONG  waveform;		// The LFO waveform
    double tickfreq;		// The tick frequency in Hz
    double ticktime;		// The time of the tick clock (in seconds)

    double phase;		// The current LFO phase (0.0 <= phase < 1.0)
    double i;			// The current LFO in-phase value
    double q;			// The current LFO quadrature value
};

#endif /* AHI_Classes_LFO_ahiclass_h */
