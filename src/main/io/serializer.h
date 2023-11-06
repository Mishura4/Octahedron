#ifndef OCTAHEDRON_SERIALIZER_H_
#define OCTAHEDRON_SERIALIZER_H_

#include <algorithm>
#include <cassert>
#include <ranges>

#include "tools/math.h"

#include "io_interface.h"

namespace octahedron
{
template <typename T> requires(std::ranges::range<T> && byteoid<std::remove_cvref<
																 std::ranges::range_value_t<T>>>)
class serializer : public
	std::conditional_t<
		std::is_const_v<std::remove_cvref<std::ranges::range_value_t<T>>>,
		io_read_interface<serializer<T>>,
		io_interface<serializer<T>>> {
public:
	using value_type = std::remove_reference_t<std::ranges::range_value_t<T>>;

	serializer() = default;

	serializer(value_type *data, size_t size) requires(std::constructible_from<
		T, value_type*, size_t>) :
		_data{data, size} { }

	serializer(std::initializer_list<value_type> values) requires(std::constructible_from<
		T, std::initializer_list<value_type>>) :
		_data(values),
		_pos(std::end(_data)) {}

	serializer(const serializer &other) requires(std::assignable_from<T&, const T&>) {
		size_t pos = other.tell();

		_data = (std::move(other._data));
		_pos = std::begin(_data);
		std::advance(_pos, pos);
	}

	serializer(serializer &&other) requires(std::assignable_from<T&, T&&>) {
		size_t pos = other.tell();

		_data = (std::move(other._data));
		_pos = std::begin(_data);
		std::advance(_pos, pos);
	}

	serializer& operator=(std::initializer_list<value_type> values) requires(std::assignable_from<
		T&, std::initializer_list<value_type>>) {
		_data = values;
		_pos = 0;
		return (*this);
	}

	serializer& operator=(const serializer &other) requires(std::assignable_from<T&, const T&>) {
		size_t pos = other.tell();

		_data = other._data;
		_pos = std::begin(_data);
		std::advance(_pos, pos);
		return (*this);
	}

	serializer& operator=(serializer &&other) requires(std::assignable_from<T&, T&&>) {
		size_t pos = other.tell();

		_data = std::move(other._data);
		_pos = std::begin(_data);
		std::advance(_pos, pos);
		return (*this);
	}

	[[nodiscard]] size_t tell() const {
		if constexpr (!std::is_same_v<decltype(std::begin(_data)), decltype(_pos)>)
			return (std::distance(std::begin(_data), static_cast<typename T::const_iterator>(_pos)));
		else
			return (std::distance(std::begin(_data), _pos));
	}

	bool seek(ssize_t pos, int whence = SEEK_SET) {
		using iterator_type = std::ranges::iterator_t<T>;

		switch (whence) {
			case SEEK_END:
				if (pos >= 0)
					_pos = std::end(_data);
				else {
					if constexpr (std::bidirectional_iterator<iterator_type>)
						std::advance(_pos, pos);
					else {
						size_t diff = size() - static_cast<size_t>(-pos);

						_pos = std::begin(_data);
						std::advance(_pos, diff);
					}
				}
				break;
			case SEEK_SET:
				_pos = std::begin(_data);
				if (pos > 0) {
					if (pos > size())
						_pos = std::end(_data);
					else
						std::advance(_pos, static_cast<size_t>(pos));
				}
				break;
			case SEEK_CUR: {
				if (pos > 0) {
					auto upos = static_cast<size_t>(pos);

					if (upos > bytes_available())
						_pos = std::end(_data);
					else
						std::advance(_pos, static_cast<size_t>(pos));
				} else {
					auto upos = static_cast<size_t>(-pos);

					if (upos > size())
						_pos = std::begin(_data);
					else if constexpr (std::bidirectional_iterator<iterator_type>)
						std::advance(_pos, pos);
					else {
						auto curpos = tell();

						_pos = std::begin(_data);
						std::advance(_pos, curpos - upos);
					}
				}
				break;
			}

			default:
				return (false);
		}
		return (true);
	}

	size_t bytes_available() const {
		return (tell() - size());
	}

	size_t size() const {
		return (std::size(_data));
	}

	template <byteoid U>
	size_t write(const U *data, size_t size) {
		return (write(std::span{reinterpret_cast<const std::byte*>(data), size}));
	}

	template <byteoid U>
	size_t read(U *data, size_t size) {
		return (read(std::span{reinterpret_cast<std::byte*>(data), size}));
	}

	size_t read(std::span<std::byte> buf) {
		std::span converted{reinterpret_cast<value_type*>(buf.data()), buf.size()};

		size_t to_read = min(buf.size(), bytes_available());

		_pos = std::ranges::copy_n(_pos, to_read, std::begin(converted)).in;
		return (to_read);
	}

	size_t write(std::span<const std::byte> buf) {
		std::span converted{reinterpret_cast<const value_type*>(buf.data()), buf.size()};

		if constexpr (requires(T &t, value_type v) { t.push_back(v); }) {
			if (size_t              remaining = (size() - tell()); buf.size() > remaining) {
				auto result = std::ranges::copy_n(std::begin(converted), remaining, _pos);

				std::ranges::copy_n(
					std::next(std::begin(converted), remaining),
					buf.size() - remaining,
					std::back_inserter(_data)
				);
				_pos = std::end(_data);
			} else {
				_pos = std::ranges::copy_n(std::begin(converted), buf.size(), _pos).out;
			}
			return (buf.size());
		} else {
			size_t to_write = min(bytes_available(), buf.size());

			_pos = std::ranges::copy_n(std::begin(converted), to_write, _pos).out;
			return (to_write);
		}
	}

	// TODO: iterators

	value_type *data() requires(std::ranges::contiguous_range<T>) {
		return (std::data(_data));
	}

	const value_type *data() const requires(std::ranges::contiguous_range<T>) {
		return (std::data(_data));
	}

	void reserve(size_t size) requires(requires(T &t) { t.reserve(size); }) {
		_data.reserve(size);
	}

	void                        pad(size_t size) {
		if constexpr (requires(T &t) { t.resize(size); }) {
			_data.resize(std::size(_data) + size);
		} else if constexpr (std::is_default_constructible_v<value_type>) {
			std::ranges::fill_n(std::inserter(_data, _pos), size, value_type{});
		}
	}

	std::vector<value_type> build() const {
		if constexpr (std::constructible_from<std::vector<value_type>, const T&>)
			return {_data};
		else if constexpr (std::constructible_from<
			std::vector<value_type>,
			std::ranges::iterator_t<T>,
			std::ranges::iterator_t<T>>)
			return {std::begin(_data), std::end(_data)};
		else {
			std::vector<T> ret;

			std::ranges::copy(std::back_inserter(ret), _data);
			return (ret);
		}
	}

	void reset() {
		_pos = std::begin(_data);
	}

	void clear() requires requires(T &t) { t.clear(); } {
		_data.clear();
		_pos = std::end(_data);
	}

private:
	T                          _data{};
	std::ranges::iterator_t<T> _pos{std::begin(_data)};
};

template <byteoid T, std::integral size_type>
serializer(T *, size_type) -> serializer<std::span<T>>;

using dynamic_buffer = serializer<std::vector<std::byte>>;

template <byteoid T, size_t Size>
using static_buffer = serializer<std::array<T, Size>>;

using ucharbuf = serializer<std::span<unsigned char>>;
} // namespace octahedron

#endif /* OCTAHEDRON_SERIALIZER_H_ */
