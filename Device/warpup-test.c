
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
      void* init = NULL;
      
      ELFGetSymbol( PPCObject, "InitWarpUp", &init );

      printf( "init: 0x%08lx\n", (ULONG) init );

      if( init != NULL )
      {
        LONG status;
        
        struct PPCArgs pps = 
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
        
        status = RunPPC( &pps );
        
        printf( "status: %ld\n", status );
        printf( "r3: 0x%08lx, r4: 0x%08lx\n", 
                pps.PP_Regs[ 0 ], pps.PP_Regs[ 1 ] );
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
