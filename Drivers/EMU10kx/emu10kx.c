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
driver! Anything that is based on this driver has to be GPL:ed.

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

#include <clib/alib_protos.h>
#include <proto/ahi_sub.h>
#include <proto/dos.h>
#include <proto/exec.h>
#include <proto/powerpci.h>
#include <proto/utility.h>

#include "DriverData.h"
#include "version.h"
#include "8010.h"

/* Public functions in main.c */
int emu10k1_init(struct emu10k1_card *card);
void emu10k1_cleanup(struct emu10k1_card *card);


typedef BOOL ASMCALL
PreTimer_proto( REG( a2, struct AHIAudioCtrlDrv* actrl ) );


typedef void ASMCALL
PostTimer_proto( REG( a2, struct AHIAudioCtrlDrv* actrl ) );


struct Driver
{
    struct Library library;
    UWORD          pad;
    BPTR           seglist;
};


/******************************************************************************
** Driver entry ***************************************************************
******************************************************************************/

int
_start( void )
{
  return -1;
}

ULONG
Null( void )
{
  return 0;
}


/******************************************************************************
** Function prototypes ********************************************************
******************************************************************************/

struct Driver* ASMCALL
LibInit( REG( d0, struct Driver*   driver ),
	 REG( a0, BPTR             seglist ),
	 REG( a6, struct ExecBase* sysbase ) );

BPTR ASMCALL
LibExpunge( REG( a6, struct Driver* driver ) );

struct Driver* ASMCALL
LibOpen( REG( a6, struct Driver* driver ) );

BPTR ASMCALL
LibClose( REG( a6, struct Driver* driver ) );

ULONG ASMCALL
LibAllocAudio( REG( a1, struct TagItem* taglist ),
	       REG( a2, struct AHIAudioCtrlDrv* audioctrl ) );

void ASMCALL
LibFreeAudio( REG( a2, struct AHIAudioCtrlDrv* audioctrl ) );

void ASMCALL
AsmDisable( REG( a2, struct AHIAudioCtrlDrv* audioctrl ) );

void ASMCALL
LibDisable( REG( a2, struct AHIAudioCtrlDrv* audioctrl ) );

void ASMCALL
AsmEnable( REG( a2, struct AHIAudioCtrlDrv* audioctrl ) );

void ASMCALL
LibEnable( REG( a2, struct AHIAudioCtrlDrv* audioctrl ) );

ULONG ASMCALL
LibStart( REG( d0, ULONG flags ),
	  REG( a2, struct AHIAudioCtrlDrv* audioctrl ) );

void ASMCALL
LibUpdate( REG( d0, ULONG flags ),
	   REG( a2, struct AHIAudioCtrlDrv* audioctrl ) );

void ASMCALL
LibStop( REG( d0, ULONG flags ),
	 REG( a2, struct AHIAudioCtrlDrv* audioctrl ) );

LONG ASMCALL
LibGetAttr( REG( d0, ULONG attribute ),
	    REG( d1, LONG argument ),
	    REG( d2, LONG def ),
	    REG( a1, struct TagItem* taglist ),
	    REG( a2, struct AHIAudioCtrlDrv* audioctrl ) );

ULONG ASMCALL
LibHardwareControl( REG( d0, ULONG attribute ),
		    REG( d1, LONG argument ),
		    REG( a2, struct AHIAudioCtrlDrv* audioctrl ) );

ULONG ASMCALL
LibSetVol( REG( d0, UWORD channel ),
	   REG( d1, Fixed volume ),
	   REG( d2, sposition pan ),
	   REG( a2, struct AHIAudioCtrlDrv* audioctrl ),
	   REG( d3, ULONG flags ) );

ULONG ASMCALL
LibSetFreq( REG( d0, UWORD channel ),
	    REG( d1, ULONG freq ),
	    REG( a2, struct AHIAudioCtrlDrv* audioctrl ),
	    REG( d2, ULONG flags ) );

ULONG ASMCALL
LibSetSound( REG( d0, UWORD channel ),
	     REG( d1, UWORD sound ),
	     REG( d2, ULONG offset ),
	     REG( d3, LONG length ),
	     REG( a2, struct AHIAudioCtrlDrv* audioctrl ),
	     REG( d4, ULONG flags ) );

ULONG ASMCALL
LibSetEffect( REG( a0, APTR effect ),
	      REG( a2, struct AHIAudioCtrlDrv* audioctrl ) );

ULONG ASMCALL
LibLoadSound( REG( d0, UWORD sound ),
	      REG( d1, ULONG type ),
	      REG( a0, APTR info ),
	      REG( a2, struct AHIAudioCtrlDrv* audioctrl ) );

ULONG ASMCALL
LibUnloadSound( REG( d0, UWORD sound ),
		REG( a2, struct AHIAudioCtrlDrv* audioctrl ) );



static ULONG ASMCALL INTERRUPT
EMU10kxInterrupt( REG( a1, struct AHIAudioCtrlDrv* audioctrl ) );

static void ASMCALL INTERRUPT
PlaybackInterrupt( REG( a1, struct AHIAudioCtrlDrv* audioctrl ) );

static void ASMCALL INTERRUPT
RecordInterrupt( REG( a1, struct AHIAudioCtrlDrv* audioctrl ) );



static void
SaveMixerState( struct DriverData* dd );

static void
RestoreMixerState( struct DriverData* dd );

static void
UpdateMonitorMixer( struct DriverData* dd );

static Fixed
Linear2MixerGain( Fixed  linear,
		  UWORD* bits );

static Fixed
Linear2RecordGain( Fixed  linear,
		   UWORD* bits );

static ULONG
SamplerateToLinearPitch( ULONG samplingrate );


/*** Stuff that should really have been in a link library ********************/

static UWORD rawputchar_m68k[] __attribute__ ((aligned (4))) =
{
  0x2C4B,             // MOVEA.L A3,A6
  0x4EAE, 0xFDFC,     // JSR     -$0204(A6)
  0x4E75              // RTS
};

void
KPrintFArgs( UBYTE* fmt,
             ULONG* args )
{
  RawDoFmt( fmt, args, (void(*)(void)) rawputchar_m68k, SysBase );
}

#define KPrintF( fmt, ... )        \
({                                 \
  ULONG _args[] = { __VA_ARGS__ }; \
  KPrintFArgs( (fmt), _args );     \
})


