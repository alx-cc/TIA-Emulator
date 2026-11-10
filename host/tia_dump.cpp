// license:GPL-2.0+
// Runs a register-write script through the TIA core and writes one sample per
// tick. Built three times: tia_dump uses core/, tia_dump_ref uses the
// unmodified MAME file in host/reference/ (TIA_REFERENCE defined), and
// tia_dump_stella uses Stella's audio channels in host/stella/ (TIA_STELLA
// defined, output scaled to match: volume << 10 per voice).
//
// Usage: tia_dump <script> <out.raw> [out.wav]
//
// Script: one item per line, '#' starts a comment.
//   <tick> <reg> <value>   write before computing sample <tick>; reg is a
//                          name (AUDC0, AUDF1, ...) or a number (0x15)
//   <tick> end             stop; output has <tick> samples
// Ticks must not decrease.
//
// out.raw: int16 little-endian, the core's output (0 to 30720).
// out.wav: 16-bit mono at 31400 Hz, centred around zero, for listening.

#include <cstdint>
#include <cstdio>
#include <fstream>
#include <iostream>
#include <sstream>
#include <string>
#include <vector>

#if defined(TIA_REFERENCE)
#include "emu.h"
#include "tiasound.h"
#elif defined(TIA_STELLA)
#include "AudioChannel.hxx"
#else
#include "tiasound.h"
#endif

namespace {

constexpr int TIA_RATE = 31400;

#if defined(TIA_REFERENCE)
// Sample rate equal to the clock gives one output per tick, like step().
class chip
{
public:
	chip() : m_chip(tia_sound_init(&m_device, TIA_RATE, TIA_RATE, 0)) {}
	~chip() { tia_sound_free(m_chip); }

	void write(uint8_t reg, uint8_t value) { tia_write(m_chip, reg, value); }
	void run(int16_t *out, int count)
	{
		sound_stream stream(out, count);
		tia_process(m_chip, stream);
	}

private:
	device_t m_device;
	void *m_chip;
};
#elif defined(TIA_STELLA)
// One audio tick is phase0 then phase1 on each channel, as in Stella's TIA.
class chip
{
public:
	chip()
	{
		m_channel[0].reset();
		m_channel[1].reset();
	}

	void write(uint8_t reg, uint8_t value)
	{
		switch (reg)
		{
		case 0x15: m_channel[0].audc(value); break;
		case 0x16: m_channel[1].audc(value); break;
		case 0x17: m_channel[0].audf(value); break;
		case 0x18: m_channel[1].audf(value); break;
		case 0x19: m_channel[0].audv(value); break;
		case 0x1a: m_channel[1].audv(value); break;
		default: break;
		}
	}

	void run(int16_t *out, int count)
	{
		for (int i = 0; i < count; i++)
		{
			for (auto &c : m_channel)
				c.phase0();
			for (auto &c : m_channel)
				c.phase1();
			out[i] = int16_t((m_channel[0].actualVolume() + m_channel[1].actualVolume()) << 10);
		}
	}

private:
	AudioChannel m_channel[2];
};
#else
class chip
{
public:
	void write(uint8_t reg, uint8_t value) { m_tia.write(reg, value); }
	void run(int16_t *out, int count)
	{
		for (int i = 0; i < count; i++)
			out[i] = m_tia.step();
	}

private:
	tia_sound m_tia;
};
#endif

bool parse_reg(const std::string &s, uint8_t &reg)
{
	static const struct { const char *name; uint8_t reg; } names[] = {
		{ "AUDC0", 0x15 }, { "AUDC1", 0x16 }, { "AUDF0", 0x17 },
		{ "AUDF1", 0x18 }, { "AUDV0", 0x19 }, { "AUDV1", 0x1a },
	};
	for (const auto &n : names)
		if (s == n.name) { reg = n.reg; return true; }
	try
	{
		reg = uint8_t(std::stoul(s, nullptr, 0));
		return true;
	}
	catch (...)
	{
		return false;
	}
}

void put_u16(std::ostream &os, uint16_t v) { os.put(char(v & 0xff)); os.put(char(v >> 8)); }
void put_u32(std::ostream &os, uint32_t v) { put_u16(os, uint16_t(v)); put_u16(os, uint16_t(v >> 16)); }

void write_wav(const char *path, const std::vector<int16_t> &samples)
{
	std::ofstream f(path, std::ios::binary);
	const uint32_t bytes = uint32_t(samples.size() * 2);
	f.write("RIFF", 4); put_u32(f, 36 + bytes); f.write("WAVE", 4);
	f.write("fmt ", 4); put_u32(f, 16); put_u16(f, 1); put_u16(f, 1);
	put_u32(f, TIA_RATE); put_u32(f, TIA_RATE * 2); put_u16(f, 2); put_u16(f, 16);
	f.write("data", 4); put_u32(f, bytes);
	for (int16_t s : samples)
		put_u16(f, uint16_t(int16_t(s - 15360)));
}

} // anonymous namespace

int main(int argc, char **argv)
{
	if (argc < 3)
	{
		std::cerr << "usage: " << argv[0] << " <script> <out.raw> [out.wav]\n";
		return 2;
	}

	std::ifstream script(argv[1]);
	if (!script)
	{
		std::cerr << "cannot open " << argv[1] << "\n";
		return 2;
	}

	chip tia;
	std::vector<int16_t> samples;
	long tick = 0;
	bool ended = false;
	std::string line;
	for (int lineno = 1; std::getline(script, line); lineno++)
	{
		line = line.substr(0, line.find('#'));
		std::istringstream in(line);
		long at;
		std::string reg_s, value_s;
		if (!(in >> at))
			continue;
		if (at < tick || !(in >> reg_s))
		{
			std::cerr << argv[1] << ":" << lineno << ": bad line\n";
			return 2;
		}

		samples.resize(at);
		tia.run(samples.data() + tick, int(at - tick));
		tick = at;

		if (reg_s == "end")
		{
			ended = true;
			break;
		}

		uint8_t reg;
		if (!parse_reg(reg_s, reg) || !(in >> value_s))
		{
			std::cerr << argv[1] << ":" << lineno << ": bad register or value\n";
			return 2;
		}
		tia.write(reg, uint8_t(std::stoul(value_s, nullptr, 0)));
	}
	if (!ended)
	{
		std::cerr << argv[1] << ": missing '<tick> end'\n";
		return 2;
	}

	std::ofstream raw(argv[2], std::ios::binary);
	for (int16_t s : samples)
		put_u16(raw, uint16_t(s));
	if (argc > 3)
		write_wav(argv[3], samples);
	return 0;
}
