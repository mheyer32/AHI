/* $Id$ */

#ifndef _EFFECTINIT_H_
#define _EFFECTINIT_H_

#include <config.h>
#include <CompilerSpecific.h>
#include "ahi_def.h"

BOOL 
update_MasterVolume ( struct AHIPrivAudioCtrl *audioctrl );

BOOL
update_DSPEcho ( struct AHIEffDSPEcho *echo,
                 struct AHIPrivAudioCtrl *audioctrl );

void
free_DSPEcho ( struct AHIPrivAudioCtrl *audioctrl );

BOOL 
update_DSPMask ( struct AHIEffDSPMask *mask,
                 struct AHIPrivAudioCtrl *audioctrl );

void
clear_DSPMask ( struct AHIPrivAudioCtrl *audioctrl );

#endif /* _EFFECTINIT_H_ */
