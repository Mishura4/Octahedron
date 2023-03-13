#include "Octahedron.h"

#include <limits>

#include <SDL.h>

#include "IOStream.h"

using namespace Octahedron;

namespace
{
  namespace RWops
  {
    Sint64 seek(SDL_RWops *rw, Sint64 pos, int whence)
    {
      SeekableStream *f = static_cast<SeekableStream *>(rw->hidden.unknown.data1);
      if ((!pos && whence == SEEK_CUR) || f->seek(pos, whence))
        return static_cast<Sint64>(f->tell());
      return (-1);
    }

    size_t read(SDL_RWops *rw, void *buf, size_t size, size_t nmemb)
    {
      SeekableStream *f = static_cast<SeekableStream *>(rw->hidden.unknown.data1);

      return (f->read({static_cast<std::byte *>(buf), size * nmemb}) / size);
    }

    size_t write(SDL_RWops *rw, const void *buf, size_t size, size_t nmemb)
    {
      SeekableStream *f = static_cast<SeekableStream *>(rw->hidden.unknown.data1);

      return (f->write({static_cast<const std::byte *>(buf), size * nmemb}) / size);
    }

    int close(SDL_RWops *rw)
    {
      SeekableStream *f = static_cast<SeekableStream *>(rw->hidden.unknown.data1);

      f->flush();
      SDL_FreeRW(rw);
      return (0);
    }
  }  // namespace RWops
}  // namespace

auto SeekableStream::toRWops() -> ManagedResource<SDL_RWops *, RWopsCleaner>
{
  SDL_RWops *rw = SDL_AllocRW();

  if (!rw)
    return {nullptr};
  rw->hidden.unknown.data1 = this;
  rw->seek                 = RWops::seek;
  rw->read                 = RWops::read;
  rw->write                = RWops::write;
  rw->close                = RWops::close;
  return (rw);
}

size_t IOStream::read(std::byte* buf, size_t size)
{
  return (read({buf, size}));
}

size_t IOStream::write(const std::byte *buf, size_t size)
{
  return (write({buf, size}));
}

size_t SeekableStream::tell()
{
  return {std::numeric_limits<size_t>::max()};
}

size_t SeekableStream::size()
{
  size_t pos = tell();
  size_t end;

  if (pos == std::numeric_limits<size_t>::max() || !seek(0, SEEK_END))
    return {std::numeric_limits<size_t>::max()};
  end = tell();
  if (pos != end)
    seek(pos, SEEK_SET);
  return (end);
}
