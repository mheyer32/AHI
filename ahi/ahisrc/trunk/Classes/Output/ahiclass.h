#ifndef AHI_Classes_Output_ahiclass_h
#define AHI_Classes_Output_ahiclass_h

#include <classes/ahi_types.h>
#include <exec/lists.h>

#include "boopsi.h"

struct AHIClassBase {
    struct CommonBase    common;
    struct ClassLibrary* super;
};

struct AHIClassData {
    ULONG mode;
};

#endif /* AHI_Classes_Output_ahiclass_h */
