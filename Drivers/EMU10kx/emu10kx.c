/*
     emu10kx.audio - AHI driver for SoundBlaster Live! series
     Copyright (C) 2002 Martin Blom <martin@blom.org>
     
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

/******************************************************************************
** WARNING  *******************************************************************
*******************************************************************************

Note that you CANNOT base proprietary drivers on this particular
driver! Anything that is based on this driver is GPL:ed.

*******************************************************************************
** WARNING  *******************************************************************
******************************************************************************/


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

#include "DriverData.h"
#include "version.h"
#include "8010.h"

struct Driver
{
    struct Library library;
    UWORD          pad;
    BPTR           seglist;
};

#define dd ((struct DriverData*) AudioCtrl->ahiac_DriverData)

const char  LibName[]     = "emu10kx.audio";
const char  LibIDString[] = "emu10kx.audio " VERS "\r\n";
const UWORD LibVersion    = VERSION;
const UWORD LibRevision   = REVISION;

struct ExecBase* SysBase    = NULL;
struct Library*  AHIsubBase = NULL;
struct Library*  ppcibase   = NULL;


/******************************************************************************
** Function prototypes ********************************************************
******************************************************************************/

struct Driver* ASMCALL
initRoutine( REG( a0, struct Driver*   driver ),
             REG( d0, BPTR             seglist ),
             REG( a6, struct ExecBase* sysbase ) );


ULONG ASMCALL SAVEDS
intAHIsub_AllocAudio( REG( a1, struct TagItem* tagList ),
		      REG( a2, struct AHIAudioCtrlDrv* AudioCtrl ) );

void ASMCALL SAVEDS
intAHIsub_FreeAudio( REG( a2, struct AHIAudioCtrlDrv* AudioCtrl ) );

void ASMCALL SAVEDS
asmAHIsub_Disable( REG( a2, struct AHIAudioCtrlDrv* AudioCtrl ) );

void ASMCALL SAVEDS
intAHIsub_Disable( REG( a2, struct AHIAudioCtrlDrv* AudioCtrl ) );

void ASMCALL SAVEDS
asmAHIsub_Enable( REG( a2, struct AHIAudioCtrlDrv* AudioCtrl ) );

void ASMCALL SAVEDS
intAHIsub_Enable( REG( a2, struct AHIAudioCtrlDrv* AudioCtrl ) );

ULONG ASMCALL SAVEDS
intAHIsub_Start( REG( d0, ULONG Flags ),
                 REG( a2, struct AHIAudioCtrlDrv* AudioCtrl ) );

void ASMCALL SAVEDS
intAHIsub_Update( REG( d0, ULONG Flags ),
                  REG( a2, struct AHIAudioCtrlDrv* AudioCtrl ) );

void ASMCALL SAVEDS
intAHIsub_Stop( REG( d0, ULONG Flags ),
                REG( a2, struct AHIAudioCtrlDrv* AudioCtrl ) );

LONG ASMCALL SAVEDS
intAHIsub_GetAttr( REG( d0, ULONG Attribute ),
                   REG( d1, LONG Argument ),
                   REG( d2, LONG Default ),
                   REG( a1, struct TagItem* tagList ),
                   REG( a2, struct AHIAudioCtrlDrv* AudioCtrl ) );

ULONG ASMCALL SAVEDS
intAHIsub_HardwareControl( REG( d0, ULONG attribute ),
                           REG( d1, LONG argument ),
                           REG( a2, struct AHIAudioCtrlDrv* AudioCtrl ) );

ULONG ASMCALL SAVEDS
intAHIsub_SetVol( REG( d0, UWORD channel ),
                  REG( d1, Fixed volume ),
                  REG( d2, sposition pan ),
                  REG( a2, struct AHIAudioCtrlDrv* AudioCtrl ),
                  REG( d3, ULONG Flags ) );

