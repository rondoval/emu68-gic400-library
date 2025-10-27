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

#define EnableInt(___irq) \
      LP1(0x30, ULONG, EnableInt , ULONG, ___irq, d0,\
      , GIC400_BASE_NAME)

#define DisableInt(___irq) \
      LP1(0x36, ULONG, DisableInt , ULONG, ___irq, d0,\
      , GIC400_BASE_NAME)

#define SetIntPriority(___irq, ___priority) \
      LP2(0x3c, ULONG, SetIntPriority , ULONG, ___irq, d0, UBYTE, ___priority, d1,\
      , GIC400_BASE_NAME)

#define GetIntPriority(___irq) \
      LP1(0x42, LONG, GetIntPriority , ULONG, ___irq, d0,\
      , GIC400_BASE_NAME)

#define SetIntTriggerEdge(___irq) \
      LP1(0x48, ULONG, SetIntTriggerEdge , ULONG, ___irq, d0,\
      , GIC400_BASE_NAME)

#define SetIntTriggerLevel(___irq) \
      LP1(0x4e, ULONG, SetIntTriggerLevel , ULONG, ___irq, d0,\
      , GIC400_BASE_NAME)

#define RouteIntToCpu(___irq, ___cpu) \
      LP2(0x54, ULONG, RouteIntToCpu , ULONG, ___irq, d0, UBYTE, ___cpu, d1,\
      , GIC400_BASE_NAME)

#define UnrouteIntFromCpu(___irq, ___cpu) \
      LP2(0x5a, ULONG, UnrouteIntFromCpu , ULONG, ___irq, d0, UBYTE, ___cpu, d1,\
      , GIC400_BASE_NAME)

#define QueryIntRoute(___irq) \
      LP1(0x60, LONG, QueryIntRoute , ULONG, ___irq, d0,\
      , GIC400_BASE_NAME)

#define SetIntPending(___irq) \
      LP1(0x66, ULONG, SetIntPending , ULONG, ___irq, d0,\
      , GIC400_BASE_NAME)

#define ClearIntPending(___irq) \
      LP1(0x6c, ULONG, ClearIntPending , ULONG, ___irq, d0,\
      , GIC400_BASE_NAME)

#define SetIntActive(___irq) \
      LP1(0x72, ULONG, SetIntActive , ULONG, ___irq, d0,\
      , GIC400_BASE_NAME)

#define ClearIntActive(___irq) \
      LP1(0x78, ULONG, ClearIntActive , ULONG, ___irq, d0,\
      , GIC400_BASE_NAME)

#define SetPriorityMask(___mask) \
      LP1(0x7e, ULONG, SetPriorityMask , UBYTE, ___mask, d0,\
      , GIC400_BASE_NAME)

#define GetPriorityMask() \
      LP0(0x84, LONG, GetPriorityMask ,\
      , GIC400_BASE_NAME)

#define GetRunningPriority() \
      LP0(0x8a, LONG, GetRunningPriority ,\
      , GIC400_BASE_NAME)

#define GetHighestPending() \
      LP0(0x90, LONG, GetHighestPending ,\
      , GIC400_BASE_NAME)

#define GetControllerInfo(___info) \
      LP1(0x96, LONG, GetControllerInfo , struct GICInfo *, ___info, a1,\
      , GIC400_BASE_NAME)

#endif /* !_INLINE_GIC400_H */
