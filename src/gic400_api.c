// SPDX-License-Identifier: MPL-2.0 OR GPL-2.0+
#include <stddef.h>
#include <exec/memory.h>
#include <strutil.h>
#include <gic400_private.h>

#define __NOLIBBASE__
#include <devtree.h>

static const char gic_dispatcher_name[] = "ARM GIC-400 dispatcher";

/* forward declarations */
extern ULONG gic400_exec_dispatcher(void); /* asm, see below */
static void gic400_disable_irq(struct GIC_Base *gicBase, u32 irq);

static s32 gic400_validate_irq(struct GIC_Base *gicBase, u32 irq)
{
    if (!gicBase)
    {
        Kprintf("[gic] %s: NULL GIC base\n", __func__);
        return GIC400_ERR_NOT_READY;
    }

    if (gicBase->max_irqs == 0)
    {
        Kprintf("[gic] %s: controller reports zero IRQs\n", __func__);
        return GIC400_ERR_NOT_READY;
    }

    if (irq >= gicBase->max_irqs)
    {
        Kprintf("[gic] %s: IRQ %lu is out of range (max %lu)\n", __func__, irq, gicBase->max_irqs);
        return GIC400_ERR_INVALID_IRQ;
    }

    return 0;
}

static s32 gic400_parse_devicetree(struct GIC_Base *gicBase)
{
    struct ExecBase *SysBase = gicBase->sysBase;
    APTR DeviceTreeBase = OpenResource((CONST_STRPTR) "devicetree.resource");
    if (DeviceTreeBase == NULL)
    {
        Kprintf("[gic] %s: Failed to open devicetree.resource\n", __func__);
        return GIC400_ERR_DEVTREE;
    }

    APTR root_key = DT_OpenKey((CONST_STRPTR) "/");
    if (root_key == NULL)
    {
        Kprintf("[gic] %s: Failed to open root key\n", __func__);
        return GIC400_ERR_DEVTREE;
    }

    const u32 gic_phandle = DT_GetPropertyValueULONG(SysBase, root_key, "interrupt-parent", 1, FALSE);

    APTR gic_key = DT_FindByPHandle(SysBase, root_key, gic_phandle);
    if (gic_key == NULL)
    {
        Kprintf("[gic] %s: Failed to find GIC key for handle %08lx\n", __func__, gic_phandle);
        DT_CloseKey(root_key);
        return GIC400_ERR_DEVTREE;
    }

    CONST_STRPTR gic_compatible = DT_GetPropValue(DT_FindProperty(gic_key, (CONST_STRPTR) "compatible"));
    if (gic_compatible == NULL || _Strnicmp((STRPTR)gic_compatible, (STRPTR) "arm,gic", 7) != 0)
    {
        Kprintf("[gic] %s: GIC compatible string is not 'arm,gic-400': %s\n", __func__, gic_compatible);
        DT_CloseKey(gic_key);
        DT_CloseKey(root_key);
        return GIC400_ERR_DEVTREE;
    }
    // TODO this is awful. rework DT_TranslateAddress

    const APTR parent_key = DT_GetParent(gic_key);
    const u32 address_cells_parent = DT_GetPropertyValueULONG(SysBase, parent_key, "#address-cells", 1, FALSE);
    const u32 size_cells_parent = DT_GetPropertyValueULONG(SysBase, parent_key, "#size-cells", 1, FALSE);
    const u32 cells_per_record = address_cells_parent + size_cells_parent;

    const u32 *value = DT_GetPropValue(DT_FindProperty(gic_key, (CONST_STRPTR) "reg"));

    gicBase->gic_base_distributor = (APTR)(ULONG)DT_GetNumber(value, address_cells_parent);
    DT_TranslateAddress(SysBase, &gicBase->gic_base_distributor, parent_key);
    if (gicBase->gic_base_distributor == NULL)
    {
        Kprintf("[gic] %s: Failed to get Distributor base address for GIC\n", __func__);
        DT_CloseKey(gic_key);
        DT_CloseKey(root_key);
        return GIC400_ERR_DEVTREE;
    }

    gicBase->gic_base_cpuif = (APTR)(ULONG)DT_GetNumber(value + cells_per_record, address_cells_parent);
    DT_TranslateAddress(SysBase, &gicBase->gic_base_cpuif, parent_key);
    if (gicBase->gic_base_cpuif == NULL)
    {
        Kprintf("[gic] %s: Failed to get CPU Interface base address for GIC\n", __func__);
        DT_CloseKey(gic_key);
        DT_CloseKey(root_key);
        return GIC400_ERR_DEVTREE;
    }

    KprintfT("[gic] %s: compatible: %s\n", __func__, gic_compatible);
    KprintfT("[gic] %s: Distributor register base: %08lx\n", __func__, gicBase->gic_base_distributor);
    KprintfT("[gic] %s: CPU Interface register base: %08lx\n", __func__, gicBase->gic_base_cpuif);

    // We're done with the device tree
    DT_CloseKey(gic_key);
    DT_CloseKey(root_key);
    return 0;
}

