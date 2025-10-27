// SPDX-License-Identifier: GPL-2.0
#include <gic400_private.h>

/* gicd_print_info: Log distributor ID and capability registers.
 * Args: none.
 * Returns: void.
 */
void gicd_print_info(struct GIC_Base *gicBase)
{
    ULONG iidr = gicBase->gicd_iidr;
    Kprintf("[gic] Distributor: Implementer=0x%03lx, Revision=%ld, Variant=%ld, ProductID=0x%02lx\n",
            GICD_IIDR_IMPLEMENTER(iidr), GICD_IIDR_REVISION(iidr), GICD_IIDR_VARIANT(iidr), GICD_IIDR_PRODUCT_ID(iidr));

    ULONG typer = gicBase->gicd_typer;
    Kprintf("[gic] Distributor: ITLinesNumber=%ld, CPUNumber=%ld, SecurityExtensions=%ld, LSPIs=%ld\n",
            (GICD_TYPER_IT_LINES_NUMBER(typer) + 1) * 32,
            GICD_TYPER_CPUS_NUMBER(typer) + 1,
            GICD_TYPER_SECURITY_EXTN(typer),
            GICD_TYPER_LSPI(typer));
}

/* gicd_enable_group: Enable forwarding of pending interrupts from the Distributor to the CPU interface
 */
void gicd_enable(struct GIC_Base *gicBase)
{

    // set enable bit in GICD_CTLR
    ULONG reg = readl(GICD_CTLR);
    reg |= 1;
    writel(reg, GICD_CTLR);
}

/* gicd_disable_group: Disable forwarding of pending interrupts from the Distributor to the CPU interface
 */
void gicd_disable(struct GIC_Base *gicBase)
{
    // clear enable bit in GICD_CTLR
    ULONG reg = readl(GICD_CTLR);
    reg &= ~1;
    writel(reg, GICD_CTLR);
}

/* gicd_is_enabled: Check enable bit for an IRQ in ISENABLER.
 * Args: irq - interrupt number.
 * Returns: TRUE when enabled, otherwise FALSE.
 */
BOOL gicd_is_enabled(struct GIC_Base *gicBase, ULONG irq)
{
    // read enabled status from GICD_ISENABLER
    ULONG reg_index = irq >> 5;
    ULONG bit_offset = irq & 0x1F;
    ULONG reg = readl(GICD_ISENABLER(reg_index));
    return (reg & (1UL << bit_offset)) != 0;
}

/* gicd_enable_irq: Set enable bit for an IRQ in ISENABLER.
 * Args: irq - interrupt number.
 * Returns: void.
 */
void gicd_enable_irq(struct GIC_Base *gicBase, ULONG irq)
{
    // set enable bit in GICD_ISENABLER
    ULONG reg_index = irq >> 5;
    ULONG bit_offset = irq & 0x1F;
    writel(1UL << bit_offset, GICD_ISENABLER(reg_index));
}

/* gicd_disable_irq: Clear enable bit for an IRQ via ICENABLER.
 * Args: irq - interrupt number.
 * Returns: void.
 */
void gicd_disable_irq(struct GIC_Base *gicBase, ULONG irq)
{
    // set disable bit in GICD_ICENABLER
    ULONG reg_index = irq >> 5;
    ULONG bit_offset = irq & 0x1F;
    writel(1UL << bit_offset, GICD_ICENABLER(reg_index));
}

/* gicd_is_pending: Check pending bit for an IRQ in ISPENDR.
 * Args: irq - interrupt number.
 * Returns: TRUE when the pending bit is set, otherwise FALSE.
 */
BOOL gicd_is_pending(struct GIC_Base *gicBase, ULONG irq)
{
    // read pending status from GICD_ISPENDR
    ULONG reg_index = irq >> 5;
    ULONG bit_offset = irq & 0x1F;
    ULONG reg = readl(GICD_ISPENDR(reg_index));
    return (reg & (1UL << bit_offset)) != 0;
}

/* gicd_set_pending: Set pending bit for an IRQ in ISPENDR.
 * Args: irq - interrupt number.
 * Returns: void.
 */
void gicd_set_pending(struct GIC_Base *gicBase, ULONG irq)
{
    // set pending bit in GICD_ISPENDR
    ULONG reg_index = irq >> 5;
    ULONG bit_offset = irq & 0x1F;
    writel(1UL << bit_offset, GICD_ISPENDR(reg_index));
}

/* gicd_clear_pending: Clear pending bit for an IRQ in ISPENDR.
 * Args: irq - interrupt number.
 * Returns: void.
 */
void gicd_clear_pending(struct GIC_Base *gicBase, ULONG irq)
{
    // clear pending bit in GICD_ICPENDR
    ULONG reg_index = irq >> 5;
    ULONG bit_offset = irq & 0x1F;
    writel(1UL << bit_offset, GICD_ICPENDR(reg_index));
}

/* gicd_get_irq_status: Report SPI active status from SPISR.
 * Args: irq - interrupt number.
 * Returns: TRUE when pending in SPISR, otherwise FALSE.
 */
BOOL gicd_get_irq_status(struct GIC_Base *gicBase, ULONG irq)
{
    // read SPI status from GICD_SPISR
    if (irq < 32)
        return FALSE; // SGI and PPI are not handled here

    ULONG spi_irq = irq - 32;
    ULONG reg_index = spi_irq >> 5;
    ULONG bit_offset = spi_irq & 0x1F;
    ULONG reg = readl(GICD_SPISR(reg_index));
    return (reg & (1 << bit_offset)) != 0;
}

