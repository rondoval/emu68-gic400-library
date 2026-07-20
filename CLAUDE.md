# emu68-gic400-library

## Role

- Provides the `gic400.library` interface and generated AmigaOS headers under `proto/`, `clib/`, and `inline/`.
- Direct runtime dependency for `genet.device` and a build dependency for PCIe MSI support.
- Repository license: `MPL-2.0 OR GPL-2.0+`; preserve SPDX headers on edited files.

## Build Commands

`devicetree.resource` and `emu68-common` must be installed first.

Build through the superbuild's container wrapper — never host `cmake` (build trees
are configured at `/work` inside the toolchain container), from the
`emu68-driver-stack` superbuild root:

```sh
./scripts/docker-build.sh --target emu68-gic400-library
```

- Debug backend: `EMU68_CONFIGURE_ARGS="-DEMU68_DEBUG_BACKEND=serial" ./scripts/docker-build.sh` (default `pistorm` | `serial` | `off`). Selected stack-wide via `emu68-common`; `serial` links `debug.lib` and is not ROM-able.
- The SFD-derived headers are generated during the build. Do not add a manual `sfd/make.sh` step.
- `gic400.library` consumes shared helpers from `emu68-common`; ensure `Emu68Common` is available in the configured prefix.

## Code Handling

- Implementation lives in `src/` (`gic400_main.c` library boilerplate/Resident, `gic400_distributor.c` GICD register helpers, `gic400_api.c` exported API + device-tree discovery, `gic400_end.c`). Exports are defined by `sfd/gic400.sfd`; private types are in `include/gic400_private.h` and the public struct in `include/libraries/gic400.h`.
- Preserve the Amiga library API shape and generated header flow.
- The library is ROM-able: the linked binary must contain no writable `.data`/`.bss`, and `emu68_rom_check(gic400_library)` in `CMakeLists.txt` enforces this at build time. Keep mutable state in the allocated library base, not in globals.
- Be careful with changes that affect interrupt enable or teardown paths; the library has no reset hook of its own, so consumers (e.g. `genet.device`) are responsible for quiescing their interrupts before a soft reset via `emu68-common`'s `reset_guard`.
- If you change exported prototypes or generated headers, expect downstream rebuilds for `emu68-pcie-library` and `emu68-genet-driver`.

## Validation

- For runtime/deployment changes, target systems expect `gic400.library` in `LIBS:`.
- For common-helper or link dependency changes, validate that `emu68-common` is installed into the same prefix before rebuilding dependents.
- If public headers or exports change, validate through `emu68-driver-stack` or at least rebuild `emu68-pcie-library` and `emu68-genet-driver`.
