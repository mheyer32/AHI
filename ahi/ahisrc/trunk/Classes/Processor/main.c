
#include <classes/ahi.h>
#include <classes/ahi/processor.h>

#include <clib/alib_protos.h>
#include <proto/dos.h>
#include <proto/exec.h>
#include <proto/intuition.h>

#include "ahiclass.h"
#include "methods.h"
#include "util.h"

BOOL
AHIClassInit(struct AHIClassBase* AHIClassBase) {
  AHIClassBase->super = (struct ClassLibrary*) OpenLibrary("AHI/" AHI_CLASS, 7);

  if (AHIClassBase->super == NULL) { 
    Req("Unable to open super class " AHI_CLASS);
    return FALSE;
  }
  
  AHIClassBase->common.cl.cl_Class =
    MakeClass(AHIClassBase->common.cl.cl_Lib.lib_Node.ln_Name,
	      NULL, AHIClassBase->super->cl_Class,
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
  CloseLibrary((struct Library*) AHIClassBase->super);
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

    case OM_ADDMEMBER:
      result = MethodAddMember(class, object, (struct opMember*) msg);
      break;

    case OM_REMMEMBER:
      result = MethodRemMember(class, object, (struct opMember*) msg);
      break;
      
    case AHIM_Processor_Prepare:
      result = MethodPrepare(class, object, (struct AHIP_Processor_Process*) msg);
      break;

    case AHIM_Processor_Process:
      result = MethodProcess(class, object, (struct AHIP_Processor_Process*) msg);
      break;

      
    default:
      KPrintF(_AHI_CLASS_NAME " passes method %lx to superclass\n", msg->MethodID);
      result = DoSuperMethodA(class, object, msg);
      break;

  }
  
  return result;
}
