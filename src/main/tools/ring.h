#ifndef OCTAHEDRON_RING_H_
#define OCTAHEDRON_RING_H_

#include <utility>
#include <cstddef>
#include <span>
#include <cstring>
#include <memory>

#include <tools/math.h>

namespace octahedron {

/**
 * \brief <a href="https://en.wikipedia.org/wiki/Circular_buffer">Circular buffer</a>.
 *
 * A circular buffer is a FIFO container with a fixed capacity. It does this by having an internal buffer of a certain size, having a start and an end marker which are pushed on IO operations; such operations wrap around the actual start and end of the internal buffer.
 *
 * \tparam T Type of object stored
 * \tparam Capacity Capacity of the buffer
 */
template <typename T>
class basic_ring_base;

template <template <typename, size_t> typename Derived, typename T, size_t Capacity>
class basic_ring_base<Derived<T, Capacity>> {
public:
	basic_ring_base() = default;

	~basic_ring_base() {
		_destroy(_get_idx(), _get_size());
	}

	template <bool Const>
	struct iterator {
		std::conditional_t<Const, std::add_const_t<basic_ring_base>, basic_ring_base> *container = nullptr;
		size_t idx{0};

		iterator &operator++() noexcept {
			++idx;
			return *this;
		}

		iterator operator++(int) noexcept {
			iterator ret{idx};
			++idx;
			return {ret};
		}

		iterator &operator--() noexcept {
			assert(idx > 0);
			--idx;
			return *this;
		}

		iterator operator--(int) noexcept {
			assert(idx > 0);
			iterator ret{idx};
			--idx;
			return {ret};
		}

		auto operator*() const noexcept -> std::conditional_t<Const, const T&, T&> {
			return (*container)[idx];
		}

		template <bool C1, bool C2>
		friend bool operator==(const iterator<C1> &a, const iterator<C2> &b) {
			return (a.idx == b.idx && a.container == b.container);
		}
	};

	T &operator[](size_t idx) noexcept {
		assert(idx < size());

		idx += _get_idx();
		if (idx >= capacity())
			idx -= capacity();
		return (*_get_elem(idx));
	}

	const T &operator[](size_t idx) const noexcept {
		assert(idx < size());

		idx += _get_idx();
		if (idx >= capacity())
			idx -= capacity();
		return (*_get_elem(idx));
	}

	iterator<false> begin() {
		return {this, 0};
	}

	iterator<true> begin() const {
		return {this, 0};
	}

	iterator<false> end() {
		return {this, _get_size()};
	}

	iterator<true> end() const {
		return {this, _get_size()};
	}

	template <typename... Args>
	T& emplace_back(Args&&... args) {
		assert(size() < capacity());
		size_t idx = _get_idx() + size();

		if (idx >= capacity())
			idx -= capacity();

		T* where = reinterpret_cast<T*>(buffer.data()) + idx;

		std::construct_at(where, std::forward<Args>(args)...);
		_set_size(size() + 1);
		return (*where);
	}

	T& front() noexcept {
		assert(!empty());

		return (reinterpret_cast<T*>(buffer.data())[_get_idx()]);
	}

	void pop_front() noexcept {
		assert(!empty());
		size_t idx = _get_idx();

		std::destroy_at(reinterpret_cast<T*>(buffer.data()) + idx);
		++idx;
		if (idx >= capacity())
			idx = 0;
		_set_idx(idx);
		_set_size(size() - 1);
	}

	T& push_back(const T& value) noexcept(std::is_nothrow_copy_constructible_v<T>) requires (std::is_copy_constructible_v<T>) {
		return (emplace_back(value));
	}

	T& push_back(T&& value) noexcept(std::is_nothrow_move_constructible_v<T>) requires (std::is_move_constructible_v<T>) {
		return (emplace_back(std::move(value)));
	}

