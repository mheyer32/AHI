
#include <stdio.h>

#include <powerpc/powerpc.h>
#include <proto/exec.h>
#include <proto/powerpc.h>

#include "elfloader.h"

int TimerBase;
int MixGeneric;
int MixPowerUp;
void* PPCObject;

struct Library* PowerPCBase = NULL;

int
main( void )
{
  PowerPCBase = OpenLibrary( "powerpc.library", 15 );

  if( PowerPCBase != NULL )
  {
    PPCObject = ELFLoadObject( "devs:ahi.elf" );

    CacheClearU();

    printf( "PPCObject: 0x%08lx\n", (ULONG) PPCObject );

    if( PPCObject != NULL )
    {
      void* init    = NULL;
      void* cleanup = NULL;
      
      ELFGetSymbol( PPCObject, "InitWarpUp", &init );
      ELFGetSymbol( PPCObject, "CleanUpWarpUp", &cleanup );

      printf( "init:    0x%08lx\n", (ULONG) init );
      printf( "cleanup: 0x%08lx\n", (ULONG) cleanup );

      if( init != NULL && cleanup != NULL )
      {
        LONG status;
        
        struct PPCArgs init_pps = 
        {
          init,
          0,
          0,
          NULL,
          0,
          { 
            (ULONG) PowerPCBase, 0xDEADC0DE, 0, 0, 0, 0, 0,
            0, 0, 0, 0, 0, 0, 0
          },
          {
            0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0
          }
        };

        struct PPCArgs cleanup_pps = 
        {
          cleanup,
          0,
          0,
          NULL,
          0,
          { 
            (ULONG) PowerPCBase, 0, 0, 0, 0, 0, 0,
            0, 0, 0, 0, 0, 0, 0
          },
          {
            0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0
          }
        };

        void* xlock = NULL;
        
        status = RunPPC( &init_pps );
        xlock  = (void*) init_pps.PP_Regs[ 0 ];
        
        printf( "status: %ld; xlock: 0x%08lx\n", status, (ULONG) xlock);

        CausePPCInterrupt();

        cleanup_pps.PP_Regs[ 1 ] = (ULONG) xlock;

        status = RunPPC( &cleanup_pps );
        
        printf( "status: %ld\n", status );
      }

      ELFUnLoadObject( PPCObject );
    }
  
    CloseLibrary( PowerPCBase );
  }

  return 0;
}


APTR
AHIAllocVec( ULONG byteSize, ULONG requirements )
{
  return AllocVec32( byteSize, requirements );
}

void
AHIFreeVec( APTR memoryBlock )
{
  FreeVec32( memoryBlock );
}
