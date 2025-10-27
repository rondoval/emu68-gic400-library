# emu68-gic400-library

*warning, this description is LLM generated :)*

**emu68-gic400-library** is a shared AmigaOS 3.x library that exposes the Arm GIC-400 interrupt controller used by Raspberry Pi 4 systems. It is designed to be consumed by drivers such as `genet.device`, consolidating interrupt routing logic in a reusable component.

## Features

- Native AmigaOS library interface with `proto/`, `clib/`, and `inline/` headers.
- Dynamic interrupt handler registration covering the full SPI range reported by the hardware.
- Device-tree driven discovery of distributor/CPU interface base addresses under Emu68.
- Helper APIs for querying interrupt state, changing trigger modes, routing, and priority masks.
- Optional debug logging to aid bring-up on new firmware or board revisions.

## Unimplemented / Planned Features

- SGI related functions (currently focused on SPIs).

## Bugs

- The system will hang after a soft reset if interrupts were enabled (e.g. when genet.device is online)

## Requirements

- Kickstart 3.0 (V39) or later.
- Emu68 firmware with device tree support (tested with Emu68 ≥ 1.0.5.1).
- Raspberry Pi 4B (Pi 3 does not have GIC-400 interrupt controller).
- Bebbo's m68k-amigaos GCC toolchain for building from source.

## Installing

When deploying alongside `genet.device`, ensure the built `gic400.library` is placed in `LIBS:` so the driver can `OpenLibrary()` it. A minimal installation step on the target Amiga system looks like:

```shell
copy install/gic400.library LIBS:
```

## Building

Generate the SFD-derived include headers first (needed when the repository is freshly cloned):

```sh
cd sfd
./make.sh
cd ..
```

Then configure and build the library using CMake and the provided toolchain file:

```sh
cmake .. -DCMAKE_TOOLCHAIN_FILE=../cmake/toolchain.cmake
make
make install
```
