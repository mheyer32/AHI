/* $Id$
* $Log$
* Revision 1.6  1997/03/24 12:41:51  lcs
* Echo rewritten
*
* Revision 1.5  1997/02/12 15:32:45  lcs
* Moved each autodoc header to the file where the function is
*
* Revision 1.4  1997/01/29 23:34:38  lcs
* *** empty log message ***
*
* Revision 1.3  1997/01/04 20:19:56  lcs
* Changed the AHI_DEBUG levels
*
* Revision 1.2  1996/12/21 23:06:35  lcs
* Replaced all EQ with ==
*
* Revision 1.1  1996/12/21 13:05:12  lcs
* Initial revision
*
*/

#include "ahi_def.h"

#include <exec/memory.h>
#include <exec/semaphores.h>
#include <dos/dostags.h>
#include <dos/dos.h>
#include <libraries/iffparse.h>
#include <strings.h>

#include <proto/exec.h>
#include <proto/dos.h>
#include <proto/iffparse.h>
#include <proto/utility.h>

#ifndef _GENPROTO
#include "database_protos.h"
#endif

/******************************************************************************
** Audio Database *************************************************************
******************************************************************************/

/* Current implementation of the database (Version 0): */

#define ADB_NAME  "Audio Mode Database"

struct AHI_AudioDatabase
{
  struct SignalSemaphore  ahidb_Semaphore;      /* The Semaphore */
  struct MinList          ahidb_AudioModes;     /* The Audio Database */
  UBYTE                   ahidb_Version;        /* Version number (0) */
  UBYTE                   ahidb_Name[sizeof(ADB_NAME)]; /* Name */
/* I think that was all. Use FreeVec() to free the structure. */
};

struct AHI_AudioMode
{
  struct MinNode          ahidbn_MinNode;
  struct TagItem          ahidbn_Tags[0];
/* Taglist, mode name and driver name follows. */
/* Size variable. Use FreeVec() to free node. */
};

/*
** Lock the database for read access. Return NULL if database not present.
*/

__asm __saveds struct AHI_AudioDatabase *LockDatabase(void)
{
  struct AHI_AudioDatabase *audiodb;

  Forbid();
  if(audiodb=(struct AHI_AudioDatabase *)FindSemaphore(ADB_NAME))
    ObtainSemaphoreShared((struct SignalSemaphore *) audiodb);
  Permit();
  return audiodb;
}

/*
** Lock the database for write access. Create it if not present.
*/

__asm __saveds struct AHI_AudioDatabase *LockDatabaseWrite(void)
{
  struct AHI_AudioDatabase *audiodb;

  Forbid();
  if(audiodb=(struct AHI_AudioDatabase *)FindSemaphore(ADB_NAME))
    ObtainSemaphore((struct SignalSemaphore *) audiodb);
  else
  {
    if(audiodb=(struct AHI_AudioDatabase *)
        AllocVec(sizeof(struct AHI_AudioDatabase), MEMF_PUBLIC|MEMF_CLEAR))
    {
      NewList((struct List *)&audiodb->ahidb_AudioModes);
      audiodb->ahidb_Semaphore.ss_Link.ln_Name=audiodb->ahidb_Name;
      strcpy(audiodb->ahidb_Semaphore.ss_Link.ln_Name,ADB_NAME);
      AddSemaphore((struct SignalSemaphore *) audiodb);
      ObtainSemaphore((struct SignalSemaphore *) audiodb);
    }
  }
  Permit();
  return audiodb;
}

__asm __saveds void UnlockDatabase(
    register __a0 struct AHI_AudioDatabase *audiodb)
{
  if(audiodb)
    ReleaseSemaphore((struct SignalSemaphore *)audiodb);
}

__asm __saveds struct TagItem *GetDBTagList(
    register __a0 struct AHI_AudioDatabase *audiodb,
    register __d0 ULONG id)
{
  struct AHI_AudioMode *node;

  if(audiodb && (id != AHI_INVALID_ID))
    for(node=(struct AHI_AudioMode *)audiodb->ahidb_AudioModes.mlh_Head;
        node->ahidbn_MinNode.mln_Succ;
        node=(struct AHI_AudioMode *)node->ahidbn_MinNode.mln_Succ)
      if(id == GetTagData(AHIDB_AudioID,AHI_INVALID_ID,node->ahidbn_Tags))
        return node->ahidbn_Tags;
  return NULL;
}


/******************************************************************************
** AHI_NextAudioID ************************************************************
******************************************************************************/

