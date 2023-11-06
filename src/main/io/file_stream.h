#ifndef OCTAHEDRON_FILESTREAM_H_
#define OCTAHEDRON_FILESTREAM_H_

#include <functional>

#include "base.h"
#include "file_system.h"
#include "io_stream.h"

namespace octahedron
{

class file_system;

class file_stream : public seekable_stream {
public:
	using io_stream::read;
	using io_stream::write;

	~file_stream() override = default;

	virtual bool flush() = 0;

	virtual bool eof() = 0;

	virtual uint32 crc32() {
		return 0;
	}

	static std::string flags_to_open_mode(bit_set<open_flags> mode);

	managed_resource<SDL_RWops*, RWopsCleaner> toRWops() override;
};

} // namespace octahedron

#endif
