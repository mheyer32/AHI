#ifndef AHI_Classes_AHIRootClass_ahiclass_h
#define AHI_Classes_AHIRootClass_ahiclass_h

#include "boopsi.h"

struct AHIClassBase {
    struct CommonBase    common;
    struct ClassLibrary* rootclass;
};

struct AHIClassData {
    struct CommonData common;
};

#endif /* AHI_Classes_AHIRootClass_ahiclass_h */
