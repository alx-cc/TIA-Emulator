// license:GPL-2.0+
// copyright-holders:Ron Fries,Dan Boris
/*****************************************************************************/
/*                                                                           */
/* Module:  TIA Chip Sound Simulator Includes, V1.1                          */
/* Purpose: Define global function prototypes and structures for the TIA     */
/*          Chip Sound Simulator.                                            */
/* Author:  Ron Fries                                                        */
/*                                                                           */
/* Revision History:                                                         */
/*    10-Sep-96 - V1.0 - Initial Release                                     */
/*    14-Jan-97 - V1.1 - Added compiler directives to facilitate compilation */
/*                       on a C++ compiler.                                  */
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
/* Adapted from MAME src/devices/sound/tiasound.h for standalone use, 2026.  */
/* MAME commit ca22ea616c031b9afeb5dad5533d419278c90c43. MAME framework code */
/* removed; the interface is now the tia_sound class below.                  */
/*                                                                           */
/*****************************************************************************/
#ifndef TIASOUND_H
#define TIASOUND_H

#pragma once

#include <cstdint>

/* TIA sound register addresses */
enum : uint8_t
{
	AUDC0 = 0x15,
	AUDC1 = 0x16,
	AUDF0 = 0x17,
	AUDF1 = 0x18,
	AUDV0 = 0x19,
	AUDV1 = 0x1a
};

/* the size (in entries) of the 4 polynomial tables */
#define POLY4_SIZE  0x000f
#define POLY5_SIZE  0x001f
#define POLY9_SIZE  0x01ff

/* Both TIA voices. Call write() to set a register and step() once per TIA  */
/* audio clock tick (~31.4 kHz NTSC); step() returns the sum of both voices, */
/* 0 to 30720 (each voice is AUDV << 10).                                    */
class tia_sound
{
public:
	tia_sound();

	void write(uint8_t offset, uint8_t data);
	int16_t step();

private:
	// structures to hold the 6 tia sound control bytes
	uint8_t AUDC[2] = { 0, 0 };        // AUDCx (15, 16)
	uint8_t AUDF[2] = { 0, 0 };        // AUDFx (17, 18)
	int16_t AUDV[2] = { 0, 0 };        // AUDVx (19, 1A)

	int16_t Outvol[2] = { 0, 0 };      // last output volume for each channel

	uint8_t Bit4[POLY4_SIZE];
	uint8_t Bit5[POLY5_SIZE];
	uint8_t Bit9[POLY9_SIZE];

	uint8_t P4[2] = { 0, 0 };           // Position pointer for the 4-bit POLY array
	uint8_t P5[2] = { 0, 0 };           // Position pointer for the 5-bit POLY array
	uint16_t P9[2] = { 0, 0 };          // Position pointer for the 9-bit POLY array

	uint8_t Div_n_cnt[2] = { 0, 0 };    // Divide by n counter. one for each channel
	uint8_t Div_n_max[2] = { 0, 0 };    // Divide by n maximum, one for each channel
	uint8_t Div_3_cnt[2] = { 3, 3 };    // Div 3 counter, used for POLY5_DIV3 mode

	template <unsigned N>
	static void poly_init(uint8_t (&poly)[N], int size, int f0, int f1);
};

#endif // TIASOUND_H
