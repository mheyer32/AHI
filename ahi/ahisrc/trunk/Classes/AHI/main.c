
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

  KPrintF(_AHI_CLASS_NAME " dispatching method %lx\n", msg->MethodID);

  switch (msg->MethodID)
  {
    case OM_NEW:
      result = DoSuperMethodA(class, object, msg);

      if (result != NULL)
      {
	LONG error = MethodNew(class, (Object*) result, msg);
	
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

    default:
      KPrintF(_AHI_CLASS_NAME " passes method %lx to superclass\n", msg->MethodID);
      result = DoSuperMethodA(class, object, msg);
      break;

  }
  
  return result;
}
