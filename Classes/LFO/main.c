
#include <config.h>

#include <classes/ahi/lfo.h>

#include <clib/alib_protos.h>
#include <proto/dos.h>
#include <proto/exec.h>
#include <proto/intuition.h>

#include "ahiclass.h"
#include "methods.h"
#include "util.h"

BOOL
AHIClassInit(struct AHIClassBase* AHIClassBase) {
  AHIClassBase->super = (struct ClassLibrary*) OpenLibrary("AHI/" AHIC_AHI, 7);

  if (AHIClassBase->super == NULL) { 
    Req("Unable to open super class " AHIC_AHI);
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

    default:
      result = DoSuperMethodA(class, object, msg);
      break;
  }
  
  return result;
}
