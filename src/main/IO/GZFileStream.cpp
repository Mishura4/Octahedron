#include "GZFileStream.h"
#include "Tools/Math.h"

#include <cassert>

#include <zlib.h>

using namespace Octahedron;

auto GZFileStream::_fromRawStream(
	std::unique_ptr<FileStream> &&stream,
	BitSet<OpenFlags>								 mode,
	int															 compression
) -> std::unique_ptr<GZFileStream>
{
	std::unique_ptr<GZFileStream> ret = std::make_unique<GZFileStream>();

	if (mode & OpenFlags::INPUT)
	{
		if (mode & OpenFlags::OUTPUT)
		{
			log(LogLevel::DEBUG, "cannot open a gzstream for both reading and writing");
			return {nullptr};
		}
		if (int err = inflateInit2(&ret->_z_stream, -MAX_WBITS); err != Z_OK)
		{
			log(LogLevel::DEBUG, "could not start gz inflate: {}", zError(err));
			return {nullptr};
		}
	}
	else if (mode & OpenFlags::OUTPUT)
	{
		if (int err = deflateInit2(
					&ret->_z_stream,
					compression,
					Z_DEFLATED,
					-MAX_WBITS,
					min(MAX_MEM_LEVEL, 8),
					Z_DEFAULT_STRATEGY);
				err != Z_OK)
		{
			log(LogLevel::DEBUG, "could not start gz deflate: {}", zError(err));
			return {nullptr};
		}
	}

	ret->_raw_stream = std::move(stream);
	ret->_mode			 = mode;
	if (mode & OpenFlags::INPUT && !ret->_readHeader())
		return {nullptr};
	if (mode & OpenFlags::OUTPUT && !ret->_writeHeader())
		return {nullptr};
	return (ret);
}

GZFileStream::~GZFileStream()
{
	if (_mode & OpenFlags::OUTPUT)
	{
		int err = Z_OK;

		while (err == Z_OK)
		{
			if (_z_stream.avail_out > 0)
				err = deflate(&_z_stream, Z_FINISH);
			GZFileStream::flush();
		}
		if (err != Z_STREAM_END)
			log(LogLevel::ERROR, "error while saving gz file: {}", zError(err));
		if (!_raw_stream->put<Endianness::LITTLE>(_crc) ||
				!_raw_stream->put<Endianness::LITTLE>(_z_stream.total_in))
			log(LogLevel::DEBUG, "could not write gz trailer");
	}
}

size_t GZFileStream::_fetch(size_t size)
{
	if (!_z_stream.avail_in)
		_z_stream.next_in = _buffer->data();
	ptrdiff_t z_buffer_size = (_z_stream.next_in - _buffer->data()) + _z_stream.avail_in;

	octa_assert(BUFFER_SIZE > z_buffer_size);
	size = min(size, (BUFFER_SIZE - z_buffer_size));
	size = _raw_stream->read(_z_stream.next_in + _z_stream.avail_in, size);
	_z_stream.avail_in += size;
	return (_z_stream.avail_in);
}

size_t GZFileStream::read(std::span<std::byte> buf)
{
	const auto data = reinterpret_cast<Bytef *>(buf.data());
	uint32		 read_size;

	if (!(_mode & OpenFlags::INPUT) || buf.empty())
		return (0);
	_z_stream.next_out	= data;
	_z_stream.avail_out = narrow_cast<uint32>(buf.size());
	while (_z_stream.avail_out > 0)
	{
		if (!_z_stream.avail_in)
		{
			if (_fetch() == 0)
			{
				_endRead();
				break;
			}
		}
		int err = inflate(&_z_stream, Z_NO_FLUSH);
		if (err != Z_OK)
		{
			if (err == Z_STREAM_END)
			{
				read_size = narrow_cast<uint32>(buf.size() - _z_stream.avail_out);
				_crc			= ::crc32(_crc, data, read_size);
				_atEof();
				return (read_size);
			}
			_endRead();
			break;
		}
	}
	read_size = narrow_cast<uint32>(buf.size() - _z_stream.avail_out);
	_crc			= ::crc32(_crc, data, read_size);
	return (read_size);
}

size_t GZFileStream::_readRaw(std::span<std::byte> buf)
{
	size_t left = buf.size();

	while (left > 0)
	{
		if (!_z_stream.avail_in && !_fetch())
			return (buf.size() - left);
		auto to_read = min(_z_stream.avail_in, buf.size());
		memcpy(buf.data(), _z_stream.next_in, to_read);
		_z_stream.avail_in -= to_read;
		_z_stream.next_in += to_read;
		left -= to_read;
	}
	return (buf.size());
}

