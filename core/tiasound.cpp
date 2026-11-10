// license:GPL-2.0+
// copyright-holders:Ron Fries,Dan Boris
/*****************************************************************************/
/*                                                                           */
/* Module:  TIA Chip Sound Simulator                                         */
/* Purpose: To emulate the sound generation hardware of the Atari TIA chip.  */
/* Author:  Ron Fries                                                        */
/*                                                                           */
/* Revision History:                                                         */
/*    10-Sep-96 - V1.0 - Initial Release                                     */
/*    14-Jan-97 - V1.1 - Cleaned up sound output by eliminating counter      */
/*                       reset.                                              */
/*    30-Oct-98 - Modified for use in MESS by Dan Boris                      */
/*    28-Jul-01 - Added support for sample rates > TIA clock rate,           */
/*                through oversampling                                       */
/*    30-Jun-07 - Updated the poly generation. Improved handling of the      */
/*                POLY5_DIV3 mode. (Wilbert Pol)                             */
/*                                                                           */
/*                                                                           */
/*                                                                           */
/*****************************************************************************/
/*                                                                           */
/*                 License Information and Copyright Notice                  */
/*                 ========================================                  */
/*                                                                           */
/* TiaSound is Copyright Ron Fries                                           */
/*                                                                           */
/* This library is free software; you can redistribute it and/or modify it   */
/* under the terms of version 2 of the GNU Library General Public License    */
/* as published by the Free Software Foundation.                             */
/*                                                                           */
/* This library is distributed in the hope that it will be useful, but       */
/* WITHOUT ANY WARRANTY; without even the implied warranty of                */
/* MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU Library */
/* General Public License for more details.                                  */
/* To obtain a copy of the GNU Library General Public License, write to the  */
/* Free Software Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.   */
/*                                                                           */
/* Any permitted reproduction of these routines, in whole or in part, must   */
/* bear this legend.                                                         */
/*                                                                           */
/*****************************************************************************/
/*                                                                           */
/* Adapted from MAME src/devices/sound/tiasound.cpp for standalone use, 2026.*/
/* MAME commit ca22ea616c031b9afeb5dad5533d419278c90c43. Changes:            */
/*  - removed MAME framework code (emu.h, device_t, save_item, sound_stream) */
/*    and the unused gain setting                                            */
/*  - removed the sample-rate counter and oversampling: step() runs one TIA  */
/*    audio clock tick and returns one sample                                */
/*  - tia_process()'s loop body is now step(), working on the members        */
/*    directly instead of local copies; offs_t replaced with uint8_t         */
/* The TIA logic (poly tables, dividers, mode handling) is unchanged.        */
/*                                                                           */
/*****************************************************************************/

#include "tiasound.h"

/* number of bits to shift left AUDV0/AUDV1 registers for 16 bit volume */
#define AUDV_SHIFT  10

/* CONSTANT DEFINITIONS */

/* definitions for AUDCx (15, 16) */
#define SET_TO_1     0x00               /* 0000 */
#define POLY4        0x01               /* 0001 */
#define DIV31_POLY4  0x02               /* 0010 */
#define POLY5_POLY4  0x03               /* 0011 */
#define PURE         0x04               /* 0100 */
#define PURE2        0x05               /* 0101 */
#define DIV31_PURE   0x06               /* 0110 */
#define POLY5_2      0x07               /* 0111 */
#define POLY9        0x08               /* 1000 */
#define POLY5        0x09               /* 1001 */
#define DIV31_POLY5  0x0a               /* 1010 */
#define POLY5_POLY5  0x0b               /* 1011 */
#define DIV3_PURE    0x0c               /* 1100 */
#define DIV3_PURE2   0x0d               /* 1101 */
#define DIV93_PURE   0x0e               /* 1110 */
#define POLY5_DIV3   0x0f               /* 1111 */

#define DIV3_MASK    0x0c

namespace {

/* I've treated the 'Div by 31' counter as another polynomial because of */
/* the way it operates.  It does not have a 50% duty cycle, but instead */
/* has a 13:18 ratio (of course, 13+18 = 31).  This could also be */
/* implemented by using counters. */

constexpr uint8_t Div31[POLY5_SIZE] =
	{0, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0};

} // anonymous namespace


tia_sound::tia_sound()
{
	// fill the polynomials
	poly_init(Bit4, 4, 4, 3);
	poly_init(Bit5, 5, 5, 3);
	poly_init(Bit9, 9, 9, 5);
}


/* Initialze the bit patterns for the polynomials. */

/* The 4bit and 5bit patterns are the identical ones used in the tia chip. */
/* Though the patterns could be packed with 8 bits per byte, using only a */
/* single bit per byte keeps the math simple, which is important for */
/* efficient processing. */

