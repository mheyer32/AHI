/* $Id$
* $Log$
* Revision 4.3  1999/01/08 23:42:55  lcs
* Switched to gcc and GNU make.
* Clean-up in source code.
*
* Revision 4.2  1998/03/11 12:31:57  lcs
* Added hardware-banging code to force VGA mode.
*
* Revision 4.1  1997/04/02 22:44:22  lcs
* Bumped to version 4
*
* Revision 1.5  1997/02/18 22:24:45  lcs
* Better DBLSCAN handling.
* The device is now opened with the AHIDF_NOMODESCAN flag.
*
* Revision 1.2  1997/01/04 00:24:51  lcs
* Added DBLSCAN switch + some other small changes
*
*/

#include <devices/ahi.h>
#include <graphics/modeid.h>
#include <graphics/gfxbase.h>
#include <intuition/screens.h>
#include <proto/ahi.h>
#include <proto/dos.h>
#include <proto/exec.h>
#include <proto/graphics.h>
#include <proto/intuition.h>
#include <stdlib.h>

#include "config.h"
#include "version.h"

struct Library    *AHIBase   = NULL;
struct MsgPort    *AHImp     = NULL;
struct AHIRequest *AHIio     = NULL;
BYTE               AHIDevice = -1;

static const char version[] = "$VER: AddAudioModes " VERS "\n\r";

#define AHIVERSION 4
LONG __OSlibversion = 37;

#define TEMPLATE "FILES/M,QUIET/S,REFRESH/S,REMOVE/S,DBLSCAN/S"

struct {
  STRPTR *files;
  ULONG   quiet;
  ULONG   refresh;
  ULONG   remove;
  ULONG   dblscan;
} args = {NULL, FALSE, FALSE, FALSE, FALSE};


void
cleanup( int rc )
{
  if( AHIDevice == 0 )
  {
    CloseDevice( (struct IORequest *) AHIio );
  }

  DeleteIORequest( (struct IORequest *) AHIio);
  DeleteMsgPort( AHImp );

  exit( rc );
}


void
OpenAHI( void )
{
  if( AHIDevice != 0 )
  {
    AHImp = CreateMsgPort();

    if( AHImp != NULL )
    {
      AHIio = (struct AHIRequest *)
                  CreateIORequest( AHImp, sizeof( struct AHIRequest ) );

      if( AHIio != NULL )
      {
        AHIio->ahir_Version = AHIVERSION;

        AHIDevice = OpenDevice( AHINAME, 
                                AHI_NO_UNIT,
                                (struct IORequest *) AHIio,
                                AHIDF_NOMODESCAN );
      }
    }

    if( AHIDevice != 0 )
    {
      Printf( "Unable to open %s version %ld\n", (ULONG) AHINAME, AHIVERSION );
      cleanup( RETURN_FAIL );
    }

    AHIBase = (struct Library *) AHIio->ahir_Std.io_Device;
  }
}


int
main( void )
{
  struct RDArgs *rdargs;
  int            rc = RETURN_OK;

  rdargs = ReadArgs( TEMPLATE , (LONG *) &args, NULL );

  if( rdargs != NULL )
  {
    /* Refresh database */

    if( args.refresh && !args.remove )
    {
      OpenAHI();
      
      if( !AHI_LoadModeFile( "DEVS:AudioModes" ) && !args.quiet )
      {
        PrintFault( IoErr(), "DEVS:AudioModes" );
        rc = RETURN_ERROR;
      }
    }

    /* Load mode files */

    if( args.files != NULL && !args.remove )
    {
      int i = 0;

      OpenAHI();

      while( args.files[i] )
      {
        if( !AHI_LoadModeFile( args.files[i] ) && !args.quiet )
        {
          PrintFault( IoErr(), args.files[i] );
          rc = RETURN_ERROR;
        }
        i++;
      }
    }

    /* Remove database */

    if( args.remove )
    {
      if( args.files || args.refresh )
      {
        PutStr( "The REMOVE switch cannot be used together with FILES or REFRESH.\n" );
        rc = RETURN_FAIL;
      }
      else
      {
        ULONG id;

        OpenAHI();

        for( id = AHI_NextAudioID( AHI_INVALID_ID );
             id != AHI_INVALID_ID;
             id = AHI_NextAudioID( AHI_INVALID_ID ) )
        {
          AHI_RemoveAudioMode( id );
        }
      }
    }

    /* Make display mode doublescan (allowing > 28 kHz sample rates) */

    if( args.dblscan )
    {
      ULONG          id;
      ULONG          bestid = INVALID_ID;
      int            minper = MAXINT;
      struct Screen *screen = NULL;

      static const struct ColorSpec colorspecs[] =
      {
        { 0, 0, 0, 0 },
        { 1, 0, 0, 0 },
        {-1, 0, 0, 0 }
      };
      
      union {
        struct MonitorInfo mon;
        struct DisplayInfo dis;
      } buffer;

      for( id = NextDisplayInfo( INVALID_ID );
           id != (ULONG) INVALID_ID;
           id = NextDisplayInfo( id ) )
      {
        int period;

        if( GetDisplayInfoData( NULL, 
                                (UBYTE*) &buffer.dis, sizeof(buffer.dis),
                                DTAG_DISP, id ) )
        {
          if( !(buffer.dis.PropertyFlags & (DIPF_IS_ECS | DIPF_IS_AA ) ) )
          {
            continue;
          }
        }

        if( GetDisplayInfoData( NULL,
                                (UBYTE*) &buffer.mon, sizeof(buffer.mon),
                                DTAG_MNTR, id ) )
        {
          period = buffer.mon.TotalColorClocks * buffer.mon.TotalRows
                   / ( 2 * ( buffer.mon.TotalRows - buffer.mon.MinRow + 1 ) );

          if( period < minper )
          {
            minper = period;
            bestid = id;
          }
        }

      }

      if( bestid != (ULONG) INVALID_ID && minper < 100 )
      {
        screen = OpenScreenTags( NULL,
                                 SA_DisplayID,  bestid,
                                 SA_Colors,    (ULONG) &colorspecs,
                                 TAG_DONE );
      }
      else if( ( GfxBase->ChipRevBits0 & (GFXF_HR_DENISE | GFXF_AA_LISA ) ) != 0 )
      {
        /* No suitable screen mode found, let's bang the hardware...
           Using code from Sebastiano Vigna <vigna@eolo.usr.dsi.unimi.it>. */

        extern struct Custom custom;

        custom.bplcon0  = 0x8211;
        custom.ddfstrt  = 0x0018;
        custom.ddfstop  = 0x0058;
        custom.hbstrt   = 0x0009;
        custom.hsstop   = 0x0017;
        custom.hbstop   = 0x0021;
        custom.htotal   = 0x0071;
        custom.vbstrt   = 0x0000;
        custom.vsstrt   = 0x0003;
        custom.vsstop   = 0x0005;
        custom.vbstop   = 0x001D;
        custom.vtotal   = 0x020E;
        custom.beamcon0 = 0x0B88;
        custom.bplcon1  = 0x0000;
        custom.bplcon2  = 0x027F;
        custom.bplcon3  = 0x00A3;
        custom.bplcon4  = 0x0011;
      }

      if( screen != NULL )
      {
        CloseScreen( screen );
      }
    }

    FreeArgs( rdargs );
  }

  cleanup( rc );
}
