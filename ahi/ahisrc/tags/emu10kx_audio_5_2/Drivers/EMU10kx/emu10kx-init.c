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
#include <proto/powerpci.h>

#include "library.h"

/* Global and not in EMU10kxBase, since it's used by linuxsupport.c */
struct Library* ppcibase;
struct Library* DOSBase;

/* Global and not in EMU10kxBase, since it's used everywhere ... */
struct ExecBase* SysBase;

/******************************************************************************
** Custom driver init *********************************************************
******************************************************************************/

BOOL
DriverInit( struct DriverBase* AHIsubBase )
{
  struct EMU10kxBase* EMU10kxBase = (struct EMU10kxBase*) AHIsubBase;

  ppcibase = OpenLibrary( "powerpci.library", 1 );
  DOSBase  = (struct DosLibrary*) OpenLibrary( DOSNAME, 37 );

  if( ppcibase == NULL )
  {
    Req( "Unable to open 'powerpci.library' version 1.\n" );
    return FALSE;
  }

  if( DOSBase == NULL )
  {
    Req( "Unable to open 'dos.library' version 37.\n" );
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

  CloseLibrary( ppcibase );
  CloseLibrary( (struct Library*) DOSBase );
}
