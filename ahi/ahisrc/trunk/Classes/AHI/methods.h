#ifndef AHI_Classes_AHIRootClass_methods_h
#define AHI_Classes_AHIRootClass_methods_h

LONG
MethodNew(Class*  class,
	  Object* object,
	  Msg     msg);

void
MethodDispose(Class*  class,
	      Object* object,
	      Msg     msg);

#endif /* AHI_Classes_AHIRootClass_methods_h */