void GZFileStream::_atEof()
{
	if (!isLogEnabled(LogLevel::DEBUG))
		return;
	constexpr BitSet<LogLevel> level = LogLevel::WARN | LogLevel::DEBUG;

	if (_z_stream.avail_in || _fetch())
	{
		log(level, "failed to fetch trailer bytes of gzip file");
		return;
	}
	uint32 crc	= 0;
	uint32 size = 0;

	if (size_t r = _readRaw({reinterpret_cast<std::byte *>(&crc), sizeof(crc)}); r != sizeof(crc))
	{
		log(level, "failed to fetch crc of gzip file: expected {} bytes, got {}", sizeof(crc), r);
		return;
	}
	if (size_t r = _readRaw({reinterpret_cast<std::byte *>(&size), sizeof(size)}); r != sizeof(size))
	{
		log(
			level,
			"failed to fetch tail size of gzip file: expected {} bytes, got {}",
			sizeof(size),
			r);
		return;
	}
	uint32 my_size = _z_stream.total_out % (1uLL << 32);

	crc	 = byteswap<Endianness::LITTLE>(crc);
	size = byteswap<Endianness::LITTLE>(size);
	if (crc != _crc)
		log(level, "gzip crc check failed: read 0x{:X}, calculated 0x{:X}", crc, _crc);
	if (size != my_size)
		log(level, "gzip size check failed: read {}, calculated {}", size, my_size);
}

void GZFileStream::_skipRaw(size_t size)
{
	while (size > 0 && _z_stream.avail_in > 0)
	{
		auto skipped = min(size, _z_stream.avail_in);

		_z_stream.avail_in -= skipped;
		_z_stream.next_in += skipped;
		size -= skipped;
	}
	if (size > 0)
		_raw_stream->seek(size, SEEK_CUR);
}

#define TYPE_OF(a) std::remove_const_t<std::remove_reference_t<decltype(a)>>

bool GZFileStream::_writeHeader()
{
	struct Header
	{
			TYPE_OF(HEADER_MAGIC) magic{HEADER_MAGIC};
			uint8	 file_flags{0};
			uint32 timestamp = static_cast<uint32>(std::time(nullptr));
			uint8	 comp_flags{0};
			uint8	 operating_system{3}; // Unix
	};

	if (_raw_stream->put(Header{}) != sizeof(Header))
	{
		log(LogLevel::DEBUG, "failed to write gz header");
		return (false);
	}
	return (true);
}

bool GZFileStream::_readHeader()
{
	_fetch(10);
	TYPE_OF(HEADER_MAGIC) magic_in;
	if (_readRaw(magic_in) != magic_in.size() || magic_in != HEADER_MAGIC)
	{
		log(
			LogLevel::DEBUG,
			"bad gz header: expected magic {}, got {}",
			fmt::join(HEADER_MAGIC, ", "),
			fmt::join(magic_in, ", "));
		return (false);
	}
	uint8_t flags;
	if (!_readRaw(flags))
	{
		log(LogLevel::DEBUG, "bad gz header: could not read flags");
		return (false);
	}
	if (flags & GZipFlags::RESERVED)
		return (false);
	_skipRaw(6); // timestamp (4), compression flags (1), OS id (1)
	if (flags & GZipFlags::EXTRA)
	{
		uint16 xlen;

		_readRaw(xlen);
		xlen = byteswap<Endianness::LITTLE>(xlen);
		_skipRaw(xlen); // skip the entire extra headers
	}
	if (flags & GZipFlags::NAME)
	{
		char c;

		while (_readRaw(c) && c != 0)
			;
	}
	if (flags & GZipFlags::COMMENT)
	{
		char c;

		while (_readRaw(c) && c != 0)
			;
	}
	if (flags & GZipFlags::CRC)
		_skipRaw(2);
	_header_size = _raw_stream->tell() - _z_stream.avail_in;
	return (_z_stream.avail_in > 0 || !_raw_stream->eof());
}

size_t GZFileStream::write(std::span<const std::byte> buf)
{
	assert(_mode & OpenFlags::OUTPUT);

	if (buf.empty())
		return (0);

	auto data = reinterpret_cast<const Bytef *>(buf.data());

	// TODO: remove const_cast, compile with ZLIB_CONST
	_z_stream.next_in = const_cast<Bytef *>(data);
	_z_stream.avail_in = buf.size();
	while (_z_stream.avail_in > 0)
	{
		if (_z_stream.avail_out == 0 && !flush())
		{
			log(LogLevel::DEBUG, "failed to flush to gz file");
			break;
		}
		if (int err = deflate(&_z_stream, Z_NO_FLUSH); err != Z_OK)
		{
			log(LogLevel::DEBUG, "failed to deflate {} bytes for gz file", buf.size());
			break;
		}
	}
	_crc = ::crc32(_crc, data, buf.size() - _z_stream.avail_in);
	return size_t();
}

