
#include <classes/ahi.h>
#include <clib/alib_protos.h>
#include <string.h>

#include "ahiclass.h"
#include "util.h"

#define HASH 2654435761UL
#define BINS 16

struct DispatcherEntry {
    ULONG  id;
    struct DispatcherEntry* next;
    Class* class;
    ULONG  (*func)(Class* cl, Object* obj, Msg msg);
};

struct DispatcherData {
    ULONG                   methods;
    ULONG                   gets;
    ULONG                   sets;
    struct DispatcherEntry* bins[BINS];
    struct DispatcherEntry  entries[0];
};

static ULONG
dispatcher(Class* class, Object* object, struct AHIP_NewDispatcher* msg) {
  return 0;
}


ULONG
MethodNewDispatcher(Class* class, Object* object, struct AHIP_NewDispatcher* msg) {
  struct AHIClassBase* AHIClassBase = (struct AHIClassBase*) class->cl_UserData;
  struct DispatcherData*  data;
  struct DispatcherData*  parent = NULL;
  struct DispatcherEntry* entry;
  ULONG entries = 0;
  ULONG methods = 0;
  ULONG gets    = 0;
  ULONG sets    = 0;
  int i;

  if (class->cl_Super->cl_Dispatcher.h_Entry == HookEntry &&
      class->cl_Super->cl_Dispatcher.h_SubEntry == dispatcher) {
    parent = (struct DispatcherData*) class->cl_Super->cl_Dispatcher.h_Data;

    methods = parent->methods;
    gets    = parent->gets;
    sets    = parent->sets;
  }

  KPrintF("Parent: %ld methods, %ld gets, %ld sets\n", methods, gets, sets);
  
  for (i = 0; msg->Methods[i].FunctionPtr != NULL; ++i);
  entries += i;
  methods += i;

  for (i = 0; msg->SetAttributes[i].FunctionPtr != NULL; ++i);
  entries += i;
  sets += i;

  for (i = 0; msg->GetAttributes[i].FunctionPtr != NULL; ++i);
  entries += i;
  gets += i;
  
  KPrintF("Data: %ld methods, %ld gets, %ld sets\n", methods, gets, sets);

  data = AllocVec(sizeof (*data) + entries * sizeof (data->entries[0]),
		  MEMF_PUBLIC);

  if (data == NULL) {
    return ERROR_NO_FREE_STORE;
  }

  // Splash in parent's hash table
  if (parent != NULL) {
    bcopy(parent->bins, data->bins, sizeof (data->bins));
  }

  // Now add our own entries before parent's.
  entry = data->entries;
  for (i = 0; msg->Methods[i].FunctionPtr != NULL; ++i) {
    ULONG bin = (msg->Methods[i].ID * HASH & (BINS-1));

    entry->id    = msg->Methods[i].ID;
    entry->next  = data->bins[bin];
    entry->class = msg->Class;
    entry->func  = msg->Methods[i].FunctionPtr;

    data->bins[bin] = entry;

    ++entry;
  }

  msg->Class->cl_Dispatcher.h_Entry    = HookEntry;
  msg->Class->cl_Dispatcher.h_SubEntry = dispatcher;
  msg->Class->cl_Dispatcher.h_Data     = data;
  
  return 0;
}

void
MethodDisposeDispatcher(Class* class, Object* object,
			struct AHIP_DisposeDispatcher* msg) {
  struct AHIClassBase* AHIClassBase = (struct AHIClassBase*) class->cl_UserData;

  FreeVec(class->cl_Dispatcher.h_Data);
  class->cl_Dispatcher.h_Entry    = NULL;
  class->cl_Dispatcher.h_SubEntry = NULL;
  class->cl_Dispatcher.h_Data     = NULL;
}

LONG
MethodNew(Class* class, Object* object, Msg msg) {
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
