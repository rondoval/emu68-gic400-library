// SPDX-License-Identifier: MPL-2.0
#include <common/compiler.h>
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

static ULONG LibExpunge(REGARG(struct GIC_Base *gicBase, "a6"))
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

struct Library *LibInit(REGARG(struct Library *base, "d0"), REGARG(ULONG seglist, "a0"), REGARG(struct ExecBase *execBase, "a6"))
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

static struct GIC_Base *LibOpen(REGARG(ULONG version, "d0"), REGARG(struct GIC_Base *gicBase, "a6"))
{
    (void)version;
    gicBase->libNode.lib_OpenCnt++;
    gicBase->libNode.lib_Flags &= ~LIBF_DELEXP;
    return gicBase;
}

static ULONG LibClose(REGARG(struct GIC_Base *gicBase, "a6"))
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