/* gic400_init: Initialize GIC state and install dispatcher.
 * Args: base - physical base address shared with Emu68.
 * Returns: 0 on success, negative GIC400_ERR_* on failure.
 */
s32 gic400_init(struct GIC_Base *gicBase)
{
    if (!gicBase)
        return GIC400_ERR_NOT_READY;
    struct ExecBase *SysBase = gicBase->sysBase;

    s32 ret = gic400_parse_devicetree(gicBase);
    if (ret < 0)
        return ret;

    gicBase->gicd_iidr = mmio_read32(GICD_IIDR);
    gicBase->gicd_typer = mmio_read32(GICD_TYPER);
    gicBase->gicc_iidr = mmio_read32(GICC_IIDR);

    gicBase->max_irqs = (GICD_TYPER_IT_LINES_NUMBER(gicBase->gicd_typer) + 1) * 32;
    /* IDs 1020-1023 are never real interrupts (GICv2: 1020/1021 reserved,
     * 1022/1023 spurious); the dispatcher relies on max_irqs <= 1020 to
     * reject the spurious IDs with its single range check. */
    if (gicBase->max_irqs > 1020)
        gicBase->max_irqs = 1020;

    gicBase->handler_count = 0;
    gicBase->handlers = NULL;
    u32 handler_bytes = gicBase->max_irqs * sizeof(struct Interrupt *);
    gicBase->handlers = AllocMem(handler_bytes, MEMF_CLEAR);
    if (!gicBase->handlers)
    {
        Kprintf("[gic] %s: Failed to allocate handler table (%lu bytes)\n", __func__, handler_bytes);
        return GIC400_ERR_NO_MEMORY;
    }

#ifdef TRACE
    gicc_print_info(gicBase->gicc_iidr);
    gicd_print_info(gicBase);
#endif

    Disable();

    /* We're not sure what the state of the GIC-400 is.
     * So, to be on the safe side, we'll unroute all SPIs
     * from CPU 0 before enabling the controller and distributor */
    for (u32 irq = 0; irq < gicBase->max_irqs; irq++)
    {
        gicd_set_cpu(gicBase, irq, 0, FALSE);
    }

    gicc_set_priority_mask(0x7F); // allow all priorities

    u32 ctlr = gicc_get_ctlr();

    ctlr &= ~GICC_CTLR_EOI_MODE_NS;        // GICC_EOIR does both priority drop and deactivate
    ctlr |= GICC_CTLR_ENABLE_GRP1;         // enable CPU interface
    ctlr |= GICC_CTLR_FIQ_BYPASS_DIS_GRP1; // disable bypassing of FIQ for Group 1
    ctlr |= GICC_CTLR_IRQ_BYPASS_DIS_GRP1; // disable bypassing of IRQ for Group 1

    gicc_set_ctlr(ctlr);

    ctlr = gicc_get_ctlr();
#ifdef TRACE
    gicc_log_ctlr((CONST_STRPTR) "Final", ctlr);
#endif

    gicd_enable(gicBase);

    gicBase->dispatcher_interrupt.is_Node.ln_Type = NT_INTERRUPT;
    gicBase->dispatcher_interrupt.is_Node.ln_Pri = 100;
    gicBase->dispatcher_interrupt.is_Node.ln_Name = (char *)gic_dispatcher_name;
    gicBase->dispatcher_interrupt.is_Data = gicBase;
    gicBase->dispatcher_interrupt.is_Code = (APTR)gic400_exec_dispatcher;
    AddIntServer(INTB_EXTER, &gicBase->dispatcher_interrupt);
    KprintfT("[gic] dispatcher installed on INTB_EXTER\n");
    Enable();

    return 0;
}

