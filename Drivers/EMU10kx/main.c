
#include <exec/memory.h>
#include <amithlon/powerpci.h>
#include <pci/powerpci_pci.h>
#include <proto/exec.h>
#include <proto/dos.h>
#include <proto/powerpci.h>
#undef REG    // Be gone!
#undef SAVEDS // Go away!

#include <CompilerSpecific.h>

#include "EMU10kx.h"
#include "version.h"

#include "linuxsupport.h"

#include <errno.h>
#include "8010.h"
#include "hwaccess.h"
#include "voicemgr.h"

const char  LibName[]     = "emu10kx.audio";
const char  LibIdString[] = "emu10kx.audio " VERS "\r\n";
const UWORD LibVersion    = VERSION;
const UWORD LibRevision   = REVISION;



struct Library *ppcibase = NULL;

struct EMU10kx*
AllocEMU10kx( ULONG card_num );

void
FreeEMU10kx( struct EMU10kx* driver_data );

void*
AllocAlignedVec( size_t size, size_t alignment, ULONG req )
{
  void* address;

  /* Add 4 bytes (for FreeVec()) and allocate aligned memory */

  address = AllocMem( size + 4 + alignment - 1, req & ~MEMF_CLEAR );

  if( address != NULL )
  {
    Forbid();
    FreeMem( address, size + 4 + alignment - 1 );
    address = AllocAbs( size + 4,
			(void*) (((ULONG) ( address + 4 + alignment - 1 )
				  / alignment * alignment ) - 4 ) );
    Permit();
  }

  if( address != NULL )
  {
    *(ULONG*) address = size + 4;
    ++address;

    if( req & MEMF_CLEAR )
    {
      memset( address, 0, size );
    }
  }

  return address;
}



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


static ASM INTERRUPT ULONG
EMU10kx_interrupt( REG( a1, struct EMU10kx* dd ) )
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




static u32 samplerate_to_linearpitch(u32 samplingrate)
{
  samplingrate = (samplingrate << 8) / 375;
  return (samplingrate >> 1) + (samplingrate & 1);
}












static void voice_init(struct emu10k1_card *card)
{
	int i;

	for (i = 0; i < NUM_G; i++)
		card->voicetable[i] = VOICE_USAGE_FREE;
}
#ifdef lcs
static void timer_init(struct emu10k1_card *card)
{
	INIT_LIST_HEAD(&card->timers);
	card->timer_delay = TIMER_STOPPED;
	card->timer_lock = SPIN_LOCK_UNLOCKED;
}
#endif

static void addxmgr_init(struct emu10k1_card *card)
{
	u32 count;

	for (count = 0; count < MAXPAGES; count++)
		card->emupagetable[count] = 0;

	/* Mark first page as used */
	/* This page is reserved by the driver */
	card->emupagetable[0] = 0x8001;
	card->emupagetable[1] = MAXPAGES - 1;
}

static void fx_cleanup(struct patch_manager *mgr)
{
	int i;
	for(i = 0; i < mgr->current_pages; i++)
		FreeVec((void*) mgr->patch[i]);
}

