
#if defined( VERSIONPOWERUP )

#include <config.h>
#include <CompilerSpecific.h>

#include <exec/types.h>
#include <hardware/intbits.h>
#include <powerup/ppclib/tasks.h>
#include <powerup/gcclib/powerup_protos.h>

#include "ahi_def.h"
#include "mixer.h"

int
entry( struct Hook *Hook, 
       void *dst, 
       struct AHIPrivAudioCtrl *audioctrl );

void
FlushCache( void* address, unsigned long length );

#ifdef USE_PPC_PROCESS

int
main( void )
{
  struct AHIPrivAudioCtrl *audioctrl;

  audioctrl = (struct AHIPrivAudioCtrl*) PPCGetTaskAttr( PPCTASKTAG_STARTUP_MSGDATA );

  while( TRUE )
  {
    ULONG signals = PPCWait( SIGBREAKF_CTRL_C | SIGBREAKF_CTRL_D );

    if( signals & SIGBREAKF_CTRL_C )
    {
      break;
    }
    
    if( signals & SIGBREAKF_CTRL_D )
    {
      entry( NULL, audioctrl->ahiac_PPCMixBuffer, audioctrl );
    }

  }

  return 0;
}

#endif

int
entry( struct Hook *Hook, 
       void *dst, 
       struct AHIPrivAudioCtrl *audioctrl )
{
  audioctrl->ahiac_PPCCommand = AHIAC_COM_INIT;
  *((WORD*) 0xdff09C)  = INTF_SETCLR | INTF_PORTS;

  while( audioctrl->ahiac_PPCCommand != AHIAC_COM_ACK );

  MixGeneric( Hook, dst, audioctrl );
  
  FlushCache( dst, audioctrl->ahiac_BuffSizeNow );

  audioctrl->ahiac_PPCCommand = AHIAC_COM_QUIT;
  *((WORD*) 0xdff09C)  = INTF_SETCLR | INTF_PORTS;
  while( audioctrl->ahiac_PPCCommand != AHIAC_COM_ACK );
  audioctrl->ahiac_PPCCommand = AHIAC_COM_FINISHED;

  return 0;
}

asm( "
/*     r3 = beginning address of data block to flush
 *     r4 = size of data block to flush (in bytes)
 *     assumes cache block granule is 32 bytes
 */

        .globl  FlushCache
      	.type   FlushCache,@function

FlushCache:
        addi    4, 4, 31
        srwi    4, 4, 5       /* convert to cache blocks to flush */
        mtctr   4
        li      4, 0
loop:
        dcbf    3, 4          /* flush data cache block to mem */
        addi    4, 4, 32
        bdnz    loop

        sync                    /* force mem transactions to complete */
        blr                     /* return to calling routine */
");

#endif /* defined( VERSIONPOWERUP ) */
