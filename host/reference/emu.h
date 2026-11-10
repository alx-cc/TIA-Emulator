// license:GPL-2.0+
// Minimal stand-in for the MAME framework pieces used by tiasound.cpp, so the
// unmodified file compiles outside MAME. Not part of MAME.
#ifndef REFERENCE_EMU_H
#define REFERENCE_EMU_H

#include <cstdint>

using offs_t = uint32_t;

#define NAME(x) x, #x

class device_t
{
public:
	template <typename T>
	void save_item(T &, const char *) {}
};

// Collects the raw value passed to put_int(), unscaled, one per sample.
class sound_stream
{
public:
	sound_stream(int16_t *out, int count) : m_out(out), m_count(count) {}

	int samples() const { return m_count; }
	void put_int(int, int index, int32_t value, int32_t) { m_out[index] = int16_t(value); }

private:
	int16_t *m_out;
	int m_count;
};

#endif // REFERENCE_EMU_H
