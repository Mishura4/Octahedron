#include <filesystem>
#include <source_location>

#include <SDL_rwops.h>

#include "octahedron.h"

#include "../tools/math.h"

#include "file_stream.h"

using namespace octahedron;

std::string file_stream::flags_to_open_mode(bit_set<open_flags> mode) {
	std::string mode_;
	using enum open_flags;

	switch (mode.value & (INPUT | OUTPUT | APPEND | TRUNCATE)) {
		case INPUT:
			mode_ = "r";
			break;

		case INPUT | OUTPUT:
			mode_ = "r+";
			break;

		case INPUT | OUTPUT | TRUNCATE:
			mode_ = "w+";
			break;

		case INPUT | OUTPUT | APPEND:
			mode_ = "a+";
			break;

		case INPUT | OUTPUT | APPEND | TRUNCATE:
			mode_ = "a+";
			break;

		case OUTPUT:
			mode_ = "a";
			break;

		case OUTPUT | TRUNCATE:
			mode_ = "w";
			break;

		case OUTPUT | APPEND:
			mode_ = "a";
			break;

		case OUTPUT | APPEND | TRUNCATE:
			mode_ = "w";
			break;

		default:
			log(log_level::ERROR, "{}: invalid mode {}", "RawFileStream::open", mode);
			return {};
			break;
	}
	if (mode & open_flags::BINARY)
		mode_.push_back('b');
	return (mode_);
}

managed_resource<SDL_RWops*, RWopsCleaner> file_stream::toRWops() {
	auto base = seekable_stream::toRWops();

	base->hidden.unknown.data1 = this;
	base->close = [](SDL_RWops *rw) {
		auto f = static_cast<file_stream*>(rw->hidden.unknown.data1);

		f->flush();
		SDL_FreeRW(rw);
		return (0);
	};
	return (base);
}
