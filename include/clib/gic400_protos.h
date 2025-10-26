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

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* CLIB_GIC400_PROTOS_H */
