#ifndef AHI_Classes_Common_util_h
#define AHI_Classes_Common_util_h

#include <exec/types.h>

/*** Put up a requester ******************************************************/

void
ReqA( const char*           fmt,
      APTR                  args );

#define Req( fmt, ... ) 		\
({					\
  ULONG _args[] = { __VA_ARGS__ };	\
  ReqA( (fmt), _args );			\
})


/*** Send debug data *********************************************************/

void
MyKPrintFArgs( const char*           fmt,
	       APTR                  args );

#define KPrintF( fmt, ... )        \
({                                 \
  ULONG _args[] = { __VA_ARGS__ }; \
  MyKPrintFArgs( (fmt), _args );   \
})

/*** SPrintF *****************************************************************/

void
MySPrintFArgs( char*       buffer,
	       const char* fmt,
	       APTR        args );

#define SPrintF( buffer, fmt, ... )        \
({                                         \
  ULONG _args[] = { __VA_ARGS__ };         \
  MySPrintFArgs( (buffer), (fmt), _args ); \
})

/*** Interrupt and process glue **********************************************/

#if defined(__MORPHOS__)
# include <emul/emulregs.h>
# define INTGW(q,t,n,f)							\
	q t n ## _code(void) { APTR d = (APTR) REG_A1; return f(d); }	\
	q struct EmulLibEntry n = { TRAP_LIB, 0, (APTR) n ## _code };
# define PROCGW(q,t,n,f)						\
	q struct EmulLibEntry n = { TRAP_LIB, 0, (APTR) f };
#elif defined(__amithlon__)
# define INTGW(q,t,n,f)							\
	__asm("	.text");						\
	__asm("	.align 4");						\
	__asm("	.type " #n "_code,@function");				\
	__asm("	.type " #n ",@function");				\
	__asm(#n "_code: movl %ebp,%eax");				\
	__asm(" bswap %eax");						\
	__asm("	pushl %eax");						\
	__asm("	call " #f );						\
	__asm(" addl $4,%esp");						\
	__asm(" ret");							\
	__asm(#n "=" #n "_code+1");					\
	__asm(".section .rodata");					\
	q t n(APTR);
# define PROCGW(q,t,n,f)						\
	__asm(#n "=" #f "+1");						\
	q t n(void);
#elif defined(__AROS__)
# include <aros/asmcall.h>
# define INTGW(q,t,n,f)							\
	q AROS_UFH4(t, n,						\
	  AROS_UFHA(ULONG, _a, A0),					\
	  AROS_UFHA(APTR, d, A1),					\
	  AROS_UFHA(ULONG, _b, A5),					\
	  AROS_UFHA(struct ExecBase *, sysbase, A6)) {			\
      AROS_USERFUNC_INIT return f(d); AROS_USERFUNC_EXIT }
# define PROCGW(q,t,n,f)						\
	q AROS_UFH0(t, n) {						\
      AROS_USERFUNC_INIT return f(); AROS_USERFUNC_EXIT }
#elif defined(__amiga__) && defined(__mc68000__)
# define INTGW(q,t,n,f)							\
	q t n(APTR d __asm("a1")) { return f(d); }
# define PROCGW(q,t,n,f)						\
	__asm("_" #n "= _" #f);						\
	q t n(void);
#else
# error Unknown OS/CPU
#endif

/*** Endian support **********************************************************/

#include <machine/endian.h>

#if BYTE_ORDER == BIG_ENDIAN

static inline UWORD
ReadUWORD_LE( UWORD x ) {
  return ((((x) >> 8) & 0x00ffUL) |
	  (((x) << 8) & 0xff00UL));
}

static inline UWORD
ReadUWORD_BE( UWORD x ) {
  return x;
}

static inline ULONG
ReadULONG_LE( ULONG x ) {
  return (((x >> 24) & 0x000000ffUL) |
	  ((x >> 8)  & 0x0000ff00UL) |
	  ((x << 8)  & 0x00ff0000UL) |
	  ((x << 24) & 0xff000000UL) );
}

static inline ULONG
ReadULONG_BE( ULONG x ) {
  return x;
}

#elif BYTE_ORDER == LITTLE_ENDIAN

static inline UWORD
ReadUWORD_LE( UWORD x ) {
  return x;
}

static inline UWORD
ReadUWORD_BE( UWORD x ) {
  return ((((x) >> 8) & 0x00ffUL) |
	  (((x) << 8) & 0xff00UL));
}

static inline ULONG
ReadULONG_LE( ULONG x ) {
  return x;
}

static inline ULONG
ReadULONG_BE( ULONG x ) {
  return (((x >> 24) & 0x000000ffUL) |
	  ((x >> 8)  & 0x0000ff00UL) |
	  ((x << 8)  & 0x00ff0000UL) |
	  ((x << 24) & 0xff000000UL) );
}

#else
# error Unsupported byte order
#endif

#endif /* AHI_Classes_Common_util_h */
