#ifndef AHI_Classes_Buffer_ahiclass_h
#define AHI_Classes_Buffer_ahiclass_h

#include <classes/ahi_types.h>

#include "boopsi.h"

struct ClassData {
    struct CommonBase common;
};

struct ObjectData {
    ULONG             sample_type;
    ULONG             capacity;
    ULONG             length;
    ULONG             pre_length;
    APTR              data;
    ULONG             sample_freq_int;
    ULONG             sample_freq_fract;
    ULONG             timestamp_hi;
    ULONG             timestamp_lo;
    ULONG             age_hi;
    ULONG             age_lo;

    ULONG             prelen_size;
    ULONG             buffer_size;
};

#endif /* AHI_Classes_Buffer_ahiclass_h */
