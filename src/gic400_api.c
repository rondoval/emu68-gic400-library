// SPDX-License-Identifier: GPL-2.0
#include <gic400_private.h>
#include <devtree.h>
#include <exec/memory.h>

extern struct ExecBase *SysBase;
static char gic_dispatcher_name[] = "ARM GIC-400 dispatcher";

static ULONG gic400_exec_dispatcher(register struct GIC_Base *gicBase asm("a1"));

static int gic400_validate_irq(struct GIC_Base *gicBase, ULONG irq)
{
    if (!gicBase)
    {
        Kprintf("[gic] %s: NULL GIC base\n", __func__);
        return -GIC_ERROR;
    }

    if (gicBase->max_irqs == 0)
    {
        Kprintf("[gic] %s: controller reports zero IRQs\n", __func__);
        return -GIC_ERROR;
    }

    if (irq >= gicBase->max_irqs)
    {
        Kprintf("[gic] %s: IRQ %lu is out of range (max %lu)\n", __func__, irq, gicBase->max_irqs);
        return -GIC_ERROR;
    }

    return 0;
}

static int gic400_parse_devicetree(struct GIC_Base *gicBase)
{
    DT_Init();

    APTR root_key = DT_OpenKey((CONST_STRPTR) "/");
    if (root_key == NULL)
    {
        Kprintf("[gic] %s: Failed to open root key\n", __func__);
        return -GIC_ERROR;
    }

    const ULONG gic_phandle = DT_GetPropertyValueULONG(root_key, "interrupt-controller", 1, FALSE);

    APTR gic_key = DT_FindByPHandle(root_key, gic_phandle);
    if (gic_key == NULL)
    {
        Kprintf("[gic] %s: Failed to find GIC key for handle %08lx\n", __func__, gic_phandle);
        DT_CloseKey(root_key);
        return -GIC_ERROR;
    }

    CONST_STRPTR gic_compatible = DT_GetPropValue(DT_FindProperty(gic_key, (CONST_STRPTR) "compatible"));
    // TODO this is awful. rework DT_TranslateAddress

    const APTR parent_key = DT_GetParent(gic_key);
    const ULONG address_cells_parent = DT_GetPropertyValueULONG(parent_key, "#address-cells", 1, FALSE);
    const ULONG size_cells_parent = DT_GetPropertyValueULONG(parent_key, "#size-cells", 1, FALSE);
    const ULONG cells_per_record = address_cells_parent + size_cells_parent;

    const ULONG *value = DT_GetPropValue(DT_FindProperty(gic_key, (CONST_STRPTR) "reg"));

    gicBase->gic_base_distributor = (APTR)(ULONG)DT_GetNumber(value, address_cells_parent);
    DT_TranslateAddress(&gicBase->gic_base_distributor, parent_key);
    if (gicBase->gic_base_distributor == NULL)
    {
        Kprintf("[gic] %s: Failed to get Distributor base address for GIC\n", __func__);
        DT_CloseKey(gic_key);
        DT_CloseKey(root_key);
        return -GIC_ERROR;
    }

    gicBase->gic_base_cpuif = (APTR)(ULONG)DT_GetNumber(value + cells_per_record, address_cells_parent);
    DT_TranslateAddress(&gicBase->gic_base_cpuif, parent_key);
    if (gicBase->gic_base_cpuif == NULL)
    {
        Kprintf("[gic] %s: Failed to get CPU Interface base address for GIC\n", __func__);
        DT_CloseKey(gic_key);
        DT_CloseKey(root_key);
        return -GIC_ERROR;
    }

    Kprintf("[gic] %s: compatible: %s\n", __func__, gic_compatible);
    Kprintf("[gic] %s: Distributor register base: %08lx\n", __func__, gicBase->gic_base_distributor);
    Kprintf("[gic] %s: CPU Interface register base: %08lx\n", __func__, gicBase->gic_base_cpuif);

    // We're done with the device tree
    DT_CloseKey(gic_key);
    DT_CloseKey(root_key);
    return 0;
}

/* gic400_init: Initialize GIC state and install dispatcher.
 * Args: base - physical base address shared with Emu68.
 * Returns: 0 on success, -GIC_ERROR on failure.
 */
