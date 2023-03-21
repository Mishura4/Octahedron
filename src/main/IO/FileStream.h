#ifndef OCTAHEDRON_FILESTREAM_H_
#define OCTAHEDRON_FILESTREAM_H_

#include "Base.h"

#include "IOStream.h"

#include <functional>

#include "FileSystem.h"

namespace Octahedron
{

  class FileSystem;

  class FileStream : public SeekableStream
  {
    public:
      using IOStream::read;
      using IOStream::write;

			~FileStream() override = default;

			bool flush() override = 0;

      virtual bool eof()   = 0;

      virtual uint32 crc32() { return 0; }

      static std::string flagsToOpenMode(BitSet<OpenFlags> mode);
  };
}  // namespace Octahedron

#endif