
#include <exec/memory.h>

#include <proto/exec.h>
#include <proto/powerpci.h>

#include "linuxsupport.h"

static void*
AllocPages( size_t size, ULONG req )
{
  void* address;

  address = AllocMem( size + PAGE_SIZE - 1, req & ~MEMF_CLEAR );

  if( address != NULL )
  {
    Forbid();
    FreeMem( address, size + PAGE_SIZE - 1 );
    address = AllocAbs( size,
			(void*) ((ULONG) ( address + PAGE_SIZE - 1 )
				 & ~(PAGE_SIZE-1) ) );
    Permit();
  }

  if( address != NULL && ( req & MEMF_CLEAR ) )
  {
    memset( address, 0, size );
  }

  return address;
}

unsigned long
__get_free_page( unsigned int gfp_mask )
{
  return (unsigned long) AllocPages( PAGE_SIZE, MEMF_PUBLIC );
}

void
free_page( unsigned long addr )
{
//  printf( "Freeing page at %08x\n", addr );
  FreeMem( (void*) addr, PAGE_SIZE );
}

void*
pci_alloc_consistent( void* pci_dev, size_t size, dma_addr_t* dma_handle )
{
  void* res;

//  res = pci_alloc_dmamem( pci_dev, size );
  res = (void*) AllocPages( size, MEMF_PUBLIC );
  *dma_handle = (dma_addr_t) pci_virt_to_bus( pci_dev, res );
  return res;
}

void
pci_free_consistent( void* pci_dev, size_t size, void* addr, dma_addr_t dma_handle )
{
//  printf( "Freeing pages (%d bytes) at %08x\n", size, addr );
  FreeMem( addr, size );
//  pci_free_dmamem( pci_dev, addr, size );
}