int gic400_init(struct GIC_Base *gicBase)
{
    if (!gicBase)
        return -GIC_ERROR;

    int ret = gic400_parse_devicetree(gicBase);
    if (ret < 0)
        return ret;

    gicBase->gicd_iidr = readl(GICD_IIDR);
    gicBase->gicd_typer = readl(GICD_TYPER);
    gicBase->gicc_iidr = readl(GICC_IIDR);

    gicBase->max_irqs = (GICD_TYPER_IT_LINES_NUMBER(gicBase->gicd_typer) + 1) * 32;

    gicBase->handler_count = 0;
    gicBase->handlers = NULL;
    ULONG handler_bytes = gicBase->max_irqs * sizeof(struct Interrupt *);
    gicBase->handlers = AllocMem(handler_bytes, MEMF_CLEAR);
    if (!gicBase->handlers)
    {
        Kprintf("[gic] %s: Failed to allocate handler table (%lu bytes)\n", __func__, handler_bytes);
        return -GIC_ERROR;
    }

    gicc_print_info(gicBase->gicc_iidr);
    gicd_print_info(gicBase);

    Disable();

    gicc_set_priority_mask(0x7F); // allow all priorities

    ULONG ctlr = gicc_get_ctlr();
    gicc_log_ctlr((CONST_STRPTR) "Initial", ctlr);

    ctlr &= ~GICC_CTLR_EOI_MODE_NS;        // GICC_EOIR does both priority drop and deactivate
    ctlr |= GICC_CTLR_ENABLE_GRP1;         // enable CPU interface
    ctlr |= GICC_CTLR_FIQ_BYPASS_DIS_GRP1; // disable bypassing of FIQ for Group 1
    ctlr |= GICC_CTLR_IRQ_BYPASS_DIS_GRP1; // disable bypassing of IRQ for Group 1

    gicc_log_ctlr((CONST_STRPTR) "Configured", ctlr);
    gicc_set_ctlr(ctlr);

    ctlr = gicc_get_ctlr();
    gicc_log_ctlr((CONST_STRPTR) "Final", ctlr);

    gicd_enable(gicBase);

    gicBase->dispatcher_interrupt.is_Node.ln_Type = NT_INTERRUPT;
    gicBase->dispatcher_interrupt.is_Node.ln_Pri = 100;
    gicBase->dispatcher_interrupt.is_Node.ln_Name = gic_dispatcher_name;
    gicBase->dispatcher_interrupt.is_Data = gicBase;
    gicBase->dispatcher_interrupt.is_Code = (APTR)gic400_exec_dispatcher;
    AddIntServer(INTB_EXTER, &gicBase->dispatcher_interrupt);
    Kprintf("[gic] dispatcher installed on INTB_EXTER\n");
    Enable();

    return 0;
}

/* forward declaration */
static void gic400_disable_irq(struct GIC_Base *gicBase, ULONG irq);

/* gic400_shutdown: Remove all handlers and dispatcher.
 * Args: none.
 * Returns: void.
 */
