/* $Id$ */

#include <config.h>
#include <CompilerSpecific.h>

#include <exec/resident.h>
#include <exec/alerts.h>
#include <exec/execbase.h>
#include <proto/exec.h>

#include "ahi_def.h"
#include "version.h"
#include "device.h"

/******************************************************************************
** Device startup strcuture ***************************************************
******************************************************************************/

int Start( void )
{
  return -1;
}

extern APTR  __FuncTable__[];
extern void EndCode;
extern const char DevName[];
extern const char IDString[];
extern const APTR InitTable[4];

static const struct Resident RomTag =
{
  RTC_MATCHWORD,
  (struct Resident *) &RomTag,
  (APTR) &EndCode,
  RTF_AUTOINIT,
  VERSION,
  NT_DEVICE,
  0,                      /* priority */
  (BYTE *) DevName,
  (BYTE *) IDString,
  (APTR) &InitTable
};

/******************************************************************************
** Globals ********************************************************************
******************************************************************************/

struct ExecBase           *SysBase        = NULL;
struct AHIBase            *AHIBase        = NULL;
struct DosLibrary         *DOSBase        = NULL;
struct Library            *GadToolsBase   = NULL;
struct GfxBase            *GfxBase        = NULL;
struct Library            *IFFParseBase   = NULL;
struct IntuitionBase      *IntuitionBase  = NULL;
struct LocaleBase         *LocaleBase     = NULL;
struct Device             *TimerBase      = NULL;
struct UtilityBase        *UtilityBase    = NULL;

const ULONG			           DriverVersion  = 2;
const ULONG			           Version        = VERSION;
const ULONG			           Revision       = REVISION;

const char DevName[]  = AHINAME;
const char IDString[] = "ahi.device " VERS "\r\n";
const char VersTag[]  = "$VER: ahi.device " VERS " "
                        "©1994-1999 Martin Blom. "
#ifdef mc68060
                        "68060"
#else
# ifdef mc68040
                        "68040"
# else
#  ifdef mc68030
                        "68030"
#  else
#   ifdef mc68020
                        "68020"
#   else
#    ifdef mc68000
                        "68000"
#    endif /* mc68000 */
#   endif /* mc68020 */
#  endif /* mc68030 */
# endif /* mc68040 */
#endif /* mc68060 */

#ifdef VERSIONPOWERUP
                        "/PowerUp"
#endif
#ifdef VERSIONWARPUP
                        "/WarpUp"
#endif
                        " version.\r\n";


/******************************************************************************
** Device code ****************************************************************
******************************************************************************/

static struct AHIBase * ASMCALL
initRoutine( REG( d0, struct AHIBase* device ),
	     REG( a0, APTR seglist ),
	     REG( a6, struct ExecBase* sysbase ) )
{
  SysBase = sysbase;
  AHIBase = device;

  AHIBase->ahib_Library.lib_Node.ln_Type = NT_DEVICE;
  AHIBase->ahib_Library.lib_Node.ln_Name = (STRPTR) DevName;
  AHIBase->ahib_Library.lib_Flags        = LIBF_SUMUSED | LIBF_CHANGED;
  AHIBase->ahib_Library.lib_Version      = VERSION;
  AHIBase->ahib_Library.lib_Revision     = REVISION;
  AHIBase->ahib_Library.lib_IdString     = (STRPTR) IDString;
  AHIBase->ahib_SysLib  = sysbase;
  AHIBase->ahib_SegList = (ULONG) seglist;

#if defined( mc68020 ) || defined( mc68030 ) || defined( mc68040 ) || defined( mc68060 )
  if( ( SysBase->AttnFlags & AFF_68020 ) == 0 )
  {
    Alert( ( AN_Unknown | ACPU_InstErr ) & (~AT_DeadEnd) );
    return NULL;
  }
#endif

  InitSemaphore( &AHIBase->ahib_Lock );

  if( !OpenLibs() )
  {
    return NULL;
  }

  return AHIBase;
}


const APTR InitTab[4] =
{
  (APTR) sizeof( struct AHIBase ),
  (APTR) &__FuncTable__[1],
  0,
  (APTR) initRoutine
};

BPTR ASMCALL
DevExpunge( REG( a6, struct AHIBase* device ) )
{
  BPTR seglist = 0;

  device->ahib_Library.lib_Flags |= LIBF_DELEXP;

  if( device->ahib_Library.lib_OpenCnt == 0)
  {
    seglist = device->ahib_SegList;

    Remove( (struct Node *) device );

    CloseLibs();

    FreeMem( (APTR) ( ( (char*) device ) - device->ahib_Library.lib_NegSize ),
	     device->ahib_Library.lib_NegSize + device->ahib_Library.lib_PosSize );
  }

  return seglist;
}


int Null( void )
{
  return 0;
}
