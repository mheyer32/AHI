/*
     emu10kx.audio - AHI driver for SoundBlaster Live! series
     Copyright (C) 2002-2003 Martin Blom <martin@blom.org>

     This program is free software; you can redistribute it and/or
     modify it under the terms of the GNU General Public License
     as published by the Free Software Foundation; either version 2
     of the License, or (at your option) any later version.

     This program is distributed in the hope that it will be useful,
     but WITHOUT ANY WARRANTY; without even the implied warranty of
     MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
     GNU General Public License for more details.

     You should have received a copy of the GNU General Public License
     along with this program; if not, write to the Free Software
     Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.
*/

#include <config.h>

#include <exec/memory.h>
#include <libraries/openpci.h>
#include <proto/exec.h>
#include <proto/openpci.h>

#include "library.h"
#include "emu10kx-misc.h"


/* We use global library bases instead of storing them in DriverBase, since
   I don't want to modify the original sources more than necessary. */

struct ExecBase*   SysBase;
struct DosLibrary* DOSBase;
struct Library*    OpenPciBase;
struct DriverBase* AHIsubBase;


/******************************************************************************
** Custom driver init *********************************************************
******************************************************************************/

BOOL
DriverInit( struct DriverBase* ahisubbase )
{
  struct EMU10kxBase* EMU10kxBase = (struct EMU10kxBase*) ahisubbase;
  struct pci_dev*     dev;
  int                 card_no;

  AHIsubBase = ahisubbase;
  
  DOSBase  = (struct DosLibrary*) OpenLibrary( DOSNAME, 37 );
  OpenPciBase = OpenLibrary( "openpci.library", 1 );

  if( DOSBase == NULL )
  {
    Req( "Unable to open 'dos.library' version 37.\n" );
    return FALSE;
  }

  if( OpenPciBase == NULL )
  {
    Req( "Unable to open 'openpci.library' version 1.\n" );
    return FALSE;
  }

  InitSemaphore( &EMU10kxBase->semaphore );

  EMU10kxBase->cards_found = 0;
  dev = NULL;

  while( ( dev = pci_find_device( PCI_VENDOR_ID_CREATIVE,
				  PCI_DEVICE_ID_CREATIVE_EMU10K1,
				  dev ) ) != NULL )
  {
    ++EMU10kxBase->cards_found;
  }
  
  // Fail if no hardware (prevents the audio modes form being added to
  // the database if the driver cannot be used).

  if( EMU10kxBase->cards_found == 0 )
  {
    Req( "No SoundBlaster Live! card present.\n" );
    return FALSE;
  }

  EMU10kxBase->driverdatas = AllocVec( sizeof( *EMU10kxBase->driverdatas ) *
				       EMU10kxBase->cards_found,
				       MEMF_PUBLIC );

  if( EMU10kxBase->driverdatas == NULL )
  {
    Req( "Out of memory." );
    return FALSE;
  }

  
  card_no = 0;

  while( ( dev = pci_find_device( PCI_VENDOR_ID_CREATIVE,
				  PCI_DEVICE_ID_CREATIVE_EMU10K1,
				  dev ) ) != NULL )
  {
    EMU10kxBase->driverdatas[ card_no ] = AllocDriverData( dev, AHIsubBase );
    ++card_no;
  }

  return TRUE;
}


/******************************************************************************
** Custom driver clean-up *****************************************************
******************************************************************************/

VOID
DriverCleanup( struct DriverBase* AHIsubBase )
{
  struct EMU10kxBase* EMU10kxBase = (struct EMU10kxBase*) AHIsubBase;
  int i;

  for( i = 0; i < EMU10kxBase->cards_found; ++i )
  {
    FreeDriverData( EMU10kxBase->driverdatas[ i ], AHIsubBase );
  }
  
  FreeVec( EMU10kxBase->driverdatas );
  
  CloseLibrary( OpenPciBase );
  CloseLibrary( (struct Library*) DOSBase );
}
