#include "Octahedron.h"

#include "../Tools/Math.h"

#include "FileStream.h"

#include <filesystem>
#include <source_location>

using namespace Octahedron;

std::string FileStream::flagsToOpenMode(BitSet<OpenFlags> mode)
{
  std::string mode_;
  using enum OpenFlags;

  switch (mode.value & (INPUT | OUTPUT | APPEND | TRUNCATE))
  {
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
      log(LogLevel::ERROR, "{}: invalid mode {}", "RawFileStream::open", mode);
      return {};
      break;
  }
  if (mode & OpenFlags::BINARY)
    mode_.push_back('b');
  return (mode_);
}

auto RawFileStream::_open(const stdfs::path &path, BitSet<OpenFlags> mode, bool tmp)
  -> std::unique_ptr<RawFileStream>
{
  std::string mode_              = flagsToOpenMode(mode);
  bool (*after)(RawFileStream *) = nullptr;
  FileResource file;

  if (mode == OpenFlags::OUTPUT)
    after = [](RawFileStream *p) { return (p->seek(0, SEEK_END)); };
  file = fopen(path.string().c_str(), mode_.c_str());
  if (!file)
    return (nullptr);

  std::unique_ptr<RawFileStream> ret{new RawFileStream{}};

  if (tmp)
  {
    ret->_onDestroy = [=]() noexcept
    {
      std::error_code err;

      std::filesystem::remove(path, err);
    };
  }
  ret->_file = std::move(file);
  if (after && !after(ret.get()))
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

size_t RawFileStream::tell()
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

auto RawFileStream::getLine(size_t max) -> std::optional<std::string>
{
  constexpr size_t BUFFER_SIZE = 512;
  std::array<char, BUFFER_SIZE> buffer;
  std::string ret;
  size_t total;
  size_t read;

  while (max > 0)
  {
    size_t step = std::min(max, BUFFER_SIZE);
    if (!fgets(buffer.data(), step, _file.get()))
      return {std::nullopt};
    size_t read = strlen(buffer.data());
    ret.append(buffer.data(), read);
    if (read < step || buffer[read - 1] == '\n')
      return (ret);
    max -= step;
  }
  return (ret);
}
