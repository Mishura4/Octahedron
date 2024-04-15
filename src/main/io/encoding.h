#ifndef OCTAHEDRON_ENCODING_H_
#define OCTAHEDRON_ENCODING_H_

#include "../base.h"

#include <array>
#include <ranges>

#include <tools/util.h>

namespace octahedron
{

using uchar = unsigned char;

using char8 = char8_t;
using char16 = char16_t;
using char32 = char32_t;

namespace _
{

// Taken from Tesseract's stream.cpp
inline constexpr std::array<char32_t, 256> CUBE_TO_UNICODE_CHARS =
{
	0, 192, 193, 194, 195, 196, 197, 198, 199, 9, 10, 11, 12, 13, 200, 201, 202, 203, 204, 205, 206,
	207, 209, 210, 211, 212, 213, 214, 216, 217, 218, 219, 32, 33, 34, 35, 36, 37, 38, 39, 40, 41, 42,
	43, 44, 45, 46, 47, 48, 49, 50, 51, 52, 53, 54, 55, 56, 57, 58, 59, 60, 61, 62, 63, 64, 65, 66,
	67, 68, 69, 70, 71, 72, 73, 74, 75, 76, 77, 78, 79, 80, 81, 82, 83, 84, 85, 86, 87, 88, 89, 90,
	91, 92, 93, 94, 95, 96, 97, 98, 99, 100, 101, 102, 103, 104, 105, 106, 107, 108, 109, 110, 111,
	112, 113, 114, 115, 116, 117, 118, 119, 120, 121, 122, 123, 124, 125, 126, 220, 221, 223, 224,
	225, 226, 227, 228, 229, 230, 231, 232, 233, 234, 235, 236, 237, 238, 239, 241, 242, 243, 244,
	245, 246, 248, 249, 250, 251, 252, 253, 255, 0x104, 0x105, 0x106, 0x107, 0x10C, 0x10D, 0x10E,
	0x10F, 0x118, 0x119, 0x11A, 0x11B, 0x11E, 0x11F, 0x130, 0x131, 0x141, 0x142, 0x143, 0x144, 0x147,
	0x148, 0x150, 0x151, 0x152, 0x153, 0x158, 0x159, 0x15A, 0x15B, 0x15E, 0x15F, 0x160, 0x161, 0x164,
	0x165, 0x16E, 0x16F, 0x170, 0x171, 0x178, 0x179, 0x17A, 0x17B, 0x17C, 0x17D, 0x17E, 0x404, 0x411,
	0x413, 0x414, 0x416, 0x417, 0x418, 0x419, 0x41B, 0x41F, 0x423, 0x424, 0x426, 0x427, 0x428, 0x429,
	0x42A, 0x42B, 0x42C, 0x42D, 0x42E, 0x42F, 0x431, 0x432, 0x433, 0x434, 0x436, 0x437, 0x438, 0x439,
	0x43A, 0x43B, 0x43C, 0x43D, 0x43F, 0x442, 0x444, 0x446, 0x447, 0x448, 0x449, 0x44A, 0x44B, 0x44C,
	0x44D, 0x44E, 0x44F, 0x454, 0x490, 0x491
};

// Taken from Tesseract's stream.cpp
inline constexpr std::array<uchar, 878> UNICODE_TO_CUBE_CHARS =
{
	0, 0, 0, 0, 0, 0, 0, 0, 0, 9, 10, 11, 12, 13, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
	0, 32, 33, 34, 35, 36, 37, 38, 39, 40, 41, 42, 43, 44, 45, 46, 47, 48, 49, 50, 51, 52, 53, 54, 55,
	56, 57, 58, 59, 60, 61, 62, 63, 64, 65, 66, 67, 68, 69, 70, 71, 72, 73, 74, 75, 76, 77, 78, 79,
	80, 81, 82, 83, 84, 85, 86, 87, 88, 89, 90, 91, 92, 93, 94, 95, 96, 97, 98, 99, 100, 101, 102,
	103, 104, 105, 106, 107, 108, 109, 110, 111, 112, 113, 114, 115, 116, 117, 118, 119, 120, 121,
	122, 123, 124, 125, 126, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
	0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
	0, 0, 0, 0, 0, 0, 0, 0, 1, 2, 3, 4, 5, 6, 7, 8, 14, 15, 16, 17, 18, 19, 20, 21, 0, 22, 23, 24, 25,
	26, 27, 0, 28, 29, 30, 31, 127, 128, 0, 129, 130, 131, 132, 133, 134, 135, 136, 137, 138, 139,
	140, 141, 142, 143, 144, 145, 0, 146, 147, 148, 149, 150, 151, 0, 152, 153, 154, 155, 156, 157, 0,
	158, 0, 0, 0, 0, 159, 160, 161, 162, 0, 0, 0, 0, 163, 164, 165, 166, 0, 0, 0, 0, 0, 0, 0, 0, 167,
	168, 169, 170, 0, 0, 171, 172, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 173, 174, 0, 0, 0,
	0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 175, 176, 177, 178, 0, 0, 179, 180, 0, 0, 0, 0, 0, 0, 0, 181,
	182, 183, 184, 0, 0, 0, 0, 185, 186, 187, 188, 0, 0, 189, 190, 191, 192, 0, 0, 193, 194, 0, 0, 0,
	0, 0, 0, 0, 0, 195, 196, 197, 198, 0, 0, 0, 0, 0, 0, 199, 200, 201, 202, 203, 204, 205, 0, 0, 0,
	0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
	0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
	0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
	0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 17, 0, 0, 206,
	83, 73, 21, 74, 0, 0, 0, 0, 0, 0, 0, 65, 207, 66, 208, 209, 69, 210, 211, 212, 213, 75, 214, 77,
	72, 79, 215, 80, 67, 84, 216, 217, 88, 218, 219, 220, 221, 222, 223, 224, 225, 226, 227, 97, 228,
	229, 230, 231, 101, 232, 233, 234, 235, 236, 237, 238, 239, 111, 240, 112, 99, 241, 121, 242, 120,
	243, 244, 245, 246, 247, 248, 249, 250, 251, 252, 0, 141, 0, 0, 253, 115, 105, 145, 106, 0, 0, 0,
	0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
	0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 254, 255, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
	0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
	0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
	0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
	0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
	0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
	0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
	0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0
};

// Taken from Tesseract's stream.cpp
inline constexpr std::array UNICODE_TO_CUBE_OFFSETS = std::to_array<int>(
	{
		0, 256, 658, 658, 512, 658, 658, 658
	}
);
}

inline constexpr auto cube_to_unicode = []<character T>(T c) constexpr noexcept -> char32 {
	return (_::CUBE_TO_UNICODE_CHARS[static_cast<uchar>(c & 0xFF)]);
};

inline constexpr auto unicode_to_cube = [](char32_t c) constexpr -> char {
	using namespace _;

	// Taken from Tesseract's stream.cpp
	if (c <= 0x7FF)
		return (UNICODE_TO_CUBE_CHARS[UNICODE_TO_CUBE_OFFSETS[c >> 8] + (c & 0xFF)]);
	return (0);
};

[[msvc::intrinsic]] inline char32_t utf32(std::uint_least32_t codepoint) {
	return (static_cast<char32_t>(codepoint));
}

constexpr char32_t utf8_to_32(uint32_t a) {
	return (a);
}

constexpr char32_t utf8_to_32(uint32_t a, uint32_t b) {
	return
		((a & 0b00011111u) << 6) |
		((b & 0b00111111u) << 0);
}

constexpr char32_t utf8_to_32(uint32_t a, uint32_t b, uint32_t c) {
	return
		((a & 0b00001111u) << 12) |
		((b & 0b00111111u) << 6) |
		((c & 0b00111111u) << 0);
}

constexpr char32_t utf8_to_32(uint32_t a, uint32_t b, uint32_t c, uint32_t d) {
	return
		((a & 0b00001111u) << 18) |
		((b & 0b00111111u) << 12) |
		((c & 0b00111111u) << 6) |
		((d & 0b00111111u) << 0);
}

constexpr char32_t utf8_to_32(uint32_t a, uint32_t b, uint32_t c, uint32_t d, uint32_t e) {
	return
		((a & 0b00000111u) << 24) |
		((b & 0b00111111u) << 18) |
		((c & 0b00111111u) << 12) |
		((d & 0b00111111u) << 6) |
		((e & 0b00111111u) << 0);
}

constexpr char32_t utf8_to_32(uint32_t a, uint32_t b, uint32_t c, uint32_t d, uint32_t e, uint32_t f) {
	return
		((a & 0b00000011u) << 30) |
		((b & 0b00111111u) << 24) |
		((c & 0b00111111u) << 18) |
		((d & 0b00111111u) << 12) |
		((e & 0b00111111u) << 6) |
		((f & 0b00111111u) << 0);
}

constexpr char8_t foo[] = u8"é";

constexpr auto bar0 = foo[0];
constexpr auto bar1 = foo[1];

constexpr auto bar = utf8_to_32(foo[0], foo[1]);

struct unicode_character {
	char32_t code_point;

