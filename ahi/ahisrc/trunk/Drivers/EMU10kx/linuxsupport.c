
#include <proto/powerpci.h>

#include "linuxsupport.h"

void*
pci_alloc_consistent( void* pci_dev, size_t size, dma_addr_t* dma_handle )
{
  void* res;

  res = pci_alloc_dmamem( pci_dev, size );
  *dma_handle = (dma_addr_t) pci_virt_to_bus( pci_dev, res );
  return res;
}

void
pci_free_consistent( void* pci_dev, size_t size, void* addr, dma_addr_t dma_handle )
{
  pci_free_dmamem( pci_dev, addr, size );
}