static int fx_init(struct emu10k1_card *card)
{
	struct patch_manager *mgr = &card->mgr;
	struct dsp_patch *patch;
	struct dsp_rpatch *rpatch;
	s32 left, right;
	int i;
	u32 pc = 0;
	u32 patch_n;

	for (i = 0; i < SOUND_MIXER_NRDEVICES; i++) {
		mgr->ctrl_gpr[i][0] = -1;
		mgr->ctrl_gpr[i][1] = -1;
	}

	for (i = 0; i < 512; i++)
		OP(6, 0x40, 0x40, 0x40, 0x40);

	for (i = 0; i < 256; i++)
		sblive_writeptr_tag(card, 0,
				    FXGPREGBASE + i, 0,
				    TANKMEMADDRREGBASE + i, 0,
				    TAGLIST_END);

	/* !! The number bellow must equal the number of patches, currently 11 !! */
	mgr->current_pages = (11 + PATCHES_PER_PAGE - 1) / PATCHES_PER_PAGE;
	for (i = 0; i < mgr->current_pages; i++) {
	        mgr->patch[i] = (void *) AllocAlignedVec( PAGE_SIZE,
							  PAGE_SIZE,
							  MEMF_PUBLIC );
		if (mgr->patch[i] == NULL) {
			mgr->current_pages = i;
			fx_cleanup(mgr);
			return -ENOMEM;
		}
		memset(mgr->patch[i], 0, PAGE_SIZE);
	}

	pc = 0;
	patch_n = 0;
	//first free GPR = 0x11b

	/* FX volume correction and Volume control*/
	INPUT_PATCH_START(patch, "Pcm L vol", 0x0, 0);
	GET_OUTPUT_GPR(patch, 0x100, 0x0);
	GET_CONTROL_GPR(patch, 0x106, "Vol", 0, 0x7fffffff);
	GET_DYNAMIC_GPR(patch, 0x112);

	OP(4, 0x112, 0x40, PCM_IN_L, 0x44); //*4
	OP(0, 0x100, 0x040, 0x112, 0x106);  //*vol
	INPUT_PATCH_END(patch);


	INPUT_PATCH_START(patch, "Pcm R vol", 0x1, 0);
	GET_OUTPUT_GPR(patch, 0x101, 0x1);
	GET_CONTROL_GPR(patch, 0x107, "Vol", 0, 0x7fffffff);
	GET_DYNAMIC_GPR(patch, 0x112);

	OP(4, 0x112, 0x40, PCM_IN_R, 0x44); 
	OP(0, 0x101, 0x040, 0x112, 0x107);

	INPUT_PATCH_END(patch);


	// CD-Digital In Volume control
	INPUT_PATCH_START(patch, "CD-Digital Vol L", 0x12, 0);
	GET_OUTPUT_GPR(patch, 0x10c, 0x12);
	GET_CONTROL_GPR(patch, 0x10d, "Vol", 0, 0x7fffffff);

	OP(0, 0x10c, 0x040, SPDIF_CD_L, 0x10d);
	INPUT_PATCH_END(patch);

	INPUT_PATCH_START(patch, "CD-Digital Vol R", 0x13, 0);
	GET_OUTPUT_GPR(patch, 0x10e, 0x13);
	GET_CONTROL_GPR(patch, 0x10f, "Vol", 0, 0x7fffffff);

	OP(0, 0x10e, 0x040, SPDIF_CD_R, 0x10f);
	INPUT_PATCH_END(patch);

	//Volume Correction for Multi-channel Inputs
	INPUT_PATCH_START(patch, "Multi-Channel Gain", 0x08, 0);
	patch->input=patch->output=0x3F00;

	GET_OUTPUT_GPR(patch, 0x113, MULTI_FRONT_L);
	GET_OUTPUT_GPR(patch, 0x114, MULTI_FRONT_R);
	GET_OUTPUT_GPR(patch, 0x115, MULTI_REAR_L);
	GET_OUTPUT_GPR(patch, 0x116, MULTI_REAR_R);
	GET_OUTPUT_GPR(patch, 0x117, MULTI_CENTER);
	GET_OUTPUT_GPR(patch, 0x118, MULTI_LFE);

	OP(4, 0x113, 0x40, MULTI_FRONT_L, 0x44);
	OP(4, 0x114, 0x40, MULTI_FRONT_R, 0x44);
	OP(4, 0x115, 0x40, MULTI_REAR_L, 0x44);
	OP(4, 0x116, 0x40, MULTI_REAR_R, 0x44);
	OP(4, 0x117, 0x40, MULTI_CENTER, 0x44);
	OP(4, 0x118, 0x40, MULTI_LFE, 0x44);
	
	INPUT_PATCH_END(patch);


	//Routing patch start
	ROUTING_PATCH_START(rpatch, "Routing");
	GET_INPUT_GPR(rpatch, 0x100, 0x0);
	GET_INPUT_GPR(rpatch, 0x101, 0x1);
	GET_INPUT_GPR(rpatch, 0x10c, 0x12);
	GET_INPUT_GPR(rpatch, 0x10e, 0x13);
	GET_INPUT_GPR(rpatch, 0x113, MULTI_FRONT_L);
	GET_INPUT_GPR(rpatch, 0x114, MULTI_FRONT_R);
	GET_INPUT_GPR(rpatch, 0x115, MULTI_REAR_L);
	GET_INPUT_GPR(rpatch, 0x116, MULTI_REAR_R);
	GET_INPUT_GPR(rpatch, 0x117, MULTI_CENTER);
	GET_INPUT_GPR(rpatch, 0x118, MULTI_LFE);

	GET_DYNAMIC_GPR(rpatch, 0x102);
	GET_DYNAMIC_GPR(rpatch, 0x103);

	GET_OUTPUT_GPR(rpatch, 0x104, 0x8);
	GET_OUTPUT_GPR(rpatch, 0x105, 0x9);
	GET_OUTPUT_GPR(rpatch, 0x10a, 0x2);
	GET_OUTPUT_GPR(rpatch, 0x10b, 0x3);


	/* input buffer */
	OP(6, 0x102, AC97_IN_L, 0x40, 0x40);
	OP(6, 0x103, AC97_IN_R, 0x40, 0x40);


	/* Digital In + PCM + MULTI_FRONT-> AC97 out (front speakers)*/
	OP(6, AC97_FRONT_L, 0x100, 0x10c, 0x113);

	CONNECT(MULTI_FRONT_L, AC97_FRONT_L);
	CONNECT(PCM_IN_L, AC97_FRONT_L);
	CONNECT(SPDIF_CD_L, AC97_FRONT_L);

	OP(6, AC97_FRONT_R, 0x101, 0x10e, 0x114);

	CONNECT(MULTI_FRONT_R, AC97_FRONT_R);
	CONNECT(PCM_IN_R, AC97_FRONT_R);
	CONNECT(SPDIF_CD_R, AC97_FRONT_R);

	/* Digital In + PCM + AC97 In + PCM1 + MULTI_REAR --> Rear Channel */ 
	OP(6, 0x104, PCM1_IN_L, 0x100, 0x115);
	OP(6, 0x104, 0x104, 0x10c, 0x102);

	CONNECT(MULTI_REAR_L, ANALOG_REAR_L);
	CONNECT(AC97_IN_L, ANALOG_REAR_L);
	CONNECT(PCM_IN_L, ANALOG_REAR_L);
	CONNECT(SPDIF_CD_L, ANALOG_REAR_L);
	CONNECT(PCM1_IN_L, ANALOG_REAR_L);

	OP(6, 0x105, PCM1_IN_R, 0x101, 0x116);
	OP(6, 0x105, 0x105, 0x10e, 0x103);

	CONNECT(MULTI_REAR_R, ANALOG_REAR_R);
	CONNECT(AC97_IN_R, ANALOG_REAR_R);
	CONNECT(PCM_IN_R, ANALOG_REAR_R);
	CONNECT(SPDIF_CD_R, ANALOG_REAR_R);
	CONNECT(PCM1_IN_R, ANALOG_REAR_R);

	/* Digital In + PCM + AC97 In + MULTI_FRONT --> Digital out */
	OP(6, 0x10a, 0x100, 0x102, 0x10c);
	OP(6, 0x10a, 0x10a, 0x113, 0x40);

	CONNECT(MULTI_FRONT_L, DIGITAL_OUT_L);
	CONNECT(PCM_IN_L, DIGITAL_OUT_L);
	CONNECT(AC97_IN_L, DIGITAL_OUT_L);
	CONNECT(SPDIF_CD_L, DIGITAL_OUT_L);

	OP(6, 0x10b, 0x101, 0x103, 0x10e);
	OP(6, 0x10b, 0x10b, 0x114, 0x40);

	CONNECT(MULTI_FRONT_R, DIGITAL_OUT_R);
	CONNECT(PCM_IN_R, DIGITAL_OUT_R);
	CONNECT(AC97_IN_R, DIGITAL_OUT_R);
	CONNECT(SPDIF_CD_R, DIGITAL_OUT_R);

	/* AC97 In --> ADC Recording Buffer */
	OP(6, ADC_REC_L, 0x102, 0x40, 0x40);

	CONNECT(AC97_IN_L, ADC_REC_L);

	OP(6, ADC_REC_R, 0x103, 0x40, 0x40);

	CONNECT(AC97_IN_R, ADC_REC_R);


	/* fx12:Analog-Center */
	OP(6, ANALOG_CENTER, 0x117, 0x40, 0x40);
	CONNECT(MULTI_CENTER, ANALOG_CENTER);

	/* fx11:Analog-LFE */
	OP(6, ANALOG_LFE, 0x118, 0x40, 0x40);
	CONNECT(MULTI_LFE, ANALOG_LFE);

	/* fx12:Digital-Center */
	OP(6, DIGITAL_CENTER, 0x117, 0x40, 0x40);
	CONNECT(MULTI_CENTER, DIGITAL_CENTER);

	/* fx11:Analog-LFE */
	OP(6, DIGITAL_LFE, 0x118, 0x40, 0x40);
	CONNECT(MULTI_LFE, DIGITAL_LFE);
	
	ROUTING_PATCH_END(rpatch);


	// Rear volume control
	OUTPUT_PATCH_START(patch, "Vol Rear L", 0x8, 0);
	GET_INPUT_GPR(patch, 0x104, 0x8);
	GET_CONTROL_GPR(patch, 0x119, "Vol", 0, 0x7fffffff);

	OP(0, ANALOG_REAR_L, 0x040, 0x104, 0x119);
	OUTPUT_PATCH_END(patch);


	OUTPUT_PATCH_START(patch, "Vol Rear R", 0x9, 0);
	GET_INPUT_GPR(patch, 0x105, 0x9);
	GET_CONTROL_GPR(patch, 0x11a, "Vol", 0, 0x7fffffff);

	OP(0, ANALOG_REAR_R, 0x040, 0x105, 0x11a);
	OUTPUT_PATCH_END(patch);


	//Master volume control on front-digital
	OUTPUT_PATCH_START(patch, "Vol Master L", 0x2, 1);
	GET_INPUT_GPR(patch, 0x10a, 0x2);
	GET_CONTROL_GPR(patch, 0x108, "Vol", 0, 0x7fffffff);

	OP(0, DIGITAL_OUT_L, 0x040, 0x10a, 0x108);
	OUTPUT_PATCH_END(patch);


	OUTPUT_PATCH_START(patch, "Vol Master R", 0x3, 1);
	GET_INPUT_GPR(patch, 0x10b, 0x3);
	GET_CONTROL_GPR(patch, 0x109, "Vol", 0, 0x7fffffff);

	OP(0, DIGITAL_OUT_R, 0x040, 0x10b, 0x109);
	OUTPUT_PATCH_END(patch);


	/* delimiter patch */
	patch = PATCH(mgr, patch_n);
	patch->code_size = 0;

	sblive_writeptr(card, DBG, 0, 0);

//	mgr->lock = SPIN_LOCK_UNLOCKED;


	//Master volume
	mgr->ctrl_gpr[SOUND_MIXER_VOLUME][0] = 8;
	mgr->ctrl_gpr[SOUND_MIXER_VOLUME][1] = 9;

//	left = card->ac97.mixer_state[SOUND_MIXER_VOLUME] & 0xff;
//	right = (card->ac97.mixer_state[SOUND_MIXER_VOLUME] >> 8) & 0xff;

	left = right = 0x43;
	
	emu10k1_set_volume_gpr(card, 8, left, 1 << 5);
	emu10k1_set_volume_gpr(card, 9, right, 1 << 5);

	//Rear volume
	mgr->ctrl_gpr[ SOUND_MIXER_OGAIN ][0] = 0x19;
	mgr->ctrl_gpr[ SOUND_MIXER_OGAIN ][1] = 0x1a;

	left = right = 67;
//	card->ac97.mixer_state[SOUND_MIXER_OGAIN] = (right << 8) | left;

//	card->ac97.supported_mixers |= SOUND_MASK_OGAIN;
//	card->ac97.stereo_mixers |= SOUND_MASK_OGAIN;

	emu10k1_set_volume_gpr(card, 0x19, left, VOL_5BIT);
	emu10k1_set_volume_gpr(card, 0x1a, right, VOL_5BIT);

	//PCM Volume
	mgr->ctrl_gpr[SOUND_MIXER_PCM][0] = 6;
	mgr->ctrl_gpr[SOUND_MIXER_PCM][1] = 7;

//	left = card->ac97.mixer_state[SOUND_MIXER_PCM] & 0xff;
//	right = (card->ac97.mixer_state[SOUND_MIXER_PCM] >> 8) & 0xff;

	emu10k1_set_volume_gpr(card, 6, left, VOL_5BIT);
	emu10k1_set_volume_gpr(card, 7, right, VOL_5BIT);

	//CD-Digital Volume
	mgr->ctrl_gpr[SOUND_MIXER_DIGITAL1][0] = 0xd;
	mgr->ctrl_gpr[SOUND_MIXER_DIGITAL1][1] = 0xf;

	left = right = 67;
//	card->ac97.mixer_state[SOUND_MIXER_DIGITAL1] = (right << 8) | left; 

//	card->ac97.supported_mixers |= SOUND_MASK_DIGITAL1;
//	card->ac97.stereo_mixers |= SOUND_MASK_DIGITAL1;

	emu10k1_set_volume_gpr(card, 0xd, left, VOL_5BIT);
	emu10k1_set_volume_gpr(card, 0xf, right, VOL_5BIT);

	//hard wire the ac97's pcm, we'll do that in dsp code instead.
//	emu10k1_ac97_write(&card->ac97, 0x18, 0x0);
//	card->ac97_supported_mixers &= ~SOUND_MASK_PCM;
//	card->ac97_stereo_mixers &= ~SOUND_MASK_PCM;

	//set Igain to 0dB by default, maybe consider hardwiring it here.
//	emu10k1_ac97_write(&card->ac97, AC97_RECORD_GAIN, 0x0000);
//	card->ac97.mixer_state[SOUND_MIXER_IGAIN] = 0x101; 

	return 0;
}

