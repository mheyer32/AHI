#ifndef AHI_Classes_ResamplerProcessor_methods_h
#define AHI_Classes_ResamplerProcessor_methods_h

#include <classes/ahi/processor/resampler.h>

LONG
MethodNew(Class* class, Object* object, struct opSet* msg);

void
MethodDispose(Class* class, Object* object, Msg msg);

ULONG
MethodUpdate(Class* class, Object* object, struct opUpdate* msg);

BOOL
MethodGet(Class* class, Object* object, struct opGet* msg);

#endif /* AHI_Classes_ResamplerProcessor_methods_h */