/* HJB: poly bits are initialized at runtime */

template <unsigned N>
void tia_sound::poly_init(uint8_t (&poly)[N], int size, int f0, int f1)
{
	const int mask = (1 << size) - 1;
	int x = mask;

	for (int i = 0; i < mask; i++)
	{
		const int bit0 = ((size - f0) ? (x >> (size - f0)) : x) & 0x01;
		const int bit1 = ((size - f1) ? (x >> (size - f1)) : x) & 0x01;
		poly[i] = x & 1;
		/* calculate next bit */
		x = (x >> 1) | ((bit0 ^ bit1) << ( size - 1) );
	}
}


/*****************************************************************************/
/* Module:  tia_sound_w()                                                    */
/* Purpose: To process the latest control values stored in the AUDF, AUDC,   */
/*          and AUDV registers.  It pre-calculates as much information as    */
/*          possible for better performance.  This routine has not been      */
/*          optimized.                                                       */
/*                                                                           */
/* Author:  Ron Fries                                                        */
/* Date:    January 14, 1997                                                 */
/*                                                                           */
/* Inputs:  addr - the address of the parameter to be changed                */
/*          val - the new value to be placed in the specified address        */
/*                                                                           */
/* Outputs: Adjusts local globals - no return value                          */
/*                                                                           */
/*****************************************************************************/

void tia_sound::write(uint8_t offset, uint8_t data)
{
	// determine which address was changed
	uint8_t chan;
	switch (offset)
	{
	case AUDC0:
		AUDC[0] = data & 0x0f;
		chan = 0;
		break;

	case AUDC1:
		AUDC[1] = data & 0x0f;
		chan = 1;
		break;

	case AUDF0:
		AUDF[0] = data & 0x1f;
		chan = 0;
		break;

	case AUDF1:
		AUDF[1] = data & 0x1f;
		chan = 1;
		break;

	case AUDV0:
		AUDV[0] = ((data & 0x0f) << AUDV_SHIFT);
		chan = 0;
		break;

	case AUDV1:
		AUDV[1] = ((data & 0x0f) << AUDV_SHIFT);
		chan = 1;
		break;

	default:
		return;
	}

	// an AUDC value of 0 is a special case
	uint16_t new_val;
	if (AUDC[chan] == SET_TO_1 || AUDC[chan] == POLY5_POLY5)
	{
		// indicate the clock is zero so no processing will occur
		new_val = 0;

		// and set the output to the selected volume
		Outvol[chan] = AUDV[chan];
	}
	else
	{
		// otherwise calculate the 'divide by N' value
		new_val = AUDF[chan] + 1;

		// if bits 2 & 3 are set, then multiply the 'div by n' count by 3
		if ((AUDC[chan] & DIV3_MASK) == DIV3_MASK && AUDC[chan] != POLY5_DIV3)
		{
			new_val *= 3;
		}
	}

	// only reset those channels that have changed
	if (new_val != Div_n_max[chan])
	{
		// reset the divide by n counters
		Div_n_max[chan] = new_val;

		// if the channel is now volume only or was volume only
		if ((Div_n_cnt[chan] == 0) || (new_val == 0))
		{
			// reset the counter (otherwise let it complete the previous)
			Div_n_cnt[chan] = new_val;
		}
	}
}


/*****************************************************************************/
/* Module:  step()  (was tia_process())                                      */
/* Purpose: To advance the tia chip by one audio clock tick and return the   */
/*          sound output based on the tia chip parameters.                   */
/*                                                                           */
/* Author:  Ron Fries                                                        */
/* Date:    September 10, 1996                                               */
/*                                                                           */
/* Outputs: the sum of both channels' output volumes                         */
/*                                                                           */
/*****************************************************************************/

