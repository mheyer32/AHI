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

#include <config.h>
#include <CompilerSpecific.h>
#include "addroutines.h"

/******************************************************************************
** Add-Routines ***************************************************************
******************************************************************************/

/*
** LONG      Samples
** Fixed     ScaleLeft
** Fixed     ScaleRight (Not used in all routines)
** Fixed64  *Offset
** Fixed64   Add
** struct    AHIPrivAudioCtrl *audioctrl
** void     *Src
** void    **Dst
** struct    AHIChannelData *cd
*/

/*

Notes:

The fraction offset is divided by two in order to make sure that the
calculation of linearsample fits a LONG (0 =< offsetf <= 32767).

The routines could be faster, of course.  One idea is to split the for loop
into two loops in order to eliminate the FirstOffsetI test in the second loop.

*/ 

/*****************************************************************************/

/* Forward mixing code */

#define offseti ( (long) ( offset >> 32 ) )

#define offsetf ( (long) ( (unsigned long) ( offset & 0xffffffffULL ) >> 17) )

LONG
AddByteMVH( ADDARGS )
{
  BYTE    *src    = Src;
  LONG    *dst    = *Dst;
  Fixed64  offset = *Offset;
  int      i;
  LONG     startpoint, endpoint;
  LONG     lastpoint;

  endpoint  = cd->cd_TempStartPointL;
  lastpoint = 0;                      // 0 doesn't affect the StopAtZero code

  for( i = 0; i < Samples; i++)
  {
    if( offseti == cd->cd_FirstOffsetI) {
      startpoint = cd->cd_StartPointL;
    }
    else
    {
      startpoint = src[ offseti - 1 ] << 8;
    }

    endpoint = src[ offseti ] << 8;

    startpoint += (((endpoint - startpoint) * offsetf ) >> 15);

    if( StopAtZero &&
        ( ( lastpoint < 0 && startpoint >= 0 ) ||
          ( lastpoint > 0 && startpoint <= 0 ) ) )
    {
      break;
    }

    lastpoint = startpoint;

    *dst++ += ScaleLeft * startpoint;

    offset += Add;
  }

  cd->cd_TempStartPointL = endpoint;

  *Dst    = dst;
  *Offset = offset;

  return i;
}


LONG
AddByteSVPH( ADDARGS )
{
  BYTE    *src    = Src;
  LONG    *dst    = *Dst;
  Fixed64  offset = *Offset;
  int      i;
  LONG     startpoint, endpoint;
  LONG     lastpoint;

  endpoint  = cd->cd_TempStartPointL;
  lastpoint = 0;                      // 0 doesn't affect the StopAtZero code
  
  for( i = 0; i < Samples; i++)
  {
    if( offseti == cd->cd_FirstOffsetI) {
      startpoint = cd->cd_StartPointL;
    }
    else
    {
      startpoint = src[ offseti - 1 ] << 8;
    }

    endpoint = src[ offseti ] << 8;

    startpoint += (((endpoint - startpoint) * offsetf ) >> 15);

    if( StopAtZero &&
        ( ( lastpoint < 0 && startpoint >= 0 ) ||
          ( lastpoint > 0 && startpoint <= 0 ) ) )
    {
      break;
    }

    lastpoint = startpoint;

    *dst++ += ScaleLeft * startpoint;
    *dst++ += ScaleRight * startpoint;

    offset += Add;
  }

  cd->cd_TempStartPointL = endpoint;

  *Dst    = dst;
  *Offset = offset;

  return i;
}


