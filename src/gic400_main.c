// SPDX-License-Identifier: GPL-2.0

#ifdef __INTELLISENSE__
#include <clib/exec_protos.h>
#else
#include <proto/exec.h>
#endif

#include <exec/memory.h>
#include <exec/execbase.h>
#include <exec/resident.h>

#include <gic400_private.h>
#include <compat.h>

int __attribute__((used, no_reorder)) doNotExecute()
{
    return -1;
}

extern const UBYTE endOfCode;
static const char libraryName[] = LIBRARY_NAME;
static const char libraryIdString[] = LIBRARY_IDSTRING;
static const APTR initTable[4];

const struct Resident gicResident __attribute__((used)) = {
    RTC_MATCHWORD,
    (struct Resident *)&gicResident,
    (APTR)&endOfCode,
    RTF_AUTOINIT,
    LIBRARY_VERSION,
    NT_LIBRARY,
    LIBRARY_PRIORITY,
    (APTR)&libraryName,
    (APTR)&libraryIdString,
    (APTR)initTable,
};

struct ExecBase *SysBase;

static ULONG LibExpunge(struct GIC_Base *gicBase asm("a6"))
{
    ULONG segList = gicBase->segList;

    if (gicBase->libNode.lib_OpenCnt > 0)
    {
        gicBase->libNode.lib_Flags |= LIBF_DELEXP;
        return 0;
    }

    gic400_shutdown(gicBase);

    Forbid();
    Remove((struct Node *)gicBase);
    Permit();

    /* Calculate size of library base and deallocate memory */
    ULONG size = gicBase->libNode.lib_NegSize + gicBase->libNode.lib_PosSize;
    FreeMem((APTR)((ULONG)gicBase - gicBase->libNode.lib_NegSize), size);

    return segList;
}

struct Library *LibInit(struct Library *base asm("d0"), ULONG seglist asm("a0"), struct ExecBase *execBase asm("a6"))
{
    struct GIC_Base *gicBase = (struct GIC_Base *)base;
    SysBase = execBase;

    gicBase->segList = seglist;
    gicBase->libNode.lib_Revision = LIBRARY_REVISION;

    int res = gic400_init(gicBase);
    if (res != 0)
    {
        Kprintf("[gic] %s: Failed to initialize GIC-400 library\n", __func__);
        LibExpunge(gicBase);
        return NULL;
    }

    InitSemaphore(&gicBase->semaphore);

    return base;
}

static struct GIC_Base *LibOpen(ULONG version asm("d0"), struct GIC_Base *gicBase asm("a6"))
{
    (void)version;
    gicBase->libNode.lib_OpenCnt++;
    gicBase->libNode.lib_Flags &= ~LIBF_DELEXP;
    return gicBase;
}

static ULONG LibClose(struct GIC_Base *gicBase asm("a6"))
{
    gicBase->libNode.lib_OpenCnt--;

    if (gicBase->libNode.lib_OpenCnt == 0)
    {
        if (gicBase->libNode.lib_Flags & LIBF_DELEXP)
            return LibExpunge(gicBase);
    }

    return 0;
}

static ULONG LibNull(void)
{
    return 0;
}

static const APTR funcTable[] = {
    (APTR)LibOpen,
    (APTR)LibClose,
    (APTR)LibExpunge,
    (APTR)LibNull,
    (APTR)AddIntServerEx,
    (APTR)RemIntServerEx,
    (APTR)GetIntStatus,
    (APTR)EnableInt,
    (APTR)DisableInt,
    (APTR)SetIntPriority,
    (APTR)GetIntPriority,
    (APTR)SetIntTriggerEdge,
    (APTR)SetIntTriggerLevel,
    (APTR)RouteIntToCpu,
    (APTR)UnrouteIntFromCpu,
    (APTR)QueryIntRoute,
    (APTR)SetIntPending,
    (APTR)ClearIntPending,
    (APTR)SetIntActive,
    (APTR)ClearIntActive,
    (APTR)SetPriorityMask,
    (APTR)GetPriorityMask,
    (APTR)GetRunningPriority,
    (APTR)GetHighestPending,
    (APTR)GetControllerInfo,
    (APTR)-1};

static const APTR initTable[4] = {
    (APTR)sizeof(struct GIC_Base),
    (APTR)funcTable,
    NULL,
    (APTR)LibInit};
