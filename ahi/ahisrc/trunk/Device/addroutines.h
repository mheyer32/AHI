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

#ifndef _ADDROUTINES_H_
#define _ADDROUTINES_H_

#include <config.h>
#include <CompilerSpecific.h>
#include "ahi_def.h"
#include "mixer.h"

LONG AddByteMVH( ADDARGS );
LONG AddByteSVPH( ADDARGS );
LONG AddBytesMVH( ADDARGS );
LONG AddBytesSVPH( ADDARGS );
LONG AddWordMVH( ADDARGS );
LONG AddWordSVPH( ADDARGS );
LONG AddWordsMVH( ADDARGS );
LONG AddWordsSVPH( ADDARGS );

LONG AddByteMVHB( ADDARGS );
LONG AddByteSVPHB( ADDARGS );
LONG AddBytesMVHB( ADDARGS );
LONG AddBytesSVPHB( ADDARGS );
LONG AddWordMVHB( ADDARGS );
LONG AddWordSVPHB( ADDARGS );
LONG AddWordsMVHB( ADDARGS );
LONG AddWordsSVPHB( ADDARGS );

#endif /* _ADDROUTINES_H_ */
