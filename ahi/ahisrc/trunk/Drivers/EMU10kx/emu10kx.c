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

#define dd ((struct DriverData*) AudioCtrl->ahiac_DriverData)

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
intAHIsub_AllocAudio( REG( a1, struct TagItem* tagList ),
		      REG( a2, struct AHIAudioCtrlDrv* AudioCtrl ) );

void ASMCALL
intAHIsub_FreeAudio( REG( a2, struct AHIAudioCtrlDrv* AudioCtrl ) );

void ASMCALL
asmAHIsub_Disable( REG( a2, struct AHIAudioCtrlDrv* AudioCtrl ) );

void ASMCALL
intAHIsub_Disable( REG( a2, struct AHIAudioCtrlDrv* AudioCtrl ) );

void ASMCALL
asmAHIsub_Enable( REG( a2, struct AHIAudioCtrlDrv* AudioCtrl ) );

void ASMCALL
intAHIsub_Enable( REG( a2, struct AHIAudioCtrlDrv* AudioCtrl ) );

ULONG ASMCALL
intAHIsub_Start( REG( d0, ULONG Flags ),
                 REG( a2, struct AHIAudioCtrlDrv* AudioCtrl ) );

void ASMCALL
intAHIsub_Update( REG( d0, ULONG Flags ),
                  REG( a2, struct AHIAudioCtrlDrv* AudioCtrl ) );

void ASMCALL
intAHIsub_Stop( REG( d0, ULONG Flags ),
                REG( a2, struct AHIAudioCtrlDrv* AudioCtrl ) );

LONG ASMCALL
intAHIsub_GetAttr( REG( d0, ULONG Attribute ),
                   REG( d1, LONG Argument ),
                   REG( d2, LONG Default ),
                   REG( a1, struct TagItem* tagList ),
                   REG( a2, struct AHIAudioCtrlDrv* AudioCtrl ) );

ULONG ASMCALL
intAHIsub_HardwareControl( REG( d0, ULONG attribute ),
                           REG( d1, LONG argument ),
                           REG( a2, struct AHIAudioCtrlDrv* AudioCtrl ) );

ULONG ASMCALL
intAHIsub_SetVol( REG( d0, UWORD channel ),
                  REG( d1, Fixed volume ),
                  REG( d2, sposition pan ),
                  REG( a2, struct AHIAudioCtrlDrv* AudioCtrl ),
                  REG( d3, ULONG Flags ) );

ULONG ASMCALL
intAHIsub_SetFreq( REG( d0, UWORD channel ),
                   REG( d1, ULONG freq ),
                   REG( a2, struct AHIAudioCtrlDrv* AudioCtrl ),
                   REG( d2, ULONG flags ) );

ULONG ASMCALL
intAHIsub_SetSound( REG( d0, UWORD channel ),
                    REG( d1, UWORD sound ),
                    REG( d2, ULONG offset ),
                    REG( d3, LONG length ),
                    REG( a2, struct AHIAudioCtrlDrv* AudioCtrl ),
                    REG( d4, ULONG flags ) );

ULONG ASMCALL
intAHIsub_SetEffect( REG( a0, APTR effect ),
                     REG( a2, struct AHIAudioCtrlDrv* AudioCtrl ) );

ULONG ASMCALL
intAHIsub_LoadSound( REG( d0, UWORD sound ),
                     REG( d1, ULONG type ),
                     REG( a0, APTR info ),
                     REG( a2, struct AHIAudioCtrlDrv* AudioCtrl ) );

ULONG ASMCALL
intAHIsub_UnloadSound( REG( d0, UWORD sound ),
                       REG( a2, struct AHIAudioCtrlDrv* AudioCtrl ) );



static ULONG ASMCALL INTERRUPT 
EMU10kx_interrupt( REG( a1, struct AHIAudioCtrlDrv* AudioCtrl ) );

static void ASMCALL INTERRUPT 
Mixer_interrupt( REG( a1, struct AHIAudioCtrlDrv* AudioCtrl ) );

