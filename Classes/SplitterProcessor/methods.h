#ifndef AHI_Classes_SplitterProcessor_methods_h
#define AHI_Classes_SplitterProcessor_methods_h

#include <classes/ahi/processor/splitter.h>

LONG
MethodNew(Class* class, Object* object, struct opSet* msg);

void
MethodDispose(Class* class, Object* object, Msg msg);

ULONG
MethodUpdate(Class* class, Object* object, struct opUpdate* msg);

BOOL
MethodGet(Class* class, Object* object, struct opGet* msg);

ULONG
MethodClone(Class* class, Object* object, Msg msg);

ULONG
MethodProcess(Class* class, Object* object, struct AHIP_Processor_Process* msg);

#endif /* AHI_Classes_SplitterProcessor_methods_h */
