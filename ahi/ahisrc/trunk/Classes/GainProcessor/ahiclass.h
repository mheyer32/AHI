#ifndef AHI_Classes_Processor_ahiclass_h
#define AHI_Classes_Processor_ahiclass_h

#include <classes/ahi_types.h>
#include <exec/lists.h>

#include "boopsi.h"

struct ClassData {
    struct CommonBase common;
};

struct ObjectData {
    ULONG  channels;
    float  gain;
    float* gains;
    float* balance;
    float* data;
    float* last_gains;
    float* sweeps;
    ULONG  length;
};

#endif /* AHI_Classes_Processor_ahiclass_h */