/* gic400_shutdown: Remove all handlers and dispatcher.
 * Args: none.
 * Returns: void.
 */
void gic400_shutdown(struct GIC_Base *gicBase)
{
    if (!gicBase)
        return;
    struct ExecBase *SysBase = gicBase->sysBase;

    Disable();

    RemIntServer(INTB_EXTER, &gicBase->dispatcher_interrupt);
    gicd_disable(gicBase);

    for (u32 irq = 0; irq < gicBase->max_irqs; irq++)
    {
        if (gicBase->handlers[irq] != NULL)
        {
            gic400_disable_irq(gicBase, irq);
            gicBase->handlers[irq] = NULL;
            Kprintf("[gic] warning: removed handler for IRQ %ld during shutdown\n", irq);
        }
    }
    gicBase->handler_count = 0;

    Enable();
    KprintfT("[gic] dispatcher removed from INTB_EXTER\n");

    if (gicBase->handlers)
    {
        u32 handler_bytes = gicBase->max_irqs * sizeof(struct Interrupt *);
        FreeMem(gicBase->handlers, handler_bytes);
        gicBase->handlers = NULL;
    }
}

/* gic400_enable_irq: Configure group 0 SPI and enable it.
 * Args:
 *  irq - interrupt number
 *  priority - priority byte to assign
 *  edge - TRUE for edge-triggered, FALSE for level-triggered
 */
static void gic400_enable_irq(struct GIC_Base *gicBase, u32 irq, u8 priority, BOOL edge)
{
    Kprintf("[gic] Enabling IRQ %ld with priority %lu\n", irq, priority);

    gicd_disable_irq(gicBase, irq); // disable IRQ before configuration

    gicd_set_priority(gicBase, irq, priority); // set priority
    gicd_set_cpu(gicBase, irq, 0, TRUE);       // route to CPU0
    gicd_set_cpu(gicBase, irq, 1, FALSE);
    gicd_set_cpu(gicBase, irq, 2, FALSE);
    gicd_set_cpu(gicBase, irq, 3, FALSE);
    gicd_set_trigger(gicBase, irq, edge); // set level-triggered

    gicd_enable_irq(gicBase, irq); // enable IRQ
}

/* gic400_disable_irq: Disable a configurable SPI.
 * Args: irq - interrupt number to disable.
 */
static void gic400_disable_irq(struct GIC_Base *gicBase, u32 irq)
{
    Kprintf("[gic] Disabling IRQ %ld\n", irq);

    gicd_disable_irq(gicBase, irq);       // disable IRQ
    gicd_set_cpu(gicBase, irq, 0, FALSE); // unroute from CPU0
}

/* GetIntStatus: Retrieve status of given IRQ.
 * Args: irq - interrupt number; pending/active/enabled - optional outputs.
 * Returns: 0 on success, negative GIC400_ERR_* on failure.
 */
LONG GetIntStatus(ULONG irq asm("d0"), BOOL *pending asm("a1"), BOOL *active asm("a2"), BOOL *enabled asm("a3"), struct GIC_Base *gicBase asm("a6"))
{
    LONG ret = gic400_validate_irq(gicBase, irq);
    if (ret < 0)
        return ret;

    if (pending)
        *pending = gicd_is_pending(gicBase, irq);
    if (active)
        *active = gicd_is_active(gicBase, irq);
    if (enabled)
        *enabled = gicd_is_enabled(gicBase, irq);

    return 0;
}

LONG EnableInt(ULONG irq asm("d0"), struct GIC_Base *gicBase asm("a6"))
{
    LONG ret = gic400_validate_irq(gicBase, irq);
    if (ret < 0)
        return ret;

    gicd_enable_irq(gicBase, irq);
    return 0;
}