ULONG ASMCALL SAVEDS
intAHIsub_SetFreq( REG( d0, UWORD channel ),
                   REG( d1, ULONG freq ),
                   REG( a2, struct AHIAudioCtrlDrv* AudioCtrl ),
                   REG( d2, ULONG flags ) );

ULONG ASMCALL SAVEDS
intAHIsub_SetSound( REG( d0, UWORD channel ),
                    REG( d1, UWORD sound ),
                    REG( d2, ULONG offset ),
                    REG( d3, LONG length ),
                    REG( a2, struct AHIAudioCtrlDrv* AudioCtrl ),
                    REG( d4, ULONG flags ) );

ULONG ASMCALL SAVEDS
intAHIsub_SetEffect( REG( a0, APTR effect ),
                     REG( a2, struct AHIAudioCtrlDrv* AudioCtrl ) );

ULONG ASMCALL SAVEDS
intAHIsub_LoadSound( REG( d0, UWORD sound ),
                     REG( d1, ULONG type ),
                     REG( a0, APTR info ),
                     REG( a2, struct AHIAudioCtrlDrv* AudioCtrl ) );

ULONG ASMCALL SAVEDS
intAHIsub_UnloadSound( REG( d0, UWORD sound ),
                       REG( a2, struct AHIAudioCtrlDrv* AudioCtrl ) );

static ULONG ASMCALL INTERRUPT 
EMU10kx_interrupt( REG( a1, struct AHIAudioCtrlDrv* AudioCtrl ) );

/******************************************************************************
** Driver entry ***************************************************************
******************************************************************************/

int
_start( void )
{
  return -1;
}


/******************************************************************************
** Globals ********************************************************************
******************************************************************************/

#define FREQUENCIES 8

static const ULONG frequency[ FREQUENCIES ] =
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

#define INPUTS 6

static const STRPTR inputs[ INPUTS ] =
{
  "Line in",
  "Mic",
  "CD",
  "Phone",
  "Video",
  "Mixer"
};


#define OUTPUTS 2

static const STRPTR outputs[ OUTPUTS ] =
{
  "Front",
  "Back"
};


/******************************************************************************
** Driver resident structure **************************************************
******************************************************************************/

static const APTR funcTable[] =
{
  intAHIsub_AllocAudio,
  intAHIsub_FreeAudio,
  asmAHIsub_Disable,
  asmAHIsub_Enable,
  intAHIsub_Start,
  intAHIsub_Update,
  intAHIsub_Stop,
  intAHIsub_SetVol,
  intAHIsub_SetFreq,
  intAHIsub_SetSound,
  intAHIsub_SetEffect,
  intAHIsub_LoadSound,
  intAHIsub_UnloadSound,
  intAHIsub_GetAttr,
  intAHIsub_HardwareControl,
  (APTR) -1
};


static const APTR InitTable[4] =
{
  (APTR) sizeof( struct Driver ),
  (APTR) &funcTable,
  NULL,
  (APTR) initRoutine
};


// This structure must reside in the text segment or the read-only data segment!
// "const" makes it happen.
static const struct Resident RomTag =
{
  RTC_MATCHWORD,
  (struct Resident *) &RomTag,
  (struct Resident *) &RomTag + 1,
  RTF_AUTOINIT,
  VERSION,
  NT_LIBRARY,
  0,                      /* priority */
  (BYTE *) LibName,
  (BYTE *) LibIDString,
  (APTR) &InitTable
};


/******************************************************************************
** Library init ***************************************************************
******************************************************************************/

