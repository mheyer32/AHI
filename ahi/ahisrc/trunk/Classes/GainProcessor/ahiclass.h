#ifndef AHI_Classes_Processor_ahiclass_h
#define AHI_Classes_Processor_ahiclass_h

#include <classes/ahi_types.h>
#include <exec/lists.h>

#include "boopsi.h"

struct AHIClassBase {
    struct CommonBase    common;
    struct ClassLibrary* super;
};

struct AHIClassData {
    ULONG  channels;
    float* gains;
    float* data;
    ULONG  length;
};

#endif /* AHI_Classes_Processor_ahiclass_h */
