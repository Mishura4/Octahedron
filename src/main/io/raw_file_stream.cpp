#include "raw_file_stream.h"

#include "../tools/math.h"

#include <fmt/std.h>

using namespace octahedron;

std::unique_ptr<raw_file_stream> raw_file_stream::_open(
	const stdfs::path & path,
	bit_set<open_flags> mode
) {
	std::string  mode_ = flags_to_open_mode(mode);
	FileResource file = ::fopen(path.string().c_str(), mode_.c_str());
	if (!file)
		return (nullptr);

	std::unique_ptr<raw_file_stream> ret{new raw_file_stream{}};

	if (mode & open_flags::TEMPORARY) {
		ret->_onDestroy = [=]() noexcept -> void {
			std::error_code err;

			std::filesystem::remove(path, err);
			if (!err)
				log(log_level::WARN, "failed to remove temporary file: {} ({})", err.message(), path);
		};
	}
	ret->_file = std::move(file);
	if (mode == open_flags::OUTPUT && !ret->seek(0, SEEK_END))
		log(log_level::WARN, "could not execute post-open function for mode {}", mode);
	return (ret);
}

bool raw_file_stream::flush() {
	return (fflush(_file) != 0);
}

bool raw_file_stream::eof() {
	return (feof(_file) != 0);
}

size_t raw_file_stream::read(std::span<std::byte> buf) {
	size_t ret = fread(buf.data(), 1, buf.size(), _file);

	if (ret == buf.size())
		return (ret);
	if (ferror(_file))
		return (error_size);
	return (ret);
}

size_t raw_file_stream::write(std::span<const std::byte> buf) {
	size_t ret = fwrite(buf.data(), 1, buf.size(), _file);

	if (ret == buf.size())
		return (ret);
	if (ferror(_file))
		return (error_size);
	return (ret);
}

size_t raw_file_stream::tell() const {
	#ifdef WIN32
	#  if defined(__GNUC__) && !defined(__MINGW32__)
  ssize_t off = ftello64(file);
	#  else
	ssize_t off = _ftelli64(_file);
	#  endif
	#else
  ssize_t off = ftello(file);
	#endif
	// ftello returns LONG_MAX for directories on some platforms
	return (static_cast<size_t>(off));
}

bool raw_file_stream::seek(ssize_t pos, int whence) {
	#ifdef WIN32
	#  if defined(__GNUC__) && !defined(__MINGW32__)
  return fseeko64(_file, pos, whence) >= 0;
	#  else
	return _fseeki64(_file, pos, whence) >= 0;
	#  endif
	#else
  return fseeko(_file, pos, whence) >= 0;
	#endif
}

std::optional<std::string> raw_file_stream::get_next_line(size_t max_size) {
	constexpr size_t              BUFFER_SIZE = 512;
	std::array<char, BUFFER_SIZE> buffer;
	std::string                   ret;

	while (max_size > 0) {
		const int step = narrow_cast<int>(min(max_size, BUFFER_SIZE));
		if (!fgets(buffer.data(), step, _file.get()))
			return {std::nullopt};
		const int read = narrow_cast<int>(strlen(buffer.data()));
		ret.append(buffer.data(), read);
		if (read < step || buffer[read - 1] == '\n')
			return (ret);
		max_size -= step;
	}
	return (ret);
}