struct Driver* ASMCALL
initRoutine( REG( a0, struct Driver*   driver ),
             REG( d0, BPTR             seglist ),
             REG( a6, struct ExecBase* sysbase ) )
{
  SysBase    = sysbase;
  AHIsubBase = (struct Library*) driver;

  driver->library.lib_Node.ln_Type = NT_LIBRARY;
  driver->library.lib_Node.ln_Name = (STRPTR) LibName;
  driver->library.lib_Flags        = LIBF_SUMUSED | LIBF_CHANGED;
  driver->library.lib_Version      = VERSION;
  driver->library.lib_Revision     = REVISION;
  driver->library.lib_IdString     = (STRPTR) LibIDString;
  driver->seglist                  = seglist;


  ppcibase = OpenLibrary( "powerpci.library", 1 );

  if( ppcibase == NULL )
  {
    // kprintf( "Unable to open 'powerpci.library' version 1.\n" );
    goto error;
  }

  // Fail if no hardware (prevents the audio modes form being added to
  // the database if the driver cannot be used).

  if( pci_find_device( PCI_VENDOR_ID_CREATIVE,
		       PCI_DEVICE_ID_CREATIVE_EMU10K1,
		       NULL ) == NULL )
  {
    // kprintf( "No SoundBlaster Live! card present.\n" );
    goto error;
  }
  
  return driver;

error:
  FreeMem( (APTR) ( ( (char*) driver ) - driver->library.lib_NegSize ),
             driver->library.lib_NegSize + driver->library.lib_PosSize );
  return NULL;
}


/******************************************************************************
** Library clean-up ***********************************************************
******************************************************************************/

BPTR ASMCALL
LibExpunge( REG( a6, struct Driver* driver ) )
{
  BPTR seglist = 0;

  if( driver->library.lib_OpenCnt == 0 )
  {
    seglist = driver->seglist;

    Remove( (struct Node *) driver );

    /* Close libraries */
    CloseLibrary( ppcibase );

    FreeMem( (APTR) ( ( (char*) driver ) - driver->library.lib_NegSize ),
             driver->library.lib_NegSize + driver->library.lib_PosSize );
  }
  else
  {
    driver->library.lib_Flags |= LIBF_DELEXP;
  }

  return seglist;
}

/******************************************************************************
** Library opening ************************************************************
******************************************************************************/

struct Driver* ASMCALL
LibOpen( REG( a6, struct Driver* driver ) )
{
  driver->library.lib_Flags &= ~LIBF_DELEXP;
  ++driver->library.lib_OpenCnt;

  return driver;
}


/******************************************************************************
** Library closing ************************************************************
******************************************************************************/

BPTR ASMCALL
LibClose( REG( a6, struct Driver* driver ) )
{
  BPTR seglist = 0;

  --driver->library.lib_OpenCnt;

  if( driver->library.lib_OpenCnt == 0 )
  {
    if( driver->library.lib_Flags & LIBF_DELEXP )
    {
      seglist = LibExpunge( driver );
    }
  }

  return seglist;
}


/******************************************************************************
** AHIsub_AllocAudio **********************************************************
******************************************************************************/

