
#include <config.h>

#include <classes/ahi.h>

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
	  if (IoErr() == 0) {
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
      DoMethod(object, AHIM_Lock);
      DoSuperMethodA(class, object, msg);
      result = MethodUpdate(class, object, (struct opUpdate*) msg);
      DoMethod(object, AHIM_Unlock);
      break; 

    case OM_GET:
      DoMethod(object, AHIM_Lock);
      result = MethodGet(class, object, (struct opGet*) msg);
      
      if (!result) {
	result = DoSuperMethodA(class, object, msg);
      }

      DoMethod(object, AHIM_Unlock);
      break;

    case OM_NOTIFY:
      DoMethod(object, AHIM_Lock);
      DoMethodA(AHIClassData->model_class, msg);
      DoMethod(object, AHIM_Unlock);
      break;

    case AHIM_AddNotify:
      DoMethod(object, AHIM_Lock);
      DoMethod(AHIClassData->model_class, OM_ADDMEMBER,
	       ((struct AHIP_Notify*) msg)->Obj);
      DoMethod(object, AHIM_Unlock);
      break;

    case AHIM_RemNotify:
      DoMethod(object, AHIM_Lock);
      DoMethod(AHIClassData->model_class, OM_REMMEMBER,
	       ((struct AHIP_Notify*) msg)->Obj);
      DoMethod(object, AHIM_Unlock);
      break;
      
    case AHIM_Lock:
      MethodLock(class, object, msg);
      break;

    case AHIM_Unlock:
      MethodUnlock(class, object, msg);
      break;

    case AHIM_GetParamValue:
      result = 0;
      break;
      
    default:
      result = DoSuperMethodA(class, object, msg);
      break;

  }
  
  return result;
}
