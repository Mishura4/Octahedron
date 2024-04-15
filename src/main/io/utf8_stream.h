#ifndef OCTAHEDRON_UTF8STREAM_H_
#define OCTAHEDRON_UTF8STREAM_H_

#include "file_stream.h"
#include "encoding.h"
#include "tools/math.h"
#include "tools/ring.h"

#include <io/raw_file_stream.h>
#include <tools/exception.h>
#include <fmt/ranges.h>

namespace octahedron
{

template <typename Stream = raw_file_stream, size_t BufSize = 4096>
class utf8_stream {
	Stream stream;
	size_t file_pos{};
	ring<char8, BufSize> buffer;
	bool at_eof = false;

	template <typename T>
	decltype(auto) _get_stream(T&& t) {
		if constexpr (std::is_pointer_v<T>) {
			return (*t);
		} else if constexpr (requires (T u) { *u; }) {
			return (*t);
		} else {
			return (t);
		}
	}

	decltype(auto) _get_stream() {
		return (_get_stream(stream));
	}

	auto _read_buf(size_t size) {
		if (buffer.size() < size)
			buffer.accept(_get_stream(stream), std::min(size, BufSize - buffer.size()));
		return (buffer.size());
	}

public:
	static constexpr size_t buffer_size = 4096;

	using underlying_stream = Stream;

	utf8_stream() requires (std::is_default_constructible_v<Stream>) = default;

	utf8_stream(const utf8_stream&) requires (std::is_copy_constructible_v<Stream>) = default;

	utf8_stream(utf8_stream&&) requires (std::is_move_constructible_v<Stream>) = default;

	utf8_stream& operator=(const utf8_stream&) requires (std::is_copy_assignable_v<Stream>) = default;

	utf8_stream& operator=(utf8_stream&&) requires (std::is_move_assignable_v<Stream>) = default;

	template <typename... Args>
	requires (std::constructible_from<Stream, Args...>)
	utf8_stream(Args&&... args) : stream(std::forward<Args>(args)...) {
	}

	size_t read_cube(char *buf, size_t size) {
		using stream_size = decltype(std::declval<utf8_stream>()._read_buf(std::declval<size_t>()));
		stream_size total = 0;

		while (total < size) {
			if (buffer.size() < size) {
				_read_buf(BufSize);
			}

			if (partial_utf8.count) {
				if (partial_utf8.needed > partial_utf8.count) {
					partial_utf8.count += partial_utf8.count + (buffer.read(partial_utf8.empty_bytes()));
				}
				auto [bytes, chr, needed] = _do_read_character(partial_utf8.available_bytes(), buf + total);
				if (needed) { // stream ended with partial utf8, we cannot proceed
					assert(partial_utf8.needed < partial_utf8.buf.size()); // bug
					return (total);
				}
				partial_utf8.clear();
				total += chr;
			}
			if (buffer.size() == 0)
				return (total);
			std::span<char8 const> data = buffer.data_first();
			auto [bytes, characters, needed] = _do_read_cube(data, buf + total, size - total);
			total += characters;
			if (needed > 0) {
				partial_utf8.count = buffer.read(partial_utf8.buf, needed);
				partial_utf8.needed = needed;
			}
			buffer.discard(bytes);
			if (total >= size)
				break;
		}
		return (total);
	}

protected:
	struct carry {
		std::array<char8, 8> buf;
		std::size_t count = 0;
		std::size_t needed = 0;

		std::span<char8> available_bytes() {
			return {buf.data(), count};
		}

		std::span<char8> empty_bytes() {
			return {buf.data() + count, count - buf.size()};
		}

		void clear() {
			count = 0;
			needed = 0;
		}
	};

	struct read_result {
		size_t bytes_read;
		size_t characters_read;
		size_t bytes_awaited;
	};

	carry partial_utf8;

	static read_result _do_read_character(std::span<char8 const> src, char *dst) {
		char8 chr = src[0];
		if (static_cast<uint8_t>(chr) < 0x80) {
			*dst = chr;
			return {1, 1, 0};
		} else {
			auto required_size = unicode_character::utf8_character_size(chr);
			if (required_size == 0) {
				throw exception{fmt::format("invalid utf8 character {:x}", static_cast<uchar>(chr))};
			}
			if (required_size > src.size()) {
				return {0, 0, required_size};
			} else {
				auto uni = unicode_character::from_utf8(src.subspan(0, required_size));

				if (!uni.code_point) {
					throw exception{fmt::format(
						"invalid utf8 code unit in ({:x})",
						fmt::join(src.subspan(0, required_size) | std::views::transform([](char8_t v) { return (static_cast<uchar>(v)); }), ",")
					)};
				}
				*dst = uni.to_cube();
				return {required_size, 1, 0};
			}
		}
	}

	static read_result _do_read_cube(std::span<char8 const> src, char* dst, size_t max) {
		size_t characters_read = 0;
		size_t bytes_read = 0;
		read_result res{};
		while (bytes_read < src.size()) {
			if (max == 0)
				break;
			res = _do_read_character(src.subspan(bytes_read), dst + characters_read);
			bytes_read += res.bytes_read + res.bytes_awaited;
			characters_read += res.characters_read;
			--max;
		}
		return {bytes_read, characters_read, res.bytes_awaited};
	}
};

utf8_stream(std::string_view) -> utf8_stream<>;

template <typename T>
utf8_stream(T&&) -> utf8_stream<T>;

}

#endif /* OCTAHEDRON_UTF8STREAM_H_ */