/****** ahi.device/AHI_NextAudioID ******************************************
*
*   NAME
*       AHI_NextAudioID -- iterate current audio mode identifiers
*
*   SYNOPSIS
*       next_ID = AHI_NextAudioID( last_ID );
*       D0                         D0
*
*       ULONG AHI_NextAudioID( ULONG );
*
*   FUNCTION
*       This function is used to itereate through all current AudioIDs in
*       the audio database.
*
*   INPUTS
*       last_ID - previous AudioID or AHI_INVALID_ID if beginning iteration.
*
*   RESULT
*       next_ID - subsequent AudioID or AHI_INVALID_ID if no more IDs.
*
*   EXAMPLE
*
*   NOTES
*
*   BUGS
*
*   SEE ALSO
*      AHI_GetAudioAttrsA(), AHI_BestAudioIDA()
*
****************************************************************************
*
*/

__asm ULONG NextAudioID( register __d0 ULONG id )
{
  struct AHI_AudioDatabase *audiodb;
  struct AHI_AudioMode *node;
  ULONG  nextid=AHI_INVALID_ID;

  if(AHIBase->ahib_DebugLevel >= AHI_DEBUG_HIGH)
  {
    KPrintF("AHI_NextAudioID(0x%08lx)",id);
  }

  if(audiodb=LockDatabase())
  {
    node=(struct AHI_AudioMode *)audiodb->ahidb_AudioModes.mlh_Head;
    if(id != AHI_INVALID_ID)
    {
      do
      {
        if(id == GetTagData(AHIDB_AudioID,AHI_INVALID_ID,node->ahidbn_Tags))
          break;
      } while (node=(struct AHI_AudioMode *)node->ahidbn_MinNode.mln_Succ);
      node=(struct AHI_AudioMode *)node->ahidbn_MinNode.mln_Succ;
    }
    if(node)
      nextid=GetTagData(AHIDB_AudioID,AHI_INVALID_ID,node->ahidbn_Tags);
    UnlockDatabase(audiodb);
  }

  if(AHIBase->ahib_DebugLevel >= AHI_DEBUG_HIGH)
  {
    KPrintF("=>0x%08lx\n",nextid);
  }

  return nextid;
}


/******************************************************************************
** AHI_AddAudioMode ***********************************************************
******************************************************************************/

/****i* ahi.device/AHI_AddAudioMode *****************************************
*
*   NAME
*       AHI_AddAudioMode -- add a audio mode to the database (V3)
*
*   SYNOPSIS
*       success = AHI_AddAudioMode( DBtags );
*       D0                          A0
*
*       ULONG AHI_AddAudioMode( struct TagItem *, UBYTE *, UBYTE *);
*
*   FUNCTION
*       Adds the audio mode described by a taglist to the audio mode
*       database. If the database does not exists, it will be created.
*
*   INPUTS
*       DBtags - Tags describing the properties of this mode.
*
*   RESULT
*       success - FALSE if the mode could not be added.
*
*   EXAMPLE
*
*   NOTES
*
*   BUGS
*
*   SEE ALSO
*
****************************************************************************
*
*/

