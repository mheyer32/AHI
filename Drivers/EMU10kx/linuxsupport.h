
#ifndef EMU10kx_pcisupport_h
#define EMU10kx_pcisupport_h

#include <stdio.h>
#include <stdlib.h>
#include <proto/exec.h>
#include <proto/dos.h>

typedef unsigned int   u32;
typedef unsigned short u16;
typedef unsigned char  u8;

typedef signed int     s32;
typedef signed short   s16;
typedef signed char    s8;

typedef unsigned int   dma_addr_t;

#define spin_lock_irqsave(lock,flags) Disable();
#define spin_unlock_irqrestore(lock,flags) Enable();

#define PAGE_SIZE      4096
#define KERN_NOTICE    ""
#define printk         printf

#define PCI_ANY_ID                     0xffff
#define PCI_VENDOR_ID_CREATIVE         0x1102
#define PCI_DEVICE_ID_CREATIVE_EMU10K1 0x0002
#define EMU_APS_SUBID                  0x40011102

#define SOUND_MIXER_NRDEVICES 25
#define SOUND_MIXER_VOLUME    0
#define SOUND_MIXER_PCM       4
#define SOUND_MIXER_IGAIN    12
#define SOUND_MIXER_OGAIN    13
#define SOUND_MIXER_DIGITAL1 17

#define SOUND_MASK_OGAIN     (1 << SOUND_MIXER_OGAIN)
#define SOUND_MASK_DIGITAL1  (1 << SOUND_MIXER_DIGITAL1)

void*
pci_alloc_consistent( void* pci_dev, size_t size, dma_addr_t* dma_handle );

void
pci_free_consistent( void* pci_dev, size_t size, void* arrd, dma_addr_t dma_handle );


static __inline__ int test_bit(int nr, const volatile void * addr)
{
  return ((1UL << (nr & 31)) & (((const volatile unsigned int *) addr)[nr >> 5])) != 0;
}

static __inline__ void set_bit(int nr, volatile void * addr)
{
  (((volatile unsigned int *) addr)[nr >> 5]) |= (1UL << (nr & 31));
}

static __inline__ u32 cpu_to_le32( u32 x )
{
  return ((((x) & 0xff000000) >> 24) | (((x) & 0x00ff0000) >>  8) | \
	  (((x) & 0x0000ff00) <<  8) | (((x) & 0x000000ff) << 24));
}

#endif /* EMU10kx_pcisupport_h */
