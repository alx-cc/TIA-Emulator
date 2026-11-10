"""Expected TIA behaviour per AUDC mode, worked out from how the chip works
(independent of the emulator code).

Each voice divides the ~31.4 kHz audio clock by n = AUDF + 1 (by 3n for AUDC
12, 13, 14). On every divided clock the 5-bit poly advances (31 steps); the
mode decides whether the output changes:

  AUDC  sound                      repeat length (ticks)
  0,11  constant (volume only)     1
  1     4-bit poly                 15 n
  2     div31 clocks the 4-bit     465 n  (4-bit poly moves 2 per 31; 15 rounds)
  3     5-bit clocks the 4-bit     465 n  (4-bit poly moves 16 per 31; 15 rounds)
  4,5   pure tone (div 2)          2 n
  6     div31 pure                 31 n
  7     5-bit clocked pure         31 n   (16 toggles per 31 steps)
  8     9-bit poly                 511 n
  9     5-bit poly                 31 n
  10    div31 pure                 31 n
  12,13 div 6 pure                 6 n    (2 x 3n)
  14    div 93 pure                93 n   (31 x 3n)
  15    5-bit -> div3              93 n   (16 edges per 31; 3 rounds for an
                                           even number of div3 toggles)
"""

TIA_RATE = 31400  # ticks per second (NTSC 3579545 Hz / 114 = 31399.5)

PERIOD_FACTOR = {
    0: None, 11: None,
    1: 15, 2: 465, 3: 465,
    4: 2, 5: 2,
    6: 31, 7: 31, 8: 511, 9: 31, 10: 31,
    12: 6, 13: 6, 14: 93, 15: 93,
}


def expected_period(audc, audf):
    """Repeat length in ticks; 1 for the constant modes."""
    factor = PERIOD_FACTOR[audc]
    return 1 if factor is None else factor * (audf + 1)


def expected_pitch(audc, audf, rate=TIA_RATE):
    """Frequency of the repeating pattern in Hz (None for constant modes)."""
    if PERIOD_FACTOR[audc] is None:
        return None
    return rate / expected_period(audc, audf)