/******************************************************************************
** Globals ********************************************************************
******************************************************************************/

#define DRIVER_NAME "emu10kx.audio"

const char  LibName[]     = DRIVER_NAME;
const char  LibIDString[] = DRIVER_NAME " " VERS "\r\n";
const UWORD LibVersion    = VERSION;
const UWORD LibRevision   = REVISION;


struct ExecBase*     SysBase     = NULL;
struct Library*      AHIsubBase  = NULL;
struct Library*      ppcibase    = NULL;
struct DosLibrary*   DOSBase     = NULL;
struct UtilityBase*  UtilityBase = NULL;


#define FREQUENCIES 8

static const ULONG Frequencies[ FREQUENCIES ] =
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

#define INPUTS 8

static const STRPTR Inputs[ INPUTS ] =
{
  "Mixer",
  "Line in",
  "Mic",
  "CD",
  "Aux",
  "Phone",
  "Video",
  "Mixer (mono)"
};

static const UWORD InputBits[ INPUTS ] =
{
  AC97_RECMUX_STEREO_MIX,
  AC97_RECMUX_LINE,
  AC97_RECMUX_MIC,
  AC97_RECMUX_CD,
  AC97_RECMUX_AUX,
  AC97_RECMUX_PHONE,
  AC97_RECMUX_VIDEO,
  AC97_RECMUX_MONO_MIX
};


#define OUTPUTS 2

static const STRPTR Outputs[ OUTPUTS ] =
{
  "Front",
  "Front & Rear"
};


/******************************************************************************
** Driver resident structure **************************************************
******************************************************************************/

static const APTR FuncTable[] =
{
  LibOpen,
  LibClose,
  LibExpunge,
  Null,

  LibAllocAudio,
  LibFreeAudio,
  AsmDisable,
  AsmEnable,
  LibStart,
  LibUpdate,
  LibStop,
  LibSetVol,
  LibSetFreq,
  LibSetSound,
  LibSetEffect,
  LibLoadSound,
  LibUnloadSound,
  LibGetAttr,
  LibHardwareControl,
  (APTR) -1
};


