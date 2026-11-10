# MAME reference (do not edit)

`tiasound.cpp` and `tiasound.h` are copied unchanged from MAME, so the adapted
core in `core/` can be compared against the original sample for sample. If
these files change, the comparison means nothing.

- Source: https://github.com/mamedev/mame, `src/devices/sound/tiasound.cpp` and
  `src/devices/sound/tiasound.h`
- Commit: `ca22ea616c031b9afeb5dad5533d419278c90c43` (master, November 2026;
  last change to `tiasound.cpp` was `d0f1c15a0f6df2dd51a754cb46e6175b7079c8f2`)
- SHA-256:
  - `tiasound.cpp`: `f8285eb107fd2e9835107d8d9146ebdea6226748ec6184b2f9d2bdf4a8223942`
  - `tiasound.h`: `2fcdbbf6fd5d5c89dea7ced0bd39715c733a009babb403e72a8209e0efbbc2d0`

`emu.h` and `tiaintf.h` are not from MAME. They are minimal stand-ins for the
MAME framework pieces the file uses, so it compiles on its own.
