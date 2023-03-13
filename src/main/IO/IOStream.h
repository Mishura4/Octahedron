#ifndef OCTAHEDRON_IOSTREAM_H_
#define OCTAHEDRON_IOSTREAM_H_

#include <bit>
#include <concepts>
#include <span>

#include <ranges>

#include "Base.h"

#include "../Tools/ManagedResource.h"
#include "IOInterface.h"

struct SDL_RWops;

extern "C" int SDL_RWclose(SDL_RWops *context);

namespace Octahedron
{
  constexpr inline auto RWopsCleaner = [](SDL_RWops *p) { SDL_RWclose(p); };

  class IOStream : public IOInterface<IOStream>
  {
    public:
      virtual ~IOStream() = default;

      //virtual size_t rawtell();

      //virtual size_t rawsize() { return size(); }

      virtual size_t read(std::span<std::byte> buf)        = 0;
      virtual size_t write(std::span<const std::byte> buf) = 0;

      virtual bool flush() = 0;

      size_t read(std::byte *buf, size_t size);
      size_t write(const std::byte *buf, size_t size);
  };

  class SeekableStream : public IOStream
  {
    public:
      using IOStream::read;
      using IOStream::write;

      virtual ~SeekableStream() = default;

      virtual size_t tell() = 0;
      virtual bool seek(ssize_t pos, int whence = SEEK_SET) = 0;

      virtual size_t size();

      virtual auto getLine(size_t max_size = std::string::npos) -> std::optional<std::string>;

      auto toRWops() -> ManagedResource<SDL_RWops *, RWopsCleaner>;
  };

}  // namespace Octahedron

#endif /* OCTAHEDRON_FILESTREAM_H_ */