ULONG ASMCALL SAVEDS
intAHIsub_AllocAudio( REG( a1, struct TagItem* tagList ),
		      REG( a2, struct AHIAudioCtrlDrv* AudioCtrl ) )
{
  int              card_num;
  UWORD            command_word;
  int              ret;

  card_num = 0;
  
  dd = AllocVec( sizeof( *dd ), MEMF_PUBLIC | MEMF_CLEAR );

  if( dd == NULL )
  {
    // kprintf( "Unable to allocate driver structure.\n " );
  }
  else
  {
    dd->interrupt.is_Node.ln_Type = NT_INTERRUPT;
    dd->interrupt.is_Node.ln_Pri  = 0;
    dd->interrupt.is_Node.ln_Name = (STRPTR) LibName;
    dd->interrupt.is_Code         = (void(*)(void)) EMU10kx_interrupt;
    dd->interrupt.is_Data         = (APTR) AudioCtrl;
    
    dd->card.pci_dev = 0;

    do
    {
      dd->card.pci_dev = pci_find_device( PCI_VENDOR_ID_CREATIVE,
						   PCI_DEVICE_ID_CREATIVE_EMU10K1,
						   dd->card.pci_dev );
    } while( dd->card.pci_dev != 0 && card_num-- != 0 );

    if( dd->card.pci_dev == NULL )
    {
      // kprintf( "Unable to find EMU10k subsystem.\n" );
      return AHISF_ERROR;
    }

//  if( pci_set_dma_mask(dd->card.pci_dev, EMU10K1_DMA_MASK) )
//  {
//    printf( "Unable to set DMA mask for card\n." );
//    goto error;
//  }

    if( pci_request( dd->card.pci_dev, (STRPTR) LibName, NULL ) )
    {
      // kprintf( "Unable to claim I/O resources.\n" );
      return AHISF_ERROR;
    }

    if( pci_enable( dd->card.pci_dev ) )
    {
      // kprintf( "Unable to enable card.\n" );
      return AHISF_ERROR;
    }

    command_word = pci_read_conf_word( dd->card.pci_dev, PCI_COMMAND );
    command_word |= PCI_CMD_IO_MASK | PCI_CMD_MEMORY_MASK | PCI_CMD_MASTER_MASK;
    pci_write_conf_word( dd->card.pci_dev, PCI_COMMAND, command_word );

    // FIXME: How about latency/pcibios_set_master()??
    
    dd->card.iobase  = (ULONG) pci_get_base_start( dd->card.pci_dev, 0 );
    dd->card.length  = ( (ULONG) pci_get_base_end( dd->card.pci_dev, 0 ) -
			 (ULONG) dd->card.iobase ) + 1;

    dd->card.irq     = pci_ask_irq( dd->card.pci_dev ) >> 8;
    
    dd->card.chiprev = pci_read_conf_byte( dd->card.pci_dev, PCI_REVISION_ID );
    dd->card.model   = pci_read_conf_word( dd->card.pci_dev, PCI_SUBSYSTEM_ID );
    dd->card.isaps   = ( pci_read_conf_long( dd->card.pci_dev,
					     PCI_SUBSYSTEM_VENDOR_ID )
			 == EMU_APS_SUBID );

    pci_add_irq( dd->card.pci_dev, &dd->interrupt );
    dd->interrupt_added = TRUE;
  }

  return AHISF_KNOWHIFI | AHISF_KNOWSTEREO | AHISF_MIXING | AHISF_TIMING;
}



/******************************************************************************
** AHIsub_FreeAudio ***********************************************************
******************************************************************************/

void ASMCALL SAVEDS
intAHIsub_FreeAudio( REG( a2, struct AHIAudioCtrlDrv* AudioCtrl ) )
{
  if( dd != NULL )
  {
    if( dd->card.pci_dev != NULL )
    {
//      emu10k1_cleanup( &dd->card );
//      emu10k1_midi_cleanup(dd->card);
//      emu10k1_mixer_cleanup(dd->card);
//      emu10k1_audio_cleanup(dd->card);
      
      pci_disable( dd->card.pci_dev );
      pci_release( dd->card.pci_dev );

      if( dd->interrupt_added )
      {
	pci_rem_irq( dd->card.pci_dev, &dd->interrupt );
      }
    }

    FreeVec( dd );
    dd = NULL;
  }
}


/******************************************************************************
** AHIsub_Disable *************************************************************
******************************************************************************/

