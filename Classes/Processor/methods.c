
#include <classes/ahi/processor.h>

#include <clib/alib_protos.h>
#include <proto/intuition.h>

#include "ahiclass.h"
#include "methods.h"
#include "util.h"
#include "version.h"


/******************************************************************************
** MethodNew ******************************************************************
******************************************************************************/

LONG
MethodNew(Class* class, Object* object, struct opSet* msg) {
  struct AHIClassBase* AHIClassBase = (struct AHIClassBase*) class->cl_UserData;
  struct AHIClassData* AHIClassData = (struct AHIClassData*) INST_DATA(class, object);

  NewList((struct List*) &AHIClassData->members);

  MethodUpdate(class, object, (struct opUpdate*) msg);

  return 0;
}


/******************************************************************************
** MethodDispose **************************************************************
******************************************************************************/

void
MethodDispose(Class* class, Object* object, Msg msg) {
  struct AHIClassBase* AHIClassBase = (struct AHIClassBase*) class->cl_UserData;
  struct AHIClassData* AHIClassData = (struct AHIClassData*) INST_DATA(class, object);

  Object* ostate = (Object*) &AHIClassData->members.mlh_Head;
  Object* member;

  while ((member = NextObject(&ostate)) != NULL) {
    DoMethod(member, OM_REMOVE);
    DoMethodA(member, msg);
  }
}


/******************************************************************************
** MethodSet ******************************************************************
******************************************************************************/

ULONG
MethodUpdate(Class* class, Object* object, struct opUpdate* msg)
{
  struct AHIClassBase* AHIClassBase = (struct AHIClassBase*) class->cl_UserData;
  struct AHIClassData* AHIClassData = (struct AHIClassData*) INST_DATA(class, object);

  struct TagItem* tstate = msg->opu_AttrList;
  struct TagItem* tag;

  while ((tag = NextTagItem(&tstate))) {
    switch (tag->ti_Tag) {
      case AHIA_Processor_Buffer:
	AHIClassData->buffer = (Object*) tag->ti_Data;
	NotifySuper(class, object, msg, tag->ti_Tag, tag->ti_Data);
	break;

      case AHIA_Processor_Parent:
	AHIClassData->parent = (Object*) tag->ti_Data;
	break;

      case AHIA_Processor_Disabled:
	AHIClassData->disabled = tag->ti_Data;
	break;
	
      case AHIA_Processor_AddChild: {
	struct opMember om = {
	  OM_ADDMEMBER, (Object*) tag->ti_Data
	};

	MethodAddMember(class, object, &om);
	break;
      }

      case AHIA_Processor_Busy:
	AHIClassData->busy = tag->ti_Data;
	break;

      default:
	KPrintF("Unknown NEW/SET attribute in " _AHI_CLASS_NAME ": %08lx, %08lx\n",
		tag->ti_Tag, tag->ti_Data);
	break;
    }
  }

  return 0;
}


/******************************************************************************
** MethodGet ******************************************************************
******************************************************************************/

BOOL
MethodGet(Class* class, Object* object, struct opGet* msg)
{
  struct AHIClassBase* AHIClassBase = (struct AHIClassBase*) class->cl_UserData;
  struct AHIClassData* AHIClassData = (struct AHIClassData*) INST_DATA(class, object);

  switch (msg->opg_AttrID)
  {
    case AHIA_Title:
      *msg->opg_Storage = (ULONG) "AHI Processor Base Class";
      break;

    case AHIA_Description:
      *msg->opg_Storage = (ULONG) "The mother of all processor classes.";
      break;
      
    case AHIA_DescriptionURL:
      *msg->opg_Storage = (ULONG) "http://www.lysator.liu.se/ahi/";
      break;
      
    case AHIA_Author:
      *msg->opg_Storage = (ULONG) "Martin Blom";
      break;
      
    case AHIA_Copyright:
      *msg->opg_Storage = (ULONG) "©2004 Martin Blom";
      break;
      
    case AHIA_Version:
      *msg->opg_Storage = (ULONG) VERS;
      break;
      
    case AHIA_Annotation:
      *msg->opg_Storage = 0;
      break;
      
    case AHIA_Processor_Buffer:
      *msg->opg_Storage = (ULONG) AHIClassData->buffer;
      break;

    case AHIA_Processor_Parent:
      *msg->opg_Storage = (ULONG) AHIClassData->parent;
      break;

    case AHIA_Processor_Disabled:
      *msg->opg_Storage = AHIClassData->disabled;
      break;
	
    case AHIA_Processor_Busy:
      *msg->opg_Storage = AHIClassData->busy;
      break;

    default:
      return FALSE;
  }

  return TRUE;
}


/******************************************************************************
** MethodPrepare **************************************************************
******************************************************************************/

BOOL
MethodPrepare(Class* class, Object* object, struct AHIP_Processor_Process* msg) {
  struct AHIClassBase* AHIClassBase = (struct AHIClassBase*) class->cl_UserData;
  struct AHIClassData* AHIClassData = (struct AHIClassData*) INST_DATA(class, object);

  Object* ostate = (Object*) &AHIClassData->members.mlh_Head;
  Object* member;

  BOOL result = FALSE;
  
  while ((member = NextObject(&ostate)) != NULL) {
    result |= DoMethodA(member, (Msg) msg);
  }

  return result != FALSE;
}


/******************************************************************************
** MethodProcess **************************************************************
******************************************************************************/

BOOL
MethodProcess(Class* class, Object* object, struct AHIP_Processor_Process* msg) {
  struct AHIClassBase* AHIClassBase = (struct AHIClassBase*) class->cl_UserData;
  struct AHIClassData* AHIClassData = (struct AHIClassData*) INST_DATA(class, object);

  Object* ostate = (Object*) &AHIClassData->members.mlh_Head;
  Object* member;

  BOOL result = FALSE;
  
  while ((member = NextObject(&ostate)) != NULL) {
    result |= DoMethodA(member, (Msg) msg);
  }

  return result != FALSE;
}


/******************************************************************************
** MethodAddMember ************************************************************
******************************************************************************/

BOOL
MethodAddMember(Class* class, Object* object, struct opMember* msg) {
  struct AHIClassBase* AHIClassBase = (struct AHIClassBase*) class->cl_UserData;
  struct AHIClassData* AHIClassData = (struct AHIClassData*) INST_DATA(class, object);

  return DoMethod(msg->opam_Object, OM_ADDTAIL, &AHIClassData->members);
}


/******************************************************************************
** MethodRemMember ************************************************************
******************************************************************************/

BOOL
MethodRemMember(Class* class, Object* object, struct opMember* msg) {
  struct AHIClassBase* AHIClassBase = (struct AHIClassBase*) class->cl_UserData;
  struct AHIClassData* AHIClassData = (struct AHIClassData*) INST_DATA(class, object);

  return DoMethod(msg->opam_Object, OM_REMOVE);
}
