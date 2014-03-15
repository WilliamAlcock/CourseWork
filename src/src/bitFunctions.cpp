#include <stdexcept>

union charToInt {
	char c[4];
	int i;
};

// returns a bit in a char
inline bool getBit(char c, int bit) {
	if (bit>7) {
		throw std::runtime_error("cannot get bit higher than 7");
	} else if (bit<0) {
		throw std::runtime_error("cannot get negative bit");
	} else {
		return (c & (1 << bit));
	}
}

// flips a bit in a char
inline char flipBit(char c, int bit) {
	if (bit>7) {
		throw std::runtime_error("cannot flip bit higher than 7");
	} else if (bit<0) {
		throw std::runtime_error("cannot flip negative bit");
	}
	if (getBit(c, bit)) {
		return c - (1 << bit);
	} else {
		return c + (1 << bit);
	}
}

inline bool isPower2(long x) {
	return ( (x > 0) && ((x & (x - 1)) == 0) );
}
