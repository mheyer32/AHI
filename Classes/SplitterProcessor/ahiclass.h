#ifndef AHI_Classes_SplitterProcessor_ahiclass_h
#define AHI_Classes_SplitterProcessor_ahiclass_h

#include <classes/ahi_types.h>
#include <exec/lists.h>

#include "boopsi.h"

struct ClassData {
    struct CommonBase common;
};

struct ObjectData {
    Object* master;
    Object* buffer;
    float*  cached_data;
    float*  data;
    ULONG   length;
    ULONG   sample_type;
    ULONG   size;
    UQUAD   last_time;
};

#endif /* AHI_Classes_SplitterProcessor_ahiclass_h */