LONG
AddBytesMVH( ADDARGS )
{
  BYTE    *src    = Src;
  LONG    *dst    = *Dst;
  Fixed64  offset = *Offset;
  int      i;
  LONG     startpointL, startpointR, endpointL, endpointR;
  LONG     lastpointL, lastpointR;

  endpointL  = cd->cd_TempStartPointL;
  endpointR  = cd->cd_TempStartPointR;
  lastpointL = lastpointR = 0;        // 0 doesn't affect the StopAtZero code

  for( i = 0; i < Samples; i++)
  {
    if( offseti == cd->cd_FirstOffsetI) {
      startpointL = cd->cd_StartPointL;
      startpointR = cd->cd_StartPointR;
    }
    else
    {
      startpointL = src[ offseti * 2 + 0 - 2 ] << 8;
      startpointR = src[ offseti * 2 + 1 - 2 ] << 8;
    }

    endpointL = src[ offseti * 2 + 0 ] << 8;
    endpointR = src[ offseti * 2 + 1 ] << 8;

    startpointL += (((endpointL - startpointL) * offsetf ) >> 15);
    startpointR += (((endpointR - startpointR) * offsetf ) >> 15);

    if( StopAtZero &&
        ( ( lastpointL < 0 && startpointL >= 0 ) ||
          ( lastpointR < 0 && startpointR >= 0 ) ||
          ( lastpointL > 0 && startpointL <= 0 ) ||
          ( lastpointR > 0 && startpointR <= 0 ) ) )
    {
      break;
    }

    lastpointL = startpointL;
    lastpointR = startpointR;

    *dst++ += ScaleLeft * startpointL + ScaleRight * startpointR;

    offset += Add;
  }

  cd->cd_TempStartPointL = endpointL;
  cd->cd_TempStartPointR = endpointR;

  *Dst    = dst;
  *Offset = offset;

  return i;
}


LONG
AddBytesSVPH( ADDARGS )
{
  BYTE    *src    = Src;
  LONG    *dst    = *Dst;
  Fixed64  offset = *Offset;
  int      i;
  LONG     startpointL, startpointR, endpointL, endpointR;
  LONG     lastpointL, lastpointR;

  endpointL  = cd->cd_TempStartPointL;
  endpointR  = cd->cd_TempStartPointR;
  lastpointL = lastpointR = 0;        // 0 doesn't affect the StopAtZero code

  for( i = 0; i < Samples; i++)
  {
    if( offseti == cd->cd_FirstOffsetI) {
      startpointL = cd->cd_StartPointL;
      startpointR = cd->cd_StartPointR;
    }
    else
    {
      startpointL = src[ offseti * 2 + 0 - 2 ] << 8;
      startpointR = src[ offseti * 2 + 1 - 2 ] << 8;
    }

    endpointL = src[ offseti * 2 + 0 ] << 8;
    endpointR = src[ offseti * 2 + 1 ] << 8;

    startpointL += (((endpointL - startpointL) * offsetf ) >> 15);
    startpointR += (((endpointR - startpointR) * offsetf ) >> 15);

    if( StopAtZero &&
        ( ( lastpointL < 0 && startpointL >= 0 ) ||
          ( lastpointR < 0 && startpointR >= 0 ) ||
          ( lastpointL > 0 && startpointL <= 0 ) ||
          ( lastpointR > 0 && startpointR <= 0 ) ) )
    {
      break;
    }

    lastpointL = startpointL;
    lastpointR = startpointR;

    *dst++ += ScaleLeft * startpointL;
    *dst++ += ScaleRight * startpointR;

    offset += Add;
  }

  cd->cd_TempStartPointL = endpointL;
  cd->cd_TempStartPointR = endpointR;

  *Dst    = dst;
  *Offset = offset;

  return i;
}


LONG
AddWordMVH( ADDARGS )
{
  WORD    *src    = Src;
  LONG    *dst    = *Dst;
  Fixed64  offset = *Offset;
  int      i;
  LONG     startpoint, endpoint;
  LONG     lastpoint;

  endpoint  = cd->cd_TempStartPointL;
  lastpoint = 0;                      // 0 doesn't affect the StopAtZero code

  for( i = 0; i < Samples; i++)
  {
    if( offseti == cd->cd_FirstOffsetI) {
      startpoint = cd->cd_StartPointL;
    }
    else
    {
      startpoint = src[ offseti - 1 ];
    }

    endpoint = src[ offseti ];

    startpoint += (((endpoint - startpoint) * offsetf ) >> 15);

    if( StopAtZero &&
        ( ( lastpoint < 0 && startpoint >= 0 ) ||
          ( lastpoint > 0 && startpoint <= 0 ) ) )
    {
      break;
    }

    lastpoint = startpoint;

    *dst++ += ScaleLeft * startpoint;

    offset += Add;
  }

  cd->cd_TempStartPointL = endpoint;

  *Dst    = dst;
  *Offset = offset;

  return i;
}


