
#include <config.h>

#include <math.h>

#include <classes/ahi/driver.h>

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
  struct ClassData* ClassData = (struct ClassData*) class->cl_UserData;
  struct ObjectData* ObjectData = (struct ObjectData*) INST_DATA(class, object);

  return 0;
}


/******************************************************************************
** MethodDispose **************************************************************
******************************************************************************/

void
MethodDispose(Class* class, Object* object, Msg msg) {
  struct ClassData* ClassData = (struct ClassData*) class->cl_UserData;
  struct ObjectData* ObjectData = (struct ObjectData*) INST_DATA(class, object);
}


/******************************************************************************
** MethodSet ******************************************************************
******************************************************************************/
		    
ULONG
MethodUpdate(Class* class, Object* object, struct opUpdate* msg) {
  struct ClassData* ClassData = (struct ClassData*) class->cl_UserData;
  struct ObjectData* ObjectData = (struct ObjectData*) INST_DATA(class, object);

  struct TagItem* tstate = msg->opu_AttrList;
  struct TagItem* tag;

  while ((tag = NextTagItem(&tstate))) {
    
    switch (tag->ti_Tag) {
      // No writable attributes
      
      default:
	break;
    }
  }

  return 0;
}


/******************************************************************************
** MethodGet ******************************************************************
******************************************************************************/

BOOL
MethodGet(Class* class, Object* object, struct opGet* msg) {
  struct ClassData* ClassData = (struct ClassData*) class->cl_UserData;
  struct ObjectData* ObjectData = (struct ObjectData*) INST_DATA(class, object);

  switch (msg->opg_AttrID) {
    case AHIA_Title:
      *msg->opg_Storage = (ULONG) "AHI Driver";
      break;

    case AHIA_Description:
      *msg->opg_Storage = (ULONG) "Sound card driver base class";
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

      // Subclasses are expected to override these:

    case AHIA_Driver_Boards:
      *msg->opg_Storage = 0;
      break;

    case AHIA_Driver_BoardClass:
      *msg->opg_Storage = 0;
      break;

    case AHIA_Driver_ID:
      *msg->opg_Storage = 0x1f; // This is the experimental ID
      break;
      
    default:
      return FALSE;
  }

  return TRUE;
}


/******************************************************************************
** MethodNewObject ************************************************************
******************************************************************************/

Object*
MethodNewObject(Class* class, Object* object, struct AHIP_Driver_NewObject* msg) {
  struct ClassData* ClassData = (struct ClassData*) class->cl_UserData;
  struct ObjectData* ObjectData = (struct ObjectData*) INST_DATA(class, object);

  return NewObjectA(msg->Class, NULL, msg->TagList);
}