int16_t tia_sound::step()
{
	uint8_t audc0 = AUDC[0];
	uint8_t audc1 = AUDC[1];
	int16_t audv0 = AUDV[0];
	int16_t audv1 = AUDV[1];

	/* Process channel 0 */
	if (Div_n_cnt[0] > 1)
	{
		Div_n_cnt[0]--;
	}
	else if (Div_n_cnt[0] == 1)
	{
		int prev_bit5 = Bit5[P5[0]];

		Div_n_cnt[0] = Div_n_max[0];

		/* the P5 counter has multiple uses, so we inc it here */
		P5[0]++;
		if (P5[0] == POLY5_SIZE)
			P5[0] = 0;

		/* check clock modifier for clock tick */
		if ((audc0 & 0x02) == 0 ||
			((audc0 & 0x01) == 0 && Div31[P5[0]]) ||
			((audc0 & 0x01) == 1 && Bit5[P5[0]]) ||
			((audc0 & 0x0f) == POLY5_DIV3 && Bit5[P5[0]] != prev_bit5))
		{
			if (audc0 & 0x04)       /* pure modified clock selected */
			{
				if ((audc0 & 0x0f) == POLY5_DIV3)   /* POLY5 -> DIV3 mode */
				{
					if ( Bit5[P5[0]] != prev_bit5 )
					{
						Div_3_cnt[0]--;
						if ( ! Div_3_cnt[0] )
						{
							Div_3_cnt[0] = 3;
							if (Outvol[0])
								Outvol[0] = 0;
							else
								Outvol[0] = audv0;
						}
					}
				}
				else if (Outvol[0])       /* if the output was set */
					Outvol[0] = 0;   /* turn it off */
				else
					Outvol[0] = audv0;   /* else turn it on */
			}
			else if (audc0 & 0x08)  /* check for p5/p9 */
			{
				if (audc0 == POLY9) /* check for poly9 */
				{
					/* inc the poly9 counter */
					P9[0]++;
					if (P9[0] == POLY9_SIZE)
						P9[0] = 0;

					if (Bit9[P9[0]])
						Outvol[0] = audv0;
					else
						Outvol[0] = 0;
				}
				else if ( audc0 & 0x02 )
				{
					if (Outvol[0] || audc0 & 0x01 )
						Outvol[0] = 0;
					else
						Outvol[0] = audv0;
				}
				else
				/* must be poly5 */
				{
					if (Bit5[P5[0]])
						Outvol[0] = audv0;
					else
						Outvol[0] = 0;
				}
			}
			else
			/* poly4 is the only remaining option */
			{
				/* inc the poly4 counter */
				P4[0]++;
				if (P4[0] == POLY4_SIZE)
					P4[0] = 0;

				if (Bit4[P4[0]])
					Outvol[0] = audv0;
				else
					Outvol[0] = 0;
			}
		}
	}


	/* Process channel 1 */
	if (Div_n_cnt[1] > 1)
	{
		Div_n_cnt[1]--;
	}
	else if (Div_n_cnt[1] == 1)
	{
		int prev_bit5 = Bit5[P5[1]];

		Div_n_cnt[1] = Div_n_max[1];

		/* the P5 counter has multiple uses, so we inc it here */
		P5[1]++;
		if (P5[1] == POLY5_SIZE)
			P5[1] = 0;

		/* check clock modifier for clock tick */
		if ((audc1 & 0x02) == 0 ||
			((audc1 & 0x01) == 0 && Div31[P5[1]]) ||
			((audc1 & 0x01) == 1 && Bit5[P5[1]]) ||
			((audc1 & 0x0f) == POLY5_DIV3 && Bit5[P5[1]] != prev_bit5))
		{
			if (audc1 & 0x04)       /* pure modified clock selected */
			{
				if ((audc1 & 0x0f) == POLY5_DIV3)   /* POLY5 -> DIV3 mode */
				{
					if ( Bit5[P5[1]] != prev_bit5 )
					{
						Div_3_cnt[1]--;
						if ( ! Div_3_cnt[1] )
						{
							Div_3_cnt[1] = 3;
							if (Outvol[1])
								Outvol[1] = 0;
							else
								Outvol[1] = audv1;
						}
					}
				}
				else if (Outvol[1])       /* if the output was set */
					Outvol[1] = 0;   /* turn it off */
				else
					Outvol[1] = audv1;   /* else turn it on */
			}
			else if (audc1 & 0x08)  /* check for p5/p9 */
			{
				if (audc1 == POLY9) /* check for poly9 */
				{
					/* inc the poly9 counter */
					P9[1]++;
					if (P9[1] == POLY9_SIZE)
						P9[1] = 0;

					if (Bit9[P9[1]])
						Outvol[1] = audv1;
					else
						Outvol[1] = 0;
				}
				else if ( audc1 & 0x02 )
				{
					if (Outvol[1] || audc1 & 0x01 )
						Outvol[1] = 0;
					else
						Outvol[1] = audv1;
				}
				else
					/* must be poly5 */
				{
					if (Bit5[P5[1]])
						Outvol[1] = audv1;
					else
						Outvol[1] = 0;
				}
			}
			else
				/* poly4 is the only remaining option */
			{
				/* inc the poly4 counter */
				P4[1]++;
				if (P4[1] == POLY4_SIZE)
					P4[1] = 0;

				if (Bit4[P4[1]])
					Outvol[1] = audv1;
				else
					Outvol[1] = 0;
			}
		}
	}

	return Outvol[0] + Outvol[1];
}