	constexpr size_t utf8_size() const noexcept {
		return (
			code_point < 0x000080 ? 1 :
			code_point < 0x000800 ? 2 :
			code_point < 0x100000 ? 3 :
			code_point < 0x110000 ? 4 :
			0
		);
	}

	template <typename Range>
	requires (std::ranges::sized_range<Range>)
	constexpr size_t write_utf8(Range&& range) {
		using range_t = std::remove_reference_t<Range>;
		using value_t = std::ranges::range_value_t<range_t>;
		size_t required_size = utf8_size();
		if (required_size < std::ranges::size(range)) {
			return (0);
		}
		auto it = std::ranges::begin(range);
		switch (required_size) {
			default:
			case 0:
				return (0);

			case 1:
				*it = static_cast<value_t>(code_point);
				return (1);

			case 2:
				*it = static_cast<value_t>((code_point & 0b00011111) | 0b11000000);
				++it;
				*it = static_cast<value_t>((code_point & 0b00111111) | 0b10000000);
				return (1);

			case 3:
				*it = static_cast<value_t>((code_point & 0b00001111) | 0b11100000);
				++it;
				*it = static_cast<value_t>((code_point & 0b00111111) | 0b10000000);
				++it;
				*it = static_cast<value_t>((code_point & 0b00111111) | 0b10000000);
				return (1);

			case 4:
				*it = static_cast<value_t>((code_point & 0b00000111) | 0b11110000);
				++it;
				*it = static_cast<value_t>((code_point & 0b00111111) | 0b10000000);
				++it;
				*it = static_cast<value_t>((code_point & 0b00111111) | 0b10000000);
				++it;
				*it = static_cast<value_t>((code_point & 0b00111111) | 0b10000000);
				return (1);
		}
	}