void gic400_shutdown(struct GIC_Base *gicBase)
{
    if (!gicBase)
        return;

    Disable();

    RemIntServer(INTB_EXTER, &gicBase->dispatcher_interrupt);
    gicd_disable(gicBase);

    for (ULONG irq = 0; irq < gicBase->max_irqs; irq++)
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
    Kprintf("[gic] dispatcher removed from INTB_EXTER\n");

    if (gicBase->handlers)
    {
        ULONG handler_bytes = gicBase->max_irqs * sizeof(struct Interrupt *);
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
static void gic400_enable_irq(struct GIC_Base *gicBase, ULONG irq, UBYTE priority, BOOL edge)
{
    Kprintf("[gic] Enabling IRQ %ld with priority %lu\n", irq, priority);

    gicd_disable_irq(gicBase, irq); // disable IRQ before configuration

    gicd_set_priority(gicBase, irq, priority); // set priority
    gicd_set_cpu(gicBase, irq, 0, TRUE);       // route to CPU0
    gicd_set_trigger(gicBase, irq, edge);      // set level-triggered

    gicd_enable_irq(gicBase, irq); // enable IRQ
}

/* gic400_disable_irq: Disable a configurable SPI.
 * Args: irq - interrupt number to disable.
 */
static void gic400_disable_irq(struct GIC_Base *gicBase, ULONG irq)
{
    Kprintf("[gic] Disabling IRQ %ld\n", irq);

    gicd_disable_irq(gicBase, irq);       // disable IRQ
    gicd_set_cpu(gicBase, irq, 0, FALSE); // unroute from CPU0
}

/* GetIntStatus: Retrieve status of given IRQ.
 * Args: irq - interrupt number; pending/active/enabled - optional outputs.
 * Returns: 0 on success, -GIC_ERROR on invalid IRQ.
 */
ULONG GetIntStatus(ULONG irq asm("d0"), BOOL *pending asm("a1"), BOOL *active asm("a2"), BOOL *enabled asm("a3"), struct GIC_Base *gicBase asm("a6"))
{
    if (gic400_validate_irq(gicBase, irq) < 0)
        return -GIC_ERROR;

    if (pending)
        *pending = gicd_is_pending(gicBase, irq);
    if (active)
        *active = gicd_is_active(gicBase, irq);
    if (enabled)
        *enabled = gicd_is_enabled(gicBase, irq);

    return 0;
}

ULONG EnableInt(ULONG irq asm("d0"), struct GIC_Base *gicBase asm("a6"))
{
    if (gic400_validate_irq(gicBase, irq) < 0)
        return -GIC_ERROR;

    gicd_enable_irq(gicBase, irq);
    return 0;
}

ULONG DisableInt(ULONG irq asm("d0"), struct GIC_Base *gicBase asm("a6"))
{
    if (gic400_validate_irq(gicBase, irq) < 0)
        return -GIC_ERROR;

    gicd_disable_irq(gicBase, irq);
    return 0;
}

ULONG SetIntPriority(ULONG irq asm("d0"), UBYTE priority asm("d1"), struct GIC_Base *gicBase asm("a6"))
{
    if (gic400_validate_irq(gicBase, irq) < 0)
        return -GIC_ERROR;

    gicd_set_priority(gicBase, irq, priority);
    return 0;
}

/* GetIntPriority: Return the priority byte for an IRQ or -GIC_ERROR on failure. */
LONG GetIntPriority(ULONG irq asm("d0"), struct GIC_Base *gicBase asm("a6"))
{
    if (gic400_validate_irq(gicBase, irq) < 0)
        return -GIC_ERROR;

    return (LONG)gicd_get_priority(gicBase, irq);
}

ULONG SetIntTriggerEdge(ULONG irq asm("d0"), struct GIC_Base *gicBase asm("a6"))
{
    if (gic400_validate_irq(gicBase, irq) < 0)
        return -GIC_ERROR;

    gicd_set_trigger(gicBase, irq, TRUE);
    return 0;
}

ULONG SetIntTriggerLevel(ULONG irq asm("d0"), struct GIC_Base *gicBase asm("a6"))
{
    if (gic400_validate_irq(gicBase, irq) < 0)
        return -GIC_ERROR;

    gicd_set_trigger(gicBase, irq, FALSE);
    return 0;
}

static int gic400_validate_cpu_target(const char *caller, ULONG irq, UBYTE cpu)
{
    if (cpu >= 8)
    {
        Kprintf("[gic] %s: CPU index %lu is out of range\n", caller, (ULONG)cpu);
        return -GIC_ERROR;
    }

    if (irq < 32)
    {
        Kprintf("[gic] %s: IRQ %lu targets SGI/PPI and cannot be rerouted\n", caller, irq);
        return -GIC_ERROR;
    }

    return 0;
}

ULONG RouteIntToCpu(ULONG irq asm("d0"), UBYTE cpu asm("d1"), struct GIC_Base *gicBase asm("a6"))
{
    if (gic400_validate_irq(gicBase, irq) < 0)
        return -GIC_ERROR;
    if (gic400_validate_cpu_target(__func__, irq, cpu) < 0)
        return -GIC_ERROR;

    gicd_set_cpu(gicBase, irq, cpu, TRUE);
    return 0;
}

ULONG UnrouteIntFromCpu(ULONG irq asm("d0"), UBYTE cpu asm("d1"), struct GIC_Base *gicBase asm("a6"))
{
    if (gic400_validate_irq(gicBase, irq) < 0)
        return -GIC_ERROR;
    if (gic400_validate_cpu_target(__func__, irq, cpu) < 0)
        return -GIC_ERROR;

    gicd_set_cpu(gicBase, irq, cpu, FALSE);
    return 0;
}

/* QueryIntRoute: Return CPU target mask for an SPI or -GIC_ERROR on failure. */
LONG QueryIntRoute(ULONG irq asm("d0"), struct GIC_Base *gicBase asm("a6"))
{
    if (gic400_validate_irq(gicBase, irq) < 0)
        return -GIC_ERROR;
    if (irq < 32)
    {
        Kprintf("[gic] %s: IRQ %lu targets SGI/PPI and cannot be rerouted\n", __func__, irq);
        return -GIC_ERROR;
    }

    return (LONG)gicd_get_cpu_mask(gicBase, irq);
}

ULONG SetIntPending(ULONG irq asm("d0"), struct GIC_Base *gicBase asm("a6"))
{
    if (gic400_validate_irq(gicBase, irq) < 0)
        return -GIC_ERROR;

    gicd_set_pending(gicBase, irq);
    return 0;
}

ULONG ClearIntPending(ULONG irq asm("d0"), struct GIC_Base *gicBase asm("a6"))
{
    if (gic400_validate_irq(gicBase, irq) < 0)
        return -GIC_ERROR;

    gicd_clear_pending(gicBase, irq);
    return 0;
}

ULONG SetIntActive(ULONG irq asm("d0"), struct GIC_Base *gicBase asm("a6"))
{
    if (gic400_validate_irq(gicBase, irq) < 0)
        return -GIC_ERROR;

    gicd_set_active(gicBase, irq);
    return 0;
}

ULONG ClearIntActive(ULONG irq asm("d0"), struct GIC_Base *gicBase asm("a6"))
{
    if (gic400_validate_irq(gicBase, irq) < 0)
        return -GIC_ERROR;

    gicd_clear_active(gicBase, irq);
    return 0;
}

ULONG SetPriorityMask(UBYTE mask asm("d0"), struct GIC_Base *gicBase asm("a6"))
{
    if (!gicBase)
    {
        Kprintf("[gic] %s: NULL GIC base\n", __func__);
        return -GIC_ERROR;
    }

    gicc_set_priority_mask(mask);
    return 0;
}

/* GetPriorityMask: Return current CPU interface priority mask or -GIC_ERROR. */
LONG GetPriorityMask(struct GIC_Base *gicBase asm("a6"))
{
    if (!gicBase)
    {
        Kprintf("[gic] %s: NULL GIC base\n", __func__);
        return -GIC_ERROR;
    }

    UBYTE mask = 0;
    gicc_get_priority_mask(gicBase, &mask);
    return (LONG)mask;
}

/* GetRunningPriority: Return the currently running priority or -GIC_ERROR. */
LONG GetRunningPriority(struct GIC_Base *gicBase asm("a6"))
{
    if (!gicBase)
    {
        Kprintf("[gic] %s: NULL GIC base\n", __func__);
        return -GIC_ERROR;
    }

    return (LONG)gicc_get_running_priority();
}

/* GetHighestPending: Return IRQID of highest priority pending interrupt or -GIC_ERROR. */
LONG GetHighestPending(struct GIC_Base *gicBase asm("a6"))
{
    if (!gicBase)
    {
        Kprintf("[gic] %s: NULL GIC base\n", __func__);
        return -GIC_ERROR;
    }

    return (LONG)gicc_get_highest_pending();
}

LONG GetControllerInfo(struct GICInfo *info asm("a1"), struct GIC_Base *gicBase asm("a6"))
{
    if (!gicBase)
    {
        Kprintf("[gic] %s: NULL GIC base\n", __func__);
        return -GIC_ERROR;
    }
    if (!info)
    {
        Kprintf("[gic] %s: NULL info pointer\n", __func__);
        return -GIC_ERROR;
    }

    ULONG typer = gicBase->gicd_typer;

    info->distributorIIDR = gicBase->gicd_iidr;
    info->distributorTyper = typer;
    info->cpuInterfaceIIDR = gicBase->gicc_iidr;
    info->maxIrqs = gicBase->max_irqs;
    info->cpuCount = (UBYTE)(GICD_TYPER_CPUS_NUMBER(typer) + 1);
    info->securityExtensions = (UBYTE)GICD_TYPER_SECURITY_EXTN(typer);
    info->lspiCount = (UBYTE)GICD_TYPER_LSPI(typer);

    return 0;
}

/* gic400_call_interrupt: Invoke interrupt server with Exec ABI.
 * Args: interrupt - Exec interrupt entry; irq - source IRQ number.
 * Returns: void.
 */
static inline void gic400_call_interrupt(struct Interrupt *interrupt, ULONG irq)
{
    if (interrupt == NULL || interrupt->is_Code == NULL)
        return;

    __asm__ __volatile__(
        "move.l %[sysbase],%%a6\n\t"
        "move.l %[irq],%%d0\n\t"
        "move.l %[data],%%a1\n\t"
        "jsr (%[code])\n\t"
        :
        : [code] "a"(interrupt->is_Code),
          [data] "r"(interrupt->is_Data),
          [irq] "r"(irq),
          [sysbase] "r"(SysBase)
        : "d0", "d1", "a0", "a1", "a6");
}

/* gic400_exec_dispatcher: Exec interrupt server for INTB_EXTER hook.
 * Args: none.
 * Returns: void.
 */
static ULONG gic400_exec_dispatcher(register struct GIC_Base *gicBase asm("a1"))
{
    if (!gicBase)
        return 0;

    ULONG iar = gicc_acknowledge_interrupt();
    ULONG irq = iar & 0x3FF;

    if (irq == 0x3FF || irq == 0x3FE)
    {
        return 0; // No pending interrupts
    }

    if (irq >= gicBase->max_irqs)
    {
        gicc_end_interrupt(iar);
        return 0;
    }

    struct Interrupt *interrupt = gicBase->handlers[irq];
    if (interrupt)
    {
        KprintfH("[gic] Invoking handler for IRQ %ld\n", irq);
        gic400_call_interrupt(interrupt, irq);
    }

    gicc_end_interrupt(iar);
    return 1;
}

/* AddIntServerEx: Register interrupt server for given SPI.
 * Args:
 *  irq - interrupt number
 *  priority - priority byte to assign (0-0x7f)
 *  edge - TRUE for edge-triggered, FALSE for level-triggered
 *  interrupt - Exec interrupt descriptor
 * Returns: 0 on success, -GIC_ERROR when registration fails.
 */
ULONG AddIntServerEx(ULONG irq asm("d0"), UBYTE priority asm("d1"), BOOL edge asm("d2"), struct Interrupt *interrupt asm("a1"), struct GIC_Base *gicBase asm("a6"))
{
    if (!gicBase)
        return -GIC_ERROR;
    if (!interrupt || !interrupt->is_Code)
    {
        Kprintf("[gic] Invalid interrupt server for IRQ %ld\n", irq);
        return -GIC_ERROR;
    }
    if (gic400_validate_irq(gicBase, irq) < 0)
    {
        return -GIC_ERROR;
    }

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
        return -GIC_ERROR;
    }

    gicBase->handlers[irq] = interrupt;
    gicBase->handler_count++;
    gic400_enable_irq(gicBase, irq, priority, edge);

    Enable();
    return 0;
}

/* RemIntServerEx: Remove interrupt server for given SPI.
 * Args: irq - interrupt number; interrupt - handler to remove.
 * Returns: 0 on success, -GIC_ERROR if not found or mismatched.
 */
ULONG RemIntServerEx(ULONG irq asm("d0"), struct Interrupt *interrupt asm("a1"), struct GIC_Base *gicBase asm("a6"))
{
    if (!gicBase)
        return -GIC_ERROR;
    if (!interrupt)
    {
        Kprintf("[gic] Invalid interrupt server for IRQ %ld\n", irq);
        return -GIC_ERROR;
    }
    if (gic400_validate_irq(gicBase, irq) < 0)
    {
        return -GIC_ERROR;
    }

    Disable();

    struct Interrupt *current = gicBase->handlers[irq];
    if (!current)
    {
        Kprintf("[gic] No handler registered for IRQ %ld\n", irq);
        Enable();
        return -GIC_ERROR;
    }
    if (current != interrupt)
    {
        Kprintf("[gic] IRQ %ld registered with a different server\n", irq);
        Enable();
        return -GIC_ERROR;
    }

    gic400_disable_irq(gicBase, irq);

    gicBase->handlers[irq] = NULL;
    if (gicBase->handler_count > 0)
        gicBase->handler_count--;

    Enable();
    return 0;
}
