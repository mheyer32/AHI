
#include "ahiclass.h"

LONG
MethodNew(Class*  class,
	  Object* object,
	  Msg     msg) {
  struct AHIClassBase* AHIClassBase = (struct AHIClassBase*) class->cl_UserData;
  struct AHIClassData* AHIClassData = (struct AHIClassData*) INST_DATA(class, object);

  return 0;
}

void
MethodDispose(Class*  class,
	      Object* object,
	      Msg     msg) {
  struct AHIClassBase* AHIClassBase = (struct AHIClassBase*) class->cl_UserData;
  struct AHIClassData* AHIClassData = (struct AHIClassData*) INST_DATA(class, object);

}