__asm ULONG AddAudioMode( register __a0 struct TagItem *DBtags )
{
  struct AHI_AudioDatabase *audiodb;
  struct AHI_AudioMode *node;
  ULONG nodesize=sizeof(struct AHI_AudioMode),tagitems=0;
  ULONG datalength=0,namelength=0,driverlength=0;
  struct TagItem *tstate=DBtags,*tp,*tag;
  ULONG rc=FALSE;

  if(AHIBase->ahib_DebugLevel >= AHI_DEBUG_HIGH)
  {
    KPrintF("AHI_AddAudioMode(0x%08lx)",DBtags);
  }

// Remove old mode if present in database
  AHI_RemoveAudioMode(GetTagData(AHIDB_AudioID,AHI_INVALID_ID,DBtags));

// Now add the new mode
  if(audiodb=LockDatabaseWrite())
  {

// Find total size
    while(tag=NextTagItem(&tstate))
    {
      if(tag->ti_Data) switch(tag->ti_Tag)
      {
        case AHIDB_Data:
          datalength=((ULONG *)tag->ti_Data)[0];
          nodesize+=datalength;
          break;
        case AHIDB_Name:
          namelength=strlen((UBYTE *)tag->ti_Data)+1;
          nodesize+=namelength;
          break;
        case AHIDB_Driver:
          driverlength=strlen((UBYTE *)tag->ti_Data)+1;
          nodesize+=driverlength;
          break;
      }
      nodesize+=sizeof(struct TagItem);
      tagitems++;
    }
    nodesize+=sizeof(struct TagItem);  // The last TAG_END
    tagitems++;

    if(node=AllocVec(nodesize,MEMF_PUBLIC|MEMF_CLEAR))
    {
      tp=node->ahidbn_Tags;
      tstate=DBtags;
      while (tag=NextTagItem(&tstate))
      {
        if(tag->ti_Data) switch(tag->ti_Tag)
        {
          case AHIDB_Data:
            tp->ti_Data=((ULONG) &node->ahidbn_Tags[tagitems]);
            CopyMem((APTR)tag->ti_Data,(APTR)tp->ti_Data,datalength);
            break;
          case AHIDB_Name:
            tp->ti_Data=((ULONG) &node->ahidbn_Tags[tagitems]) + datalength;
            strcpy((UBYTE *)tp->ti_Data,(UBYTE *)tag->ti_Data);
            break;
          case AHIDB_Driver:
            tp->ti_Data=((ULONG) &node->ahidbn_Tags[tagitems]) + datalength + namelength;
            strcpy((UBYTE *)tp->ti_Data,(UBYTE *)tag->ti_Data);
            break;
          default:
            tp->ti_Data=tag->ti_Data;
            break;
        }
        tp->ti_Tag=tag->ti_Tag;
        tp++;
      }
      tp->ti_Tag=TAG_DONE;

      AddHead((struct List *)&audiodb->ahidb_AudioModes,(struct Node *)node);
      rc=TRUE;
    }
    UnlockDatabase(audiodb);
  }

  if(AHIBase->ahib_DebugLevel >= AHI_DEBUG_HIGH)
  {
    KPrintF("=>%ld\n",rc);
  }

  return rc;
}


/******************************************************************************
** AHI_RemoveAudioMode ********************************************************
******************************************************************************/

/****i* ahi.device/AHI_RemoveAudioMode **************************************
*
*   NAME
*       AHI_RemoveAudioMode -- remove a audio mode to the database (V3)
*
*   SYNOPSIS
*       success = AHI_RemoveAudioMode( ID );
*       D0                             D0
*
*       ULONG AHI_RemoveAudioMode( ULONG );
*
*   FUNCTION
*       Removes the audio mode from the audio mode database.
*
*   INPUTS
*       ID - The audio ID of the mode to be removed.
*
*   RESULT
*       success - FALSE if the mode could not be removed.
*
*   EXAMPLE
*
*   NOTES
*
*   BUGS
*
*   SEE ALSO
*
****************************************************************************
*
*/

__asm ULONG RemoveAudioMode( register __d0 ULONG id )
{
  struct AHI_AudioMode *node;
  struct AHI_AudioDatabase *audiodb;
  ULONG rc=FALSE;

  if(AHIBase->ahib_DebugLevel >= AHI_DEBUG_HIGH)
  {
    KPrintF("AHI_RemoveAudioMode(0x%08lx)",id);
  }

  if(audiodb=LockDatabaseWrite())
    UnlockDatabase(audiodb);

  if(audiodb=LockDatabaseWrite())
  {
    if(id != AHI_INVALID_ID)
    {
      for(node=(struct AHI_AudioMode *)audiodb->ahidb_AudioModes.mlh_Head;
          node->ahidbn_MinNode.mln_Succ;
          node=(struct AHI_AudioMode *)node->ahidbn_MinNode.mln_Succ)
      {
        if(id == GetTagData(AHIDB_AudioID,AHI_INVALID_ID,node->ahidbn_Tags))
        {
          Remove((struct Node *)node);
          FreeVec(node);
          rc=TRUE;
          break;
        }
      }
// Remove the entire database if it's empty
      if(!audiodb->ahidb_AudioModes.mlh_Head->mln_Succ)
      {
        UnlockDatabase(audiodb);
        Forbid();
        if(audiodb=(struct AHI_AudioDatabase *)FindSemaphore(ADB_NAME))
        {
          RemSemaphore((struct SignalSemaphore *) audiodb);
          ObtainSemaphore((struct SignalSemaphore *) audiodb);
          ReleaseSemaphore((struct SignalSemaphore *) audiodb);
        }
        FreeVec(audiodb);
        audiodb=NULL;
        Permit();
      }
    }
    UnlockDatabase(audiodb);
  }

  if(AHIBase->ahib_DebugLevel >= AHI_DEBUG_HIGH)
  {
    KPrintF("=>%ld\n",rc);
  }

  return rc;
}


/******************************************************************************
** AHI_LoadModeFile ***********************************************************
******************************************************************************/

