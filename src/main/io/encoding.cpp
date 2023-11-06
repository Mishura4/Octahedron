#include "encoding.h"

#include <bit>

using namespace octahedron;

std::string utf8_to_cube(std::u8string_view src) {
	std::string ret;

	ret.resize(src.size());
	auto dst = std::begin(ret);
	for (auto it = src.begin(); it != src.end();) {
		auto [in, out] = utf8_char_to_cube(it, src.end(), dst);

		if (in == it) // not enough characters in string
			return (std::move(ret));
		dst = out;
	}
	if (dst != ret.end())
		ret.erase(++dst, ret.end());
	return (std::move(ret));
}
