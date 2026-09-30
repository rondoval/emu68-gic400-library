// SPDX-License-Identifier: MPL-2.0 OR GPL-2.0+
/*
 * gic400_dispatch.c - the Exec interrupt server, in file-scope assembly.
 *
 * Its own translation unit because file-scope asm() carries no IR: LTO does not know which
 * symbols it defines, and which partition it would land in is unspecified, so this TU is
 * compiled -fno-lto (see the CMakeLists).  Keeping it separate means only these few lines
 * opt out, instead of all of gic400_api.c.
 *
 * The asm cannot move to a .S file: its struct offsets come from offsetof() through "i"
 * operands, which needs the C compiler.
 */
#include <stddef.h>
#include <exec/memory.h>
#include <gic400_private.h>

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
    /* Its own section, as EMU68_INTSERVER() gives a C server one -- see
     * <intserver.h>.  GCC emits a top-level asm() before any function body and
     * tracks the current section itself, so this block hands .text back at the end
     * or anything after it follows in here. */
    "	.section .text.isr.gic400_exec_dispatcher,\"ax\"\n"
    "	.even\n"
    /* .globl: gic400_api.c used to hold this asm itself, so a local label sufficed.
       Now that it lives in its own TU the symbol has to be exported. */
    "	.globl	_gic400_exec_dispatcher\n"
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
    "	.text\n"                              /* back to where GCC thinks it is */
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
