#ifndef AHI_Classes_Common_boopsi_h
#define AHI_Classes_Common_boopsi_h

#include <exec/types.h>
#include <proto/exec.h>
#include <proto/intuition.h>
#include <proto/utility.h>

extern const char  ClassName[];
extern const char  ClassIDString[];
extern const UWORD ClassVersion;
extern const UWORD ClassRevision;

struct CommonBase {
    struct ClassLibrary  cl;
    BPTR                 seglist;
    struct ClassLibrary* super;
};

struct ClassData;
struct ObjectData;

BOOL
AHIClassInit(struct ClassData* ClassData);

VOID
AHIClassCleanup(struct ClassData* ClassData);

ULONG
AHIClassDispatch(Class*  class,
		 Object* object,
		 Msg     msg);

#endif /* AHI_Classes_Common_boopsi_h */
