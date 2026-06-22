# Release notes — gic400.library 1.4.0

Changes since v1.3.

---

## Breaking changes

None.

---

## Improvements / Maintenance

### Verified ROM-able: no writable `.data`/`.bss`

`gic400_library` is now covered by the shared `emu68_rom_check` build-time guard.
The check runs POST_BUILD and fails the build if the linked `gic400.library`
contains a non-empty `.data` or `.bss` section.  ROM modules must keep all
mutable state in allocated memory (the library base); a stray writable static
would silently break that invariant.  The library was already free of such
statics, so this commit adds a regression guard rather than fixing an offender.
The same guard now protects `nvme.device`, `genet.device`, `xhci.device`, and
`bcmpcie.library` across the stack.

### Resident priority raised to 126

The library's `Resident` node priority (`rt_Pri`) was changed from `0` to `126`.
This affects only the order in which `RTF_AUTOINIT` residents are initialised
during the system resident scan; it does not change the library's public API,
its on-demand `OpenLibrary()` behaviour, or its interrupt-routing semantics.
There is no user-visible effect — `gic400.library` is opened on demand by the
drivers that consume it (such as `genet.device`), not selected by resident
priority.


# Release notes — gic400.library 1.3

## What's Changed
* Fixes for #4 and #5 
* ROM-able

## Internal changes
* Adjusted to a new build system and rewritten emu68-common


**Full Changelog**: https://github.com/rondoval/emu68-gic400-library/compare/v1.1...v1.3


# Release notes — gic400.library 1.1


- unroute all SPIs from CPU 0 before enabling the controller
- Updated register clobber lists in inline assembly
- Changed return value when an out-of-range IRQ is acknowledged

**Full Changelog**: https://github.com/rondoval/emu68-gic400-library/compare/v1.0...v1.1


# Release notes — gic400.library 1.0

Initial release of the gic400.library. 

See README.md
