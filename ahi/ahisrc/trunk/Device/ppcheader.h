/* $Id$ */

/*
     AHI - Hardware independent audio subsystem
     Copyright (C) 1996-1999 Martin Blom <martin@blom.org>
     
     This library is free software; you can redistribute it and/or
     modify it under the terms of the GNU Library General Public
     License as published by the Free Software Foundation; either
     version 2 of the License, or (at your option) any later version.
     
     This library is distributed in the hope that it will be useful,
     but WITHOUT ANY WARRANTY; without even the implied warranty of
     MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
     Library General Public License for more details.
     
     You should have received a copy of the GNU Library General Public
     License along with this library; if not, write to the
     Free Software Foundation, Inc., 59 Temple Place - Suite 330, Cambridge,
     MA 02139, USA.
*/

#ifndef _PPCHEADER_H_
#define _PPCHEADER_H_

#include <powerpc/powerpc.h>

#include "ahi_def.h"

#define WARPUP_INVALIDATE_CACHE
#define POWERUP_USE_MIXTASK

struct WarpUpContext
{
  int                 Active;
  struct AudioCtrl*   AudioCtrl;
  struct Library*     PowerPCBase;
  void*               XLock;
  struct Hook*        Hook;
  void*	              Dst;
#ifndef WARPUP_INVALIDATE_CACHE
  void*               MixBuffer;
  int                 MixLongWords;
#endif
};

struct PowerUpContext
{
  void*               Port;
  void*               Msg;
  void*               Task;
  struct AudioCtrl*   AudioCtrl;
  struct Hook*        Hook;
  void*	              Dst;
};

#endif /* _PPCHEADER_H_ */