LONG
AddWordSVPH( ADDARGS )
{
  WORD    *src    = Src;
  LONG    *dst    = *Dst;
  Fixed64  offset = *Offset;
  int      i;
  LONG     startpoint, endpoint;
  LONG     lastpoint;

  endpoint  = cd->cd_TempStartPointL;
  lastpoint = 0;                      // 0 doesn't affect the StopAtZero code
  
  for( i = 0; i < Samples; i++)
  {
    if( offseti == cd->cd_FirstOffsetI) {
      startpoint = cd->cd_StartPointL;
    }
    else
    {
      startpoint = src[ offseti - 1 ];
    }

    endpoint = src[ offseti ];

    startpoint += (((endpoint - startpoint) * offsetf ) >> 15);

    if( StopAtZero &&
        ( ( lastpoint < 0 && startpoint >= 0 ) ||
          ( lastpoint > 0 && startpoint <= 0 ) ) )
    {
      break;
    }

    lastpoint = startpoint;

    *dst++ += ScaleLeft * startpoint;
    *dst++ += ScaleRight * startpoint;

    offset += Add;
  }

  cd->cd_TempStartPointL = endpoint;

  *Dst    = dst;
  *Offset = offset;

  return i;
}

LONG
AddWordsMVH( ADDARGS )
{
  WORD    *src    = Src;
  LONG    *dst    = *Dst;
  Fixed64  offset = *Offset;
  int      i;
  LONG     startpointL, startpointR, endpointL, endpointR;
  LONG     lastpointL, lastpointR;

  endpointL  = cd->cd_TempStartPointL;
  endpointR  = cd->cd_TempStartPointR;
  lastpointL = lastpointR = 0;        // 0 doesn't affect the StopAtZero code

  for( i = 0; i < Samples; i++)
  {
    if( offseti == cd->cd_FirstOffsetI) {
      startpointL = cd->cd_StartPointL;
      startpointR = cd->cd_StartPointR;
    }
    else
    {
      startpointL = src[ offseti * 2 + 0 - 2 ];
      startpointR = src[ offseti * 2 + 1 - 2 ];
    }

    endpointL = src[ offseti * 2 + 0 ];
    endpointR = src[ offseti * 2 + 1 ];

    startpointL += (((endpointL - startpointL) * offsetf ) >> 15);
    startpointR += (((endpointR - startpointR) * offsetf ) >> 15);

    if( StopAtZero &&
        ( ( lastpointL < 0 && startpointL >= 0 ) ||
          ( lastpointR < 0 && startpointR >= 0 ) ||
          ( lastpointL > 0 && startpointL <= 0 ) ||
          ( lastpointR > 0 && startpointR <= 0 ) ) )
    {
      break;
    }

    lastpointL = startpointL;
    lastpointR = startpointR;

    *dst++ += ScaleLeft * startpointL + ScaleRight * startpointR;

    offset += Add;
  }

  cd->cd_TempStartPointL = endpointL;
  cd->cd_TempStartPointR = endpointR;

  *Dst    = dst;
  *Offset = offset;

  return i;
}


LONG
AddWordsSVPH( ADDARGS )
{
  WORD    *src    = Src;
  LONG    *dst    = *Dst;
  Fixed64  offset = *Offset;
  int      i;
  LONG     startpointL, startpointR, endpointL, endpointR;
  LONG     lastpointL, lastpointR;

  endpointL  = cd->cd_TempStartPointL;
  endpointR  = cd->cd_TempStartPointR;
  lastpointL = lastpointR = 0;        // 0 doesn't affect the StopAtZero code

  for( i = 0; i < Samples; i++)
  {
    if( offseti == cd->cd_FirstOffsetI) {
      startpointL = cd->cd_StartPointL;
      startpointR = cd->cd_StartPointR;
    }
    else
    {
      startpointL = src[ offseti * 2 + 0 - 2 ];
      startpointR = src[ offseti * 2 + 1 - 2 ];
    }

    endpointL = src[ offseti * 2 + 0 ];
    endpointR = src[ offseti * 2 + 1 ];

    startpointL += (((endpointL - startpointL) * offsetf ) >> 15);
    startpointR += (((endpointR - startpointR) * offsetf ) >> 15);

    if( StopAtZero &&
        ( ( lastpointL < 0 && startpointL >= 0 ) ||
          ( lastpointR < 0 && startpointR >= 0 ) ||
          ( lastpointL > 0 && startpointL <= 0 ) ||
          ( lastpointR > 0 && startpointR <= 0 ) ) )
    {
      break;
    }

    lastpointL = startpointL;
    lastpointR = startpointR;

    *dst++ += ScaleLeft * startpointL;
    *dst++ += ScaleRight * startpointR;

    offset += Add;
  }

  cd->cd_TempStartPointL = endpointL;
  cd->cd_TempStartPointR = endpointR;

  *Dst    = dst;
  *Offset = offset;

  return i;
}

