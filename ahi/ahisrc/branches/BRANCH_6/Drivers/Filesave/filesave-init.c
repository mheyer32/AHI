
#include <config.h>

#include <dos/dos.h>
#include <graphics/gfxbase.h>
#include <libraries/asl.h>
#include <proto/exec.h>
#include <proto/datatypes.h>

#include "library.h"

#include "DriverData.h"


/******************************************************************************
** Custom driver init *********************************************************
******************************************************************************/

BOOL
DriverInit( struct DriverBase* AHIsubBase )
{
  struct FilesaveBase* FilesaveBase = (struct FilesaveBase*) AHIsubBase;

  FilesaveBase->dosbase = OpenLibrary( DOSNAME, 37 );
  FilesaveBase->gfxbase = OpenLibrary( GRAPHICSNAME, 37 );
  FilesaveBase->aslbase = NULL;

  InitSemaphore(&FilesaveBase->aslsema);

  if( DOSBase == NULL )
  {
    Req( "Unable to open '" DOSNAME "' version 37.\n" );
    return FALSE;
  }

  if( GfxBase == NULL )
  {
    Req( "Unable to open '" GRAPHICSNAME "' version 37.\n" );
    return FALSE;
  }

#ifdef __AMIGAOS4__
  if ((IDOS = (struct DOSIFace *) GetInterface((struct Library *) DOSBase, "main", 1, NULL)) == NULL)
  {
    Req("Couldn't open IDOS interface!\n");
    return FALSE;
  }

  IAsl = NULL;
#endif

  return TRUE;
}


/******************************************************************************
** Custom driver clean-up *****************************************************
******************************************************************************/

VOID
DriverCleanup( struct DriverBase* AHIsubBase )
{
  struct FilesaveBase* FilesaveBase = (struct FilesaveBase*) AHIsubBase;

#ifdef __AMIGAOS4__
  DropInterface( (struct Interface *) IDOS);
  DropInterface( (struct Interface *) IAsl);
#endif

  CloseLibrary( FilesaveBase->dosbase );
  CloseLibrary( FilesaveBase->gfxbase );
  CloseLibrary( FilesaveBase->aslbase );
}
