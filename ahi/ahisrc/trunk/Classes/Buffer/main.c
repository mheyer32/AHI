
#include <config.h>

#include <classes/ahi.h>
#include <classes/ahi/buffer.h>

#include <clib/alib_protos.h>
#include <proto/dos.h>
#include <proto/exec.h>
#include <proto/intuition.h>

#include "ahiclass.h"
#include "methods.h"
#include "util.h"

BOOL
AHIClassInit(struct AHIClassBase* AHIClassBase) {
  return TRUE;
}


VOID
AHIClassCleanup(struct AHIClassBase* AHIClassBase) {
}


ULONG
AHIClassDispatch(Class*  class,
		 Object* object,
		 Msg     msg) {
  struct AHIClassBase* AHIClassBase = (struct AHIClassBase*) class->cl_UserData;
  struct AHIClassData* AHIClassData = (struct AHIClassData*) INST_DATA(class, object);
  ULONG result = 0;

  switch (msg->MethodID)
  {
    case OM_NEW:
      result = DoSuperMethodA(class, object, msg);

      if (result != NULL)
      {
	LONG error = MethodNew(class, (Object*) result, (struct opSet*) msg);
	
	if (error)
	{
	  if (IoErr() != 0) {
	    SetIoErr(error);
	  }
	  CoerceMethod(class, (Object*) result, OM_DISPOSE);
	  result = NULL;
	}
      }
      break;

    case OM_DISPOSE:
      MethodDispose(class, object, msg);
      
      result = DoSuperMethodA(class, object, msg);
      break;

    case OM_SET:
    case OM_UPDATE:
      DoSuperMethod(class,object, AHIM_Lock);
      DoSuperMethodA(class, object, msg);
      result = MethodUpdate(class, object, (struct opUpdate*) msg);
      DoSuperMethod(class,object, AHIM_Unlock);
      break; 

    case OM_GET:
      DoSuperMethod(class,object, AHIM_Lock);
      result = MethodGet(class, object, (struct opGet*) msg);
      
      if (!result) {
	result = DoSuperMethodA(class, object, msg);
      }

      DoSuperMethod(class,object, AHIM_Unlock);
      break;

    case AHIM_Buffer_SampleFrameSize:
      DoSuperMethod(class,object, AHIM_Lock);
      result = MethodSampleFrameSize(class, object,
				     (struct AHIP_Buffer_SampleFrameSize*) msg);
      DoSuperMethod(class,object, AHIM_Unlock);
      break;

    case AHIM_Buffer_Load:
      DoSuperMethod(class,object, AHIM_Lock);
      result = MethodLoad(class, object, (struct AHIP_Buffer_Load*) msg);
      DoSuperMethod(class,object, AHIM_Unlock);
      break;

    case AHIM_Buffer_Clone:
      DoSuperMethod(class,object, AHIM_Lock);
      result = (ULONG) MethodClone(class, object, msg);
      DoSuperMethod(class,object, AHIM_Unlock);
      break;
      
    case AHIM_Buffer_Shift:
      DoSuperMethod(class,object, AHIM_Lock);
      result = (ULONG) MethodShift(class, object, msg);
      DoSuperMethod(class,object, AHIM_Unlock);
      break;
      
    default:
      result = DoSuperMethodA(class, object, msg);
      break;

  }
  
  return result;
}