#undef offsetf

/*****************************************************************************/

/* Backward mixing code */

#define offsetf ( (long) ( 32768 - ( (unsigned long) ( offset & 0xffffffffULL ) >> 17 ) ) )

LONG
AddByteMVHB( ADDARGS )
{
  BYTE    *src    = Src;
  LONG    *dst    = *Dst;
  Fixed64  offset = *Offset;
  int      i;
  LONG     startpoint, endpoint;
  LONG     lastpoint;

  endpoint  = cd->cd_TempStartPointL;
  lastpoint = 0;                      // 0 doesn't affect the StopAtZero code

  for( i = 0; i < Samples; i++)
  {
    if( offseti == cd->cd_FirstOffsetI) {
      startpoint = cd->cd_StartPointL;
    }
    else
    {
      startpoint = src[ offseti + 1 ] << 8;
    }

    endpoint = src[ offseti ] << 8;

    startpoint += (((endpoint - startpoint) * offsetf ) >> 15);

    if( StopAtZero &&
        ( ( lastpoint < 0 && startpoint >= 0 ) ||
          ( lastpoint > 0 && startpoint <= 0 ) ) )
    {
      break;
    }

    lastpoint = startpoint;

    *dst++ += ScaleLeft * startpoint;

    offset -= Add;
  }

  cd->cd_TempStartPointL = endpoint;

  *Dst    = dst;
  *Offset = offset;

  return i;
}


LONG
AddByteSVPHB( ADDARGS )
{
  BYTE    *src    = Src;
  LONG    *dst    = *Dst;
  Fixed64  offset = *Offset;
  int      i;
  LONG     startpoint, endpoint;
  LONG     lastpoint;

  endpoint  = cd->cd_TempStartPointL;
  lastpoint = 0;                      // 0 doesn't affect the StopAtZero code
  
  for( i = 0; i < Samples; i++)
  {
    if( offseti == cd->cd_FirstOffsetI) {
      startpoint = cd->cd_StartPointL;
    }
    else
    {
      startpoint = src[ offseti + 1 ] << 8;
    }

    endpoint = src[ offseti ] << 8;

    startpoint += (((endpoint - startpoint) * offsetf ) >> 15);

    if( StopAtZero &&
        ( ( lastpoint < 0 && startpoint >= 0 ) ||
          ( lastpoint > 0 && startpoint <= 0 ) ) )
    {
      break;
    }

    lastpoint = startpoint;

    *dst++ += ScaleLeft * startpoint;
    *dst++ += ScaleRight * startpoint;

    offset -= Add;
  }

  cd->cd_TempStartPointL = endpoint;

  *Dst    = dst;
  *Offset = offset;

  return i;
}


LONG
AddBytesMVHB( ADDARGS )
{
  BYTE    *src    = Src;
  LONG    *dst    = *Dst;
  Fixed64  offset = *Offset;
  int      i;
  LONG     startpointL, startpointR, endpointL, endpointR;
  LONG     lastpointL, lastpointR;

  endpointL  = cd->cd_TempStartPointL;
  endpointR  = cd->cd_TempStartPointR;
  lastpointL = lastpointR = 0;        // 0 doesn't affect the StopAtZero code

  for( i = 0; i < Samples; i++)
  {
    if( offseti == cd->cd_FirstOffsetI) {
      startpointL = cd->cd_StartPointL;
      startpointR = cd->cd_StartPointR;
    }
    else
    {
      startpointL = src[ offseti * 2 + 0 + 2 ] << 8;
      startpointR = src[ offseti * 2 + 1 + 2 ] << 8;
    }

    endpointL = src[ offseti * 2 + 0 ] << 8;
    endpointR = src[ offseti * 2 + 1 ] << 8;

    startpointL += (((endpointL - startpointL) * offsetf ) >> 15);
    startpointR += (((endpointR - startpointR) * offsetf ) >> 15);

    if( StopAtZero &&
        ( ( lastpointL < 0 && startpointL >= 0 ) ||
          ( lastpointR < 0 && startpointR >= 0 ) ||
          ( lastpointL > 0 && startpointL <= 0 ) ||
          ( lastpointR > 0 && startpointR <= 0 ) ) )
    {
      break;
    }

    lastpointL = startpointL;
    lastpointR = startpointR;

    *dst++ += ScaleLeft * startpointL + ScaleRight * startpointR;

    offset -= Add;
  }

  cd->cd_TempStartPointL = endpointL;
  cd->cd_TempStartPointR = endpointR;

  *Dst    = dst;
  *Offset = offset;

  return i;
}