/* gicd_is_active: Check active bit for an IRQ in ISACTIVER.
 * Args: irq - interrupt number.
 * Returns: TRUE when the active bit is set, otherwise FALSE.
 */
BOOL gicd_is_active(struct GIC_Base *gicBase, ULONG irq)
{
    // read active status from GICD_ISACTIVER
    ULONG reg_index = irq >> 5;
    ULONG bit_offset = irq & 0x1F;
    ULONG reg = readl(GICD_ISACTIVER(reg_index));
    return (reg & (1UL << bit_offset)) != 0;
}

void gicd_set_active(struct GIC_Base *gicBase, ULONG irq)
{
    ULONG reg_index = irq >> 5;
    ULONG bit_offset = irq & 0x1F;
    writel(1UL << bit_offset, GICD_ISACTIVER(reg_index));
}

void gicd_clear_active(struct GIC_Base *gicBase, ULONG irq)
{
    ULONG reg_index = irq >> 5;
    ULONG bit_offset = irq & 0x1F;
    writel(1UL << bit_offset, GICD_ICACTIVER(reg_index));
}

/* gicd_get_priority: Fetch per-IRQ priority value.
 * Args: irq - interrupt number.
 * Returns: priority byte read from IPRIORITYR.
 */
UBYTE gicd_get_priority(struct GIC_Base *gicBase, ULONG irq)
{
    // read priority from GICD_IPRIORITYR
    ULONG reg_index = irq >> 2;
    ULONG byte_offset = irq & 0x03;
    ULONG reg = readl(GICD_IPRIORITYR(reg_index));
    return (reg >> (byte_offset * 8)) & 0xFF;
}

/* gicd_set_priority: Program per-IRQ priority value.
 * Note that in non-secure mode the LSB is always 0.
 * Args: irq - interrupt number; priority - byte to store.
 * Returns: void.
 */
void gicd_set_priority(struct GIC_Base *gicBase, ULONG irq, UBYTE priority)
{
    // write priority to GICD_IPRIORITYR
    ULONG reg_index = irq >> 2;
    ULONG byte_offset = irq & 0x03;
    ULONG reg = readl(GICD_IPRIORITYR(reg_index));
    reg &= ~(0xFF << (byte_offset * 8));
    reg |= (priority & 0xFF) << (byte_offset * 8);
    writel(reg, GICD_IPRIORITYR(reg_index));
}

/* gicd_is_cpu_enabled: Check CPU target bit for an IRQ.
 * Args: irq - interrupt number; cpu - CPU index.
 * Returns: TRUE when the CPU is targeted, otherwise FALSE.
 */
BOOL gicd_is_cpu_enabled(struct GIC_Base *gicBase, ULONG irq, UBYTE cpu)
{
    // read target from GICD_ITARGETSR and check if cpu bit is set
    if (irq < 32)
        return FALSE; // SGI and PPI are not handled here

    ULONG reg_index = irq >> 2;
    ULONG byte_offset = irq & 0x03;
    ULONG reg = readl(GICD_ITARGETSR(reg_index));
    UBYTE target = (reg >> (byte_offset * 8)) & 0xFF;
    return (target & (1 << cpu)) != 0;
}

UBYTE gicd_get_cpu_mask(struct GIC_Base *gicBase, ULONG irq)
{
    ULONG reg_index = irq >> 2;
    ULONG byte_offset = irq & 0x03;
    ULONG reg = readl(GICD_ITARGETSR(reg_index));
    return (UBYTE)((reg >> (byte_offset * 8)) & 0xFF);
}

/* gicd_set_cpu: Set or clear CPU target bit for an IRQ.
 * Args: irq - interrupt number; cpu - CPU index; enable - TRUE to set.
 * Returns: void.
 */
void gicd_set_cpu(struct GIC_Base *gicBase, ULONG irq, UBYTE cpu, BOOL enable)
{
    // write target to GICD_ITARGETSR
    if (irq < 32)
        return; // SGI and PPI are not handled here

    ULONG reg_index = irq >> 2;
    ULONG byte_offset = irq & 0x03;
    ULONG reg = readl(GICD_ITARGETSR(reg_index));
    UBYTE target = (reg >> (byte_offset * 8)) & 0xFF;
    if (enable)
        target |= (1 << cpu);
    else
        target &= ~(1 << cpu);
    reg &= ~(0xFF << (byte_offset * 8));
    reg |= (target & 0xFF) << (byte_offset * 8);
    writel(reg, GICD_ITARGETSR(reg_index));
}

/* gicd_set_trigger: Configure trigger mode for an IRQ.
 * Args: irq - interrupt number; edge - TRUE for edge triggered.
 * Returns: void.
 */
void gicd_set_trigger(struct GIC_Base *gicBase, ULONG irq, BOOL edge)
{
    // write trigger mode to GICD_ICFGR
    if (irq < 16)
        return; // SGIs have fixed configuration

    ULONG reg_index = irq >> 4;
    ULONG bit_offset = (irq & 0x0F) * 2;
    ULONG reg = readl(GICD_ICFGR(reg_index));
    if (edge)
        reg |= (2UL << bit_offset); // 10b for edge-triggered
    else
        reg &= ~(2UL << bit_offset); // 00b for level-triggered
    writel(reg, GICD_ICFGR(reg_index));

    reg = readl(GICD_ICFGR(reg_index));
    BOOL is_edge = ((reg >> bit_offset) & 0x02) != 0;
    if (is_edge != edge)
    {
        Kprintf("[gic] Failed to set GICD IRQ %lu trigger to %s\n", irq, edge ? "edge" : "level");
    }
}
