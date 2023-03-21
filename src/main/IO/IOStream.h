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

      using IOInterface<IOStream>::get;
      using IOInterface<IOStream>::put;

      //virtual size_t rawtell();

      //virtual size_t rawsize() { return size(); }

      template <typename T>
      requires(std::is_scalar_v<T> && !std::same_as<T, std::byte>)
      size_t read(std::span<T> values)
      {
        std::byte *data = reinterpret_cast<std::byte *>(values.data());

        return (read(std::span{data, values.size() * sizeof(T)}) / sizeof(T));
      }

      template <typename T>
      requires(std::is_scalar_v<T>)
      size_t read(T *values, size_t size)
      {
        std::byte *data = reinterpret_cast<std::byte *>(values);

        return (read(std::span{data, size * sizeof(T)}) / sizeof(T));
      }

      template <typename T>
      requires(std::is_scalar_v<T> && !std::same_as<T, std::byte>)
      size_t write(std::span<const T> values)
      {
        const std::byte *data = reinterpret_cast<const std::byte *>(values.data());

        return (write(std::span{data, values.size() * sizeof(T)}) / sizeof(T));
      }

      template <typename T>
      requires(std::is_scalar_v<T>)
      size_t write(const T *values, size_t size)
      {
        const std::byte *data = reinterpret_cast<const std::byte *>(values);

        return (write(std::span{data, size * sizeof(T)}) / sizeof(T));
      }

			virtual auto getLine(size_t max_size = (2uLL << 15)) -> std::optional<std::string>;

      virtual size_t read(std::span<std::byte> buf)        = 0;
      virtual size_t write(std::span<const std::byte> buf) = 0;

      virtual bool flush() = 0;
  };

  class SeekableStream : public IOStream
  {
    public:
      using IOStream::read;
      using IOStream::write;

      virtual ~SeekableStream() = default;

			[[nodiscard]] virtual size_t tell() const = 0;
      virtual bool   seek(ssize_t pos, int whence = SEEK_SET) = 0;

      virtual size_t size();

      auto toRWops() -> ManagedResource<SDL_RWops *, RWopsCleaner>;
	};

}  // namespace Octahedron

#endif /* OCTAHEDRON_FILESTREAM_H_ */
