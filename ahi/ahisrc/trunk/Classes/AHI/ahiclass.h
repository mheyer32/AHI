#ifndef AHI_Classes_AHI_ahiclass_h
#define AHI_Classes_AHI_ahiclass_h

#include <exec/semaphores.h>

#include "boopsi.h"

struct ClassData {
    struct CommonBase common;
};

struct ObjectData {
    struct SignalSemaphore semaphore;
    Object*                model_class;
    struct Locale*         locale;
    Object*                owner;
    ULONG                  interrupt_safe;
    ULONG                  error;
    ULONG                  user_data;
    char                   error_message[128];
};

#endif /* AHI_Classes_AHI_ahiclass_h */
