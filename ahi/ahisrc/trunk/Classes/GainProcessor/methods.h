#ifndef AHI_Classes_GainProcessor_methods_h
#define AHI_Classes_GainProcessor_methods_h

LONG
MethodNew(Class* class, Object* object, struct opSet* msg);

void
MethodDispose(Class* class, Object* object, Msg msg);

ULONG
MethodUpdate(Class* class, Object* object, struct opUpdate* msg);

BOOL
MethodGet(Class* class, Object* object, struct opGet* msg);

ULONG
MethodProcess(Class* class, Object* object, struct AHIP_Processor_Process* msg);

BOOL
MethodSetGain(Class* class, Object* object, struct AHIP_GainProcessor_Gain* msg);

BOOL
MethodGetGain(Class* class, Object* object, struct AHIP_GainProcessor_Gain* msg);

BOOL
MethodSetAllGain(Class* class, Object* object, struct AHIP_GainProcessor_GainAll* msg);

#endif /* AHI_Classes_GainProcessor_methods_h */
