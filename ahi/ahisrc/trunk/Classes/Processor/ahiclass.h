#ifndef AHI_Classes_Processor_ahiclass_h
#define AHI_Classes_Processor_ahiclass_h

#include <classes/ahi_types.h>
#include <exec/lists.h>

#include "boopsi.h"

struct ClassData {
    struct CommonBase common;
};

struct ObjectData {
    struct MinList members;
    struct MinList buffers;
    ULONG          num_members;
    Object*        ic;
    Object*        buffer;
    Object*        parent;
    ULONG          disabled;
    ULONG          busy;
    ULONG          ready;
};

#endif /* AHI_Classes_Processor_ahiclass_h */