size_t GZFileStream::tell() const
{
	if (_mode & OpenFlags::INPUT)
		return (_z_stream.total_out);
	if (_mode & OpenFlags::OUTPUT)
		return (_z_stream.total_in);
	return (error_size);
}

size_t GZFileStream::tellRaw() const
{
	return (_raw_stream->tell());
}

size_t GZFileStream::sizeRaw() const
{
	return (_raw_stream->size());
}

bool GZFileStream::seek(ssize_t pos, int whence)
{
	if (!(_mode & (OpenFlags::INPUT | OpenFlags::OUTPUT)))
		return (false);

	constexpr auto seek_end = [](GZFileStream *me, ssize_t pos)
	{
		if (pos > 0)
			pos = 0;

		size_t										 my_pos	 = me->tell();
		size_t										 to_skip = me->size() - abs(pos);
		std::array<std::byte, 512> buffer;

		while (to_skip > 0)
		{
			size_t to_read = min(to_skip, 512u);

			if (me->read({buffer.data(), to_read}) != to_read)
			{
				log(LogLevel::DEBUG, "error while skipping bytes for gz seek_end");
				me->_endRead();
				return (false);
			}
			to_skip -= to_read;
		}
		return (true);
	};

	if (whence == SEEK_END)
		return (seek_end(this, pos));

	size_t target_pos;

	if (whence == SEEK_CUR)
	{
		if (pos < 0 && (-pos) > _z_stream.total_out)
			target_pos = 0;
		else
			target_pos = _z_stream.total_out + pos;
	}
	else /* whence == SEEK_SET */
		target_pos = (pos < 0 ? 0 : to_unsigned(pos));

	if (target_pos == _z_stream.total_out)
		return (true);

	std::array<std::byte, 512> buffer;

	if (target_pos < _z_stream.total_out)
	{
		_z_stream.avail_in = 0;
		_z_stream.next_in	 = nullptr;
		inflateReset(&_z_stream);
		_crc = ::crc32(0, nullptr, 0);
	}
	size_t to_skip = target_pos - _z_stream.total_out;
	size_t to_read = min(to_skip, buffer.size());

	if (read({buffer.data(), to_read}) != to_read)
	{
		log(LogLevel::DEBUG, "error while skipping bytes for gz seek");
		_endRead();
		return (false);
	}
	return (true);
}

uint32 GZFileStream::crc32() const noexcept
{
	return (_crc);
}

bool GZFileStream::flush(bool full)
{
	if (full)
		deflate(&_z_stream, Z_SYNC_FLUSH);
	if (_z_stream.next_out && _z_stream.avail_out < BUFFER_SIZE)
	{
		size_t write_size = BUFFER_SIZE - _z_stream.avail_out;
		if (_raw_stream->write(_buffer->data(), write_size) != write_size)
			return (false);
		if (full && !_raw_stream->flush())
			return (false);
	}
	_z_stream.next_out	= reinterpret_cast<Bytef *>(_buffer->data());
	_z_stream.avail_out = BUFFER_SIZE;
	return (true);
}

bool GZFileStream::flush()
{
	return (flush(true));
}

bool GZFileStream::eof()
{
	return (!(_mode & (OpenFlags::INPUT | OpenFlags::OUTPUT)));
}

int GZFileStream::_endWrite()
{
	octa_assert(_mode & OpenFlags::OUTPUT);

	if (int ret = deflateEnd(&_z_stream); ret != Z_OK)
	{
		if (ret == Z_DATA_ERROR)
			_mode &= ~OpenFlags::OUTPUT;
		return (ret);
	}
	_mode &= ~OpenFlags::OUTPUT;
	return (Z_OK);
}

int GZFileStream::_endRead()
{
	octa_assert(_mode & OpenFlags::INPUT);

	if (int ret = inflateEnd(&_z_stream); ret != Z_OK)
	{
		if (ret == Z_DATA_ERROR)
			_mode &= ~OpenFlags::INPUT;
		return (ret);
	}
	_mode &= ~OpenFlags::INPUT;
	return (Z_OK);
}
