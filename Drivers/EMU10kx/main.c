
#include <amithlon/powerpci.h>
#include <exec/memory.h>

#include <proto/exec.h>
#include <proto/dos.h>
#include <proto/powerpci.h>

#include "EMU10k1.h"

struct Library *ppcibase = NULL;


struct EMU10k1*
AllocEMU10k1( ULONG card_num );

void
FreeEMU10k1( struct EMU10k1* card );


  
#define PCI_ANY_ID                     0xffff
#define PCI_VENDOR_ID_CREATIVE         0x1102
#define PCI_DEVICE_ID_CREATIVE_EMU10K1 0x0002
#define PCI_REVISION_ID                0x08
#define PCI_SUBSYSTEM_ID               0x2e

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


struct EMU10k1*
AllocEMU10k1( ULONG card_num )
{
  struct EMU10k1* card;

  card = AllocVec( sizeof( *card ), MEMF_PUBLIC | MEMF_CLEAR );

  if( card == NULL )
  {
    Printf( "Unable to allocate card structure.\n " );
  }
  else
  {
    card->m_PCIDev = 0;

    do
    {
      card->m_PCIDev = pci_find_device( PCI_VENDOR_ID_CREATIVE,
				     PCI_DEVICE_ID_CREATIVE_EMU10K1,
				     card->m_PCIDev );
    } while( card->m_PCIDev != 0 && card_num-- != 0 );

    if( card->m_PCIDev == NULL )
    {
      Printf( "Unable to find EMU10k subsystem.\n" );
      goto error;
    }

//  if( pci_set_dma_mask(card->m_PCIDev, EMU10K1_DMA_MASK) )
//  {
//    Printf( "Unable to set DMA mask for card\n." );
//    goto error;
//  }

    if( pci_enable( card->m_PCIDev ) )
    {
      Printf( "Unable to enable card.\n" );
      goto error;
    }

    if( pci_request( card->m_PCIDev, "EMU10k1 AHI driver", NULL ) )
    {
      Printf( "Unable to claim I/O resources.\n" );
      goto error;
    }

#if 0
    card->m_IOBase   = pci_get_base_start( card->m_PCIDev, 0 );
    card->m_IOLength = ( (ULONG) pci_get_base_end( card->m_PCIDev, 0 ) -
			 (ULONG) card->m_IOBase ) + 1;
#endif
    card->m_IRQ      = pci_ask_irq( card->m_PCIDev ) >> 8;
    
    card->m_ChipRev  = pci_read_conf_byte( card->m_PCIDev, PCI_REVISION_ID );
    card->m_Model    = pci_read_conf_word( card->m_PCIDev, PCI_SUBSYSTEM_ID );

    
    return card;

error:
    FreeEMU10k1( card );
  }

  return NULL;
}


void
FreeEMU10k1( struct EMU10k1* card )
{
  if( card != NULL )
  {
    if( card->m_PCIDev != NULL )
    {
      pci_release( card->m_PCIDev );
    }
  }
}


int
main( void )
{
  if( OpenLibs() )
  {
    struct EMU10k1* card = AllocEMU10k1( 0 );
    
    if( card == NULL )
    {
      Printf( "No SBLive! card?\n" );
    }
    else
    {
      Printf( "pci_dev: %08lx; iobase: %08lx; iolen: %ld; irq: %ld; "
	      "chip_rev: %ld; model: %04lx\n",
	      card->m_PCIDev,
	      card->m_IOBase,
	      card->m_IOLength,
	      card->m_IRQ,
	      card->m_ChipRev,
	      card->m_Model );
      
      FreeEMU10k1( card );
    }
  }

  CloseLibs();

  return 0;
}
