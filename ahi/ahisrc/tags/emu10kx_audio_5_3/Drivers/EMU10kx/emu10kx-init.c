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

#include <proto/exec.h>
#include <proto/openpci.h>

#include "library.h"

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

  // Fail if no hardware (prevents the audio modes form being added to
  // the database if the driver cannot be used).

  if( pci_find_device( PCI_VENDOR_ID_CREATIVE,
		       PCI_DEVICE_ID_CREATIVE_EMU10K1,
		       NULL ) == NULL )
  {
    Req( "No SoundBlaster Live! card present.\n" );
    return FALSE;
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

  CloseLibrary( OpenPciBase );
  CloseLibrary( (struct Library*) DOSBase );
}
