#include "Octahedron.h"

#include "../Tools/Math.h"

#include "FileStream.h"

#include <filesystem>
#include <source_location>

#include "../../../out/build/x64-Debug/dep/SDL/include/SDL_rwops.h"

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

auto FileStream::toRWops() -> ManagedResource<SDL_RWops*, RWopsCleaner>
{
  auto base = SeekableStream::toRWops();

	base->hidden.unknown.data1 = this;
	base->close = [](SDL_RWops * rw)
	{
		auto f = static_cast<FileStream *>(rw->hidden.unknown.data1);

		f->flush();
		SDL_FreeRW(rw);
		return (0);
	};
	return (base);
}