static int hw_init(struct emu10k1_card *card)
{
	int nCh;
	u32 pagecount; /* tmp */
	int ret;

	/* Disable audio and lock cache */
	emu10k1_writefn0(card, HCFG, HCFG_LOCKSOUNDCACHE | HCFG_LOCKTANKCACHE_MASK | HCFG_MUTEBUTTONENABLE);

	/* Reset recording buffers */
	sblive_writeptr_tag(card, 0,
			    MICBS, ADCBS_BUFSIZE_NONE,
			    MICBA, 0,
			    FXBS, ADCBS_BUFSIZE_NONE,
			    FXBA, 0,
			    ADCBS, ADCBS_BUFSIZE_NONE,
			    ADCBA, 0,
			    TAGLIST_END);

	/* Disable channel interrupt */
	emu10k1_writefn0(card, INTE, 0);
	sblive_writeptr_tag(card, 0,
			    CLIEL, 0,
			    CLIEH, 0,
			    SOLEL, 0,
			    SOLEH, 0,
			    TAGLIST_END);

	/* Init envelope engine */
	for (nCh = 0; nCh < NUM_G; nCh++) {
		sblive_writeptr_tag(card, nCh,
				    DCYSUSV, 0,
				    IP, 0,
				    VTFT, 0xffff,
				    CVCF, 0xffff,
				    PTRX, 0,
				    //CPF, 0,
				    CCR, 0,

				    PSST, 0,
				    DSL, 0x10,
				    CCCA, 0,
				    Z1, 0,
				    Z2, 0,
				    FXRT, 0xd01c0000,

				    ATKHLDM, 0,
				    DCYSUSM, 0,
				    IFATN, 0xffff,
				    PEFE, 0,
				    FMMOD, 0,
				    TREMFRQ, 24,	/* 1 Hz */
				    FM2FRQ2, 24,	/* 1 Hz */
				    TEMPENV, 0,

				    /*** These are last so OFF prevents writing ***/
				    LFOVAL2, 0,
				    LFOVAL1, 0,
				    ATKHLDV, 0,
				    ENVVOL, 0,
				    ENVVAL, 0,
                                    TAGLIST_END);
		sblive_writeptr(card, CPF, nCh, 0);
	}
	

	/*
	 ** Init to 0x02109204 :
	 ** Clock accuracy    = 0     (1000ppm)
	 ** Sample Rate       = 2     (48kHz)
	 ** Audio Channel     = 1     (Left of 2)
	 ** Source Number     = 0     (Unspecified)
	 ** Generation Status = 1     (Original for Cat Code 12)
	 ** Cat Code          = 12    (Digital Signal Mixer)
	 ** Mode              = 0     (Mode 0)
	 ** Emphasis          = 0     (None)
	 ** CP                = 1     (Copyright unasserted)
	 ** AN                = 0     (Digital audio)
	 ** P                 = 0     (Consumer)
	 */

	sblive_writeptr_tag(card, 0,

			    /* SPDIF0 */
			    SPCS0, (SPCS_CLKACCY_1000PPM | 0x002000000 |
				    SPCS_CHANNELNUM_LEFT | SPCS_SOURCENUM_UNSPEC | SPCS_GENERATIONSTATUS | 0x00001200 | SPCS_EMPHASIS_NONE | SPCS_COPYRIGHT),

			    /* SPDIF1 */
			    SPCS1, (SPCS_CLKACCY_1000PPM | 0x002000000 |
				    SPCS_CHANNELNUM_LEFT | SPCS_SOURCENUM_UNSPEC | SPCS_GENERATIONSTATUS | 0x00001200 | SPCS_EMPHASIS_NONE | SPCS_COPYRIGHT),

			    /* SPDIF2 & SPDIF3 */
			    SPCS2, (SPCS_CLKACCY_1000PPM | 0x002000000 |
				    SPCS_CHANNELNUM_LEFT | SPCS_SOURCENUM_UNSPEC | SPCS_GENERATIONSTATUS | 0x00001200 | SPCS_EMPHASIS_NONE | SPCS_COPYRIGHT),

			    TAGLIST_END);

	ret = fx_init(card);		/* initialize effects engine */
	if (ret < 0)
		return ret;

	card->tankmem.size = 0;

	card->virtualpagetable.size = MAXPAGES * sizeof(u32);

	card->virtualpagetable.addr = pci_alloc_consistent(card->pci_dev, card->virtualpagetable.size, &card->virtualpagetable.dma_handle);
	if (card->virtualpagetable.addr == NULL) {
		ERROR();
		ret = -ENOMEM;
		goto err0;
	}

	card->silentpage.size = EMUPAGESIZE;

	card->silentpage.addr = pci_alloc_consistent(card->pci_dev, card->silentpage.size, &card->silentpage.dma_handle);
	if (card->silentpage.addr == NULL) {
		ERROR();
		ret = -ENOMEM;
		goto err1;
	}

	for (pagecount = 0; pagecount < MAXPAGES; pagecount++)
		((u32 *) card->virtualpagetable.addr)[pagecount] = cpu_to_le32((card->silentpage.dma_handle * 2) | pagecount);

	/* Init page table & tank memory base register */
	sblive_writeptr_tag(card, 0,
			    PTB, card->virtualpagetable.dma_handle,
			    TCB, 0,
			    TCBS, 0,
			    TAGLIST_END);

	for (nCh = 0; nCh < NUM_G; nCh++) {
		sblive_writeptr_tag(card, nCh,
				    MAPA, MAP_PTI_MASK | (card->silentpage.dma_handle * 2),
				    MAPB, MAP_PTI_MASK | (card->silentpage.dma_handle * 2),
				    TAGLIST_END);
	}

	/* Hokay, now enable the AUD bit */
	/* Enable Audio = 1 */
	/* Mute Disable Audio = 0 */
	/* Lock Tank Memory = 1 */
	/* Lock Sound Memory = 0 */
	/* Auto Mute = 1 */

	if (card->model == 0x20 || card->model == 0xc400 ||
	  (card->model == 0x21 && card->chiprev < 6))
	        emu10k1_writefn0(card, HCFG, HCFG_AUDIOENABLE  | HCFG_LOCKTANKCACHE_MASK | HCFG_AUTOMUTE);
	else
		emu10k1_writefn0(card, HCFG, HCFG_AUDIOENABLE  | HCFG_LOCKTANKCACHE_MASK | HCFG_AUTOMUTE | HCFG_JOYENABLE);

	/* Enable Vol_Ctrl irqs */
//	emu10k1_irq_enable(card, INTE_VOLINCRENABLE | INTE_VOLDECRENABLE | INTE_MUTEENABLE | INTE_FXDSPENABLE);

	/* FIXME: TOSLink detection */
	card->has_toslink = 0;
#ifdef lcs
	/* Initialize digital passthrough variables */
	card->pt.pos_gpr = card->pt.intr_gpr = card->pt.enable_gpr = -1;
	card->pt.selected = 0;
	card->pt.state = PT_STATE_INACTIVE;
	card->pt.spcs_to_use = 0x01;
	card->pt.patch_name = "AC3pass";
	card->pt.intr_gpr_name = "count";
	card->pt.enable_gpr_name = "enable";
	card->pt.pos_gpr_name = "ptr";
	spin_lock_init(&card->pt.lock);
	init_waitqueue_head(&card->pt.wait);
#endif
/*	tmp = sblive_readfn0(card, HCFG);
	if (tmp & (HCFG_GPINPUT0 | HCFG_GPINPUT1)) {
		sblive_writefn0(card, HCFG, tmp | 0x800);

		udelay(512);

		if (tmp != (sblive_readfn0(card, HCFG) & ~0x800)) {
			card->has_toslink = 1;
			sblive_writefn0(card, HCFG, tmp);
		}
	}
*/
	return 0;

  err1:
	pci_free_consistent(card->pci_dev, card->virtualpagetable.size, card->virtualpagetable.addr, card->virtualpagetable.dma_handle);
  err0:
	fx_cleanup(&card->mgr);

	return ret;
}

