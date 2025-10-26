/* Automatically generated header (sfdc 1.11f)! Do not edit! */

#ifndef _INLINE_GIC400_H
#define _INLINE_GIC400_H

#ifndef _SFDC_VARARG_DEFINED
#define _SFDC_VARARG_DEFINED
#ifdef __HAVE_IPTR_ATTR__
typedef APTR _sfdc_vararg __attribute__((iptr));
#else
typedef ULONG _sfdc_vararg;
#endif /* __HAVE_IPTR_ATTR__ */
#endif /* _SFDC_VARARG_DEFINED */

#ifndef __INLINE_MACROS_H
#include <inline/macros.h>
#endif /* !__INLINE_MACROS_H */

#ifndef GIC400_BASE_NAME
#define GIC400_BASE_NAME GIC400_Base
#endif /* !GIC400_BASE_NAME */

#define LibInit(___library, ___seglist, ___SysBase) \
      LP3NB(0x0, struct Library*, LibInit , struct Library*, ___library, d0, BPTR, ___seglist, a0, struct ExecBase*, ___SysBase, a6)

#define LibOpen(___version) \
      LP1(0x6, struct Library*, LibOpen , ULONG, ___version, d0,\
      , GIC400_BASE_NAME)

#define LibClose() \
      LP0(0xc, BPTR, LibClose ,\
      , GIC400_BASE_NAME)

#define LibExpunge() \
      LP0(0x12, BPTR, LibExpunge ,\
      , GIC400_BASE_NAME)

#define LibNull() \
      LP0(0x18, ULONG, LibNull ,\
      , GIC400_BASE_NAME)

#define AddIntServerEx(___irq, ___priority, ___edge, ___interrupt) \
      LP4(0x1e, ULONG, AddIntServerEx , ULONG, ___irq, d0, UBYTE, ___priority, d1, BOOL, ___edge, d2, struct Interrupt *, ___interrupt, a1,\
      , GIC400_BASE_NAME)

#define RemIntServerEx(___irq, ___interrupt) \
      LP2(0x24, ULONG, RemIntServerEx , ULONG, ___irq, d0, struct Interrupt *, ___interrupt, a1,\
      , GIC400_BASE_NAME)

#define GetIntStatus(___irq, ___pending, ___active, ___enabled) \
      LP4(0x2a, ULONG, GetIntStatus , ULONG, ___irq, d0, BOOL *, ___pending, a1, BOOL *, ___active, a2, BOOL *, ___enabled, a3,\
      , GIC400_BASE_NAME)

#endif /* !_INLINE_GIC400_H */
