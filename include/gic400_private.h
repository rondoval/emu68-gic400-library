#ifndef _GIC400_PRIVATE_H
#define _GIC400_PRIVATE_H

#include <exec/libraries.h>
#include <exec/types.h>
#include <exec/semaphores.h>
#include <exec/interrupts.h>
#include <hardware/intbits.h>
#include <debug.h>
#include <compat.h>
#include <libraries/gic400.h>

#if defined(__INTELLISENSE__)
#define asm(x)
#define __attribute__(x)
#endif

/* These are overriden by cmake */
#ifndef LIBRARY_NAME
#define LIBRARY_NAME "gic400.library"
#endif

#ifndef LIBRARY_IDSTRING
#define LIBRARY_IDSTRING "gic400.library 1.0 (26.10.2025)"
#endif

#ifndef LIBRARY_VERSION
#define LIBRARY_VERSION 1
#endif

#ifndef LIBRARY_REVISION
#define LIBRARY_REVISION 0
#endif

#ifndef LIBRARY_PRIORITY
#define LIBRARY_PRIORITY 0
#endif

#define GIC_ERROR 1
#define GIC_MAX_REGISTERED_IRQS 16U /* ...should be enough for everyone ;) */

struct gic_irq_handler
{
    ULONG irq;
    struct Interrupt *interrupt;
};

struct GIC_Base
{
    struct Library libNode;
    ULONG segList;

    struct SignalSemaphore semaphore;

    ULONG gicd_iidr;
    ULONG gicd_typer;
    ULONG gicc_iidr;

    APTR gic_base_distributor;
    APTR gic_base_cpuif;
    ULONG max_irqs;

    struct gic_irq_handler handlers[GIC_MAX_REGISTERED_IRQS];
    ULONG handler_count;

    struct Interrupt dispatcher_interrupt;
};

/* GIC Distributor and CPU interface identification helpers. */
#define GICD_IIDR_PRODUCT_ID(value) (((value) >> 24) & 0xFF)
#define GICD_IIDR_VARIANT(value) (((value) >> 16) & 0x0F)
#define GICD_IIDR_REVISION(value) (((value) >> 12) & 0x0F)
#define GICD_IIDR_IMPLEMENTER(value) ((value) & 0x0FFF)

#define GICD_TYPER_IT_LINES_NUMBER(value) ((value) & 0x1F)
#define GICD_TYPER_CPUS_NUMBER(value) (((value) >> 5) & 0x07)
#define GICD_TYPER_SECURITY_EXTN(value) (((value) >> 10) & 0x01)
#define GICD_TYPER_LSPI(value) (((value) >> 11) & 0x1F)

#define GICC_IIDR_PRODUCT_ID(value) (((value) >> 20) & 0x0FFF)
#define GICC_IIDR_ARCHITECTURE(value) (((value) >> 16) & 0x0F)
#define GICC_IIDR_REVISION(value) (((value) >> 12) & 0x0F)
#define GICC_IIDR_IMPLEMENTER(value) ((value) & 0x0FFF)

/* Bit definitions for GICC_CTLR */
#define GICC_CTLR_ENABLE_GRP1 (1UL << 0)
#define GICC_CTLR_FIQ_BYPASS_DIS_GRP1 (1UL << 5)
#define GICC_CTLR_IRQ_BYPASS_DIS_GRP1 (1UL << 6)
#define GICC_CTLR_EOI_MODE_NS (1UL << 9)

#define GICC_CTLR_FLAG(value, mask) ((ULONG)(((value) & (mask)) != 0))

/* CPU Interface */
#define GICC_CTLR (gicBase->gic_base_cpuif + 0x000) // CPU Interface Control Register
#define GICC_PMR (gicBase->gic_base_cpuif + 0x004)  // Interrupt Priority Mask Register

#define GICC_BPR (gicBase->gic_base_cpuif + 0x008)   // Binary Point Register
#define GICC_IAR (gicBase->gic_base_cpuif + 0x00C)   // Interrupt Acknowledge Register
#define GICC_EOIR (gicBase->gic_base_cpuif + 0x010)  // End of Interrupt Register
#define GICC_RPR (gicBase->gic_base_cpuif + 0x014)   // Running Priority Register
#define GICC_HPPIR (gicBase->gic_base_cpuif + 0x018) // Highest Priority Pending Interrupt Register

#define GICC_ABPR (gicBase->gic_base_cpuif + 0x01C)   // Aliased Binary Point Register
#define GICC_AIAR (gicBase->gic_base_cpuif + 0x020)   // Aliased Interrupt Acknowledge Register
#define GICC_AEOIR (gicBase->gic_base_cpuif + 0x024)  // Aliased End of Interrupt Register
#define GICC_AHPPIR (gicBase->gic_base_cpuif + 0x028) // Aliased Highest Priority Pending Interrupt Register

