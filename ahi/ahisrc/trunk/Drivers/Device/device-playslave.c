
#include <config.h>

#include <devices/ahi.h>
#include <libraries/ahi_sub.h>

#include "DriverData.h"
#include "library.h"

#define dd ((struct DeviceData*) AudioCtrl->ahiac_DriverData)

/******************************************************************************
** The slave process **********************************************************
******************************************************************************/

#undef SysBase

static void Slave( struct ExecBase* SysBase );

#if defined( __AROS__ )

#include <aros/asmcall.h>

AROS_UFH3(LONG, SlaveEntry,
	  AROS_UFHA(STRPTR, argPtr, A0),
	  AROS_UFHA(ULONG, argSize, D0),
	  AROS_UFHA(struct ExecBase *, SysBase, A6))
{
   AROS_USERFUNC_INIT
   Slave( SysBase );
   return 0;
   AROS_USERFUNC_EXIT
}

#else

void SlaveEntry(void)
{
  struct ExecBase* SysBase = *((struct ExecBase**) 4);

  Slave( SysBase );
}

#endif


static void
Slave( struct ExecBase* SysBase )
{
  struct AHIAudioCtrlDrv* AudioCtrl;
  struct DriverBase*      AHIsubBase;
  struct AROSBase*        AROSBase;
  BOOL                    running;
  ULONG                   signals;

  int bytes_in_buffer  = 0;
  int offset_in_buffer = 0;

  AudioCtrl  = (struct AHIAudioCtrlDrv*) FindTask( NULL )->tc_UserData;
  AHIsubBase = (struct DriverBase*) dd->ahisubbase;
  AROSBase   = (struct AROSBase*) AHIsubBase;

  dd->slavesignal = AllocSignal( -1 );

  if( dd->slavesignal != -1 )
  {
    // Everything set up. Tell Master we're alive and healthy.

    Signal( (struct Task*) dd->mastertask,
            1L << dd->mastersignal );

    running = TRUE;

    // The main playback loop follow

    while( running )
    {      
      signals = SetSignal(0L,0L);

      if( signals & ( SIGBREAKF_CTRL_C | (1L << dd->slavesignal) ) )
      {
        running = FALSE;
      }
      else
      {
	int skip_mix;

	skip_mix = CallHookA( AudioCtrl->ahiac_PreTimerFunc,
			      (Object*) AudioCtrl, 0 );

	CallHookPkt( AudioCtrl->ahiac_PlayerFunc, AudioCtrl, NULL );

	if( ! skip_mix )
	{
	  CallHookPkt( AudioCtrl->ahiac_MixerFunc, AudioCtrl,
		       dd->mixbuffer );
	}

	CallHookA( AudioCtrl->ahiac_PostTimerFunc, (Object*) AudioCtrl, 0 );
      }
    }
  }

  FreeSignal( dd->slavesignal );
  dd->slavesignal = -1;

  Forbid();

  // Tell the Master we're dying

  Signal( (struct Task*) dd->mastertask,
          1L << dd->mastersignal );

  dd->slavetask = NULL;

  // Multitaking will resume when we are dead.
}
