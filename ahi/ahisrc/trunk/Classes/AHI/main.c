
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
  AHIClassBase->common.cl.cl_Class =
    MakeClass(AHIClassBase->common.cl.cl_Lib.lib_Node.ln_Name,
	      ROOTCLASS, NULL,
	      sizeof (struct AHIClassData), 0);

  if (AHIClassBase->common.cl.cl_Class == NULL)
  {
    Req("Unable to create " _AHI_CLASS_NAME " Class.");
    return FALSE;
  }

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
	  SetIoErr(error);
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
      DoSuperMethodA(class, object, msg);
      result = MethodUpdate(class, object, (struct opUpdate*) msg);
      break; 

    case OM_GET:
      result = MethodGet(class, object, (struct opGet*) msg);
      
      if (!result) {
	result = DoSuperMethodA(class, object, msg);
      }
      break;

    case OM_NOTIFY:
      DoMethodA(AHIClassData->model_class, msg);
      break;

    case AHIM_AddNotify:
      DoMethod(AHIClassData->model_class, OM_ADDMEMBER,
	       ((struct AHIP_Notify*) msg)->Obj);
      break;

    case AHIM_RemNotify:
      DoMethod(AHIClassData->model_class, OM_REMMEMBER,
	       ((struct AHIP_Notify*) msg)->Obj);
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
