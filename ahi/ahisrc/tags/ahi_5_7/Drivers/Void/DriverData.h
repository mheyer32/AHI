#ifndef  DriverData_h
#define  DriverData_h

#include <exec/types.h>
#include <exec/libraries.h>

struct DriverBase;
struct Process;

struct DriverData
{
    UBYTE		flags;
    UBYTE		pad1;
    BYTE		mastersignal;
    BYTE		slavesignal;
    struct Process*	mastertask;
    struct Process*	slavetask;
    struct DriverBase*	ahisubbase;
    APTR		mixbuffer;
};

BOOL
DriverInit( struct DriverBase* AHIsubBase );

VOID
DriverCleanup( struct DriverBase* AHIsubBase );

#endif /* DriverData_h */
