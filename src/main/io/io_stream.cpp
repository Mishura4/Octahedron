#include "octahedron.h"

#include <limits>

#include <SDL.h>

#include "io_stream.h"

using namespace octahedron;

namespace
{
namespace RWops
{
Sint64 seek(SDL_RWops *rw, Sint64 pos, int whence) {
	auto f = static_cast<seekable_stream*>(rw->hidden.unknown.data1);
	if ((!pos && whence == SEEK_CUR) || f->seek(pos, whence))
		return static_cast<Sint64>(f->tell());
	return (-1);
}

size_t read(SDL_RWops *rw, void *buf, size_t size, size_t nmemb) {
	auto f = static_cast<seekable_stream*>(rw->hidden.unknown.data1);

	return (f->read({static_cast<std::byte*>(buf), size * nmemb}) / size);
}

size_t write(SDL_RWops *rw, const void *buf, size_t size, size_t nmemb) {
	auto f = static_cast<seekable_stream*>(rw->hidden.unknown.data1);

	return (f->write({static_cast<const std::byte*>(buf), size * nmemb}) / size);
}

int close(SDL_RWops *rw) {
	auto f = static_cast<seekable_stream*>(rw->hidden.unknown.data1);

	SDL_FreeRW(rw);
	return (0);
}
} // namespace RWops
} // namespace

managed_resource<SDL_RWops*, RWopsCleaner> seekable_stream::toRWops() {
	SDL_RWops *rw = SDL_AllocRW();

	struct toto {
		unsigned char test[5];
		unsigned char foo;
		unsigned char foobar[4];
	};

	toto test;

	get(test);

	if (!rw)
		return {nullptr};
	rw->hidden.unknown.data1 = this;
	rw->seek = RWops::seek;
	rw->read = RWops::read;
	rw->write = RWops::write;
	rw->close = RWops::close;
	return (rw);
}

size_t seekable_stream::tell() const {
	return (std::numeric_limits<size_t>::max());
}

size_t seekable_stream::size() {
	size_t pos = tell();
	size_t end;

	if (pos == std::numeric_limits<size_t>::max() || !seek(0, SEEK_END))
		return {std::numeric_limits<size_t>::max()};
	end = tell();
	if (pos != end)
		seek(pos, SEEK_SET);
	return (end);
}

std::optional<std::string> io_stream::get_next_line(size_t max_size) {
	char        c;
	std::string ret;

	for (int i = 0; i < max_size; ++i) {
		if (read({reinterpret_cast<std::byte*>(&c), 1}) != 1)
			return (ret);
		if (c == 0 || c == '\n')
			return (ret);
		ret.push_back(c);
	}
	return (ret);
}