#define GICC_APR(n) (gicBase->gic_base_cpuif + 0x0D0 + (n) * 4)   // Active Priority Register
#define GICC_NSAPR(n) (gicBase->gic_base_cpuif + 0x0E0 + (n) * 4) // Non-secure Active Priority Register
#define GICC_IIDR (gicBase->gic_base_cpuif + 0x0FC)               // CPU Interface Implementer Identification Register
#define GICC_DIR (gicBase->gic_base_cpuif + 0x1000)               // Deactivate Interrupt Register

/* Distributor */
#define GICD_CTLR (gicBase->gic_base_distributor + 0x000)                    // Distributor Control Register
#define GICD_TYPER (gicBase->gic_base_distributor + 0x004)                   // Interrupt Controller Type Register
#define GICD_IIDR (gicBase->gic_base_distributor + 0x008)                    // Distributor Implementer ID Register
#define GICD_IGROUPR(n) (gicBase->gic_base_distributor + 0x080 + (n) * 4)    // Interrupt Group Registers
#define GICD_ISENABLER(n) (gicBase->gic_base_distributor + 0x100 + (n) * 4)  // Interrupt Set-Enable Registers
#define GICD_ICENABLER(n) (gicBase->gic_base_distributor + 0x180 + (n) * 4)  // Interrupt Clear-Enable Registers
#define GICD_ISPENDR(n) (gicBase->gic_base_distributor + 0x200 + (n) * 4)    // Interrupt Set-Pending Registers
#define GICD_ICPENDR(n) (gicBase->gic_base_distributor + 0x280 + (n) * 4)    // Interrupt Clear-Pending Registers
#define GICD_ISACTIVER(n) (gicBase->gic_base_distributor + 0x300 + (n) * 4)  // Interrupt Set-Active Registers
#define GICD_ICACTIVER(n) (gicBase->gic_base_distributor + 0x380 + (n) * 4)  // Interrupt Clear-Active Registers
#define GICD_IPRIORITYR(n) (gicBase->gic_base_distributor + 0x400 + (n) * 4) // Interrupt Priority Registers
#define GICD_ITARGETSR(n) (gicBase->gic_base_distributor + 0x800 + (n) * 4)  // Interrupt Processor Targets Registers
#define GICD_ICFGR(n) (gicBase->gic_base_distributor + 0xC00 + (n) * 4)      // Interrupt Configuration Registers
#define GICD_SPISR(n) (gicBase->gic_base_distributor + 0xD04 + (n) * 4)      // Shared Peripheral Interrupt Status Registers
#define GICD_COMPONENT_ID (gicBase->gic_base_distributor + 0xFF0)            // Component ID Register
#define GICD_PERIPHERAL_ID (gicBase->gic_base_distributor + 0xFE0)           // Peripheral ID Register
// TODO SGIR(n)
// TODO CPENDSGIR(n)
// TODO SPENDSGIR(n)

/* API function prototypes */
int gic400_init(struct GIC_Base *gicBase);
void gic400_shutdown(struct GIC_Base *gicBase);
ULONG AddIntServerEx(ULONG irq asm("d0"), UBYTE priority asm("d1"), BOOL edge asm("d2"), struct Interrupt *interrupt asm("a1"), struct GIC_Base *gicBase asm("a6"));
ULONG RemIntServerEx(ULONG irq asm("d0"), struct Interrupt *interrupt asm("a1"), struct GIC_Base *gicBase asm("a6"));
ULONG GetIntStatus(ULONG irq asm("d0"), BOOL *pending asm("a1"), BOOL *active asm("a2"), BOOL *enabled asm("a3"), struct GIC_Base *gicBase asm("a6"));
ULONG EnableInt(ULONG irq asm("d0"), struct GIC_Base *gicBase asm("a6"));
ULONG DisableInt(ULONG irq asm("d0"), struct GIC_Base *gicBase asm("a6"));
ULONG SetIntPriority(ULONG irq asm("d0"), UBYTE priority asm("d1"), struct GIC_Base *gicBase asm("a6"));
LONG GetIntPriority(ULONG irq asm("d0"), struct GIC_Base *gicBase asm("a6"));
ULONG SetIntTriggerEdge(ULONG irq asm("d0"), struct GIC_Base *gicBase asm("a6"));
ULONG SetIntTriggerLevel(ULONG irq asm("d0"), struct GIC_Base *gicBase asm("a6"));
ULONG RouteIntToCpu(ULONG irq asm("d0"), UBYTE cpu asm("d1"), struct GIC_Base *gicBase asm("a6"));
ULONG UnrouteIntFromCpu(ULONG irq asm("d0"), UBYTE cpu asm("d1"), struct GIC_Base *gicBase asm("a6"));
LONG QueryIntRoute(ULONG irq asm("d0"), struct GIC_Base *gicBase asm("a6"));
ULONG SetIntPending(ULONG irq asm("d0"), struct GIC_Base *gicBase asm("a6"));
ULONG ClearIntPending(ULONG irq asm("d0"), struct GIC_Base *gicBase asm("a6"));
ULONG SetIntActive(ULONG irq asm("d0"), struct GIC_Base *gicBase asm("a6"));
ULONG ClearIntActive(ULONG irq asm("d0"), struct GIC_Base *gicBase asm("a6"));
ULONG SetPriorityMask(UBYTE mask asm("d0"), struct GIC_Base *gicBase asm("a6"));
LONG GetPriorityMask(struct GIC_Base *gicBase asm("a6"));
LONG GetRunningPriority(struct GIC_Base *gicBase asm("a6"));
LONG GetHighestPending(struct GIC_Base *gicBase asm("a6"));
LONG GetControllerInfo(struct GICInfo *info asm("a1"), struct GIC_Base *gicBase asm("a6"));