LONG DisableInt(ULONG irq asm("d0"), struct GIC_Base *gicBase asm("a6"))
{
    LONG ret = gic400_validate_irq(gicBase, irq);
    if (ret < 0)
        return ret;

    gicd_disable_irq(gicBase, irq);
    return 0;
}

LONG SetIntPriority(ULONG irq asm("d0"), UBYTE priority asm("d1"), struct GIC_Base *gicBase asm("a6"))
{
    LONG ret = gic400_validate_irq(gicBase, irq);
    if (ret < 0)
        return ret;

    gicd_set_priority(gicBase, irq, priority);
    return 0;
}

/* GetIntPriority: Return the priority byte for an IRQ or a negative GIC400_ERR_*. */
LONG GetIntPriority(ULONG irq asm("d0"), struct GIC_Base *gicBase asm("a6"))
{
    LONG ret = gic400_validate_irq(gicBase, irq);
    if (ret < 0)
        return ret;

    return (LONG)gicd_get_priority(gicBase, irq);
}

LONG SetIntTriggerEdge(ULONG irq asm("d0"), struct GIC_Base *gicBase asm("a6"))
{
    LONG ret = gic400_validate_irq(gicBase, irq);
    if (ret < 0)
        return ret;

    gicd_set_trigger(gicBase, irq, TRUE);
    return 0;
}

LONG SetIntTriggerLevel(ULONG irq asm("d0"), struct GIC_Base *gicBase asm("a6"))
{
    LONG ret = gic400_validate_irq(gicBase, irq);
    if (ret < 0)
        return ret;

    gicd_set_trigger(gicBase, irq, FALSE);
    return 0;
}

static s32 gic400_validate_cpu_target(const char *caller, u32 irq, u8 cpu)
{
#ifndef DEBUG
    (void)caller; /* only referenced by debug logging */
#endif
    if (cpu >= 8)
    {
        Kprintf("[gic] %s: CPU index %lu is out of range\n", caller, (ULONG)cpu);
        return GIC400_ERR_INVALID_ARGUMENT;
    }

    if (irq < 32)
    {
        Kprintf("[gic] %s: IRQ %lu targets SGI/PPI and cannot be rerouted\n", caller, irq);
        return GIC400_ERR_NOT_ROUTABLE;
    }

    return 0;
}

LONG RouteIntToCpu(ULONG irq asm("d0"), UBYTE cpu asm("d1"), struct GIC_Base *gicBase asm("a6"))
{
    LONG ret = gic400_validate_irq(gicBase, irq);
    if (ret < 0)
        return ret;
    ret = gic400_validate_cpu_target(__func__, irq, cpu);
    if (ret < 0)
        return ret;

    gicd_set_cpu(gicBase, irq, cpu, TRUE);
    return 0;
}

LONG UnrouteIntFromCpu(ULONG irq asm("d0"), UBYTE cpu asm("d1"), struct GIC_Base *gicBase asm("a6"))
{
    LONG ret = gic400_validate_irq(gicBase, irq);
    if (ret < 0)
        return ret;
    ret = gic400_validate_cpu_target(__func__, irq, cpu);
    if (ret < 0)
        return ret;

    gicd_set_cpu(gicBase, irq, cpu, FALSE);
    return 0;
}

/* QueryIntRoute: Return CPU target mask for an SPI or a negative GIC400_ERR_*. */
LONG QueryIntRoute(ULONG irq asm("d0"), struct GIC_Base *gicBase asm("a6"))
{
    LONG ret = gic400_validate_irq(gicBase, irq);
    if (ret < 0)
        return ret;
    if (irq < 32)
    {
        Kprintf("[gic] %s: IRQ %lu targets SGI/PPI and cannot be rerouted\n", __func__, irq);
        return GIC400_ERR_NOT_ROUTABLE;
    }

    return (LONG)gicd_get_cpu_mask(gicBase, irq);
}

LONG SetIntPending(ULONG irq asm("d0"), struct GIC_Base *gicBase asm("a6"))
{
    LONG ret = gic400_validate_irq(gicBase, irq);
    if (ret < 0)
        return ret;

    gicd_set_pending(gicBase, irq);
    return 0;
}

