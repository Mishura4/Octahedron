#ifndef OCTAHEDRON_GZFILESTREAM_H_
#define OCTAHEDRON_GZFILESTREAM_H_

#include <zlib.h>

#include "file_stream.h"
#include "raw_file_stream.h"
#include "../base.h"

namespace octahedron
{

class gz_file_stream : public file_stream {
public:
	~gz_file_stream() override;

	size_t               read(std::span<std::byte> buf) override;
	size_t               write(std::span<const std::byte> buf) override;
	[[nodiscard]] size_t tell() const override;
	bool                 seek(ssize_t pos, int whence) override;
	bool                 flush() override;
	bool                 eof() override;
	[[nodiscard]] uint32 crc32() const noexcept;

	[[nodiscard]] size_t tell_raw() const;
	[[nodiscard]] size_t size_raw() const;

	bool flush(bool full);

private:
	friend std::unique_ptr<file_stream> file_system::open_gz(
		std::string_view    path,
		bit_set<open_flags> mode
	);

	class _GZipFlags {
	public:
		enum Values {
			ASCII    = 0x01,
			CRC      = 0x02,
			EXTRA    = 0x04,
			NAME     = 0x08,
			COMMENT  = 0x10,
			RESERVED = 0xE0
		};
	};

	// Z_DEFLATED technically not part of the magic but, it is expected to be there anyways
	constexpr static auto HEADER_MAGIC = byte_array({0x1F, 0x8B, Z_DEFLATED});

	using GZipFlags = _GZipFlags::Values;

	static constexpr size_t BUFFER_SIZE = 16384;
	using buffer = std::array<Bytef, BUFFER_SIZE>;

	static std::unique_ptr<gz_file_stream> _from_raw_stream(
		std::unique_ptr<file_stream> &&stream,
		bit_set<open_flags>            mode,
		int                            compression = 9
	);

	void   _at_eof();
	int    _end_write();
	int    _end_read();
	size_t _fetch(size_t size = BUFFER_SIZE);
	size_t _read_raw(std::span<std::byte> buf);
	void   _skip_raw(size_t size);
	bool   _write_header();
	bool   _read_header();

	template <typename T> requires(std::is_scalar_v<T>)
	bool _read_raw(T &value) {
		auto data = reinterpret_cast<std::byte*>(&value);

		return (_read_raw({data, sizeof(T)}) == sizeof(T));
	}

	std::unique_ptr<file_stream> _raw_stream{nullptr};
	std::unique_ptr<buffer>      _buffer{std::make_unique<buffer>()};
	z_stream                     _z_stream{};
	bit_set<open_flags>          _mode{0};
	uint32                       _crc{0};
	size_t                       _header_size{0};
};

}

#endif
