# emu68-gic400-library Agent Notes

## Role

- This repo provides the `gic400.library` interface and generated AmigaOS headers under `proto/`, `clib/`, and `inline/`.
- It is a direct runtime dependency for `genet.device` and a build dependency for PCIe MSI support.

## Build

- `devicetree.resource` and `emu68-common` must be installed first.
- Preferred commands:
  - `cmake -S . -B build -DCMAKE_TOOLCHAIN_FILE=cmake/toolchain.cmake -DCMAKE_PREFIX_PATH=/path/to/emu68-driver-stack -DCMAKE_INSTALL_PREFIX=/path/to/emu68-driver-stack`
  - `cmake --build build`
  - `cmake --install build`
- The SFD-derived headers are generated during the build. Do not add a manual `sfd/make.sh` step.
- `gic400.library` now consumes shared helpers from `emu68-common`, so make sure `Emu68Common` is available in the configured prefix.

## Code Handling

- Preserve the Amiga library API shape and generated header flow.
- Be careful with changes that affect interrupt enable or teardown paths; the README notes a known soft-reset hang if interrupts were enabled.
- If you change exported prototypes or generated headers, expect downstream rebuilds for `emu68-pcie-library` and `emu68-genet-driver`.

## Validation

- For runtime/deployment changes, remember that target systems expect `gic400.library` in `LIBS:`.
- For common-helper or link dependency changes, validate that `emu68-common` is installed into the same prefix before rebuilding dependents.
- If public headers or exports change, validate through `emu68-driver-stack` or at least rebuild `emu68-pcie-library` and `emu68-genet-driver`.

