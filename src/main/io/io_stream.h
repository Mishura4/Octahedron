#ifndef OCTAHEDRON_IOSTREAM_H_
#define OCTAHEDRON_IOSTREAM_H_

#include <bit>
#include <concepts>
#include <span>

#include <ranges>

#include "base.h"

#include "io_interface.h"
#include "../tools/managed_resource.h"

struct SDL_RWops;

extern "C" int SDL_RWclose(SDL_RWops *context);

namespace octahedron
{

constexpr inline auto RWopsCleaner = [](SDL_RWops *p) {
	SDL_RWclose(p);
};

class io_stream : public io_interface<io_stream> {
public:
	io_stream() = default;
	io_stream(const io_stream &) = default;
	io_stream(io_stream &&) = default;

	virtual ~io_stream() = default;

	io_stream& operator=(const io_stream &) = default;
	io_stream& operator=(io_stream &&) = default;

	using io_interface::get;
	using io_interface::put;
	using io_interface::read;
	using io_interface::write;

	virtual size_t read(std::span<std::byte> buf) = 0;
	virtual size_t write(std::span<const std::byte> buf) = 0;

	virtual std::optional<std::string> get_next_line(size_t max_size = (2uLL << 15));

protected:
	friend io_interface;
};

class seekable_stream : public io_stream {
public:
	using io_stream::read;
	using io_stream::write;

	[[nodiscard]] virtual size_t tell() const = 0;
	virtual bool                 seek(ssize_t pos, int whence = SEEK_SET) = 0;

	virtual size_t size();

	virtual managed_resource<SDL_RWops*, RWopsCleaner> toRWops();
};

} // namespace octahedron

#endif /* OCTAHEDRON_FILESTREAM_H_ */
