
#include <amithlon/powerpci.h>
#include <pci/powerpci_pci.h>
#include <exec/memory.h>

#include <proto/exec.h>
#include <proto/dos.h>
#include <proto/powerpci.h>

#include "8010.h"
#include "hwaccess.h"
#include "EMU10k1.h"


struct Library *ppcibase = NULL;


struct emu10k1_card*
AllocEMU10k1( ULONG card_num );

void
FreeEMU10k1( struct emu10k1_card* card );

#define PCI_ANY_ID                     0xffff
#define PCI_VENDOR_ID_CREATIVE         0x1102
#define PCI_DEVICE_ID_CREATIVE_EMU10K1 0x0002
#define EMU_APS_SUBID                  0x40011102

static BOOL
OpenLibs( void )
{ 
  ppcibase = OpenLibrary( "powerpci.library", 1 );

  if( ppcibase == NULL )
  {
    Printf( "Unable to open 'powerpci.library' version 1.\n" );
    return FALSE;
  }

  return TRUE;
}


static void
CloseLibs( void )
{
  CloseLibrary( ppcibase );
  ppcibase = NULL;
}












static void emu10k1_cleanup(struct emu10k1_card *card)
{
  int ch;

  emu10k1_writefn0(card, INTE, 0);

  /** Shutdown the chip **/
  for (ch = 0; ch < NUM_G; ch++)
    sblive_writeptr(card, DCYSUSV, ch, 0);

  for (ch = 0; ch < NUM_G; ch++) {
    sblive_writeptr_tag(card, ch,
			VTFT, 0,
			CVCF, 0,
			PTRX, 0,
			//CPF, 0,
			TAGLIST_END);
    sblive_writeptr(card, CPF, ch, 0);
  }

  /* Disable audio and lock cache */
  emu10k1_writefn0(card, HCFG, HCFG_LOCKSOUNDCACHE | HCFG_LOCKTANKCACHE_MASK | HCFG_MUTEBUTTONENABLE);

  sblive_writeptr_tag(card, 0,
		      PTB, 0,

		      /* Reset recording buffers */
		      MICBS, ADCBS_BUFSIZE_NONE,
		      MICBA, 0,
		      FXBS, ADCBS_BUFSIZE_NONE,
		      FXBA, 0,
		      FXWC, 0,
		      ADCBS, ADCBS_BUFSIZE_NONE,
		      ADCBA, 0,
		      TCBS, 0,
		      TCB, 0,
		      DBG, 0x8000,

		      /* Disable channel interrupt */
		      CLIEL, 0,
		      CLIEH, 0,
		      SOLEL, 0,
		      SOLEH, 0,
		      TAGLIST_END);


//  pci_free_consistent(card->pci_dev, card->virtualpagetable.size, card->virtualpagetable.addr, card->virtualpagetable.dma_handle);
//  pci_free_consistent(card->pci_dev, card->silentpage.size, card->silentpage.addr, card->silentpage.dma_handle);
  
  if(card->tankmem.size != 0)
//    pci_free_consistent(card->pci_dev, card->tankmem.size, card->tankmem.addr, card->tankmem.dma_handle);

  /* release patch storage memory */
  fx_cleanup(&card->mgr);
}










struct emu10k1_card*
AllocEMU10k1( ULONG card_num )
{
  struct emu10k1_card* card;
  UWORD                command_word;
  int                  ret;

  card = AllocVec( sizeof( *card ), MEMF_PUBLIC | MEMF_CLEAR );

  if( card == NULL )
  {
    Printf( "Unable to allocate card structure.\n " );
  }
  else
  {
    card->pci_dev = 0;

    do
    {
      card->pci_dev = pci_find_device( PCI_VENDOR_ID_CREATIVE,
				     PCI_DEVICE_ID_CREATIVE_EMU10K1,
				     card->pci_dev );
    } while( card->pci_dev != 0 && card_num-- != 0 );

    if( card->pci_dev == NULL )
    {
      Printf( "Unable to find EMU10k subsystem.\n" );
      goto error;
    }

    // Audigy not supported yet
    card->audigy = FALSE;
    
//  if( pci_set_dma_mask(card->pci_dev, EMU10K1_DMA_MASK) )
//  {
//    Printf( "Unable to set DMA mask for card\n." );
//    goto error;
//  }

    if( pci_request( card->pci_dev, "EMU10k1 AHI driver", NULL ) )
    {
      Printf( "Unable to claim I/O resources.\n" );
      goto error;
    }

    if( pci_enable( card->pci_dev ) )
    {
      Printf( "Unable to enable card.\n" );
      goto error;
    }

    command_word = pci_read_conf_word( card->pci_dev, PCI_COMMAND );
    command_word |= PCI_CMD_IO_MASK | PCI_CMD_MEMORY_MASK | PCI_CMD_MASTER_MASK;
    pci_write_conf_word( card->pci_dev, PCI_COMMAND, command_word );

    // FIXME: How about latency/pcibios_set_master()??
    
    card->iobase  = (ULONG) pci_get_base_start( card->pci_dev, 0 );
    card->length  = ( (ULONG) pci_get_base_end( card->pci_dev, 0 ) -
			 (ULONG) card->iobase ) + 1;

    card->irq     = pci_ask_irq( card->pci_dev ) >> 8;
    
    card->chiprev = pci_read_conf_byte( card->pci_dev, PCI_REVISION_ID );
    card->model   = pci_read_conf_word( card->pci_dev, PCI_SUBSYSTEM_ID );
    card->isaps   = ( pci_read_conf_long( card->pci_dev, PCI_SUBSYSTEM_VENDOR_ID )
		      == EMU_APS_SUBID );

#if 0
    ret = emu10k1_audio_init(card);
    if(ret < 0) {
      Printf( "emu10k1: cannot initialize audio devices\n");
      goto error;
    }

    ret = emu10k1_mixer_init(card);
    if(ret < 0) {
      Printf( "emu10k1: cannot initialize AC97 codec\n");
      goto error;
    }

    ret = emu10k1_midi_init(card);
    if (ret < 0) {
      Printf( "emu10k1: cannot register midi device\n");
      goto error;
    }
#endif

    ret = emu10k1_init(card);
    if (ret < 0) {
      Printf( "emu10k1: cannot initialize device\n");
      goto error;
    }

//    if (card->isaps)
//      emu10k1_ecard_init(card);

    return card;

error:
    FreeEMU10k1( card );
  }

  return NULL;
}


void
FreeEMU10k1( struct emu10k1_card* card )
{
  if( card != NULL )
  {
    if( card->pci_dev != NULL )
    {
      emu10k1_cleanup(card);
//      emu10k1_midi_cleanup(card);
//      emu10k1_mixer_cleanup(card);
//      emu10k1_audio_cleanup(card);
      
      pci_disable( card->pci_dev );
      pci_release( card->pci_dev );
    }
  }
}


int
main( void )
{
  if( OpenLibs() )
  {
    struct emu10k1_card* card = AllocEMU10k1( 0 );
    
    if( card == NULL )
    {
      Printf( "No SBLive! card?\n" );
    }
    else
    {
      Printf( "pci_dev: %08lx; iobase: %08lx; iolen: %ld; irq: %ld; "
	      "chip_rev: %ld; model: %04lx\n",
	      (ULONG) card->pci_dev,
	      (ULONG) card->iobase,
	      card->length,
	      card->irq,
	      card->chiprev,
	      card->model );
      
      FreeEMU10k1( card );
    }
  }

  CloseLibs();

  return 0;
}
