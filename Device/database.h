
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
