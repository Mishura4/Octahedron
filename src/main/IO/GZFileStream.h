#ifndef OCTAHEDRON_GZFILESTREAM_H_
#define OCTAHEDRON_GZFILESTREAM_H_

#include <zlib.h>

#include "../Base.h"
#include "FileStream.h"
#include "RawFileStream.h"

namespace Octahedron
{
	class GZFileStream : public FileStream
	{
	public:
		~GZFileStream() override;
		size_t               read(std::span<std::byte> buf) override;
		size_t               write(std::span<const std::byte> buf) override;
		[[nodiscard]] size_t tell() const override;
		bool                 seek(ssize_t pos, int whence) override;
		bool                 flush() override;
		bool                 eof() override;
		uint32               crc32() const noexcept;

		[[nodiscard]] size_t tellRaw() const;
		[[nodiscard]] size_t sizeRaw() const;

		bool flush(bool full);

	private:
		friend auto FileSystem::openGZ(std::string_view path, BitSet<OpenFlags> mode)
			-> std::unique_ptr<FileStream>;

		class _GZipFlags
		{
		public:
				enum Values
				{
					ASCII		 = 0x01,
					CRC			 = 0x02,
					EXTRA		 = 0x04,
					NAME		 = 0x08,
					COMMENT	 = 0x10,
					RESERVED = 0xE0
				};
		};

		// Z_DEFLATED technically not part of the magic but, it is expected to be there anyways
		constexpr static auto HEADER_MAGIC	= byte_array({0x1F, 0x8B, Z_DEFLATED});

		using GZipFlags = _GZipFlags::Values;

		static constexpr size_t BUFFER_SIZE = 16384;
		using buffer												= std::array<Bytef, BUFFER_SIZE>;


		static auto _fromRawStream(
			std::unique_ptr<FileStream>&& stream,
			BitSet<OpenFlags>             mode,
			int                           compression = 9
		) -> std::unique_ptr<GZFileStream>;

		void   _atEof();
		int    _endWrite();
		int    _endRead();
		size_t _fetch(size_t size = BUFFER_SIZE);
		size_t _readRaw(std::span<std::byte> buf);
		void   _skipRaw(size_t size);
		bool   _writeHeader();
		bool   _readHeader();

		template <typename T>
		requires(std::is_scalar_v<T>)
		bool _readRaw(T &value)
		{
			auto data = reinterpret_cast<std::byte *>(&value);

			return (_readRaw({data, sizeof(T)}) == sizeof(T));
		}

		std::unique_ptr<FileStream> _raw_stream{nullptr};
		std::unique_ptr<buffer>				 _buffer{std::make_unique<buffer>()};
		z_stream                       _z_stream{};
		BitSet<OpenFlags>              _mode{0};
		uint32                         _crc{0};
		size_t                         _header_size{0};
	};
}

#endif