/****i* ahi.device/AHI_LoadModeFile *****************************************
*
*   NAME
*       AHI_LoadModeFile -- Add all modes in a mode file to the database (V3)
*
*   SYNOPSIS
*       success = AHI_LoadModeFile( name );
*       D0                          A0
*
*       ULONG AHI_LoadModeFile( STRPTR );
*
*   FUNCTION
*       This function takes the name of a file or a directory and either
*       adds all modes in the file or the modes of all files in the
*       directory to the audio mode database. Directories inside the
*       given directory will not be recursed. The file format is IFF-AHIM.
*
*   INPUTS
*       name - A pointer to the name of a file or directory.
*
*   RESULT
*       success - FALSE on error. Check dos.library/IOErr() for more
*           information.
*
*   EXAMPLE
*
*   NOTES
*
*   BUGS
*
*   SEE ALSO
*
****************************************************************************
*
*/

__asm ULONG LoadModeFile( register __a0 UBYTE *name )
{
  ULONG rc=FALSE;
  struct FileInfoBlock  *fib;
  BPTR  lock,thisdir;

  if(AHIBase->ahib_DebugLevel >= AHI_DEBUG_HIGH)
  {
    KPrintF("AHI_LoadModeFile(%s)",name);
  }

  SetIoErr(NULL);
  if(fib=AllocDosObject(DOS_FIB,TAG_DONE))
  {
    if(lock=Lock(name,ACCESS_READ))
    {
      if(Examine(lock,fib))
      {
        if(fib->fib_DirEntryType>0) // Directory?
        {
          thisdir=CurrentDir(lock);
          while(ExNext(lock,fib))
          {
            if(fib->fib_DirEntryType>0)
            {
              continue;     // AHI_LoadModeFile(fib->fib_FileName); for recursion
            }
            else
            {
              if(!(rc=AddModeFile(fib->fib_FileName)))
              {
                break;
              }
            }
          }
          if(IoErr() == ERROR_NO_MORE_ENTRIES)
          {
            SetIoErr(NULL);
          }
          CurrentDir(thisdir);
        }
        else  // Plain file
        {
          rc=AddModeFile(name);
        }
      }
      UnLock(lock);
    }
    FreeDosObject(DOS_FIB,fib);
  }

  if(AHIBase->ahib_DebugLevel >= AHI_DEBUG_HIGH)
  {
    KPrintF("=>%ld\n",rc);
  }

  return rc;
}

ULONG AddModeFile(UBYTE *filename)
{
  struct IFFHandle *iff;
  struct StoredProperty *name,*data;
  struct CollectionItem *ci;
  struct TagItem *tag,*tstate;
  struct TagItem extratags[]=
  {
    AHIDB_Driver, NULL,
    AHIDB_Data, NULL,
    TAG_MORE,   NULL
  };
  ULONG rc=FALSE;

  if(iff=AllocIFF())
  {
    iff->iff_Stream=Open(filename, MODE_OLDFILE);
    if(iff->iff_Stream)
    {
      InitIFFasDOS(iff);
      if(!OpenIFF(iff,IFFF_READ))
      {

        if(!(PropChunk(iff,ID_AHIM,ID_AUDN)
          || PropChunk(iff,ID_AHIM,ID_AUDD)
          || CollectionChunk(iff,ID_AHIM,ID_AUDM)
          || StopOnExit(iff,ID_AHIM,ID_FORM)))
        {
          if(ParseIFF(iff,IFFPARSE_SCAN) == IFFERR_EOC)
          {
            name=FindProp(iff,ID_AHIM,ID_AUDN);
            data=FindProp(iff,ID_AHIM,ID_AUDD);
            if(name)
              extratags[0].ti_Data=(ULONG)name->sp_Data;
            if(data)
              extratags[1].ti_Data=(ULONG)data->sp_Data;
            ci=FindCollection(iff,ID_AHIM,ID_AUDM);
            rc=TRUE;
            while(ci && rc)
            {
// Relocate loaded taglist
              tstate=(struct TagItem *)ci->ci_Data;
              while(tag=NextTagItem(&tstate))
              {
                if(tag->ti_Tag & (AHI_TagBaseR ^ AHI_TagBase))
                {
                  tag->ti_Data+=(ULONG)ci->ci_Data;
                }
              }
// Link taglists
              extratags[2].ti_Data=(ULONG)ci->ci_Data;
              rc=AHI_AddAudioMode(&extratags[0]);
              ci=ci->ci_Next;
            }
          }
        }
        CloseIFF(iff);
      }
      Close(iff->iff_Stream);
    }
    FreeIFF(iff);
  }
  return rc;
}
