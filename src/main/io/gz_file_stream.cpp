#include "gz_file_stream.h"
#include "tools/math.h"

#include <cassert>

#include <zlib.h>

using namespace octahedron;

std::unique_ptr<gz_file_stream> gz_file_stream::_from_raw_stream(
	std::unique_ptr<file_stream> &&stream,
	bit_set<open_flags>            mode,
	int                            compression
) {
	auto ret = std::make_unique<gz_file_stream>();

	if (mode & open_flags::INPUT) {
		if (mode & open_flags::OUTPUT) {
			log(log_level::DEBUG, "cannot open a gzstream for both reading and writing");
			return {nullptr};
		}
		if (int err = inflateInit2(&ret->_z_stream, -MAX_WBITS); err != Z_OK) {
			log(log_level::DEBUG, "could not start gz inflate: {}", zError(err));
			return {nullptr};
		}
	} else if (mode & open_flags::OUTPUT) {
		if (int err = deflateInit2(
				&ret->_z_stream,
				compression,
				Z_DEFLATED,
				-MAX_WBITS,
				min(MAX_MEM_LEVEL, 8),
				Z_DEFAULT_STRATEGY
			);
			err != Z_OK) {
			log(log_level::DEBUG, "could not start gz deflate: {}", zError(err));
			return {nullptr};
		}
	}

	ret->_raw_stream = std::move(stream);
	ret->_mode = mode;
	if (mode & open_flags::INPUT && !ret->_read_header())
		return {nullptr};
	if (mode & open_flags::OUTPUT && !ret->_write_header())
		return {nullptr};
	return (ret);
}

gz_file_stream::~gz_file_stream() {
	if (_mode & open_flags::OUTPUT) {
		int err = Z_OK;

		while (err == Z_OK) {
			if (_z_stream.avail_out > 0)
				err = deflate(&_z_stream, Z_FINISH);
			gz_file_stream::flush();
		}
		if (err != Z_STREAM_END)
			log(log_level::ERROR, "error while saving gz file: {}", zError(err));
		if (!_raw_stream->put<endianness::little>(_crc) ||
				!_raw_stream->put<endianness::little>(_z_stream.total_in))
			log(log_level::DEBUG, "could not write gz trailer");
	}
}

size_t gz_file_stream::_fetch(size_t size) {
	if (!_z_stream.avail_in)
		_z_stream.next_in = _buffer->data();
	ptrdiff_t z_buffer_size = (_z_stream.next_in - _buffer->data()) + _z_stream.avail_in;

	octa_assert(BUFFER_SIZE > z_buffer_size);
	size = min(size, (BUFFER_SIZE - z_buffer_size));
	size = _raw_stream->read(_z_stream.next_in + _z_stream.avail_in, size);
	_z_stream.avail_in += size;
	return (_z_stream.avail_in);
}

size_t gz_file_stream::read(std::span<std::byte> buf) {
	const auto data = reinterpret_cast<Bytef*>(buf.data());
	uint32     read_size;

	if (!(_mode & open_flags::INPUT) || buf.empty())
		return (0);
	_z_stream.next_out = data;
	_z_stream.avail_out = narrow_cast<uint32>(buf.size());
	while (_z_stream.avail_out > 0) {
		if (!_z_stream.avail_in) {
			if (_fetch() == 0) {
				_end_read();
				break;
			}
		}
		int err = inflate(&_z_stream, Z_NO_FLUSH);
		if (err != Z_OK) {
			if (err == Z_STREAM_END) {
				read_size = narrow_cast<uint32>(buf.size() - _z_stream.avail_out);
				_crc = ::crc32(_crc, data, read_size);
				_at_eof();
				return (read_size);
			}
			_end_read();
			break;
		}
	}
	read_size = narrow_cast<uint32>(buf.size() - _z_stream.avail_out);
	_crc = ::crc32(_crc, data, read_size);
	return (read_size);
}