	template <typename Range>
	size_t append_range(Range&& rng, size_t max_count) noexcept(std::is_nothrow_copy_constructible_v<T>) {
		using range_t = std::remove_cvref_t<Range>;

		if constexpr (std::ranges::sized_range<range_t>) {
			assert(max_count <= std::ranges::size(rng));
			if (size() == Capacity)
				return (0);
			size_t capacity = Capacity - size();
			size_t to_write = min(max_count, capacity);
			size_t my_end = _get_end_idx();
			if (my_end + to_write < Capacity) { // contiguous
				return (_do_write_unchecked(std::forward<Range>(rng), to_write));
			}
			// end + to_write > Size: wraps around
			size_t first_pass = _do_write_unchecked(std::forward<Range>(rng), Capacity - my_end);

			_do_write_unchecked(std::forward<Range>(rng) | std::views::drop(first_pass), to_write - first_pass);
			return (to_write);
		} else {
			auto it = std::ranges::begin(rng);
			size_t i = 0;
			while (i < max_count && it != std::ranges::end(rng)) {
				push_back(*it);
				++i;
			}
			return (i);
		}
	}

	template <typename Range>
	size_t extract_range(Range&& rng, size_t max_count) {
		using range_t = std::remove_cvref_t<Range>;

		size_t to_read = min(size(), max_count);
		if constexpr (std::ranges::sized_range<range_t>) {
			assert(max_count <= std::ranges::size(rng));
			if (empty())
				return (0);
			if (_get_idx() + to_read < Capacity) { // contiguous
				return (_do_read_unchecked(std::forward<Range>(rng), to_read));
			}
			// start + to_read > Size: wraps around
			size_t first_pass = _do_read_unchecked(std::forward<Range>(rng), Capacity - _get_idx());

			_do_read_unchecked(std::forward<Range>(rng) | std::views::drop(first_pass), to_read - first_pass);
			return (to_read);
		} else {
			auto it = std::ranges::begin(rng);
			size_t i = 0;
			while (i < to_read && it != std::ranges::end(rng)) {
				assert(i < max_count);
				*it = std::move(front());
				pop_front();
				++i;
			}
			return (i);
		}
	}

	template <typename Range>
	size_t extract_range(Range&& rng) {
		using range_t = std::remove_cvref_t<Range>;

		if constexpr (std::ranges::sized_range<range_t>) {
			return (extract_range(std::forward<Range>(rng), std::ranges::size(rng)));
		} else {
			auto it = std::ranges::begin(rng);
			size_t i = 0;
			while (it != std::ranges::end(rng)) {
				assert(i < size());
				*it = std::move(front());
				pop_front();
				++i;
			}
			return (i);
		}
	}

	template <typename Range>
	size_t append_range(Range&& rng) noexcept(std::is_nothrow_copy_constructible_v<T>) {
		using range_t = std::remove_cvref_t<Range>;

		if constexpr (std::ranges::sized_range<range_t>) {
			return (append_range(std::forward<Range>(rng), std::ranges::size(rng)));
		} else {
			auto   it = std::ranges::begin(rng);
			size_t i = 0;
			while (it != std::ranges::end(rng)) {
				push_back(*it);
				++i;
			}
			return (i);
		}
	}

	size_t size() const noexcept {
		return (_get_size());
	}

	bool empty() const noexcept {
		return (!(size() > 0));
	}

	void reset() {
		_destroy(_get_idx(), _get_size());
		_set_size(0);
		_set_idx(0);
	}

	consteval static size_t capacity() noexcept {
		return Capacity;
	}

private:
	T* _get_elem(size_t idx) {
		return (reinterpret_cast<T*>(buffer.data()) + idx);
	}

	T* _get_elem(size_t idx) const {
		return (reinterpret_cast<T*>(buffer.data()) + idx);
	}

	size_t _get_idx() const {
		return (static_cast<Derived<T, Capacity> const *>(this)->_idx());
	}

	size_t _get_size() const {
		return (static_cast<Derived<T, Capacity> const *>(this)->_size());
	}

	size_t _get_end_idx() const {
		size_t my_end = _get_idx() + size();
		if (my_end > Capacity)
			my_end -= Capacity;
		return (my_end);
	}

	void _set_idx(size_t new_idx) {
		return (static_cast<Derived<T, Capacity> *>(this)->_idx(new_idx));
	}

	void _set_size(size_t new_size) {
		return (static_cast<Derived<T, Capacity> *>(this)->_size(new_size));
	}

