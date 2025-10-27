/* Automatically generated header (sfdc 1.11f)! Do not edit! */

#ifndef CLIB_GIC400_PROTOS_H
#define CLIB_GIC400_PROTOS_H

/*
**   $VER: gic400_protos.h 1.0.0 $Id: gic400.sfd 1.0.0 $
**
**   C prototypes. For use with 32 bit integers only.
**
**   Copyright (c) 2001 Amiga, Inc.
**       All Rights Reserved
*/

#include <dos/dos.h>
#include <exec/execbase.h>
#include <exec/types.h>
#include <exec/interrupts.h>
#include <libraries/gic400.h>

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */


/*--- functions in V0 or higher ---*/
struct Library* LibInit(struct Library* library, BPTR seglist, struct ExecBase* SysBase);
struct Library* LibOpen(ULONG version);
BPTR LibClose(void);
BPTR LibExpunge(void);
ULONG LibNull(void);

/*--- functions in V1 or higher ---*/

/* "gic400.library" */
ULONG AddIntServerEx(ULONG irq, UBYTE priority, BOOL edge, struct Interrupt * interrupt);
ULONG RemIntServerEx(ULONG irq, struct Interrupt * interrupt);
ULONG GetIntStatus(ULONG irq, BOOL * pending, BOOL * active, BOOL * enabled);
ULONG EnableInt(ULONG irq);
ULONG DisableInt(ULONG irq);
ULONG SetIntPriority(ULONG irq, UBYTE priority);
LONG GetIntPriority(ULONG irq);
ULONG SetIntTriggerEdge(ULONG irq);
ULONG SetIntTriggerLevel(ULONG irq);
ULONG RouteIntToCpu(ULONG irq, UBYTE cpu);
ULONG UnrouteIntFromCpu(ULONG irq, UBYTE cpu);
LONG QueryIntRoute(ULONG irq);
ULONG SetIntPending(ULONG irq);
ULONG ClearIntPending(ULONG irq);
ULONG SetIntActive(ULONG irq);
ULONG ClearIntActive(ULONG irq);
ULONG SetPriorityMask(UBYTE mask);
LONG GetPriorityMask(void);
LONG GetRunningPriority(void);
LONG GetHighestPending(void);
LONG GetControllerInfo(struct GICInfo * info);

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* CLIB_GIC400_PROTOS_H */