LONG ClearIntPending(ULONG irq asm("d0"), struct GIC_Base *gicBase asm("a6"))
{
    LONG ret = gic400_validate_irq(gicBase, irq);
    if (ret < 0)
        return ret;

    gicd_clear_pending(gicBase, irq);
    return 0;
}

LONG SetIntActive(ULONG irq asm("d0"), struct GIC_Base *gicBase asm("a6"))
{
    LONG ret = gic400_validate_irq(gicBase, irq);
    if (ret < 0)
        return ret;

    gicd_set_active(gicBase, irq);
    return 0;
}

LONG ClearIntActive(ULONG irq asm("d0"), struct GIC_Base *gicBase asm("a6"))
{
    LONG ret = gic400_validate_irq(gicBase, irq);
    if (ret < 0)
        return ret;

    gicd_clear_active(gicBase, irq);
    return 0;
}

LONG SetPriorityMask(UBYTE mask asm("d0"), struct GIC_Base *gicBase asm("a6"))
{
    if (!gicBase)
    {
        Kprintf("[gic] %s: NULL GIC base\n", __func__);
        return GIC400_ERR_NOT_READY;
    }

    gicc_set_priority_mask(mask);
    return 0;
}

/* GetPriorityMask: Return current CPU interface priority mask or a negative GIC400_ERR_*. */
LONG GetPriorityMask(struct GIC_Base *gicBase asm("a6"))
{
    if (!gicBase)
    {
        Kprintf("[gic] %s: NULL GIC base\n", __func__);
        return GIC400_ERR_NOT_READY;
    }

    u8 mask = 0;
    gicc_get_priority_mask(gicBase, &mask);
    return (LONG)mask;
}

/* GetRunningPriority: Return the currently running priority or a negative GIC400_ERR_*. */
LONG GetRunningPriority(struct GIC_Base *gicBase asm("a6"))
{
    if (!gicBase)
    {
        Kprintf("[gic] %s: NULL GIC base\n", __func__);
        return GIC400_ERR_NOT_READY;
    }

    return (LONG)gicc_get_running_priority();
}

/* GetHighestPending: Return IRQID of highest priority pending interrupt or a negative GIC400_ERR_*. */
LONG GetHighestPending(struct GIC_Base *gicBase asm("a6"))
{
    if (!gicBase)
    {
        Kprintf("[gic] %s: NULL GIC base\n", __func__);
        return GIC400_ERR_NOT_READY;
    }

    return (LONG)gicc_get_highest_pending();
}

LONG GetControllerInfo(struct GICInfo *info asm("a1"), struct GIC_Base *gicBase asm("a6"))
{
    if (!gicBase)
    {
        Kprintf("[gic] %s: NULL GIC base\n", __func__);
        return GIC400_ERR_NOT_READY;
    }
    if (!info)
    {
        Kprintf("[gic] %s: NULL info pointer\n", __func__);
        return GIC400_ERR_INVALID_ARGUMENT;
    }

    u32 typer = gicBase->gicd_typer;

    info->distributorIIDR = gicBase->gicd_iidr;
    info->distributorTyper = typer;
    info->cpuInterfaceIIDR = gicBase->gicc_iidr;
    info->maxIrqs = gicBase->max_irqs;
    info->cpuCount = (UBYTE)(GICD_TYPER_CPUS_NUMBER(typer) + 1);
    info->securityExtensions = (UBYTE)GICD_TYPER_SECURITY_EXTN(typer);
    info->lspiCount = (UBYTE)GICD_TYPER_LSPI(typer);

    return 0;
}

