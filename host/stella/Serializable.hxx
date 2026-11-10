// license:GPL-2.0+
// Minimal stand-in for Stella's Serializable.hxx (save states), so the
// unmodified AudioChannel files compile on their own. Not part of Stella.
#ifndef SERIALIZABLE_HXX
#define SERIALIZABLE_HXX

#include "bspf.hxx"

class Serializer
{
public:
	void putByte(uInt8) {}
	void putBool(bool) {}
	uInt8 getByte() { return 0; }
	bool getBool() { return false; }
};

class Serializable
{
public:
	virtual ~Serializable() = default;
	virtual bool save(Serializer &out) const = 0;
	virtual bool load(Serializer &in) = 0;
};

#endif // SERIALIZABLE_HXX
