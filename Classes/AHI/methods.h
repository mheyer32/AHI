#ifndef AHI_Classes_AHI_methods_h
#define AHI_Classes_AHI_methods_h

LONG
MethodNew(Class* class, Object* object, struct opSet* msg);

void
MethodDispose(Class* class, Object* object, Msg msg);

ULONG
MethodUpdate(Class* class, Object* object, struct opUpdate* msg);

BOOL
MethodGet(Class* class, Object* object, struct opGet* msg);

void
MethodLock(Class* class, Object* object, Msg msg);

void
MethodUnlock(Class* class, Object* object, Msg msg);

#endif /* AHI_Classes_AHI_methods_h */