static ULONG
samplerate_to_linearpitch( ULONG samplingrate );


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
  LibOpen,
  LibClose,
  LibExpunge,
  Null,
  
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
intAHIsub_AllocAudio( REG( a1, struct TagItem* tagList ),
		      REG( a2, struct AHIAudioCtrlDrv* AudioCtrl ) )
{
  int              card_num;
  UWORD            command_word;
  int              ret;

  card_num = ( GetTagData( AHIDB_AudioID, 0, tagList) & 0x0000f000 ) >> 12;
  
  dd = AllocVec( sizeof( *dd ), MEMF_PUBLIC | MEMF_CLEAR );

  if( dd == NULL )
  {
    KPrintF( DRIVER_NAME ": Unable to allocate driver structure.\n" );
  }
  else
  {
    dd->interrupt.is_Node.ln_Type = NT_INTERRUPT;
    dd->interrupt.is_Node.ln_Pri  = 0;
    dd->interrupt.is_Node.ln_Name = (STRPTR) LibName;
    dd->interrupt.is_Code         = (void(*)(void)) EMU10kx_interrupt;
    dd->interrupt.is_Data         = (APTR) AudioCtrl;
    
    dd->software_interrupt.is_Node.ln_Type = NT_INTERRUPT;
    dd->software_interrupt.is_Node.ln_Pri  = 0;
    dd->software_interrupt.is_Node.ln_Name = (STRPTR) LibName;
    dd->software_interrupt.is_Code         = (void(*)(void)) Mixer_interrupt;
    dd->software_interrupt.is_Data         = (APTR) AudioCtrl;
    
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

    if( pci_enable( dd->card.pci_dev ) )
    {
      KPrintF( DRIVER_NAME ": Unable to enable card.\n" );
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

    
    /* Initialize mixer */

    emu10k1_writeac97( &dd->card, AC97_RESET, 0L);
	
    Delay( 1 );

    if (emu10k1_readac97( &dd->card, AC97_RESET ) & 0x8000) {
      KPrintF( DRIVER_NAME ": ac97 codec not present.\n");
      return AHISF_ERROR;
    }
    else
    {
      emu10k1_writeac97( &dd->card, AC97_MASTER_VOL_STEREO, 0x0000 );
      emu10k1_writeac97( &dd->card, AC97_PCMOUT_VOL,        0x0000 );
      emu10k1_writeac97( &dd->card, AC97_PCBEEP_VOL,        0x000a );
      emu10k1_writeac97( &dd->card, AC97_LINEIN_VOL,        0x0a0a );
      emu10k1_writeac97( &dd->card, AC97_MIC_VOL,           AC97_MUTE );
      emu10k1_writeac97( &dd->card, AC97_CD_VOL,            0x0a0a );
      emu10k1_writeac97( &dd->card, AC97_RECORD_GAIN,       0x0a0a );
      emu10k1_writeac97( &dd->card, AC97_AUX_VOL,           0x0a0a );
      emu10k1_writeac97( &dd->card, AC97_PHONE_VOL,         0x000a );
      emu10k1_writeac97( &dd->card, AC97_MASTER_VOL_MONO,   0x000a );
      emu10k1_writeac97( &dd->card, AC97_VIDEO_VOL,         0x0a0a );

      if (emu10k1_readac97( &dd->card, AC97_EXTENDED_ID ) & 0x0080 )
      {
	sblive_writeptr( &dd->card, AC97SLOT, 0, AC97SLOT_CNTR | AC97SLOT_LFE);
	emu10k1_writeac97( &dd->card, AC97_SURROUND_MASTER, 0x0 );
      }
    }

    /* Initialize chip */

    if( emu10k1_init( &dd->card ) < 0 )
    {
      KPrintF( DRIVER_NAME ": Unable to initialize EMU10kx subsystem.\n");
      return AHISF_ERROR;
    }

    /* Since the EMU10kx chips can play a voice at any sample rate, we
       do not have to examine/modify AudioCtrl->ahiac_MixFreq here.

       Had this not been the case, AudioCtrl->ahiac_MixFreq should be
       set to the frequency we will use.  */
  }

  return AHISF_KNOWHIFI | AHISF_KNOWSTEREO | AHISF_MIXING | AHISF_TIMING;
}



/******************************************************************************
** AHIsub_FreeAudio ***********************************************************
******************************************************************************/

void ASMCALL
intAHIsub_FreeAudio( REG( a2, struct AHIAudioCtrlDrv* AudioCtrl ) )
{
  if( dd != NULL )
  {
    if( dd->card.pci_dev != NULL )
    {
      emu10k1_cleanup( &dd->card );
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


void ASMCALL
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


void ASMCALL
intAHIsub_Enable( REG( a2, struct AHIAudioCtrlDrv* AudioCtrl ) )
{
  Enable();
}


/******************************************************************************
** AHIsub_Start ***************************************************************
******************************************************************************/

ULONG ASMCALL
intAHIsub_Start( REG( d0, ULONG Flags ),
                 REG( a2, struct AHIAudioCtrlDrv* AudioCtrl ) )
{
  AHIsub_Stop( Flags, AudioCtrl );

  if(Flags & AHISF_PLAY)
  {
    ULONG dma_buffer_size;
    ULONG dma_sample_frame_size;
    
    /* Stop playback, free old buffers (if any) */
    
    AHIsub_Stop( AHISF_PLAY, AudioCtrl );

    /* Update cached/syncronized variables */

    AHIsub_Update( AHISF_PLAY, AudioCtrl );
    
    /* Allocate a new mixing buffer */

    dd->mixbuffer = AllocVec( AudioCtrl->ahiac_BuffSize,
			      MEMF_ANY | MEMF_PUBLIC );

    if( dd->mixbuffer == NULL )
    {
      KPrintF( DRIVER_NAME ": Unable to allocate %ld bytes for mixing buffer.\n",
	       AudioCtrl->ahiac_BuffSize );
      return AHIE_NOMEM;
    }

    /* Allocate a voice buffer large enough for 16-bit double-buffered
       playback (mono or stereo) */


    if( AudioCtrl->ahiac_Flags & AHIACF_STEREO )
    {
      dma_buffer_size = AudioCtrl->ahiac_MaxBuffSamples * sizeof( WORD ) * 2;
      dma_sample_frame_size = 4;
    }
    else
    {
      dma_buffer_size = AudioCtrl->ahiac_MaxBuffSamples * sizeof( WORD );
      dma_sample_frame_size = 2;
    }

    if( emu10k1_voice_alloc_buffer( &dd->card,
				    &dd->voice.mem,
				    ( dma_buffer_size * 2 + PAGE_SIZE - 1 )
				    / PAGE_SIZE ) < 0 )
    {
      KPrintF( DRIVER_NAME ": Unable to allocate voice buffer.\n" );
      return AHIE_NOMEM;
    }

    dd->voice_buffer_allocated = TRUE;

    {
      int i;
      char value;
      char* ptr = dd->voice.mem.addr;

      for( i = 0; i < dma_buffer_size*2; ++i  ) {*ptr++ = value; value++; }
//      memset( dd->voice.mem.addr, 0, dma_buffer_size * 2 );
    }

    dd->voice.usage = VOICE_USAGE_PLAYBACK;

    if( AudioCtrl->ahiac_Flags & AHIACF_STEREO )
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
    
    dd->voice.initial_pitch = (u16) ( srToPitch( AudioCtrl->ahiac_MixFreq ) >> 8 );
    dd->voice.pitch_target  = samplerate_to_linearpitch( AudioCtrl->ahiac_MixFreq );

    DPD(2, "Initial pitch --> %#x\n", dd->voice.initial_pitch);

    /* start, startloop and endloop is unit sample frames, not bytes */

    dd->voice.start     = ( ( dd->voice.mem.emupageindex << 12 )
			    / dma_sample_frame_size );
    dd->voice.endloop   = dd->voice.start + AudioCtrl->ahiac_BuffSamples *2;
    dd->voice.startloop = dd->voice.start;
//    dd->voice.endloop   = dd->voice.start + AudioCtrl->ahiac_BuffSamples;
//    dd->voice.startloop = dd->voice.endloop;

    dd->emu_start      = dd->voice.start;
    dd->emu_startloop  = dd->voice.startloop;
      
    KPrintF( "Playing %08lx to %08lx, repeating at %08lx\n",
	     dd->voice.start, dd->voice.endloop, dd->voice.startloop, );

    if( dd->voice.flags & VOICE_FLAGS_STEREO )
    {
      dd->voice.params[0].send_a             = 0xff;
      dd->voice.params[0].send_b             = 0x0;
      dd->voice.params[0].send_c             = 0x0;
      dd->voice.params[0].send_d             = 0x0;
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
      dd->voice.params[0].send_c             = 0x0;
      dd->voice.params[0].send_d             = 0x0;
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

    DPF(2, "emu10k1_waveout_start()\n");

    /* Enable channel loop interrupts for channel 0 */
    sblive_writeptr( &dd->card, CLIEL, dd->voice.num, 1 );

    /* Enable timer interrupts */
    emu10k1_irq_enable( &dd->card, INTE_INTERVALTIMERENB );
    emu10k1_writefn0( &dd->card, TIMER_RATE,
		      48000 * AudioCtrl->ahiac_BuffSamples /
		      AudioCtrl->ahiac_MixFreq );
    
    emu10k1_voices_start( &dd->voice, 1, 0 );
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

void ASMCALL
intAHIsub_Update( REG( d0, ULONG Flags ),
                  REG( a2, struct AHIAudioCtrlDrv* AudioCtrl ) )
{
  dd->current_length = AudioCtrl->ahiac_BuffSamples;
}


/******************************************************************************
** AHIsub_Stop ****************************************************************
******************************************************************************/

void ASMCALL
intAHIsub_Stop( REG( d0, ULONG Flags ),
                REG( a2, struct AHIAudioCtrlDrv* AudioCtrl ) )
{
  if( Flags & AHISF_PLAY )
  {
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

    dd->emu_start      = 0;
    dd->emu_startloop  = 0;
    dd->current_length = 0;
    dd->current_size   = 0;
    dd->current_buffer = NULL;

    FreeVec( dd->mixbuffer );
    dd->mixbuffer = NULL;
  }

  if(Flags & AHISF_RECORD)
  {
    // Do nothing
  }
}


/******************************************************************************
** AHIsub_GetAttr *************************************************************
******************************************************************************/

LONG ASMCALL
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

ULONG ASMCALL
intAHIsub_HardwareControl( REG( d0, ULONG attribute ),
                           REG( d1, LONG argument ),
                           REG( a2, struct AHIAudioCtrlDrv* AudioCtrl ) )
{
  return NULL;
}


/******************************************************************************
** AHIsub_SetVol **************************************************************
******************************************************************************/

ULONG ASMCALL
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

ULONG ASMCALL
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

ULONG ASMCALL
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

ULONG ASMCALL
intAHIsub_SetEffect( REG( a0, APTR effect ),
                     REG( a2, struct AHIAudioCtrlDrv* AudioCtrl ) )
{
  return AHIS_UNKNOWN;
}


/******************************************************************************
** AHIsub_LoadSound ***********************************************************
******************************************************************************/

ULONG ASMCALL
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

ULONG ASMCALL
intAHIsub_UnloadSound( REG( d0, UWORD sound ),
                       REG( a2, struct AHIAudioCtrlDrv* AudioCtrl ) )
{
  return AHIS_UNKNOWN;
}


/******************************************************************************
** Hardware interrupt handler *************************************************
******************************************************************************/

static ULONG ASMCALL INTERRUPT 
EMU10kx_interrupt( REG( a1, struct AHIAudioCtrlDrv* AudioCtrl ) )
{
  ULONG intreq;
  BOOL  handled = FALSE;

  while( ( intreq = *(ULONG*) ( dd->card.iobase + IPR ) ) != 0 )
  {
    KPrintF( "intreq: %08lx\n", *(ULONG*) ( dd->card.iobase + IPR ) );

    if( intreq & IPR_INTERVALTIMER )
//    if( ( intreq & IPR_CHANNELLOOP ) &&
//	sblive_readptr( &dd->card, CLIPL, dd->voice.num ) )
    {
      int   lr;
      ULONG new_startloop;

      sblive_writeptr( &dd->card, CLIPL, dd->voice.num, 0 );
      
      if( AudioCtrl->ahiac_Flags & AHIACF_STEREO )
      {
	dd->current_size = dd->current_length * 4;
      }
      else
      {
	dd->current_size = dd->current_length * 2;
      }

      if( dd->emu_startloop == dd->emu_start )
      {
	new_startloop      = dd->emu_start + dd->current_length;
	dd->current_buffer = dd->voice.mem.addr;
      }
      else
      {
	new_startloop      = dd->emu_start;
	dd->current_buffer = dd->voice.mem.addr + dd->current_size;;
      }
#if 0
      for( lr = 0; lr < (dd->voice.flags & VOICE_FLAGS_STEREO ? 2 : 1); ++lr )
      {
	ULONG old_val;

	/* Set start loop */

	old_val = sblive_readptr( &dd->card, PSST, dd->voice.num + lr );

	sblive_writeptr( &dd->card, PSST, dd->voice.num + lr,
			 ( old_val & 0xff000000 ) | new_startloop );

	/* Set end loop */
    
	old_val = sblive_readptr( &dd->card, DSL, dd->voice.num + lr );

	sblive_writeptr( &dd->card, DSL, dd->voice.num + lr,
			 ( old_val & 0xff000000 ) |
			 ( dd->emu_startloop + dd->current_length ) );
      }
#endif
      dd->emu_startloop = new_startloop;
      
      /* Invoke softint to fetch new sample data */
	 
      Cause( &dd->software_interrupt );
//      Mixer_interrupt( AudioCtrl );
    }

    /* Clear interrupt pending bit(s) */
    *(ULONG*) (dd->card.iobase + IPR) = intreq;
    
    handled = TRUE;
  }

  return handled;
}


/******************************************************************************
** Mixer interrupt handler ****************************************************
******************************************************************************/

static void ASMCALL INTERRUPT 
Mixer_interrupt( REG( a1, struct AHIAudioCtrlDrv* AudioCtrl ) )
{
  if( dd->mixbuffer != NULL && dd->current_buffer != NULL )
  {
    PreTimer_proto*  pretimer  = (PreTimer_proto*)  AudioCtrl->ahiac_PreTimer;
    PostTimer_proto* posttimer = (PostTimer_proto*) AudioCtrl->ahiac_PostTimer;
    BOOL             pretimer_rc;

    WORD*            src;
    WORD*            dst;
    size_t           skip;
    size_t           samples;

    pretimer_rc = pretimer( AudioCtrl );
	
    CallHookPkt( AudioCtrl->ahiac_PlayerFunc, (Object*) AudioCtrl, NULL );

    if( ! pretimer_rc )
    {
      CallHookPkt( AudioCtrl->ahiac_MixerFunc, (Object*) AudioCtrl, dd->mixbuffer );
    }

#if 1
    /* Now translate and transfer to the DMA buffer */

    skip    = AudioCtrl->ahiac_Flags & AHIACF_HIFI   ? 2 : 1;
    samples = ( AudioCtrl->ahiac_BuffSamples *
		( AudioCtrl->ahiac_Flags & AHIACF_STEREO ? 2 : 1 ) );

    src     = dd->mixbuffer;
    dst     = dd->current_buffer;

    KPrintF( "Mixer: %08lx (%ld samples) from %08lx.\n",
	     dst, samples, src );
  
//    KPrintF( ">>> hw_pos = %08lx <<<\n", sblive_readptr( &dd->card,
//							 CCCA_CURRADDR,
//							 dd->voice.num ) );

      while( samples > 0 )
    {
//      static short value;

//      *src = value;
//      value += 256;
      *dst = ( ( *src & 0xff ) << 8 ) | ( ( *src & 0xff00 ) >> 8 );

      src += skip;
      dst += 1;

      --samples;
    }
  
    /* Flush to RAM */
  
    CacheClearE( dd->current_buffer, dd->current_size, CACRF_ClearD );
  
    KPrintF( ">>> hw_pos = %08lx <<<\n", sblive_readptr( &dd->card,
							 CCCA_CURRADDR,
							 dd->voice.num ) );
    
#endif
    posttimer( AudioCtrl );
  }
}


/******************************************************************************
** Misc. **********************************************************************
******************************************************************************/

static ULONG
samplerate_to_linearpitch( ULONG samplingrate )
{
  samplingrate = (samplingrate << 8) / 375;
  return (samplingrate >> 1) + (samplingrate & 1);
}
