#ifndef AHI_Classes_Processor_methods_h
#define AHI_Classes_Processor_methods_h

LONG
MethodNew(Class* class, Object* object, struct opSet* msg);

void
MethodDispose(Class* class, Object* object, Msg msg);

ULONG
MethodUpdate(Class* class, Object* object, struct opUpdate* msg);

BOOL
MethodGet(Class* class, Object* object, struct opGet* msg);

BOOL
MethodPrepare(Class* class, Object* object, struct AHIP_Processor_Process* msg);

BOOL
MethodProcess(Class* class, Object* object, struct AHIP_Processor_Process* msg);

BOOL
MethodAddMember(Class* class, Object* object, struct opMember* msg);

BOOL
MethodRemMember(Class* class, Object* object, struct opMember* msg);

#endif /* AHI_Classes_Processor_methods_h */
