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
    struct MinList members;
    Object*        buffer;
    Object*        parent;
    ULONG          disabled;
    ULONG          busy;
};

#endif /* AHI_Classes_Processor_ahiclass_h */
