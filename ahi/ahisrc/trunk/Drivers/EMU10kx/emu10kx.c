
#include <config.h>
#include <CompilerSpecific.h>

#define AMITHLON_AMITHLONSPEC_H // Go away!

#include <amithlon/powerpci.h>
#include <exec/exec.h>
#include <devices/ahi.h>
#include <libraries/ahi_sub.h>
#include <pci/powerpci_pci.h>

#include <proto/ahi_sub.h>
#include <proto/dos.h>
#include <proto/exec.h>
#include <proto/powerpci.h>
#include <proto/utility.h>

#include "EMU10kx.h"
#include "version.h"

#define dd ((struct EMU10kx*) AudioCtrl->ahiac_DriverData)


const char  LibName[]     = "emu10kx.audio";
const char  LibIdString[] = "emu10kx.audio " VERS "\r\n";
const UWORD LibVersion    = VERSION;
const UWORD LibRevision   = REVISION;

struct ExecBase* SysBase    = NULL;
struct Library*  AHIsubBase = NULL;
struct Library*  ppcibase   = NULL;


/******************************************************************************
** Globals ********************************************************************
******************************************************************************/

#define FREQUENCIES 8

static const ULONG frequency[FREQUENCIES] =
{
  8000,     // µ- and A-Law
  11025,    // CD/4
  16000,    // DAT/3
  22050,    // CD/2
  24000,    // DAT/2
  32000,    // DAT/1.5
  44100,    // CD
  48000     // DAT
};


/******************************************************************************
** Library init ***************************************************************
******************************************************************************/

int
__UserLibInit( struct Library* libbase )
{
  SysBase = *(struct ExecBase**) 4;

  ppcibase = OpenLibrary( "powerpci.library", 1 );

  if( ppcibase == NULL )
  {
    // kprintf( "Unable to open 'powerpci.library' version 1.\n" );
    return 1;
  }

  // Fail if no hardware (prevents the audio modes form being added to
  // the database if the driver cannot be used).

  if( pci_find_device( PCI_VENDOR_ID_CREATIVE,
		       PCI_DEVICE_ID_CREATIVE_EMU10K1,
		       NULL ) == NULL )
  {
    // kprintf( "No SoundBlaster Live! card present.\n" );
    return 1;
  }
  
  return 0;
  
}


/******************************************************************************
** Library clean-up ***********************************************************
******************************************************************************/

void
__UserLibCleanup( void )
{
  CloseLibrary( ppcibase );

  ppcibase   = NULL;
  AHIsubBase = NULL;
}
