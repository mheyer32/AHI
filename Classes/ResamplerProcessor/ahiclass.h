#ifndef AHI_Classes_ResamplerProcessor_ahiclass_h
#define AHI_Classes_ResamplerProcessor_ahiclass_h

#include <classes/ahi_types.h>
#include <exec/lists.h>

#include "boopsi.h"

struct ClassData {
    struct CommonBase common;
};

struct ObjectData {
    Object* buffer;
    double  frequency;
    double  pitch;
    double  phase_offset;
};

#endif /* AHI_Classes_ResamplerProcessor_ahiclass_h */