/* gic400_exec_dispatcher: the EXTER server Exec calls, in assembly so the
 * Exec server ABI is explicit rather than left to the compiler: A1 = is_Data
 * (gicBase); D0/D1/A0/A1/A5/A6 scratch; everything else preserved; return
 * with Z set to let the chain go on (AddIntServer autodoc - Exec tests the
 * flag, not D0).  Struct offsets come from offsetof() via "i" operands.
 *
 * Drains the CPU interface: acknowledge, run the server and EOI every pending
 * IRQ in one INT6 pass.  Each IRQ left for another pass would cost a level-6
 * exception plus Exec's INTENAR/INTREQR reads and INTREQ clear, all trapped
 * Amiga-bus cycles; draining costs one IAR read that comes back spurious.
 * Servers are called with the Exec server ABI plus D0 = IRQ number.
 *
 * max_irqs <= 1020 (gic400_init), so the single range check also
 * rejects the spurious IDs 1022/1023, which must not be EOI'd.
 *
 * No barrier after the IAR read (as Linux GICv2): Device memory keeps the
 * EOIR -> IAR order on the same peripheral and the value is consumed at once.
 * The one barrier (NOP = dsb sy on Emu68) sits before the EOI so a server's
 * device write that dropped a level source completes before the GIC samples
 * the line again.
 *
 * Registers: A2 = GICC register base, A3 = handler table, A4 = SysBase, D2 = raw IAR,
 * D3 = max_irqs - all callee-saved, so they survive the servers.
 */
#ifndef __INTELLISENSE__ /* no file-scope extended asm there */
__asm__(
    "	.text\n"
    "	.even\n"
    "_gic400_exec_dispatcher:\n"
    "	movem.l	%%d2-%%d3/%%a2-%%a4,-(%%sp)\n" /* our loop state must survive the servers */
    "	movea.l	%c[cpuif](%%a1),%%a2\n"       /* A2 = gicBase->gic_base_cpuif (GICC regs) */
    "	move.l	%c[max](%%a1),%%d3\n"         /* D3 = max_irqs (<= 1020) */
    "	move.l	%c[iar](%%a2),%%d2\n"         /* ack: D2 = raw IAR (LE), kept for the EOI */
    "	move.l	%%d2,%%d0\n"
    "	swap	%%d0\n"                       /* LE b0 b1 b2 b3 -> b2 b3 b0 b1 */
    "	ror.w	#8,%%d0\n"                    /* low word b0 b1 -> b1 b0 = IAR[15:0] */
    "	andi.l	#0x3ff,%%d0\n"                /* D0 = interrupt ID */
    "	cmp.l	%%d3,%%d0\n"
    "	bcc.s	3f\n"                         /* ID >= max_irqs = 1022/1023 spurious: not ours */
    "	movea.l	%c[handlers](%%a1),%%a3\n"    /* A3 = handler table */
    "	movea.l	%c[sysbase](%%a1),%%a4\n"     /* A4 = SysBase (no $4 bus read) */
    "1:	move.l	%%d0,%%d1\n"                  /* --- per IRQ: D0 = ID, D2 = raw IAR --- */
    "	lsl.l	#2,%%d1\n"                    /* D1 = ID * sizeof(APTR) */
    "	move.l	0(%%a3,%%d1.l),%%d1\n"        /* D1 = handlers[ID] */
    "	beq.s	2f\n"                         /* none registered: just EOI */
    "	movea.l	%%d1,%%a0\n"                  /* A0 = struct Interrupt */
    "	movea.l	%c[data](%%a0),%%a1\n"        /* Exec server ABI: A1 = is_Data, */
    "	movea.l	%c[code](%%a0),%%a5\n"        /* A5 = is_Code, */
    "	movea.l	%%a4,%%a6\n"                  /* A6 = SysBase, D0 = IRQ number */
    "	jsr	(%%a5)\n"                         /* may trash D0/D1/A0/A1/A5/A6 */
    "2:	nop\n"                                /* dsb sy: server's device writes land first */
    "	move.l	%%d2,%c[eoir](%%a2)\n"        /* EOI with the raw IAR value */
    "	move.l	%c[iar](%%a2),%%d2\n"         /* ack the next one (same swap as above) */
    "	move.l	%%d2,%%d0\n"
    "	swap	%%d0\n"
    "	ror.w	#8,%%d0\n"
    "	andi.l	#0x3ff,%%d0\n"
    "	cmp.l	%%d3,%%d0\n"
    "	bcs.s	1b\n"                         /* another IRQ pending: stay in this pass */
    "	movem.l	(%%sp)+,%%d2-%%d3/%%a2-%%a4\n" /* movem leaves the CCR alone... */
    "	moveq	#1,%%d0\n"                    /* ...so this sets Z clear: handled, end chain */
    "	rts\n"
    "3:	movem.l	(%%sp)+,%%d2-%%d3/%%a2-%%a4\n"
    "	moveq	#0,%%d0\n"                    /* Z set: not ours, Exec continues the chain */
    "	rts\n"
    :
    : [cpuif] "i"(offsetof(struct GIC_Base, gic_base_cpuif)),
      [max] "i"(offsetof(struct GIC_Base, max_irqs)),
      [handlers] "i"(offsetof(struct GIC_Base, handlers)),
      [sysbase] "i"(offsetof(struct GIC_Base, sysBase)),
      [data] "i"(offsetof(struct Interrupt, is_Data)),
      [code] "i"(offsetof(struct Interrupt, is_Code)),
      [iar] "i"(0x00C),  /* GICC_IAR */
      [eoir] "i"(0x010)); /* GICC_EOIR */
