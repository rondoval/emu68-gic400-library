# emu68-gic400-library Agent Notes

## Role

- This repo provides the `gic400.library` interface and generated AmigaOS headers under `proto/`, `clib/`, and `inline/`.
- It is a direct runtime dependency for `genet.device` and a build dependency for PCIe MSI support.
- The repository presents as `MPL-2.0 OR GPL-2.0+`; preserve SPDX headers on edited files.

## Build

- `devicetree.resource` and `emu68-common` must be installed first.
- Build through the superbuild's container wrapper — never host `cmake` (build trees
  are configured at `/work` inside the toolchain container):
  - from the `emu68-driver-stack` superbuild root: `./scripts/docker-build.sh --target emu68-gic400-library`
- Debug backend: `EMU68_CONFIGURE_ARGS="-DEMU68_DEBUG_BACKEND=serial" ./scripts/docker-build.sh` (default `pistorm` | `serial` | `off`); selected stack-wide via `emu68-common`, `serial` links `debug.lib` and is not ROM-able.
- The SFD-derived headers are generated during the build. Do not add a manual `sfd/make.sh` step.
- `gic400.library` now consumes shared helpers from `emu68-common`, so make sure `Emu68Common` is available in the configured prefix.

## Code Handling

- Preserve the Amiga library API shape and generated header flow.
- Be careful with changes that affect interrupt enable or teardown paths; the library has no reset hook of its own, so consumers (e.g. `genet.device`) are responsible for quiescing their interrupts before a soft reset via `emu68-common`'s `reset_guard`.
- If you change exported prototypes or generated headers, expect downstream rebuilds for `emu68-pcie-library` and `emu68-genet-driver`.

## Validation

- For runtime/deployment changes, remember that target systems expect `gic400.library` in `LIBS:`.
- For common-helper or link dependency changes, validate that `emu68-common` is installed into the same prefix before rebuilding dependents.
- If public headers or exports change, validate through `emu68-driver-stack` or at least rebuild `emu68-pcie-library` and `emu68-genet-driver`.

