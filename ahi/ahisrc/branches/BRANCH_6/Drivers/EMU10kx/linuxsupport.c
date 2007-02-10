/*
     emu10kx.audio - AHI driver for SoundBlaster Live! series
     Copyright (C) 2002-2005 Martin Blom <martin@blom.org>
     
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
#include <utility/hooks.h>

#include <proto/exec.h>

#include "linuxsupport.h"
#include <string.h>

#include "emu10kx-misc.h"
#include "pci_wrapper.h"

struct page_header {
    APTR  address;
    ULONG size;
};

static APTR
AllocPages( size_t size, ULONG req )
{
  size_t alignment = PAGE_SIZE;
  size_t extra = sizeof (struct page_header) + alignment;

  APTR address = ahi_pci_allocdma_mem( size + extra, req );

  if( address != NULL )
  {
    unsigned long a = (unsigned long) address;

    // get a 4K-aligned memory pointer
    a = (a + extra - 1) & ~(PAGE_SIZE - 1); 

    ((struct page_header*) a)[-1].address = address;
    ((struct page_header*) a)[-1].size    = size + extra;

    address = (void*) a;
  }

  return address;
}

static void
FreePages( APTR address )
{
  if( address != NULL ) 
  {
    struct page_header* ph = ((struct page_header*) address - 1);

    ahi_pci_freedma_mem( ph->address, ph->size );
  }
}

unsigned long
__get_free_page( unsigned int gfp_mask )
{
  return (unsigned long) AllocPages( PAGE_SIZE, MEMF_PUBLIC );
}

void
free_page( unsigned long addr )
{
  FreePages( (APTR) addr );
}

void*
pci_alloc_consistent( void* pci_dev, size_t size, dma_addr_t* dma_handle )
{
  void* res;

  res = (void*) AllocPages( size, MEMF_PUBLIC | MEMF_CLEAR );

  *dma_handle = (dma_addr_t) ahi_pci_logic_to_physic_addr( res, pci_dev );
  
  return res;
}

void
pci_free_consistent( void* pci_dev, size_t size, void* addr, dma_addr_t dma_handle )
{
  FreePages( addr );
}