	template <typename Range>
	size_t _do_read_unchecked(Range&& buf, size_t size) noexcept(noexcept(*std::declval<std::ranges::iterator_t<std::remove_cvref_t<Range>>>() = std::declval<T&&>())) {
		using value_t = std::ranges::range_value_t<std::remove_cvref_t<Range>>;

		T* pos = _get_elem(_get_idx());
		if constexpr (std::is_trivial_v<T> && std::is_same_v<T, value_t> && std::ranges::contiguous_range<std::remove_cvref_t<Range>>) {
			std::memcpy(std::ranges::data(buf), pos, size * sizeof(T));
		} else {
			std::move(pos, pos + size, buf);
			for (size_t i = 0; i < size; ++i) {
				std::destroy_at(pos + i);
			}
		}
		size_t new_idx = _get_idx() + size;
		_set_size(_get_size() - size);
		_set_idx(new_idx >= capacity() ? 0 : new_idx);
		return (size);
	}

	template <typename Range>
	size_t _do_write_unchecked(Range&& buf, size_t size) noexcept(std::is_nothrow_copy_constructible_v<T>) {
		using value_t = std::ranges::range_value_t<std::remove_cvref_t<Range>>;

		size_t idx = _get_idx() + _get_size();
		if (idx >= capacity())
			idx -= capacity();
		T* pos = _get_elem(idx);
		if constexpr (std::is_trivial_v<T> && std::is_same_v<T, value_t> && std::ranges::contiguous_range<std::remove_cvref_t<Range>>) {
			std::memcpy(pos, std::ranges::data(buf), size * sizeof(T));
		} else {
			auto it = std::ranges::begin(buf);
			for (size_t i = 0; i < size; ++i) {
				it = std::ranges::next(it);
				std::construct_at(pos + i, *it);
			}
		}
		_set_size(_get_size() + size);
		return (size);
	}

	void _destroy(size_t idx, size_t count) {
		if constexpr (std::is_trivial_v<T>) {
			return; // no need to do anything, those are implicit lifetime types
		} else {
			T* as_t = reinterpret_cast<T*>(buffer.data());
			assert(idx <= capacity());
			assert(count <= capacity());
			if (idx + count < capacity()) { // contiguous
				while (idx < count) {
					std::destroy_at<T>(as_t + idx);
					++idx;
				}
			} else {
				count -= capacity() - idx;
				while (idx < capacity()) {
					std::destroy_at<T>(as_t + idx);
					++idx;
				}
				idx = 0;
				while (idx < count) {
					std::destroy_at<T>(as_t + idx);
					++idx;
				}
			}
		}
	}

	std::array<std::byte, Capacity * sizeof(T)> buffer alignas(std::array<T, Capacity>);
};

template <typename T>
class basic_ring : public basic_ring_base<T> {};

template <template <typename, size_t> typename Derived, typename T, size_t Capacity>
requires (sizeof(T) == 1)
class basic_ring<Derived<T, Capacity>> : public basic_ring_base<Derived<T, Capacity>> {
public:
	using basic_ring_base<Derived<T, Capacity>>::basic_ring_base;
	using basic_ring_base<Derived<T, Capacity>>::operator=;

	/**
	 * \brief Read data from the buffer.
	 *
	 * \param buf Span of Ts to read into.
	 * \return size_t Number of Ts actually read.
	 */
	size_t read(std::span<T> buf) requires (byteoid<T>) {
		return (this->extract_range(buf));
	}

	/**
	 * \brief Read data from the buffer.
	 *
	 * \param buf Span of Ts to read into.
	 * \param max_read Maximum size to read.
	 * \return size_t Number of Ts actually read.
	 */
	size_t read(std::span<T> buf, size_t max_read) requires (byteoid<T>) {
		return (this->extract_range(buf, max_read));
	}

	size_t write(std::span<T const> buf) requires (byteoid<T>) {
		return (this->append_range(buf));
	}

	size_t write(std::span<T const> buf, size_t max_write) requires (byteoid<T>) {
		return (this->append_range(buf, max_write));
	}
};

template <typename T, size_t Capacity>
class ring : public basic_ring<ring<T, Capacity>> {
private:
	friend class basic_ring_base<ring>;

	size_t _size() const noexcept {
		return (stored_size);
	}

	size_t _idx() const noexcept {
		return (start);
	}

	void _size(size_t new_size) noexcept {
		stored_size = new_size;
	}

	void _idx(size_t new_idx) noexcept {
		start = new_idx;
	}

	size_t stored_size{0};
	size_t start{0};
};

}

#endif /* OCTAHEDRON_RING_H_ */
