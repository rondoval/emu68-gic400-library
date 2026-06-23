# Release notes — gic400.library 1.5

Changes since v1.4.

---

## Breaking changes

None.

---

## Improvements / Maintenance

### Debug output now follows the stack-wide backend

The library no longer hard-codes `-DDEBUG`.  Debug output is selected through
emu68-common's stack-wide `EMU68_DEBUG_BACKEND` (`pistorm` | `serial` | `off`):
the build now calls `emu68_debug_backend_definitions()` for the compile defines and
`emu68_debug_backend_finalize(gic400_library ROMABLE)` in place of the direct
`emu68_rom_check`.  For the `serial` backend `finalize` links `debug.lib` (plus the
weak `__divsi3` glue) and skips the ROM check; otherwise it runs the ROM check as
before.  The debug-only print helpers (`gicc_print_info`, `gicc_log_ctlr`,
`gicd_print_info`) are now wrapped in `#ifdef DEBUG` so they compile out cleanly
when the backend is `off`, and `gic400_validate_cpu_target()` casts its `caller`
argument to `(void)` in non-debug builds to avoid an unused-parameter warning.

### Built at `-O3`

Compile options changed from `-Os` to `-O3`, matching the rest of the stack.  No
source changes were required.

### Versioning workflow

A `.github/workflows/versioning.yml` that gates every PR on a version bump + a
`RELEASE-NOTES.md` update + a clean build inside the stack, and auto-tags
`v<version>` on merge to `main`, via the stack's reusable
`component-versioning.yml` so all components stay in lock-step.

### `$VER` string reports `major.minor`

The project version was simplified to `1.5` (from `1.4.0`) and the embedded
`$VER:` string now uses `PROJECT_VERSION_MAJOR.PROJECT_VERSION_MINOR`, so it
matches the library's `LIBRARY_VERSION` / `LIBRARY_REVISION` (e.g. `1.5` instead
of `1.5.0`).


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
