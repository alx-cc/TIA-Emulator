# Stella reference (do not edit)

`AudioChannel.cxx` and `AudioChannel.hxx` are copied unchanged from Stella, the
Atari 2600 emulator. Stella models the TIA's audio circuit directly (divider,
5-bit pulse counter, 4-bit noise LFSR), independently of Ron Fries' code, so
it is a second opinion on our output. Differences are reported, not failed:
the core is never changed to match Stella.

- Source: https://github.com/stella-emu/stella, `src/emucore/tia/`
- Commit: `b66e7dc9428270e931913f0afe827929a6f77f5b` (master, November 2026;
  last change to `AudioChannel.cxx` was `e6e5779cf758893a2478c912c27cf7e42e9d768a`)
- License: GPL-2.0-or-later (Stella's Copyright.txt: "version 2 of the
  License, or any later version")
- SHA-256:
  - `AudioChannel.cxx`: `c0fd1e92325d5b9acb708b545048adf6aa304def5c86f752e2f533e476909f4d`
  - `AudioChannel.hxx`: `46595afc28257e472b1cf84cde19cf290669c81934f9d7a80a27fd95dda52568`

Stella clocks each channel twice per audio tick: `phase0()` (divider) then
`phase1()` (counters and output), at TIA colour clocks 9/37 and 81/149 of each
scanline. `actualVolume()` is 0 to 15.

`bspf.hxx` and `Serializable.hxx` are not from Stella. They are minimal
stand-ins for the Stella framework pieces the files use.
