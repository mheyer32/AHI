#ifndef DriverBase_h
#define DriverBase_h

#include <proto/dos.h>
#include <proto/exec.h>
#include <proto/intuition.h>
#include <proto/utility.h>

struct DriverBase
{
    struct Library  library;
    UWORD           pad;
    BPTR            seglist;
    struct Library* sysbase;
    struct Library* intuitionbase;
    struct Library* utilitybase;

    struct Library* dosbase;
};

#define SysBase       ((struct ExecBase*)      AHIsubBase->sysbase)
#define IntuitionBase ((struct IntuitionBase*) AHIsubBase->intuitionbase)
#define UtilityBase   ((struct UtilityBase*)   AHIsubBase->utilitybase)

#define DOSBase       ((struct DosLibrary*)    AHIsubBase->dosbase)

#endif /* DriverBase_h */