/* Internal function prototypes and macros */
#define gicc_set_ctlr(ctlr_value) writel((ctlr_value), GICC_CTLR)
#define gicc_get_ctlr() readl(GICC_CTLR)
#define gicc_set_priority_mask(priority_value) writel((priority_value), GICC_PMR)
#define gicc_acknowledge_interrupt() readl(GICC_IAR)
#define gicc_end_interrupt(irq_value) writel((irq_value), GICC_EOIR)
#define gicc_deactivate_interrupt(irq_value) writel((irq_value), GICC_DIR)
#define gicc_get_running_priority() (readl(GICC_RPR) & 0xFF)
#define gicc_get_highest_pending() (readl(GICC_HPPIR) & 0x3FF)

static inline void gicc_print_info(ULONG gicc_iidr)
{
    Kprintf("[gic] Controller: Implementer=0x%03lx, Revision=%ld, Architecture=%ld, ProductID=0x%03lx\n",
            GICC_IIDR_IMPLEMENTER(gicc_iidr),
            GICC_IIDR_REVISION(gicc_iidr),
            GICC_IIDR_ARCHITECTURE(gicc_iidr),
            GICC_IIDR_PRODUCT_ID(gicc_iidr));
}

static inline void gicc_log_ctlr(CONST_STRPTR label, ULONG ctlr)
{
    Kprintf("[gic] %s GICC_CTLR=0x%08lx: enable_grp1=%ld, fiq_bypass_dis_grp1=%ld, irq_bypass_dis_grp1=%ld, eoi_mode_ns=%ld\n",
            label,
            ctlr,
            GICC_CTLR_FLAG(ctlr, GICC_CTLR_ENABLE_GRP1),
            GICC_CTLR_FLAG(ctlr, GICC_CTLR_FIQ_BYPASS_DIS_GRP1),
            GICC_CTLR_FLAG(ctlr, GICC_CTLR_IRQ_BYPASS_DIS_GRP1),
            GICC_CTLR_FLAG(ctlr, GICC_CTLR_EOI_MODE_NS));
}

static inline void gicc_get_priority_mask(struct GIC_Base *gicBase, UBYTE *priority)
{
    if (!gicBase || !priority)
        return;

    *priority = readl(GICC_PMR) & 0xFF;
}

void gicd_print_info(struct GIC_Base *gicBase);
void gicd_enable(struct GIC_Base *gicBase);
void gicd_disable(struct GIC_Base *gicBase);
BOOL gicd_get_irq_status(struct GIC_Base *gicBase, ULONG irq);
BOOL gicd_is_pending(struct GIC_Base *gicBase, ULONG irq);
void gicd_set_pending(struct GIC_Base *gicBase, ULONG irq);
void gicd_clear_pending(struct GIC_Base *gicBase, ULONG irq);
BOOL gicd_is_active(struct GIC_Base *gicBase, ULONG irq);
BOOL gicd_is_enabled(struct GIC_Base *gicBase, ULONG irq);
void gicd_enable_irq(struct GIC_Base *gicBase, ULONG irq);
void gicd_disable_irq(struct GIC_Base *gicBase, ULONG irq);
UBYTE gicd_get_priority(struct GIC_Base *gicBase, ULONG irq);
void gicd_set_priority(struct GIC_Base *gicBase, ULONG irq, UBYTE priority);
BOOL gicd_is_cpu_enabled(struct GIC_Base *gicBase, ULONG irq, UBYTE cpu);
void gicd_set_cpu(struct GIC_Base *gicBase, ULONG irq, UBYTE cpu, BOOL enable);
void gicd_set_trigger(struct GIC_Base *gicBase, ULONG irq, BOOL edge);
void gicd_set_active(struct GIC_Base *gicBase, ULONG irq);
void gicd_clear_active(struct GIC_Base *gicBase, ULONG irq);
UBYTE gicd_get_cpu_mask(struct GIC_Base *gicBase, ULONG irq);

#endif /* _GIC400_PRIVATE_H */