static const APTR InitTable[4] =
{
  (APTR) sizeof( struct Driver ),
  (APTR) &FuncTable,
  NULL,
  (APTR) LibInit
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
LibInit( REG( d0, struct Driver*   driver ),
	 REG( a0, BPTR             seglist ),
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

  ppcibase    = OpenLibrary( "powerpci.library", 1 );
  DOSBase     = (struct DosLibrary*) OpenLibrary( "dos.library", 37 );
  UtilityBase = (struct UtilityBase*) OpenLibrary( "utility.library", 37 );

  if( ppcibase == NULL )
  {
    KPrintF( DRIVER_NAME ": Unable to open 'powerpci.library' version 1.\n" );
    goto error;
  }

  if( DOSBase == NULL )
  {
    KPrintF( DRIVER_NAME ": Unable to open 'dos.library' version 37.\n" );
    goto error;
  }

  if( UtilityBase == NULL )
  {
    KPrintF( DRIVER_NAME ": Unable to open 'utility.library' version 37.\n" );
    goto error;
  }

  // Fail if no hardware (prevents the audio modes form being added to
  // the database if the driver cannot be used).

  if( pci_find_device( PCI_VENDOR_ID_CREATIVE,
		       PCI_DEVICE_ID_CREATIVE_EMU10K1,
		       NULL ) == NULL )
  {
    KPrintF( DRIVER_NAME ": No SoundBlaster Live! card present.\n" );
    goto error;
  }

  return driver;

error:
  LibExpunge( driver );
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

    /* Since LibInit() calls us on failure, we have to check if we're
       really added to the library list before removing us. */

    if( driver->library.lib_Node.ln_Succ != NULL )
    {
      Remove( (struct Node *) driver );
    }

    /* Close libraries */
    CloseLibrary( ppcibase );
    CloseLibrary( (struct Library*) DOSBase );
    CloseLibrary( (struct Library*) UtilityBase );

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
  driver->library.lib_OpenCnt++;

  return driver;
}


/******************************************************************************
** Library closing ************************************************************
******************************************************************************/

BPTR ASMCALL
LibClose( REG( a6, struct Driver* driver ) )
{
  BPTR seglist = 0;

  driver->library.lib_OpenCnt--;

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

ULONG ASMCALL
LibAllocAudio( REG( a1, struct TagItem* taglist ),
	       REG( a2, struct AHIAudioCtrlDrv* audioctrl ) )
{
  struct DriverData* dd;
  int                card_num;
  UWORD              command_word;
  int                ret;
  int                i;

  card_num = ( GetTagData( AHIDB_AudioID, 0, taglist) & 0x0000f000 ) >> 12;

  // FIXME: This shoule be non-cachable, DMA-able memory
  dd = AllocVec( sizeof( *dd ), MEMF_PUBLIC | MEMF_CLEAR );

  if( dd == NULL )
  {
    KPrintF( DRIVER_NAME ": Unable to allocate driver structure.\n" );
  }
  else
  {
    audioctrl->ahiac_DriverData = dd;

    dd->interrupt.is_Node.ln_Type = NT_INTERRUPT;
    dd->interrupt.is_Node.ln_Pri  = 0;
    dd->interrupt.is_Node.ln_Name = (STRPTR) LibName;
    dd->interrupt.is_Code         = (void(*)(void)) EMU10kxInterrupt;
    dd->interrupt.is_Data         = (APTR) audioctrl;

    dd->playback_interrupt.is_Node.ln_Type = NT_INTERRUPT;
    dd->playback_interrupt.is_Node.ln_Pri  = 0;
    dd->playback_interrupt.is_Node.ln_Name = (STRPTR) LibName;
    dd->playback_interrupt.is_Code         = (void(*)(void)) PlaybackInterrupt;
    dd->playback_interrupt.is_Data         = (APTR) audioctrl;

    dd->record_interrupt.is_Node.ln_Type = NT_INTERRUPT;
    dd->record_interrupt.is_Node.ln_Pri  = 0;
    dd->record_interrupt.is_Node.ln_Name = (STRPTR) LibName;
    dd->record_interrupt.is_Code         = (void(*)(void)) RecordInterrupt;
    dd->record_interrupt.is_Data         = (APTR) audioctrl;

    dd->card.pci_dev = 0;

    do
    {
      dd->card.pci_dev = pci_find_device( PCI_VENDOR_ID_CREATIVE,
					  PCI_DEVICE_ID_CREATIVE_EMU10K1,
					  dd->card.pci_dev );
    } while( dd->card.pci_dev != 0 && card_num-- != 0 );

    if( dd->card.pci_dev == NULL )
    {
      KPrintF( DRIVER_NAME ": Unable to find EMU10k subsystem.\n" );
      return AHISF_ERROR;
    }

//  if( pci_set_dma_mask(dd->card.pci_dev, EMU10K1_DMA_MASK) )
//  {
//    printf( "Unable to set DMA mask for card\n." );
//    goto error;
//  }

    if( pci_request( dd->card.pci_dev, (STRPTR) LibName, NULL ) )
    {
      KPrintF( DRIVER_NAME ": Unable to claim I/O resources.\n" );
      return AHISF_ERROR;
    }

    dd->pci_requested = TRUE;

    if( pci_enable( dd->card.pci_dev ) )
    {
      KPrintF( DRIVER_NAME ": Unable to enable card.\n" );
      return AHISF_ERROR;
    }

    dd->pci_enabled = TRUE;

    command_word = pci_read_conf_word( dd->card.pci_dev, PCI_COMMAND );
    command_word |= PCI_CMD_IO_MASK | PCI_CMD_MEMORY_MASK | PCI_CMD_MASTER_MASK;
    pci_write_conf_word( dd->card.pci_dev, PCI_COMMAND, command_word );

    // FIXME: How about latency/pcibios_set_master()??

    dd->pci_master_enabled = TRUE;

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

//    KPrintF( DRIVER_NAME ": I/O status: Base: 0x%08lx; Length:%08lx\n",
//	     dd->card.iobase, dd->card.length );
//    KPrintF( DRIVER_NAME ": IRQ status: Bus: %ld; Amiga: %ld\n",
//	     dd->card.irq, pci_ask_irq( dd->card.pci_dev ) & 0xff );
//    KPrintF( DRIVER_NAME ": Model: %04lx; Chip: 0x%02lx\n",
//	     dd->card.model, dd->card.chiprev );

    /* Initialize chip */

    if( emu10k1_init( &dd->card ) < 0 )
    {
      KPrintF( DRIVER_NAME ": Unable to initialize EMU10kx subsystem.\n");
      return AHISF_ERROR;
    }

    /* Initialize mixer */

    emu10k1_writeac97( &dd->card, AC97_RESET, 0L);

    Delay( 1 );

    if (emu10k1_readac97( &dd->card, AC97_RESET ) & 0x8000) {
      KPrintF( DRIVER_NAME ": ac97 codec not present.\n");
      return AHISF_ERROR;
    }

    dd->input          = 0;
    dd->output         = 0;
    dd->monitor_volume = Linear2MixerGain( 0, &dd->monitor_volume_bits );
    dd->input_gain     = Linear2RecordGain( 0x10000, &dd->input_gain_bits );
    dd->output_volume  = Linear2MixerGain( 0x10000, &dd->output_volume_bits );

    // No attenuation and natural tone for all outputs
    emu10k1_writeac97( &dd->card, AC97_MASTER_VOL_STEREO, 0x0000 );
    emu10k1_writeac97( &dd->card, AC97_HEADPHONE_VOL,     0x0000 );
    emu10k1_writeac97( &dd->card, AC97_MASTER_VOL_MONO,   0x0000 );
    emu10k1_writeac97( &dd->card, AC97_MASTER_TONE,       0x0f0f );

    emu10k1_writeac97( &dd->card, AC97_RECORD_GAIN,       0x0000 );
    emu10k1_writeac97( &dd->card, AC97_RECORD_SELECT,     InputBits[ 0 ] );

    emu10k1_writeac97( &dd->card, AC97_PCMOUT_VOL,        0x0808 );
    emu10k1_writeac97( &dd->card, AC97_PCBEEP_VOL,        0x0000 );

    emu10k1_writeac97( &dd->card, AC97_LINEIN_VOL,        0x0808 );
    emu10k1_writeac97( &dd->card, AC97_MIC_VOL,           AC97_MUTE | 0x0008 );
    emu10k1_writeac97( &dd->card, AC97_CD_VOL,            0x0808 );
    emu10k1_writeac97( &dd->card, AC97_AUX_VOL,           0x0808 );
    emu10k1_writeac97( &dd->card, AC97_PHONE_VOL,         0x0008 );
    emu10k1_writeac97( &dd->card, AC97_VIDEO_VOL,         0x0808 );


    if (emu10k1_readac97( &dd->card, AC97_EXTENDED_ID ) & 0x0080 )
    {
      sblive_writeptr( &dd->card, AC97SLOT, 0, AC97SLOT_CNTR | AC97SLOT_LFE);
      emu10k1_writeac97( &dd->card, AC97_SURROUND_MASTER, 0x0 );
    }

    dd->emu10k1_initialized = TRUE;

    /* Since the EMU10kx chips can play a voice at any sample rate, we
       do not have to examine/modify audioctrl->ahiac_MixFreq here.

       Had this not been the case, audioctrl->ahiac_MixFreq should be
       set to the frequency we will use.

       However, recording can only be performed at the fixed sampling
       rates.
    */
  }

  ret = AHISF_KNOWHIFI | AHISF_KNOWSTEREO | AHISF_MIXING | AHISF_TIMING;

  for( i = 0; i < FREQUENCIES; ++i )
  {
    if( audioctrl->ahiac_MixFreq == Frequencies[ i ] )
    {
      ret |= AHISF_CANRECORD;
      break;
    }
  }

  return ret;
}



/******************************************************************************
** AHIsub_FreeAudio ***********************************************************
******************************************************************************/

void ASMCALL
LibFreeAudio( REG( a2, struct AHIAudioCtrlDrv* audioctrl ) )
{
  struct DriverData* dd = (struct DriverData*) audioctrl->ahiac_DriverData;

  if( dd != NULL )
  {
    if( dd->card.pci_dev != NULL )
    {
      if( dd->emu10k1_initialized )
      {
	emu10k1_cleanup( &dd->card );
      }

      if( dd->pci_master_enabled )
      {
	UWORD cmd;

	cmd = pci_read_conf_word( dd->card.pci_dev, PCI_COMMAND );
	cmd &= ~( PCI_CMD_IO_MASK | PCI_CMD_MEMORY_MASK | PCI_CMD_MASTER_MASK );
	pci_write_conf_word( dd->card.pci_dev, PCI_COMMAND, cmd );
      }

      if( dd->pci_enabled )
      {
	pci_disable( dd->card.pci_dev );
      }

      if( dd->pci_requested )
      {
	pci_release( dd->card.pci_dev );
      }

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
_AsmDisable:
	moveml	d0-d1/a0-a1,-(sp)
	bsr	_LibDisable
	moveml	(sp)+,d0-d1/a0-a1
	rts
");


void ASMCALL
LibDisable( REG( a2, struct AHIAudioCtrlDrv* audioctrl ) )
{
  Disable();
}


/******************************************************************************
** AHIsub_Enable **************************************************************
******************************************************************************/

__asm__("
_AsmEnable:
	moveml	d0-d1/a0-a1,-(sp)
	bsr	_LibEnable
	moveml	(sp)+,d0-d1/a0-a1
	rts
");


void ASMCALL
LibEnable( REG( a2, struct AHIAudioCtrlDrv* audioctrl ) )
{
  Enable();
}


/******************************************************************************
** AHIsub_Start ***************************************************************
******************************************************************************/

ULONG ASMCALL
LibStart( REG( d0, ULONG flags ),
	  REG( a2, struct AHIAudioCtrlDrv* audioctrl ) )
{
  struct DriverData* dd = (struct DriverData*) audioctrl->ahiac_DriverData;

  AHIsub_Stop( flags, audioctrl );

  if( flags & AHISF_PLAY )
  {
    ULONG dma_buffer_size;
    ULONG dma_sample_frame_size;

    /* Stop playback, free old buffers (if any) */

    AHIsub_Stop( AHISF_PLAY, audioctrl );

    /* Update cached/syncronized variables */

    AHIsub_Update( AHISF_PLAY, audioctrl );

    /* Allocate a new mixing buffer. Note: The buffer must be cleared, since
       it might not be filled by the mixer software interrupt because of
       pretimer/posttimer! */

    dd->mix_buffer = AllocVec( audioctrl->ahiac_BuffSize,
			       MEMF_ANY | MEMF_PUBLIC | MEMF_CLEAR );

    if( dd->mix_buffer == NULL )
    {
      KPrintF( DRIVER_NAME ": Unable to allocate %ld bytes for mixing buffer.\n",
	       audioctrl->ahiac_BuffSize );
      return AHIE_NOMEM;
    }

    /* Allocate a voice buffer large enough for 16-bit double-buffered
       playback (mono or stereo) */


    if( audioctrl->ahiac_Flags & AHIACF_STEREO )
    {
      dma_sample_frame_size = 4;
      dma_buffer_size = audioctrl->ahiac_MaxBuffSamples * dma_sample_frame_size;
    }
    else
    {
      dma_sample_frame_size = 2;
      dma_buffer_size = audioctrl->ahiac_MaxBuffSamples * dma_sample_frame_size;
    }

    if( emu10k1_voice_alloc_buffer( &dd->card,
				    &dd->voice.mem,
				    ( dma_buffer_size * 2 + PAGE_SIZE - 1 )
				    / PAGE_SIZE ) < 0 )
    {
      KPrintF( DRIVER_NAME ": Unable to allocate voice buffer.\n" );
      return AHIE_NOMEM;
    }

    memset( dd->voice.mem.addr, 0, dma_buffer_size * 2 );

    dd->voice_buffer_allocated = TRUE;


    dd->voice.usage = VOICE_USAGE_PLAYBACK;

    if( audioctrl->ahiac_Flags & AHIACF_STEREO )
    {
      dd->voice.flags = VOICE_FLAGS_STEREO | VOICE_FLAGS_16BIT;
    }
    else
    {
      dd->voice.flags = VOICE_FLAGS_16BIT;
    }

    if( emu10k1_voice_alloc( &dd->card, &dd->voice ) < 0 )
    {
      KPrintF( DRIVER_NAME ":Unable to allocate voice.\n" );
      return AHIE_UNKNOWN;
    }

    dd->voice_allocated = TRUE;

    dd->voice.initial_pitch = (u16) ( srToPitch( audioctrl->ahiac_MixFreq ) >> 8 );
    dd->voice.pitch_target  = SamplerateToLinearPitch( audioctrl->ahiac_MixFreq );

    DPD(2, "Initial pitch --> %#x\n", dd->voice.initial_pitch);

    /* start, startloop and endloop is unit sample frames, not bytes */

    dd->voice.start     = ( ( dd->voice.mem.emupageindex << 12 )
			    / dma_sample_frame_size );
    dd->voice.endloop   = dd->voice.start + audioctrl->ahiac_MaxBuffSamples * 2;
    dd->voice.startloop = dd->voice.start;


    /* Make interrupt routine start at the correct location */

    dd->current_position = dd->current_length;
    dd->current_buffer   = ( dd->voice.mem.addr +
			     dd->current_position * dma_sample_frame_size );


    if( dd->voice.flags & VOICE_FLAGS_STEREO )
    {
      dd->voice.params[0].send_a             = 0xff;
      dd->voice.params[0].send_b             = 0x00;
      dd->voice.params[0].send_c             = 0x00;
      dd->voice.params[0].send_d             = 0x00;
      dd->voice.params[0].send_routing       = 0x3210;
      dd->voice.params[0].volume_target      = 0xffff;
      dd->voice.params[0].initial_fc         = 0xff;
      dd->voice.params[0].initial_attn       = 0x00;
      dd->voice.params[0].byampl_env_sustain = 0x7f;
      dd->voice.params[0].byampl_env_decay   = 0x7f;

      dd->voice.params[1].send_a             = 0x00;
      dd->voice.params[1].send_b             = 0xff;
      dd->voice.params[1].send_c             = 0x00;
      dd->voice.params[1].send_d             = 0x00;
      dd->voice.params[1].send_routing       = 0x3210;
      dd->voice.params[1].volume_target      = 0xffff;
      dd->voice.params[1].initial_fc         = 0xff;
      dd->voice.params[1].initial_attn       = 0x00;
      dd->voice.params[1].byampl_env_sustain = 0x7f;
      dd->voice.params[1].byampl_env_decay   = 0x7f;
    }
    else
    {
      dd->voice.params[0].send_a             = 0xff;
      dd->voice.params[0].send_b             = 0xff;
      dd->voice.params[0].send_c             = 0x00;
      dd->voice.params[0].send_d             = 0x00;
      dd->voice.params[0].send_routing       = 0x3210;
      dd->voice.params[0].volume_target      = 0xffff;
      dd->voice.params[0].initial_fc         = 0xff;
      dd->voice.params[0].initial_attn       = 0x00;
      dd->voice.params[0].byampl_env_sustain = 0x7f;
      dd->voice.params[0].byampl_env_decay   = 0x7f;
    }

    DPD(2, "voice: startloop=%x, endloop=%x\n",
	dd->voice.startloop, dd->voice.endloop);

    emu10k1_voice_playback_setup( &dd->voice );

    dd->playback_interrupt_enabled = TRUE;

    /* Enable timer interrupts (TIMER_INTERRUPT_FREQUENCY Hz) */

    emu10k1_writefn0( &dd->card, TIMER_RATE, 48000 / TIMER_INTERRUPT_FREQUENCY );
    emu10k1_irq_enable( &dd->card, INTE_INTERVALTIMERENB );

    emu10k1_voices_start( &dd->voice, 1, 0 );

    dd->is_playing = TRUE;
  }

  if( flags & AHISF_RECORD )
  {
    int adcctl = 0;

    /* Stop current recording, free old buffer (if any) */

    AHIsub_Stop( AHISF_RECORD, audioctrl );

    /* Find out the recording frequency */

    switch( audioctrl->ahiac_MixFreq )
    {
      case 48000:
	adcctl = ADCCR_SAMPLERATE_48;
	break;

      case 44100:
	adcctl = ADCCR_SAMPLERATE_44;
	break;

      case 32000:
	adcctl = ADCCR_SAMPLERATE_32;
	break;

      case 24000:
	adcctl = ADCCR_SAMPLERATE_24;
	break;

      case 22050:
	adcctl = ADCCR_SAMPLERATE_22;
	break;

      case 16000:
	adcctl = ADCCR_SAMPLERATE_16;
	break;

      case 11025:
	adcctl = ADCCR_SAMPLERATE_11;
	break;

      case 8000:
	adcctl = ADCCR_SAMPLERATE_8;
	break;

      default:
	return AHIE_UNKNOWN;
    }

    adcctl |= ADCCR_LCHANENABLE | ADCCR_RCHANENABLE;

    /* Allocate a new recording buffer (page aligned!) */

    dd->record_buffer = pci_alloc_consistent( &dd->card.pci_dev,
					      RECORD_BUFFER_SAMPLES * 4,
					      &dd->record_dma_handle );

    if( dd->record_buffer == NULL )
    {
      KPrintF( DRIVER_NAME ": Unable to allocate %ld bytes for "
	       "the recording buffer.\n",
	       RECORD_BUFFER_SAMPLES * 4 );
      return AHIE_NOMEM;
    }

    SaveMixerState( dd );
    UpdateMonitorMixer( dd );

    sblive_writeptr( &dd->card, ADCBA, 0, dd->record_dma_handle );
    sblive_writeptr( &dd->card, ADCBS, 0, RECORD_BUFFER_SIZE_VALUE );
    sblive_writeptr( &dd->card, ADCCR, 0, adcctl );

    dd->record_interrupt_enabled = TRUE;

    /* Enable ADC interrupts  */

    emu10k1_irq_enable( &dd->card, INTE_ADCBUFENABLE );

    dd->is_recording = TRUE;
  }

  return AHIE_OK;
}


/******************************************************************************
** AHIsub_Update **************************************************************
******************************************************************************/

void ASMCALL
LibUpdate( REG( d0, ULONG flags ),
	   REG( a2, struct AHIAudioCtrlDrv* audioctrl ) )
{
  struct DriverData* dd = (struct DriverData*) audioctrl->ahiac_DriverData;

  dd->current_length = audioctrl->ahiac_BuffSamples;

  if( audioctrl->ahiac_Flags & AHIACF_STEREO )
  {
    dd->current_size = dd->current_length * 4;
  }
  else
  {
    dd->current_size = dd->current_length * 2;
  }
}


/******************************************************************************
** AHIsub_Stop ****************************************************************
******************************************************************************/

void ASMCALL
LibStop( REG( d0, ULONG flags ),
	 REG( a2, struct AHIAudioCtrlDrv* audioctrl ) )
{
  struct DriverData* dd = (struct DriverData*) audioctrl->ahiac_DriverData;

  if( flags & AHISF_PLAY )
  {
    dd->is_playing= FALSE;

    if( dd->voice_started )
    {
      emu10k1_irq_disable( &dd->card, INTE_INTERVALTIMERENB );

      sblive_writeptr( &dd->card, CLIEL, dd->voice.num, 0 );
      emu10k1_voices_stop( &dd->voice, 1 );
      dd->voice_started = FALSE;
    }

    if( dd->voice_allocated )
    {
      emu10k1_voice_free( &dd->voice );
      dd->voice_allocated = FALSE;
    }

    if( dd->voice_buffer_allocated )
    {
      emu10k1_voice_free_buffer( &dd->card, &dd->voice.mem );
      dd->voice_buffer_allocated = FALSE;
    }

    memset( &dd->voice, 0, sizeof( dd->voice ) );

    dd->current_length   = 0;
    dd->current_size     = 0;
    dd->current_buffer   = NULL;
    dd->current_position = 0;

    FreeVec( dd->mix_buffer );
    dd->mix_buffer = NULL;
  }

  if( flags & AHISF_RECORD )
  {
    emu10k1_irq_disable( &dd->card, INTE_ADCBUFENABLE );

    sblive_writeptr( &dd->card, ADCCR, 0, 0 );
    sblive_writeptr( &dd->card, ADCBS, 0, ADCBS_BUFSIZE_NONE );

    if( dd->is_recording )
    {
      // Do not restore mixer unless they have been saved
      RestoreMixerState( dd );
    }

    if( dd->record_buffer != NULL )
    {
      pci_free_consistent( dd->card.pci_dev,
			   RECORD_BUFFER_SAMPLES * 4,
			   dd->record_buffer,
			   dd->record_dma_handle );
    }

    dd->record_buffer = NULL;
    dd->record_dma_handle = NULL;

    dd->is_recording = FALSE;
  }
}


/******************************************************************************
** AHIsub_GetAttr *************************************************************
******************************************************************************/

LONG ASMCALL
LibGetAttr( REG( d0, ULONG attribute ),
	    REG( d1, LONG argument ),
	    REG( d2, LONG def ),
	    REG( a1, struct TagItem* taglist ),
	    REG( a2, struct AHIAudioCtrlDrv* audioctrl ) )
{
  int i;

  switch( attribute )
  {
    case AHIDB_Bits:
      return 16;

    case AHIDB_Frequencies:
      return FREQUENCIES;

    case AHIDB_Frequency: // Index->Frequency
      return (LONG) Frequencies[ argument ];

    case AHIDB_Index: // Frequency->Index
      if( argument <= (LONG) Frequencies[ 0 ] )
      {
        return 0;
      }

      if( argument >= (LONG) Frequencies[ FREQUENCIES - 1 ] )
      {
        return FREQUENCIES-1;
      }

      for( i = 1; i < FREQUENCIES; i++ )
      {
        if( (LONG) Frequencies[ i ] > argument )
        {
          if( ( argument - (LONG) Frequencies[ i - 1 ] )
	      < ( (LONG) Frequencies[ i ] - argument ) )
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
      return (LONG) "Martin 'Leviticus' Blom, Bertrand Lee et al.";

    case AHIDB_Copyright:
      return (LONG) "GNU GPL";

    case AHIDB_Version:
      return (LONG) LibIDString;

    case AHIDB_Annotation:
      return (LONG)
	"Based on the Linux kernel driver. Funded by Hyperion Entertainment.";

    case AHIDB_Record:
      return TRUE;

    case AHIDB_FullDuplex:
      return TRUE;

    case AHIDB_Realtime:
      return TRUE;

    case AHIDB_MaxRecordSamples:
      return RECORD_BUFFER_SAMPLES;

    case AHIDB_MinMonitorVolume:
      return 0x00000;

    case AHIDB_MaxMonitorVolume:
      return 0x40000;

    case AHIDB_MinInputGain:
      return 0x10000;

    case AHIDB_MaxInputGain:
      return 0xD7450;

    case AHIDB_MinOutputVolume:
      return 0x00000;

    case AHIDB_MaxOutputVolume:
      return 0x40000;

    case AHIDB_Inputs:
      return INPUTS;

    case AHIDB_Input:
      return (LONG) Inputs[ argument ];

    case AHIDB_Outputs:
      return OUTPUTS;

    case AHIDB_Output:
      return (LONG) Outputs[ argument ];

    default:
      return def;
  }
}


/******************************************************************************
** AHIsub_HardwareControl *****************************************************
******************************************************************************/

ULONG ASMCALL
LibHardwareControl( REG( d0, ULONG attribute ),
		    REG( d1, LONG argument ),
		    REG( a2, struct AHIAudioCtrlDrv* audioctrl ) )
{
  struct DriverData* dd = (struct DriverData*) audioctrl->ahiac_DriverData;

  switch( attribute )
  {
    case AHIC_MonitorVolume:
      dd->monitor_volume = Linear2MixerGain( (Fixed) argument,
					     &dd->monitor_volume_bits );
      if( dd->is_recording )
      {
	UpdateMonitorMixer( dd );
      }
      return TRUE;

    case AHIC_MonitorVolume_Query:
      return dd->monitor_volume;

    case AHIC_InputGain:
      dd->input_gain = Linear2RecordGain( (Fixed) argument,
					  &dd->input_gain_bits );
      emu10k1_writeac97( &dd->card, AC97_RECORD_GAIN, dd->input_gain_bits );
      return TRUE;

    case AHIC_InputGain_Query:
      return dd->input_gain;

    case AHIC_OutputVolume:
      dd->output_volume = Linear2MixerGain( (Fixed) argument,
					    &dd->output_volume_bits );
      emu10k1_writeac97( &dd->card, AC97_PCMOUT_VOL, dd->output_volume_bits );
      return TRUE;

    case AHIC_OutputVolume_Query:
      return dd->output_volume;

    case AHIC_Input:
      dd->input = argument;
      emu10k1_writeac97( &dd->card, AC97_RECORD_SELECT, InputBits[ dd->input ] );

      if( dd->is_recording )
      {
	UpdateMonitorMixer( dd );
      }

      return TRUE;

    case AHIC_Input_Query:
      return dd->input;

    case AHIC_Output:
      dd->output = argument;

      if( dd->output == 0 )
      {
	emu10k1_set_volume_gpr( &dd->card, 0x19, 0, VOL_5BIT);
	emu10k1_set_volume_gpr( &dd->card, 0x1a, 0, VOL_5BIT);
      }
      else
      {
	emu10k1_set_volume_gpr( &dd->card, 0x19, 80, VOL_5BIT);
	emu10k1_set_volume_gpr( &dd->card, 0x1a, 80, VOL_5BIT);
      }
      return TRUE;

    case AHIC_Output_Query:
      return dd->output;

    default:
      return FALSE;
  }
}


/******************************************************************************
** AHIsub_SetVol **************************************************************
******************************************************************************/

ULONG ASMCALL
LibSetVol( REG( d0, UWORD channel ),
	   REG( d1, Fixed volume ),
	   REG( d2, sposition pan ),
	   REG( a2, struct AHIAudioCtrlDrv* audioctrl ),
	   REG( d3, ULONG flags ) )
{
  return AHIS_UNKNOWN;
}


/******************************************************************************
** AHIsub_SetFreq *************************************************************
******************************************************************************/

ULONG ASMCALL
LibSetFreq( REG( d0, UWORD channel ),
	    REG( d1, ULONG freq ),
	    REG( a2, struct AHIAudioCtrlDrv* audioctrl ),
	    REG( d2, ULONG flags ) )
{
  return AHIS_UNKNOWN;
}


/******************************************************************************
** AHIsub_SetSound ************************************************************
******************************************************************************/

ULONG ASMCALL
LibSetSound( REG( d0, UWORD channel ),
	     REG( d1, UWORD sound ),
	     REG( d2, ULONG offset ),
	     REG( d3, LONG length ),
	     REG( a2, struct AHIAudioCtrlDrv* audioctrl ),
	     REG( d4, ULONG flags ) )
{
  return AHIS_UNKNOWN;
}


/******************************************************************************
** AHIsub_SetEffect ***********************************************************
******************************************************************************/

ULONG ASMCALL
LibSetEffect( REG( a0, APTR effect ),
	      REG( a2, struct AHIAudioCtrlDrv* audioctrl ) )
{
  return AHIS_UNKNOWN;
}


/******************************************************************************
** AHIsub_LoadSound ***********************************************************
******************************************************************************/

ULONG ASMCALL
LibLoadSound( REG( d0, UWORD sound ),
	      REG( d1, ULONG type ),
	      REG( a0, APTR info ),
	      REG( a2, struct AHIAudioCtrlDrv* audioctrl ) )
{
  return AHIS_UNKNOWN;
}


/******************************************************************************
** AHIsub_UnloadSound *********************************************************
******************************************************************************/

ULONG ASMCALL
LibUnloadSound( REG( d0, UWORD sound ),
		REG( a2, struct AHIAudioCtrlDrv* audioctrl ) )
{
  return AHIS_UNKNOWN;
}


/******************************************************************************
** Hardware interrupt handler *************************************************
******************************************************************************/

static ULONG ASMCALL INTERRUPT
EMU10kxInterrupt( REG( a1, struct AHIAudioCtrlDrv* audioctrl ) )
{
  struct DriverData* dd = (struct DriverData*) audioctrl->ahiac_DriverData;

  ULONG intreq;
  BOOL  handled = FALSE;

  while( ( intreq = *(ULONG*) ( dd->card.iobase + IPR ) ) != 0 )
  {
    if( intreq & IPR_INTERVALTIMER )
    {
      int hw   = sblive_readptr( &dd->card, CCCA_CURRADDR, dd->voice.num );
      int diff = dd->current_position - ( hw - dd->voice.start );

      if( diff < 0 )
      {
	diff += audioctrl->ahiac_MaxBuffSamples * 2;
      }

//      KPrintF( ">>> hw_pos = %08lx; current_pos = %08lx; diff=%ld <<<\n",
//	       hw, dd->current_position, diff );

      if( (ULONG) diff < dd->current_length )
      {
	if( dd->playback_interrupt_enabled )
	{
	  /* Invoke softint to fetch new sample data */

	  dd->playback_interrupt_enabled = FALSE;
	  Cause( &dd->playback_interrupt );
	}
      }
    }

    if( intreq & ( IPR_ADCBUFHALFFULL | IPR_ADCBUFFULL ) )
    {
      if( intreq & IPR_ADCBUFHALFFULL )
      {
	dd->current_record_buffer = dd->record_buffer;
      }
      else
      {
	dd->current_record_buffer = ( dd->record_buffer +
				      RECORD_BUFFER_SAMPLES * 4 / 2 );
      }

      if( dd->record_interrupt_enabled )
      {
	/* Invoke softint to convert and feed AHI with the new sample data */

	dd->record_interrupt_enabled = FALSE;
	Cause( &dd->record_interrupt );
      }
    }

    /* Clear interrupt pending bit(s) */
    *(ULONG*) (dd->card.iobase + IPR) = intreq;

    handled = TRUE;
  }

  return handled;
}


/******************************************************************************
** Playback interrupt handler *************************************************
******************************************************************************/

static void ASMCALL INTERRUPT
PlaybackInterrupt( REG( a1, struct AHIAudioCtrlDrv* audioctrl ) )
{
  struct DriverData* dd = (struct DriverData*) audioctrl->ahiac_DriverData;

  if( dd->mix_buffer != NULL && dd->current_buffer != NULL )
  {
    PreTimer_proto*  pretimer  = (PreTimer_proto*)  audioctrl->ahiac_PreTimer;
    PostTimer_proto* posttimer = (PostTimer_proto*) audioctrl->ahiac_PostTimer;
    BOOL             pretimer_rc;

    WORD*            src;
    WORD*            dst;
    size_t           skip;
    size_t           samples;
    int              i;

    pretimer_rc = pretimer( audioctrl );

    CallHookA( audioctrl->ahiac_PlayerFunc, (Object*) audioctrl, NULL );

    if( ! pretimer_rc )
    {
      CallHookA( audioctrl->ahiac_MixerFunc, (Object*) audioctrl, dd->mix_buffer );
    }

    /* Now translate and transfer to the DMA buffer */

    skip    = ( audioctrl->ahiac_Flags & AHIACF_HIFI ) ? 2 : 1;
    samples = dd->current_length;

    src     = dd->mix_buffer;
    dst     = dd->current_buffer;

    i = min( samples,
	     audioctrl->ahiac_MaxBuffSamples * 2 - dd->current_position );

    /* Update 'current_position' and 'samples' before destroying 'i' */

    dd->current_position += i;

    samples -= i;

    if( audioctrl->ahiac_Flags & AHIACF_STEREO )
    {
      i *= 2;
    }

    while( i > 0 )
    {
      *dst = ( ( *src & 0xff ) << 8 ) | ( ( *src & 0xff00 ) >> 8 );

      src += skip;
      dst += 1;

      --i;
    }

    if( dd->current_position == audioctrl->ahiac_MaxBuffSamples * 2 )
    {
      dst = dd->voice.mem.addr;
      dd->current_position = 0;
    }

    if( samples > 0 )
    {
      /* Update 'current_position' before destroying 'samples' */

      dd->current_position += samples;

      if( audioctrl->ahiac_Flags & AHIACF_STEREO )
      {
	samples *= 2;
      }

      while( samples > 0 )
      {
	*dst = ( ( *src & 0xff ) << 8 ) | ( ( *src & 0xff00 ) >> 8 );

	src += skip;
	dst += 1;

	--samples;
      }
    }

    /* Update 'current_buffer' */

    dd->current_buffer = dst;

    posttimer( audioctrl );
  }

  dd->playback_interrupt_enabled = TRUE;
}


/******************************************************************************
** Record interrupt handler ***************************************************
******************************************************************************/

static void ASMCALL INTERRUPT
RecordInterrupt( REG( a1, struct AHIAudioCtrlDrv* audioctrl ) )
{
  struct DriverData* dd = (struct DriverData*) audioctrl->ahiac_DriverData;


  struct AHIRecordMessage rm =
  {
    AHIST_S16S,
    dd->current_record_buffer,
    RECORD_BUFFER_SAMPLES / 2
  };

  int   i   = 0;
  WORD* ptr = dd->current_record_buffer;

  while( i < RECORD_BUFFER_SAMPLES / 2 * 2 )
  {
    *ptr = ( ( *ptr & 0xff ) << 8 ) | ( ( *ptr & 0xff00 ) >> 8 );

    ++i;
    ++ptr;
  }

  CallHookA( audioctrl->ahiac_SamplerFunc, (Object*) audioctrl, &rm );

  dd->record_interrupt_enabled = TRUE;
}

/******************************************************************************
** Misc. **********************************************************************
******************************************************************************/

static void
SaveMixerState( struct DriverData* dd )
{
  dd->ac97_mic    = emu10k1_readac97( &dd->card, AC97_MIC_VOL );
  dd->ac97_cd     = emu10k1_readac97( &dd->card, AC97_CD_VOL );
  dd->ac97_video  = emu10k1_readac97( &dd->card, AC97_VIDEO_VOL );
  dd->ac97_aux    = emu10k1_readac97( &dd->card, AC97_AUX_VOL );
  dd->ac97_linein = emu10k1_readac97( &dd->card, AC97_LINEIN_VOL );
  dd->ac97_phone  = emu10k1_readac97( &dd->card, AC97_PHONE_VOL );
}


static void
RestoreMixerState( struct DriverData* dd )
{
  emu10k1_writeac97( &dd->card, AC97_MIC_VOL,    dd->ac97_mic );
  emu10k1_writeac97( &dd->card, AC97_CD_VOL,     dd->ac97_cd );
  emu10k1_writeac97( &dd->card, AC97_VIDEO_VOL,  dd->ac97_video );
  emu10k1_writeac97( &dd->card, AC97_AUX_VOL,    dd->ac97_aux );
  emu10k1_writeac97( &dd->card, AC97_LINEIN_VOL, dd->ac97_linein );
  emu10k1_writeac97( &dd->card, AC97_PHONE_VOL,  dd->ac97_phone );
}

static void
UpdateMonitorMixer( struct DriverData* dd )
{
  int   i  = InputBits[ dd->input ];
  UWORD m  = dd->monitor_volume_bits & 0x801f;
  UWORD s  = dd->monitor_volume_bits;
  UWORD mm = AC97_MUTE | 0x0008;
  UWORD sm = AC97_MUTE | 0x0808;

  if( i == AC97_RECMUX_STEREO_MIX ||
      i == AC97_RECMUX_MONO_MIX )
  {
    // Use the original mixer settings
    RestoreMixerState( dd );
  }
  else
  {
    emu10k1_writeac97( &dd->card, AC97_MIC_VOL,
		       i == AC97_RECMUX_MIC ? m : mm );

    emu10k1_writeac97( &dd->card, AC97_CD_VOL,
		       i == AC97_RECMUX_CD ? s : sm );

    emu10k1_writeac97( &dd->card, AC97_VIDEO_VOL,
		       i == AC97_RECMUX_VIDEO ? s : sm );

    emu10k1_writeac97( &dd->card, AC97_AUX_VOL,
		       i == AC97_RECMUX_AUX ? s : sm );

    emu10k1_writeac97( &dd->card, AC97_LINEIN_VOL,
		       i == AC97_RECMUX_LINE ? s : sm );

    emu10k1_writeac97( &dd->card, AC97_PHONE_VOL,
		       i == AC97_RECMUX_PHONE ? m : mm );
  }
}


static Fixed
Linear2MixerGain( Fixed  linear,
		  UWORD* bits )
{
  static const Fixed gain[ 33 ] =
  {
    260904, // +12.0 dB
    219523, // +10.5 dB
    184706, //  +9.0 dB
    155410, //  +7.5 dB
    130762, //  +6.0 dB
    110022, //  +4.5 dB
    92572,  //  +3.0 dB
    77890,  //  +1.5 dB
    65536,  //  ±0.0 dB
    55142,  //  -1.5 dB
    46396,  //  -3.0 dB
    39037,  //  -4.5 dB
    32846,  //  -6.0 dB
    27636,  //  -7.5 dB
    23253,  //  -9.0 dB
    19565,  // -10.5 dB
    16462,  // -12.0 dB
    13851,  // -13.5 dB
    11654,  // -15.0 dB
    9806,   // -16.5 dB
    8250,   // -18.0 dB
    6942,   // -19.5 dB
    5841,   // -21.0 dB
    4915,   // -22.5 dB
    4135,   // -24.0 dB
    3479,   // -25.5 dB
    2927,   // -27.0 dB
    2463,   // -28.5 dB
    2072,   // -30.0 dB
    1744,   // -31.5 dB
    1467,   // -33.0 dB
    1234,   // -34.5 dB
    0       //   -oo dB
  };

  int v = 0;

  while( linear < gain[ v ] )
  {
    ++v;
  }

  if( v == 32 )
  {
    *bits = 0x8000; // Mute
  }
  else
  {
    *bits = ( v << 8 ) | v;
  }

//  KPrintF( "l2mg %08lx -> %08lx (%04lx)\n", linear, gain[ v ], *bits );
  return gain[ v ];
}

static Fixed
Linear2RecordGain( Fixed  linear,
		   UWORD* bits )
{
  static const Fixed gain[ 17 ] =
  {
    873937, // +22.5 dB
    735326, // +21.0 dB
    618700, // +19.5 dB
    520571, // +18.0 dB
    438006, // +16.5 dB
    368536, // +15.0 dB
    310084, // +13.5 dB
    260904, // +12.0 dB
    219523, // +10.5 dB
    184706, //  +9.0 dB
    155410, //  +7.5 dB
    130762, //  +6.0 dB
    110022, //  +4.5 dB
    92572,  //  +3.0 dB
    77890,  //  +1.5 dB
    65536,  //  ±0.0 dB
    0       //  -oo dB
  };

  int v = 0;

  while( linear < gain[ v ] )
  {
    ++v;
  }

  if( v == 16 )
  {
    *bits = 0x8000; // Mute
  }
  else
  {
    *bits = ( ( 15 - v ) << 8 ) | ( 15 - v );
  }

  return gain[ v ];
}


static ULONG
SamplerateToLinearPitch( ULONG samplingrate )
{
  samplingrate = (samplingrate << 8) / 375;
  return (samplingrate >> 1) + (samplingrate & 1);
}
