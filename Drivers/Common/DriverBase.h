#ifndef AHI_Drivers_Common_DriverBase_h
#define AHI_Drivers_Common_DriverBase_h

#include <proto/exec.h>
#include <proto/intuition.h>
#include <proto/utility.h>

struct DriverBase
{
    struct Library  library;
    UWORD           pad;
    BPTR            seglist;
#ifndef DRIVER_NEED_GLOBAL_EXECBASE
    struct ExecBase* execbase;
#endif
    struct Library* intuitionbase;
    struct Library* utilitybase;
};

#ifndef DRIVER_NEED_GLOBAL_EXECBASE
#define SysBase       (*(struct ExecBase**)      &AHIsubBase->execbase)
#endif

#define IntuitionBase (*(struct Library**)       &AHIsubBase->intuitionbase)
#define UtilityBase   (*(struct Library**)       &AHIsubBase->utilitybase)


struct DriverData
{
};


BOOL
DriverInit( struct DriverBase* AHIsubBase );

VOID
DriverCleanup( struct DriverBase* AHIsubBase );

#endif /* AHI_Drivers_Common_DriverBase_h */
