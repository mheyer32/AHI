
#if defined( VERSIONPOWERUP )

#include <config.h>
#include <CompilerSpecific.h>

#include <exec/types.h>
#include <hardware/intbits.h>

#include "ahi_def.h"
#include "mixer.h"

int
entry( struct Hook *Hook, 
       void *dst, 
       volatile struct AHIPrivAudioCtrl *audioctrl )
{
//  audioctrl->ahiac_Com = AHIAC_COM_INIT;
//  *((WORD*) 0xdff09C)  = INTF_SETCLR | INTF_PORTS;

//  while( audioctrl->ahiac_Com != AHIAC_COM_ACK );

  //MixGeneric( Hook, dst, audioctrl );

  // TODO: Clear cache here!

//  audioctrl->ahiac_Com = AHIAC_COM_QUIT;
//  while( audioctrl->ahiac_Com != AHIAC_COM_ACK );

  return -1974;
}

#endif /* defined( VERSIONPOWERUP ) */