size_t gz_file_stream::_read_raw(std::span<std::byte> buf) {
	size_t left = buf.size();

	while (left > 0) {
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

void gz_file_stream::_at_eof() {
	if (!is_log_enabled(log_level::DEBUG))
		return;
	constexpr bit_set<log_level> level = log_level::WARN | log_level::DEBUG;

	if (_z_stream.avail_in || _fetch()) {
		log(level, "failed to fetch trailer bytes of gzip file");
		return;
	}
	uint32 crc = 0;
	uint32 size = 0;

	if (size_t r = _read_raw({reinterpret_cast<std::byte*>(&crc), sizeof(crc)}); r != sizeof(crc)) {
		log(level, "failed to fetch crc of gzip file: expected {} bytes, got {}", sizeof(crc), r);
		return;
	}
	if (size_t r = _read_raw({reinterpret_cast<std::byte*>(&size), sizeof(size)});
		r != sizeof(size)) {
		log(
			level,
			"failed to fetch tail size of gzip file: expected {} bytes, got {}",
			sizeof(size),
			r
		);
		return;
	}
	uint32 my_size = _z_stream.total_out % (1uLL << 32);

	crc = byteswap<endianness::little>(crc);
	size = byteswap<endianness::little>(size);
	if (crc != _crc)
		log(level, "gzip crc check failed: read 0x{:X}, calculated 0x{:X}", crc, _crc);
	if (size != my_size)
		log(level, "gzip size check failed: read {}, calculated {}", size, my_size);
}

void gz_file_stream::_skip_raw(size_t size) {
	while (size > 0 && _z_stream.avail_in > 0) {
		auto skipped = min(size, _z_stream.avail_in);

		_z_stream.avail_in -= skipped;
		_z_stream.next_in += skipped;
		size -= skipped;
	}
	if (size > 0)
		_raw_stream->seek(size, SEEK_CUR);
}

#define TYPE_OF(a) std::remove_const_t<std::remove_reference_t<decltype(a)>>

bool gz_file_stream::_write_header() {
	struct header {
		TYPE_OF(HEADER_MAGIC) magic{HEADER_MAGIC};
		uint8                 file_flags{0};
		uint32                timestamp = static_cast<uint32>(std::time(nullptr));
		uint8                 comp_flags{0};
		uint8                 operating_system{3}; // Unix
	};

	if (_raw_stream->put(header{}) != sizeof(header)) {
		log(log_level::DEBUG, "failed to write gz header");
		return (false);
	}
	return (true);
}

bool gz_file_stream::_read_header() {
	_fetch(10);
	TYPE_OF(HEADER_MAGIC) magic_in;
	if (_read_raw(magic_in) != magic_in.size() || magic_in != HEADER_MAGIC) {
		log(
			log_level::DEBUG,
			"bad gz header: expected magic {}, got {}",
			fmt::join(HEADER_MAGIC, ", "),
			fmt::join(magic_in, ", ")
		);
		return (false);
	}
	uint8_t flags;
	if (!_read_raw(flags)) {
		log(log_level::DEBUG, "bad gz header: could not read flags");
		return (false);
	}
	if (flags & GZipFlags::RESERVED)
		return (false);
	_skip_raw(6); // timestamp (4), compression flags (1), OS id (1)
	if (flags & GZipFlags::EXTRA) {
		uint16 xlen;

		_read_raw(xlen);
		xlen = byteswap<endianness::little>(xlen);
		_skip_raw(xlen); // skip the entire extra headers
	}
	if (flags & GZipFlags::NAME) {
		char c;

		while (_read_raw(c) && c != 0);
	}
	if (flags & GZipFlags::COMMENT) {
		char c;

		while (_read_raw(c) && c != 0);
	}
	if (flags & GZipFlags::CRC)
		_skip_raw(2);
	_header_size = _raw_stream->tell() - _z_stream.avail_in;
	return (_z_stream.avail_in > 0 || !_raw_stream->eof());
}

size_t gz_file_stream::write(std::span<const std::byte> buf) {
	assert(_mode & open_flags::OUTPUT);

	if (buf.empty())
		return (0);

	auto data = reinterpret_cast<const Bytef*>(buf.data());

	// TODO: remove const_cast, compile with ZLIB_CONST
	_z_stream.next_in = const_cast<Bytef*>(data);
	_z_stream.avail_in = buf.size();
	while (_z_stream.avail_in > 0) {
		if (_z_stream.avail_out == 0 && !flush()) {
			log(log_level::DEBUG, "failed to flush to gz file");
			break;
		}
		if (int err = deflate(&_z_stream, Z_NO_FLUSH); err != Z_OK) {
			log(log_level::DEBUG, "failed to deflate {} bytes for gz file", buf.size());
			break;
		}
	}
	_crc = ::crc32(_crc, data, buf.size() - _z_stream.avail_in);
	return size_t();
}

size_t gz_file_stream::tell() const {
	if (_mode & open_flags::INPUT)
		return (_z_stream.total_out);
	if (_mode & open_flags::OUTPUT)
		return (_z_stream.total_in);
	return (error_size);
}

size_t gz_file_stream::tell_raw() const {
	return (_raw_stream->tell());
}

size_t gz_file_stream::size_raw() const {
	return (_raw_stream->size());
}

bool gz_file_stream::seek(ssize_t pos, int whence) {
	if (!(_mode & (open_flags::INPUT | open_flags::OUTPUT)))
		return (false);

	constexpr auto seek_end = [](gz_file_stream *me, ssize_t pos) {
		if (pos > 0)
			pos = 0;

		size_t                     my_pos = me->tell();
		size_t                     to_skip = me->size() - abs(pos);
		std::array<std::byte, 512> buffer;

		while (to_skip > 0) {
			size_t to_read = min(to_skip, 512u);

			if (me->read({buffer.data(), to_read}) != to_read) {
				log(log_level::DEBUG, "error while skipping bytes for gz seek_end");
				me->_end_read();
				return (false);
			}
			to_skip -= to_read;
		}
		return (true);
	};

	if (whence == SEEK_END)
		return (seek_end(this, pos));

	size_t target_pos;

	if (whence == SEEK_CUR) {
		if (pos < 0 && (-pos) > _z_stream.total_out)
			target_pos = 0;
		else
			target_pos = _z_stream.total_out + pos;
	} else /* whence == SEEK_SET */
		target_pos = (pos < 0 ? 0 : to_unsigned(pos));

	if (target_pos == _z_stream.total_out)
		return (true);

	std::array<std::byte, 512> buffer;

	if (target_pos < _z_stream.total_out) {
		_z_stream.avail_in = 0;
		_z_stream.next_in = nullptr;
		inflateReset(&_z_stream);
		_crc = ::crc32(0, nullptr, 0);
	}
	size_t to_skip = target_pos - _z_stream.total_out;
	size_t to_read = min(to_skip, buffer.size());

	if (read({buffer.data(), to_read}) != to_read) {
		log(log_level::DEBUG, "error while skipping bytes for gz seek");
		_end_read();
		return (false);
	}
	return (true);
}

uint32 gz_file_stream::crc32() const noexcept {
	return (_crc);
}

bool gz_file_stream::flush(bool full) {
	if (full)
		deflate(&_z_stream, Z_SYNC_FLUSH);
	if (_z_stream.next_out && _z_stream.avail_out < BUFFER_SIZE) {
		size_t write_size = BUFFER_SIZE - _z_stream.avail_out;
		if (_raw_stream->write(_buffer->data(), write_size) != write_size)
			return (false);
		if (full && !_raw_stream->flush())
			return (false);
	}
	_z_stream.next_out = reinterpret_cast<Bytef*>(_buffer->data());
	_z_stream.avail_out = BUFFER_SIZE;
	return (true);
}

bool gz_file_stream::flush() {
	return (flush(true));
}

bool gz_file_stream::eof() {
	return (!(_mode & (open_flags::INPUT | open_flags::OUTPUT)));
}

int gz_file_stream::_end_write() {
	octa_assert(_mode & open_flags::OUTPUT);

	if (int ret = deflateEnd(&_z_stream); ret != Z_OK) {
		if (ret == Z_DATA_ERROR)
			_mode &= ~open_flags::OUTPUT;
		return (ret);
	}
	_mode &= ~open_flags::OUTPUT;
	return (Z_OK);
}

int gz_file_stream::_end_read() {
	octa_assert(_mode & open_flags::INPUT);

	if (int ret = inflateEnd(&_z_stream); ret != Z_OK) {
		if (ret == Z_DATA_ERROR)
			_mode &= ~open_flags::INPUT;
		return (ret);
	}
	_mode &= ~open_flags::INPUT;
	return (Z_OK);
}