	constexpr char to_cube() const noexcept {
		return (unicode_to_cube(code_point));
	}

	static constexpr size_t utf8_character_size(char8 first_byte) noexcept {
		switch (std::countl_one(static_cast<uint8>(first_byte))) {
			case 0:
				return (1);
			case 2:
				return (2);
			case 3:
				return (3);
			case 4:
				return (4);
			default:
				return (0);
		}
	}

	static constexpr unicode_character from_utf8(std::span<char8 const> buf) noexcept {
		int bits = std::countl_one(static_cast<uint8>(buf[0]));

		switch (bits) {
			case 0:
				return {buf[0]};

			case 2: // 110xxxxx 10xxxxxx
				return { buf.size() < 2 ? U'\0' : utf8_to_32(buf[0], buf[1]) };

			case 3: // 1110xxxx 10xxxxxx 10xxxxxx
				return { buf.size() < 3 ? U'\0' : utf8_to_32(buf[0], buf[1], buf[2]) };

			case 4: // 11110xxx 10xxxxxx 10xxxxxx 10xxxxxx
				return { buf.size() < 4 ? U'\0' : utf8_to_32(buf[0], buf[1], buf[2], buf[4]) };

			default:
				return {};
		}
	}
};

template <std::input_iterator InIt, typename OutIt> requires (
	(std::output_iterator<OutIt, char> || std::output_iterator<OutIt, uchar>)
)
constexpr std::ranges::in_out_result<InIt, OutIt> utf8_char_to_cube(
	InIt first, InIt last, OutIt out
) {
	auto  it = first;
	uchar c = *it;

	++it;
	if (!(c & 0b10000000)) // 1 byte
	{
		*out = unicode_to_cube(c);
		return {it, ++out};
	}

	auto   bytes = std::countl_one(static_cast<unsigned char>(c)) - 1;
	char32 uni;

	if (bytes >= 7) // invalid utf8
		return {it, out};
	if (std::distance(it, last) < bytes)
		return {first, out};
	uni = (c & (0xFF >> (bytes + 1)));

	while (bytes > 0) {
		c = *it;
		if (!((c & 0b11000000) == 0b10000000)) // invalid
		{
			uni = 0;
			return {it, out};
		}
		uni = (uni << 6) | (c & 0x3F);
		++it;
		--bytes;
	}
	uni = unicode_to_cube(uni);
	if (!uni)
		return {it, out};
	*out = static_cast<uchar>(uni);
	return {it, ++out};
}

template <typename In, typename OutIt> requires (
	std::ranges::viewable_range<std::remove_cvref_t<In>> &&
	std::same_as<std::ranges::range_value_t<In>, char8> &&
	(std::output_iterator<OutIt, char> || std::output_iterator<OutIt, uchar>)
)
constexpr auto utf8_char_to_cube(In &&src, OutIt dst) {
	return (utf8_char_to_cube(std::begin(src), std::end(src), dst));
}

std::u8string cube_to_utf8(std::string_view src);

std::string utf8_to_cube(std::u8string_view src);

}

#endif
