
#include <config.h>

#include <intuition/intuition.h>
#include <proto/intuition.h>
#include <proto/exec.h>

#include "boopsi.h"
#include "util.h"

/******************************************************************************
** Message requester **********************************************************
******************************************************************************/

void
ReqA( const char*           text,
      APTR                  args ) {
  struct EasyStruct es = {
    sizeof (struct EasyStruct),
    0,
    (STRPTR) ClassName,
    (STRPTR) text,
    "OK"
  };

  EasyRequestArgs( NULL, &es, NULL, args );
}

/******************************************************************************
** Serial port debugging ******************************************************
******************************************************************************/

#if defined( __AROS__ ) && !defined( __mc68000__ )

#include <aros/asmcall.h>

AROS_UFH2( void,
	   rawputchar_m68k,
	   AROS_UFHA( UBYTE,            c,       D0 ),
	   AROS_UFHA( struct ExecBase*, sysbase, A3 ) ) {
  AROS_USERFUNC_INIT
  __RawPutChar_WB( sysbase, c );
  AROS_USERFUNC_EXIT  
}

#else

static UWORD rawputchar_m68k[] = {
  0x2C4B,             // MOVEA.L A3,A6
  0x4EAE, 0xFDFC,     // JSR     -$0204(A6)
  0x4E75              // RTS
};

#endif

void
MyKPrintFArgs( const char*           fmt,
	       APTR                  args ) {
  RawDoFmt( fmt, args, (void (*)(void)) rawputchar_m68k, SysBase );
}

/******************************************************************************
** SPrintF ********************************************************************
******************************************************************************/

#if defined( __AROS__ ) && !defined( __mc68000__ )

AROS_UFH2( void,
	   copychar_m68k,
	   AROS_UFHA( UBYTE,  c,      D0 ),
	   AROS_UFHA( UBYTE** buffer, A3 ) ) {
  AROS_USERFUNC_INIT
  *(*buffer++) = c;
  AROS_USERFUNC_EXIT  
}

#else

static UWORD copychar_m68k[] = {
  0x2053,             //  MOVEA.L     (A3),A0
  0x10C0,             //  MOVE.B      D0,(A0)+
  0x2688,             //  MOVE.L      A0,(A3)
  0x4E75,             //  RTS
};

#endif

void
MySPrintFArgs( char*       buffer,
	       const char* fmt,
	       APTR        args ) {
  RawDoFmt( fmt, args, (void (*) (void)) copychar_m68k, &buffer );
}
