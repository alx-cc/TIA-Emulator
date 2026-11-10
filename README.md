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
host/              PC build of the core, for verification
host/reference/    MAME's original tiasound.cpp, unmodified
host/stella/       Stella's TIA audio channels, unmodified
tools/verify/      verification scripts (Python 3, standard library only)
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

## Verification

The core is checked on a PC, without hardware. Requires a C++ compiler, CMake,
Ninja and Python 3:

```
tools/verify/run.sh
```

This builds `tia_dump` (the core), `tia_dump_ref` (MAME's original file) and
`tia_dump_stella` (Stella's audio channels), runs the same register-write
script through all three, and checks:

1. **Exact match with MAME**, sample for sample, over every AUDC × AUDF
   setting on each voice, every volume, and 5000 random register writes.
   Pass/fail.
2. **Repeat length and levels** of every AUDC × AUDF setting on each voice,
   against the expected period for each mode (`tools/verify/tia_model.py`),
   and the frequencies against Eckhard Stolberg's published tables. Pass/fail.
3. **Comparison with Stella**, which models the TIA circuit independently.
   Report only: differences are for a person to look at, and the core is not
   changed to match.

`tia_dump` also writes `build-host/verify/ours.wav` (the whole sweep, about
3.5 minutes) for listening. To render your own sounds, write a script (see the
format at the top of `host/tia_dump.cpp`) and run
`build-host/tia_dump script.txt out.raw out.wav`.

## License

GPL-2.0-or-later. See [LICENSE](LICENSE).

The TIA sound code is based on Ron Fries' TIA sound emulation as found in
MAME; see the notice at the top of the core source file.
