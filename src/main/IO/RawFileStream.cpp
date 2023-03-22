#include "RawFileStream.h"

#include "../Tools/Math.h"

using namespace Octahedron;

auto RawFileStream::_open(const stdfs::path &path, BitSet<OpenFlags> mode)
  -> std::unique_ptr<RawFileStream>
{
  std::string mode_              = flagsToOpenMode(mode);
  FileResource file = ::fopen(path.string().c_str(), mode_.c_str());
  if (!file)
    return (nullptr);

  std::unique_ptr<RawFileStream> ret{new RawFileStream{}};

  if (mode & OpenFlags::TEMPORARY)
  {
    ret->_onDestroy = [=]() noexcept -> void
    {
      std::error_code err;

      std::filesystem::remove(path, err);
    	if (!err)
				log(LogLevel::WARN, "failed to remove temporary file: {} ({})", err.message(), path);
    };
  }
	ret->_file = std::move(file);
	if (mode == OpenFlags::OUTPUT && !ret->seek(0, SEEK_END))
    log(LogLevel::WARN, "could not execute post-open function for mode {}", mode);
  return (ret);
}

bool RawFileStream::flush()
{
  return (fflush(_file) != 0);
}

bool RawFileStream::eof()
{
  return (feof(_file) != 0);
}

size_t RawFileStream::read(std::span<std::byte> buf)
{
  size_t ret = fread(buf.data(), 1, buf.size(), _file);

  if (ret == buf.size())
    return (ret);
  if (ferror(_file))
    return (error_size);
  return (ret);
}

size_t RawFileStream::write(std::span<const std::byte> buf)
{
  size_t ret = fwrite(buf.data(), 1, buf.size(), _file);

  if (ret == buf.size())
    return (ret);
  if (ferror(_file))
    return (error_size);
  return (ret);
}

size_t RawFileStream::tell() const
{
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

bool RawFileStream::seek(ssize_t pos, int whence)
{
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

auto RawFileStream::getLine(size_t max_size) -> std::optional<std::string>
{
  constexpr size_t BUFFER_SIZE = 512;
  std::array<char, BUFFER_SIZE> buffer;
  std::string ret;

  while (max_size > 0)
  {
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
