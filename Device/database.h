/* $Id$ */

/*
     AHI - Hardware independent audio subsystem
     Copyright (C) 1997-1999 Martin Blom <martin@blom.org>
     
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

#ifndef _DATABASE_H_
#define _DATABASE_H_

#include <exec/types.h>
#include <exec/semaphores.h>
#include <exec/lists.h>

/* Current implementation of the database (Version 0):
   Use FreeVec() to free the structure. */

#define ADB_NAME  "Audio Mode Database"

struct AHI_AudioDatabase
{
  struct SignalSemaphore  ahidb_Semaphore;      /* The Semaphore */
  struct MinList          ahidb_AudioModes;     /* The Audio Database */
  UBYTE                   ahidb_Version;        /* Version number (0) */
  UBYTE                   ahidb_Name[sizeof(ADB_NAME)]; /* Name */

};


struct AHI_AudioDatabase*
LockDatabase( void );

struct AHI_AudioDatabase*
LockDatabaseWrite( void );

void
UnlockDatabase( struct AHI_AudioDatabase* audiodb );

struct TagItem*
GetDBTagList( struct AHI_AudioDatabase* audiodb,
              ULONG id );

#endif /* _DATABASE_H_ */