#endif

/* AddIntServerEx: Register interrupt server for given SPI.
 * Args:
 *  irq - interrupt number
 *  priority - priority byte to assign (0-0x7f)
 *  edge - TRUE for edge-triggered, FALSE for level-triggered
 *  interrupt - Exec interrupt descriptor
 * Returns: 0 on success, negative GIC400_ERR_* on failure.
 */
LONG AddIntServerEx(ULONG irq asm("d0"), UBYTE priority asm("d1"), BOOL edge asm("d2"), struct Interrupt *interrupt asm("a1"), struct GIC_Base *gicBase asm("a6"))
{
    if (!gicBase)
        return GIC400_ERR_NOT_READY;
    struct ExecBase *SysBase = gicBase->sysBase;
    if (!interrupt || !interrupt->is_Code)
    {
        Kprintf("[gic] Invalid interrupt server for IRQ %ld\n", irq);
        return GIC400_ERR_INVALID_ARGUMENT;
    }
    LONG ret = gic400_validate_irq(gicBase, irq);
    if (ret < 0)
        return ret;

    Disable();

    struct Interrupt *existing = gicBase->handlers[irq];
    if (existing)
    {
        if (existing == interrupt)
        {
            Kprintf("[gic] IRQ %ld is already registered\n", irq);
            Enable();
            return 0;
        }

        Enable();
        Kprintf("[gic] IRQ %ld already has a different server registered\n", irq);
        return GIC400_ERR_ALREADY_REGISTERED;
    }

    gicBase->handlers[irq] = interrupt;
    gicBase->handler_count++;
    gic400_enable_irq(gicBase, irq, priority, edge);

    Enable();
    return 0;
}

/* RemIntServerEx: Remove interrupt server for given SPI.
 * Args: irq - interrupt number; interrupt - handler to remove.
 * Returns: 0 on success, negative GIC400_ERR_* on failure.
 */
LONG RemIntServerEx(ULONG irq asm("d0"), struct Interrupt *interrupt asm("a1"), struct GIC_Base *gicBase asm("a6"))
{
    if (!gicBase)
        return GIC400_ERR_NOT_READY;
    struct ExecBase *SysBase = gicBase->sysBase;
    if (!interrupt)
    {
        Kprintf("[gic] Invalid interrupt server for IRQ %ld\n", irq);
        return GIC400_ERR_INVALID_ARGUMENT;
    }
    LONG ret = gic400_validate_irq(gicBase, irq);
    if (ret < 0)
        return ret;

    Disable();

    struct Interrupt *current = gicBase->handlers[irq];
    if (!current)
    {
        Kprintf("[gic] No handler registered for IRQ %ld\n", irq);
        Enable();
        return GIC400_ERR_NOT_FOUND;
    }
    if (current != interrupt)
    {
        Kprintf("[gic] IRQ %ld registered with a different server\n", irq);
        Enable();
        return GIC400_ERR_INVALID_ARGUMENT;
    }

    gic400_disable_irq(gicBase, irq);

    gicBase->handlers[irq] = NULL;
    if (gicBase->handler_count > 0)
        gicBase->handler_count--;

    Enable();
    return 0;
}