LONG
AddBytesSVPHB( ADDARGS )
{
  BYTE    *src    = Src;
  LONG    *dst    = *Dst;
  Fixed64  offset = *Offset;
  int      i;
  LONG     startpointL, startpointR, endpointL, endpointR;
  LONG     lastpointL, lastpointR;

  endpointL  = cd->cd_TempStartPointL;
  endpointR  = cd->cd_TempStartPointR;
  lastpointL = lastpointR = 0;        // 0 doesn't affect the StopAtZero code

  for( i = 0; i < Samples; i++)
  {
    if( offseti == cd->cd_FirstOffsetI) {
      startpointL = cd->cd_StartPointL;
      startpointR = cd->cd_StartPointR;
    }
    else
    {
      startpointL = src[ offseti * 2 + 0 + 2 ] << 8;
      startpointR = src[ offseti * 2 + 1 + 2 ] << 8;
    }

    endpointL = src[ offseti * 2 + 0 ] << 8;
    endpointR = src[ offseti * 2 + 1 ] << 8;

    startpointL += (((endpointL - startpointL) * offsetf ) >> 15);
    startpointR += (((endpointR - startpointR) * offsetf ) >> 15);

    if( StopAtZero &&
        ( ( lastpointL < 0 && startpointL >= 0 ) ||
          ( lastpointR < 0 && startpointR >= 0 ) ||
          ( lastpointL > 0 && startpointL <= 0 ) ||
          ( lastpointR > 0 && startpointR <= 0 ) ) )
    {
      break;
    }

    lastpointL = startpointL;
    lastpointR = startpointR;

    *dst++ += ScaleLeft * startpointL;
    *dst++ += ScaleRight * startpointR;

    offset -= Add;
  }

  cd->cd_TempStartPointL = endpointL;
  cd->cd_TempStartPointR = endpointR;

  *Dst    = dst;
  *Offset = offset;

  return i;
}


LONG
AddWordMVHB( ADDARGS )
{
  WORD    *src    = Src;
  LONG    *dst    = *Dst;
  Fixed64  offset = *Offset;
  int      i;
  LONG     startpoint, endpoint;
  LONG     lastpoint;

  endpoint  = cd->cd_TempStartPointL;
  lastpoint = 0;                      // 0 doesn't affect the StopAtZero code

  for( i = 0; i < Samples; i++)
  {
    if( offseti == cd->cd_FirstOffsetI) {
      startpoint = cd->cd_StartPointL;
    }
    else
    {
      startpoint = src[ offseti + 1 ];
    }

    endpoint = src[ offseti ];

    startpoint += (((endpoint - startpoint) * offsetf ) >> 15);

    if( StopAtZero &&
        ( ( lastpoint < 0 && startpoint >= 0 ) ||
          ( lastpoint > 0 && startpoint <= 0 ) ) )
    {
      break;
    }

    lastpoint = startpoint;

    *dst++ += ScaleLeft * startpoint;

    offset -= Add;
  }

  cd->cd_TempStartPointL = endpoint;

  *Dst    = dst;
  *Offset = offset;

  return i;
}


LONG
AddWordSVPHB( ADDARGS )
{
  WORD    *src    = Src;
  LONG    *dst    = *Dst;
  Fixed64  offset = *Offset;
  int      i;
  LONG     startpoint, endpoint;
  LONG     lastpoint;

  endpoint  = cd->cd_TempStartPointL;
  lastpoint = 0;                      // 0 doesn't affect the StopAtZero code
  
  for( i = 0; i < Samples; i++)
  {
    if( offseti == cd->cd_FirstOffsetI) {
      startpoint = cd->cd_StartPointL;
    }
    else
    {
      startpoint = src[ offseti + 1 ];
    }

    endpoint = src[ offseti ];

    startpoint += (((endpoint - startpoint) * offsetf ) >> 15);

    if( StopAtZero &&
        ( ( lastpoint < 0 && startpoint >= 0 ) ||
          ( lastpoint > 0 && startpoint <= 0 ) ) )
    {
      break;
    }

    lastpoint = startpoint;

    *dst++ += ScaleLeft * startpoint;
    *dst++ += ScaleRight * startpoint;

    offset -= Add;
  }

  cd->cd_TempStartPointL = endpoint;

  *Dst    = dst;
  *Offset = offset;

  return i;
}


