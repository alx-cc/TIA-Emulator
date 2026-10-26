# TIA sound emulator

A platform-agnostic emulator of the Atari 2600 TIA sound chip, meant to be
ported to microcontrollers and other targets.

The core is adapted from MAME's `tiasound.cpp` (Ron Fries' TIA sound code). It
has no hardware access, no heap and no timing of its own. A platform port runs
it at the chip's native rate (about 31.4 kHz) and sends each sample to its
audio output.

## Core interface

```
  platform/<target>/                       core/
+-----------------------------+          +----------------------+
| controls (pots, keypad, ..) |-write()->| AUDC/AUDF/AUDV regs  |
|                             |          |                      |
| timer tick, ~31.4 kHz       |-step()-->| dividers, polys,     |
|   '-> audio out (DAC, ..)   |<-sample--| 2-voice mix          |
+-----------------------------+          +----------------------+
```

- `write(reg, value)`, platform → core: set an AUDC, AUDF or AUDV register.
- `step()`, platform → core: advance one tick.
- sample, core → platform: the return value of `step()`, both voices mixed.

The core never calls into the platform. The platform drives it: it decides when
to write registers, calls `step()` once per tick, and sends each sample to its
audio output.

## Layout

```
core/              TIA sound core, hardware-independent
platform/<target>/ one directory per port
```

## Platforms

| Platform  | Status      |
|-----------|-------------|
| `stm32g4` | in progress |

The STM32G4 port targets the STM32G431 (WeAct Studio core board). A timer
triggers the DAC at the native rate, and pots on the ADC set the registers.

## Building

Requires CMake and the toolchain for the target platform (`arm-none-eabi` GCC
for STM32G4). Build instructions will be added once the first port builds.

## License

GPL-2.0-or-later. See [LICENSE](LICENSE).

The TIA sound code is based on Ron Fries' TIA sound emulation as found in
MAME; see the notice at the top of the core source file.
