#ifndef AHI_Classes_AHIRootClass_ahiclass_h
#define AHI_Classes_AHIRootClass_ahiclass_h

#include <exec/semaphores.h>

#include "boopsi.h"

struct AHIClassBase {
    struct CommonBase    common;
};

struct AHIClassData {
    struct CommonData      common;
    struct SignalSemaphore semaphore;
    Object*                model_class;
    struct Locale*         locale;
    Object*                owner;
    ULONG                  interrupt_safe;
    ULONG                  error;
    ULONG                  user_data;
    char                   error_message[128];
};

#endif /* AHI_Classes_AHIRootClass_ahiclass_h */