__asm__("
_asmAHIsub_Disable:
	moveml	d0-d1/a0-a1,-(sp)
	bsr	_intAHIsub_Disable
	moveml	(sp)+,d0-d1/a0-a1
	rts
");


void ASMCALL SAVEDS
intAHIsub_Disable( REG( a2, struct AHIAudioCtrlDrv* AudioCtrl ) )
{
  Disable();
}  


/******************************************************************************
** AHIsub_Enable **************************************************************
******************************************************************************/

__asm__("
_asmAHIsub_Enable:
	moveml	d0-d1/a0-a1,-(sp)
	bsr	_intAHIsub_Enable
	moveml	(sp)+,d0-d1/a0-a1
	rts
");


void ASMCALL SAVEDS
intAHIsub_Enable( REG( a2, struct AHIAudioCtrlDrv* AudioCtrl ) )
{
  Enable();
}


/******************************************************************************
** AHIsub_Start ***************************************************************
******************************************************************************/

ULONG ASMCALL SAVEDS
intAHIsub_Start( REG( d0, ULONG Flags ),
                 REG( a2, struct AHIAudioCtrlDrv* AudioCtrl ) )
{
  AHIsub_Stop( Flags, AudioCtrl );

  if(Flags & AHISF_PLAY)
  {
    return AHIE_UNKNOWN;
  }

  if( Flags & AHISF_RECORD )
  {
    return AHIE_UNKNOWN;
  }

  return AHIE_OK;
}


/******************************************************************************
** AHIsub_Update **************************************************************
******************************************************************************/

void ASMCALL SAVEDS
intAHIsub_Update( REG( d0, ULONG Flags ),
                  REG( a2, struct AHIAudioCtrlDrv* AudioCtrl ) )
{
  // Empty function
}


/******************************************************************************
** AHIsub_Stop ****************************************************************
******************************************************************************/

void ASMCALL SAVEDS
intAHIsub_Stop( REG( d0, ULONG Flags ),
                REG( a2, struct AHIAudioCtrlDrv* AudioCtrl ) )
{
  if( Flags & AHISF_PLAY )
  {
  }

  if(Flags & AHISF_RECORD)
  {
    // Do nothing
  }
}


/******************************************************************************
** AHIsub_GetAttr *************************************************************
******************************************************************************/

LONG ASMCALL SAVEDS
intAHIsub_GetAttr( REG( d0, ULONG Attribute ),
                   REG( d1, LONG Argument ),
                   REG( d2, LONG Default ),
                   REG( a1, struct TagItem* tagList ),
                   REG( a2, struct AHIAudioCtrlDrv* AudioCtrl ) )
{
  int i;

  switch( Attribute )
  {
    case AHIDB_Bits:
      return 16;

    case AHIDB_Frequencies:
      return FREQUENCIES;

    case AHIDB_Frequency: // Index->Frequency
      return (LONG) frequency[ Argument ];

    case AHIDB_Index: // Frequency->Index
      if( Argument <= (LONG) frequency[ 0 ] )
      {
        return 0;
      }

      if( Argument >= (LONG) frequency[ FREQUENCIES - 1 ] )
      {
        return FREQUENCIES-1;
      }

      for( i = 1; i < FREQUENCIES; i++ )
      {
        if( (LONG) frequency[ i ] > Argument )
        {
          if( ( Argument - (LONG) frequency[ i - 1 ] )
	      < ( (LONG) frequency[ i ] - Argument ) )
          {
            return i-1;
          }
          else
          {
            return i;
          }
        }
      }

      return 0;  // Will not happen

    case AHIDB_Author:
      return (LONG) "Martin 'Leviticus' Blom/Bertrand Lee et al.";

    case AHIDB_Copyright:
      return (LONG) "GNU GPL";

    case AHIDB_Version:
      return (LONG) LibIDString;

    case AHIDB_Annotation:
      return (LONG)
	"Based on the Linux kernel driver\n"
	"Funded by Hyperion Entertainment.";

    case AHIDB_Record:
      return FALSE;

    case AHIDB_FullDuplex:
      return TRUE;

    case AHIDB_Realtime:
      return TRUE;

    case AHIDB_MaxRecordSamples:
      return RECORD_BUFFER_SAMPLES;

    case AHIDB_MinMonitorVolume:
      return 0x00000;

    case AHIDB_MaxMonitorVolume:
      return 0x00000;

    case AHIDB_MinInputGain:
      return 0x00000;
      
    case AHIDB_MaxInputGain:
      return 0x00000;
      
    case AHIDB_MinOutputVolume:
      return 0x00000;

    case AHIDB_MaxOutputVolume:
      return 0x10000;

    case AHIDB_Inputs:
      return INPUTS;

    case AHIDB_Input:
      return (LONG) inputs[ Argument ];

    case AHIDB_Outputs:
      return OUTPUTS;

    case AHIDB_Output:
      return (LONG) outputs[ Argument ];

    default:
      return Default;
  }
}


/******************************************************************************
** AHIsub_HardwareControl *****************************************************
******************************************************************************/

ULONG ASMCALL SAVEDS
intAHIsub_HardwareControl( REG( d0, ULONG attribute ),
                           REG( d1, LONG argument ),
                           REG( a2, struct AHIAudioCtrlDrv* AudioCtrl ) )
{
  return NULL;
}


/******************************************************************************
** AHIsub_SetVol **************************************************************
******************************************************************************/

ULONG ASMCALL SAVEDS
intAHIsub_SetVol( REG( d0, UWORD channel ),
                  REG( d1, Fixed volume ),
                  REG( d2, sposition pan ),
                  REG( a2, struct AHIAudioCtrlDrv* AudioCtrl ),
                  REG( d3, ULONG Flags ) )
{
  return AHIS_UNKNOWN;
}


/******************************************************************************
** AHIsub_SetFreq *************************************************************
******************************************************************************/

ULONG ASMCALL SAVEDS
intAHIsub_SetFreq( REG( d0, UWORD channel ),
                   REG( d1, ULONG freq ),
                   REG( a2, struct AHIAudioCtrlDrv* AudioCtrl ),
                   REG( d2, ULONG flags ) )
{
  return AHIS_UNKNOWN;
}


/******************************************************************************
** AHIsub_SetSound ************************************************************
******************************************************************************/

ULONG ASMCALL SAVEDS
intAHIsub_SetSound( REG( d0, UWORD channel ),
                    REG( d1, UWORD sound ),
                    REG( d2, ULONG offset ),
                    REG( d3, LONG length ),
                    REG( a2, struct AHIAudioCtrlDrv* AudioCtrl ),
                    REG( d4, ULONG flags ) )
{
  return AHIS_UNKNOWN;
}


/******************************************************************************
** AHIsub_SetEffect ***********************************************************
******************************************************************************/

ULONG ASMCALL SAVEDS
intAHIsub_SetEffect( REG( a0, APTR effect ),
                     REG( a2, struct AHIAudioCtrlDrv* AudioCtrl ) )
{
  return AHIS_UNKNOWN;
}


/******************************************************************************
** AHIsub_LoadSound ***********************************************************
******************************************************************************/

ULONG ASMCALL SAVEDS
intAHIsub_LoadSound( REG( d0, UWORD sound ),
                     REG( d1, ULONG type ),
                     REG( a0, APTR info ),
                     REG( a2, struct AHIAudioCtrlDrv* AudioCtrl ) )
{ 
  return AHIS_UNKNOWN;
}


/******************************************************************************
** AHIsub_UnloadSound *********************************************************
******************************************************************************/

ULONG ASMCALL SAVEDS
intAHIsub_UnloadSound( REG( d0, UWORD sound ),
                       REG( a2, struct AHIAudioCtrlDrv* AudioCtrl ) )
{
  return AHIS_UNKNOWN;
}


/******************************************************************************
** Interrupt handler **********************************************************
******************************************************************************/

static ULONG ASMCALL INTERRUPT 
EMU10kx_interrupt( REG( a1, struct AHIAudioCtrlDrv* AudioCtrl ) )
{
  ULONG intreq;
  BOOL  handled = FALSE;
  
  while( (intreq = *(ULONG*) (dd->card.iobase + IPR) ) != 0 )
  {
    if( intreq & IPR_INTERVALTIMER )
    {
      // ...
    }
    
    if( intreq & IPR_FXDSP )
    {
      // ...
    }

    // Clear interrupt pending bit(s)
    *(ULONG*) (dd->card.iobase + IPR) = intreq;
    
    handled = TRUE;
  }

  return handled;
}