static int emu10k1_init(struct emu10k1_card *card)
{
	/* Init Card */
	if (hw_init(card) < 0)
		return -1;

	voice_init(card);
//	timer_init(card);
	addxmgr_init(card);

	DPD(2, "  hw control register -> %#x\n", emu10k1_readfn0(card, HCFG));

	return 0;
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


  pci_free_consistent(card->pci_dev, card->virtualpagetable.size, card->virtualpagetable.addr, card->virtualpagetable.dma_handle);
  pci_free_consistent(card->pci_dev, card->silentpage.size, card->silentpage.addr, card->silentpage.dma_handle);
  
  if(card->tankmem.size != 0)
    pci_free_consistent(card->pci_dev, card->tankmem.size, card->tankmem.addr, card->tankmem.dma_handle);

  /* release patch storage memory */
  fx_cleanup(&card->mgr);
}










struct EMU10kx*
AllocEMU10kx( ULONG card_num )
{
  struct EMU10kx*  dd;
  UWORD            command_word;
  int              ret;

  dd = AllocVec( sizeof( *dd ), MEMF_PUBLIC | MEMF_CLEAR );

  if( dd == NULL )
  {
    Printf( "Unable to allocate driver structure.\n " );
  }
  else
  {
    dd->interrupt.is_Node.ln_Type = NT_INTERRUPT;
    dd->interrupt.is_Node.ln_Pri  = 0;
    dd->interrupt.is_Node.ln_Name = (STRPTR) LibName;
    dd->interrupt.is_Code         = (void(*)(void)) EMU10kx_interrupt;
    dd->interrupt.is_Data         = (APTR) dd;
    
    dd->card.pci_dev = 0;

    do
    {
      dd->card.pci_dev = pci_find_device( PCI_VENDOR_ID_CREATIVE,
						   PCI_DEVICE_ID_CREATIVE_EMU10K1,
						   dd->card.pci_dev );
    } while( dd->card.pci_dev != 0 && card_num-- != 0 );

    if( dd->card.pci_dev == NULL )
    {
      Printf( "Unable to find EMU10k subsystem.\n" );
      goto error;
    }

//  if( pci_set_dma_mask(dd->card.pci_dev, EMU10K1_DMA_MASK) )
//  {
//    Printf( "Unable to set DMA mask for card\n." );
//    goto error;
//  }

    if( pci_request( dd->card.pci_dev, (STRPTR) LibName, NULL ) )
    {
      Printf( "Unable to claim I/O resources.\n" );
      goto error;
    }

    if( pci_enable( dd->card.pci_dev ) )
    {
      Printf( "Unable to enable card.\n" );
      goto error;
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
    dd->card.isaps   = ( pci_read_conf_long( dd->card.pci_dev, PCI_SUBSYSTEM_VENDOR_ID )
		      == EMU_APS_SUBID );

    pci_add_irq( dd->card.pci_dev, &dd->interrupt );
    dd->interrupt_added = TRUE;

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

    ret = emu10k1_init(&dd->card);
    if (ret < 0) {
      Printf( "emu10k1: cannot initialize device\n");
      goto error;
    }

//    if (dd->card.isaps)
//      emu10k1_ecard_init(card);

    return dd;

error:
    FreeEMU10kx( dd );
  }

  return NULL;
}


void
FreeEMU10kx( struct EMU10kx* dd )
{
  if( dd != NULL )
  {
    if( dd->card.pci_dev != NULL )
    {
      emu10k1_cleanup( &dd->card );
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
  }
}


int
main( void )
{
  if( OpenLibs() )
  {
    struct EMU10kx* dd = AllocEMU10kx( 0 );
    
    if( dd == NULL )
    {
      Printf( "No SBLive! card?\n" );
    }
    else
    {
      printf( "pci_dev: %08x; iobase: %08x; iolen: %d; irq: %d; "
	      "chip_rev: %d; model: %04x\n",
	      (ULONG) dd->card.pci_dev,
	      (ULONG) dd->card.iobase,
	      dd->card.length,
	      dd->card.irq,
	      dd->card.chiprev,
	      dd->card.model );

      Delay( 50 );

      printf( "Starting sound.\n" );

      {
	struct emu_voice voice;

	if( emu10k1_voice_alloc_buffer( &dd->card,
					&voice.mem,
					4096 / PAGE_SIZE ) < 0 )
	{
	  printf( "Unable to allocate voice buffer" );
	}
	else
	{
	  voice.usage = VOICE_USAGE_PLAYBACK;
	  voice.flags = VOICE_FLAGS_STEREO | VOICE_FLAGS_16BIT;

	  if( emu10k1_voice_alloc( &dd->card, &voice ) < 0 )
	  {
	    printf( "Unable to allocate voice.\n" );
	  }
	  else
	  {
	    voice.initial_pitch = (u16) ( srToPitch( 17640 ) >> 8 );
	    voice.pitch_target  = samplerate_to_linearpitch( 17640 );

	    DPD(2, "Initial pitch --> %#x\n", voice.initial_pitch);

	    voice.startloop = (voice.mem.emupageindex << 12) / 4; // bytespervoicesample
	    voice.endloop = voice.startloop + 4096 / 4;
	    voice.start = voice.startloop;

	    if( voice.flags & VOICE_FLAGS_STEREO )
	    {
	      voice.params[0].send_a             = 0xff;
	      voice.params[0].send_b             = 0x0;
	      voice.params[0].send_c             = 0x0;
	      voice.params[0].send_d             = 0x0;
	      voice.params[0].send_routing       = 0x3210;
	      voice.params[0].volume_target      = 0xffff;
	      voice.params[0].initial_fc         = 0xff;
	      voice.params[0].initial_attn       = 0x00;
	      voice.params[0].byampl_env_sustain = 0x7f;
	      voice.params[0].byampl_env_decay   = 0x7f;

	      voice.params[1].send_a             = 0x00;
	      voice.params[1].send_b             = 0xff;
	      voice.params[1].send_c             = 0x00;
	      voice.params[1].send_d             = 0x00;
	      voice.params[1].send_routing       = 0x3210;
	      voice.params[1].volume_target      = 0xffff;
	      voice.params[1].initial_fc         = 0xff;
	      voice.params[1].initial_attn       = 0x00;
	      voice.params[1].byampl_env_sustain = 0x7f;
	      voice.params[1].byampl_env_decay   = 0x7f;
	    }
	    else
	    {
	      voice.params[0].send_a             = 0xff;
	      voice.params[0].send_b             = 0xff;
	      voice.params[0].send_c             = 0x0;
	      voice.params[0].send_d             = 0x0;
	      voice.params[0].send_routing       = 0x3210;
	      voice.params[0].volume_target      = 0xffff;
	      voice.params[0].initial_fc         = 0xff;
	      voice.params[0].initial_attn       = 0x00;
	      voice.params[0].byampl_env_sustain = 0x7f;
	      voice.params[0].byampl_env_decay   = 0x7f;
	    }

	    DPD(2, "voice: startloop=%x, endloop=%x\n",
		voice.startloop, voice.endloop);
	    
	    emu10k1_voice_playback_setup( &voice );

	    DPF(2, "emu10k1_waveout_start()\n");

//	    emu10k1_voices_start( &voice, 1, 0 );
	    
	    Delay( 50 );

//	    emu10k1_voices_stop(&voice, 1);

	    emu10k1_voice_free( &voice );
	  }

	  emu10k1_voice_free_buffer( &dd->card, &voice.mem );
	}
      }
     
      
      Delay( 50 );
      
      FreeEMU10kx( dd );
    }
  }

  CloseLibs();

  return 0;
}
