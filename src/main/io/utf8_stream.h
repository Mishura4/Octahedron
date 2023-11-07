#ifndef OCTAHEDRON_UTF8STREAM_H_
#define OCTAHEDRON_UTF8STREAM_H_

#include "file_stream.h"
#include "encoding.h"
#include "tools/math.h"
#include "tools/ring.h"

namespace octahedron
{

template <typename Stream, size_t BufSize = 4096>
class utf8_stream {
public:
	static constexpr size_t buffer_size = 4096;

	using underlying_stream = Stream;

	utf8_stream() requires (std::is_default_constructible_v<Stream>) = default;

	utf8_stream(const utf8_stream&) requires (std::is_copy_constructible_v<Stream>) = default;

	utf8_stream(utf8_stream&&) requires (std::is_move_constructible_v<Stream>) = default;

	utf8_stream& operator=(const utf8_stream&) requires (std::is_copy_assignable_v<Stream>) = default;

	utf8_stream& operator=(utf8_stream&&) requires (std::is_move_assignable_v<Stream>) = default;

	size_t read(std::span<char> buf) {
		
	}

protected:
	Stream stream;
	size_t file_pos{};
	ring<char8, BufSize> buffer;
};

}

#endif /* OCTAHEDRON_UTF8STREAM_H_ */