LONG
AddWordsMVHB( ADDARGS )
{
  WORD    *src    = Src;
  LONG    *dst    = *Dst;
  Fixed64  offset = *Offset;
  int      i;
  LONG     startpointL, startpointR, endpointL, endpointR;
  LONG     lastpointL, lastpointR;

  endpointL  = cd->cd_TempStartPointL;
  endpointR  = cd->cd_TempStartPointR;
  lastpointL = lastpointR = 0;        // 0 doesn't affect the StopAtZero code

  for( i = 0; i < Samples; i++)
  {
    if( offseti == cd->cd_FirstOffsetI) {
      startpointL = cd->cd_StartPointL;
      startpointR = cd->cd_StartPointR;
    }
    else
    {
      startpointL = src[ offseti * 2 + 0 + 2 ];
      startpointR = src[ offseti * 2 + 1 + 2 ];
    }

    endpointL = src[ offseti * 2 + 0 ];
    endpointR = src[ offseti * 2 + 1 ];

    startpointL += (((endpointL - startpointL) * offsetf ) >> 15);
    startpointR += (((endpointR - startpointR) * offsetf ) >> 15);

    if( StopAtZero &&
        ( ( lastpointL < 0 && startpointL >= 0 ) ||
          ( lastpointR < 0 && startpointR >= 0 ) ||
          ( lastpointL > 0 && startpointL <= 0 ) ||
          ( lastpointR > 0 && startpointR <= 0 ) ) )
    {
      break;
    }

    lastpointL = startpointL;
    lastpointR = startpointR;

    *dst++ += ScaleLeft * startpointL + ScaleRight * startpointR;

    offset -= Add;
  }

  cd->cd_TempStartPointL = endpointL;
  cd->cd_TempStartPointR = endpointR;

  *Dst    = dst;
  *Offset = offset;

  return i;
}


LONG
AddWordsSVPHB( ADDARGS )
{
  WORD    *src    = Src;
  LONG    *dst    = *Dst;
  Fixed64  offset = *Offset;
  int      i;
  LONG     startpointL, startpointR, endpointL, endpointR;
  LONG     lastpointL, lastpointR;

  endpointL  = cd->cd_TempStartPointL;
  endpointR  = cd->cd_TempStartPointR;
  lastpointL = lastpointR = 0;        // 0 doesn't affect the StopAtZero code

  for( i = 0; i < Samples; i++)
  {
    if( offseti == cd->cd_FirstOffsetI) {
      startpointL = cd->cd_StartPointL;
      startpointR = cd->cd_StartPointR;
    }
    else
    {
      startpointL = src[ offseti * 2 + 0 + 2 ];
      startpointR = src[ offseti * 2 + 1 + 2 ];
    }

    endpointL = src[ offseti * 2 + 0 ];
    endpointR = src[ offseti * 2 + 1 ];

    startpointL += (((endpointL - startpointL) * offsetf ) >> 15);
    startpointR += (((endpointR - startpointR) * offsetf ) >> 15);

    if( StopAtZero &&
        ( ( lastpointL < 0 && startpointL >= 0 ) ||
          ( lastpointR < 0 && startpointR >= 0 ) ||
          ( lastpointL > 0 && startpointL <= 0 ) ||
          ( lastpointR > 0 && startpointR <= 0 ) ) )
    {
      break;
    }

    lastpointL = startpointL;
    lastpointR = startpointR;

    *dst++ += ScaleLeft * startpointL;
    *dst++ += ScaleRight * startpointR;

    offset -= Add;
  }

  cd->cd_TempStartPointL = endpointL;
  cd->cd_TempStartPointR = endpointR;

  *Dst    = dst;
  *Offset = offset;

  return i;
}


#undef offseti
#undef offsetf

/*****************************************************************************/
