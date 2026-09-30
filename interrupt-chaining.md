# Multiple servers on one interrupt — the contract

`gic400.library` accepts **one interrupt server per GIC interrupt**. `AddIntServerEx()` refuses
a second one with `GIC400_ERR_ALREADY_REGISTERED`, and the dispatcher's per-IRQ storage is a flat
array of one `struct Interrupt *` per interrupt.

This document is the contract a chained implementation must satisfy. It is written **before**
the implementation so that consumers can be made ready first — `bcmpcie.library` and the drivers
already follow the parts that apply to them, and nothing in here changes how a single-server
line behaves.

## What a server must do

**Return "not handled" unless the interrupt was yours.** This is `exec.library/AddIntServer`'s
convention and it is unchanged:

> Each link in the chain will be called in priority order until the chain ends or one of the
> servers returns with the 68000's Z condition code clear (indicating non-zero). Servers on the
> chain should return with the Z flag clear if the interrupt was specifically for that server,
> and no one else.

So: **Z clear ends the walk, Z set continues it.** A server that always claims the interrupt
starves everything below it on the line.

Deciding whether it was yours means reading a status register of your own device. For a PCIe
function that has to be a memory-mapped register — config space on bus ≥ 1 is an index/data pair
and is not safe from an interrupt server.

### The Z flag, and why it needs checking

The autodoc's own warning is the practical problem:

> Some compilers or assemblers may optimize code in unexpected ways, affecting the conditions
> codes returned from the function. Watch out for a "MOVEM" instruction (which does not affect
> the condition codes) turning into "MOVE" (which does).

Every interrupt server in this stack returns `1` or `0` in `D0`. That normally leaves Z correct,
because GCC emits `moveq`/`clr.l` for the return value and then `movem.l (sp)+,…`, which does
not touch the CCR. But with a single saved data register GCC emits `move.l (sp)+,dN` instead —
and that sets Z from the **restored register**, not from the return value. Which one you get
depends on register allocation, so it changes under an innocent edit and it fails silently.

Nothing in the C source, the compiler flags or the type system can state this, so it is enforced
from the build instead.

#### When the check fails

Write that server in assembly — a C function with an asm wrapper around it is not the answer,
because it leaves the same fragile epilogue in the middle. `gic400_exec_dispatcher` in
`src/gic400_dispatch.c` is the pattern: a file-scope `__asm__` block, struct offsets passed in as
`offsetof()` `"i"` operands and referenced with `%c[name]`, guarded by `#ifndef
__INTELLISENSE__`.

This has happened once. `bcmgenet_isr0` in the SANA-II genet compiled to:

```
 de:  moveq  #1,d0        ; return value: Z clear, "handled"
 e0:  move.l (sp)+,d2     ; MOVE sets Z from the RESTORED d2
 e2:  movea.l (sp)+,a6
 e4:  rts
```

The flag the caller saw came from the caller's own `d2`. It was harmless — nothing read the flag,
and GENET's INTRL2_0 is a dedicated SPI — and it even worked by accident, because the dispatcher
keeps the never-zero raw IAR value in `d2` across the call. That accident is precisely what must
not be relied on. It is now written out in assembly, where the body turns out to need no
callee-saved register at all (`Signal()` is the last thing it does, so nothing has to survive a
call), which leaves a bare `rts` for an epilogue and the `moveq` reaching the caller intact.

One other server passes only *incidentally*: the legacy xhci's not-handled path leaves Z set from
the `and.l` that computed its status mask, and its epilogue happens to be `movea`/`addq`-to-`sp`
instructions that do not disturb it. Correct, but not by design — which is why the check runs on
every build rather than once.

## What the library must do

### 1. Storage

Thread the chain through the servers' own `ln_Succ`, keeping `handlers[irq]` as the head
pointer. No allocation — the library is ROM-able, so it has no writable `.data`/`.bss` and all
mutable state lives in the allocated `struct GIC_Base`, which does not need to grow for this.

It also keeps the dispatcher's hot path intact: one `move.l 0(%a3,%d1.l),%d1` from an index
scaled by `lsl.l #2`, and `beq` when nothing is registered.

### 2. Order

Descending `ln_Node.ln_Pri`, insertion order within a priority — as `exec.library/AddIntServer`
does. A caller that does not care leaves `ln_Pri` at 0.

Priority is worth thinking about on a busy line: every member ahead of the one that claims the
interrupt pays for its own status-register read first, and for a PCIe function that is an MMIO
read over the link. Put the busy device first.

### 3. The walk

After `jsr (%a5)`, branch on the CCR the server left: Z clear ends the walk, Z set moves on to
`ln_Succ`.

EOI stays **once per acknowledged interrupt** — after the walk and after the existing `nop`
(`dsb sy` under Emu68), not once per server. The barrier's job is unchanged: it makes a server's
level-clearing device write land before the GIC re-samples the line.

### 4. Line configuration

`priority` and `edge` are properties of the **line**, not of the server. The first registration
configures it. A later registration whose values differ must be refused with a new
`GIC400_ERR_CONFIG_CONFLICT` rather than silently inheriting the first registrant's settings —
otherwise the second caller gets a line configured in a way it never asked for, and the bug
surfaces as a missed or storming interrupt far from its cause.

Every caller in the stack registers with priority 0 and level trigger today, so nothing
conflicts.

### 5. Enable and disable — the dangerous part

The first server in enables and routes the line. **Only the last server out disables and
unroutes it.**

`RemIntServerEx()` currently disables and unroutes unconditionally. Left that way, one driver
shutting down would silently kill a co-registered driver's interrupts, with no error anywhere
and a failure that looks like a hung device. This is the single most important thing to get
right in the implementation.

### 6. Errors

- `GIC400_ERR_ALREADY_REGISTERED` narrows to "this exact `struct Interrupt` is already on this
  line". Re-adding the same pointer stays idempotent and returns 0, as it does now.
- `GIC400_ERR_NOT_FOUND` when removing a server that is not on the line.
- `GIC400_ERR_CONFIG_CONFLICT` as described above.

### 7. Bookkeeping

`handler_count` — written and never read today — becomes the live total across all lines. The
`SignalSemaphore` in `struct GIC_Base` stays unused: add and remove keep using
`Disable()`/`Enable()`, because the dispatcher walks the chain at interrupt level and a
semaphore cannot protect it.
