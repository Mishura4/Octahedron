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

template <template <typename, size_t, bool> typename Derived, typename T, size_t Capacity, bool Overwrite>
class basic_ring_base<Derived<T, Capacity, Overwrite>> {
	using Self = Derived<T, Capacity, Overwrite>;

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
		size_t idx = _get_idx() + size();

		if (idx >= capacity())
			idx -= capacity();

		T* where = reinterpret_cast<T*>(buffer.data()) + idx;

		if constexpr (Overwrite) {
			if (size() == capacity()) {
				std::destroy_at(where);
				std::construct_at(where, std::forward<Args>(args)...);
				idx = (idx + 1 == capacity() ? 0 : idx + 1);
				_set_idx(idx);
			} else {
				std::construct_at(where, std::forward<Args>(args)...);
				_set_size(size() + 1);
			}
		} else {
			assert(size() < capacity());
			std::construct_at(where, std::forward<Args>(args)...);
			_set_size(size() + 1);
		}
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
	size_t append_range(Range&& rng, size_t max_count) {
		using range_t = std::remove_cvref_t<Range>;

		if constexpr (std::ranges::sized_range<range_t>) {
			size_t total;
			if constexpr (Overwrite) {
				total = min(rng.size(), max_count);
			} else {
				if (size() == Capacity)
					return (0);
				total = min(Capacity - size(), min(rng.size(), max_count));
			}
			size_t my_end = _get_end_idx();
			if (my_end + total < Capacity) { // contiguous
				return (_do_write_unchecked(std::forward<Range>(rng), total));
			}
			// end + total > Capacity: wraps around
			if constexpr (Overwrite) {
				size_t written = 0;
				while (written < total) {
					size_t to_write = std::min(total - written, Capacity);

					if (my_end + to_write < Capacity) {
						written += _do_write_unchecked(std::forward<Range>(rng) | std::views::drop(written), to_write);
					} else {
						size_t first_pass = _do_write_unchecked(std::forward<Range>(rng) | std::views::drop(written), Capacity - my_end);
						written += first_pass;
						written += _do_write_unchecked(std::forward<Range>(rng) | std::views::drop(written), to_write - first_pass);
					}
					my_end = _get_end_idx();
				}
				return (written);
			} else {
				size_t first_pass = _do_write_unchecked(std::forward<Range>(rng), Capacity - my_end);

				return (first_pass + _do_write_unchecked(std::forward<Range>(rng) | std::views::drop(first_pass), total - first_pass));
			}
		} else {
			auto it = std::ranges::begin(rng);
			size_t i = 0;
			while (i < max_count && it != std::ranges::end(rng)) {
				if constexpr (std::is_rvalue_reference_v<Range>)push_back(*it);
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
	size_t peek_range(Range&& rng, size_t max_count) const {
		using range_t = std::remove_cvref_t<Range>;

		size_t to_read = min(size(), max_count);
		if constexpr (std::ranges::sized_range<range_t>) {
			assert(max_count <= std::ranges::size(rng));
			if (empty())
				return (0);
			if (_get_idx() + to_read < Capacity) { // contiguous
				return (_do_peek_unchecked(std::forward<Range>(rng), to_read));
			}
			// start + to_read > Size: wraps around
			size_t first_pass = _do_peek_unchecked(std::forward<Range>(rng), Capacity - _get_idx());

			_do_peek_unchecked(std::forward<Range>(rng) | std::views::drop(first_pass), to_read - first_pass, first_pass);
			return (to_read);
		} else {
			auto it = std::ranges::begin(rng);
			size_t i = 0;
			while (i < to_read && it != std::ranges::end(rng)) {
				*it = operator[](i);
				++i;
			}
			return (i);
		}
	}

	template <typename Range>
	size_t peek_range(Range&& rng) const {
		using range_t = std::remove_cvref_t<Range>;

		if constexpr (std::ranges::sized_range<range_t>) {
			return (peek_range(std::forward<Range>(rng), std::ranges::size(rng)));
		} else {
			auto it = std::ranges::begin(rng);
			size_t i = 0;
			while (it != std::ranges::end(rng)) {
				assert(i < size());
				*it = operator[](i);
				++i;
			}
			return (i);
		}
	}

	template <typename Range>
	size_t extract_range(Range&& rng) noexcept(std::is_nothrow_move_assignable_v<T>) {
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
	size_t append_range(Range&& rng) {
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

protected:
	T* _get_elem(size_t idx) {
		return (reinterpret_cast<T*>(buffer.data()) + idx);
	}

	T const* _get_elem(size_t idx) const {
		return (reinterpret_cast<T const*>(buffer.data()) + idx);
	}

	size_t _get_idx() const {
		return (static_cast<Self const *>(this)->_idx());
	}

	size_t _get_size() const {
		return (static_cast<Self const *>(this)->_size());
	}

	size_t _get_end_idx() const {
		size_t my_end = _get_idx() + size();
		if (my_end > Capacity)
			my_end -= Capacity;
		return (my_end);
	}

	void _set_idx(size_t new_idx) {
		return (static_cast<Self *>(this)->_idx(new_idx));
	}

	void _set_size(size_t new_size) {
		return (static_cast<Self *>(this)->_size(new_size));
	}

	template <typename Range>
	size_t _do_peek_unchecked(Range&& buf, size_t size, size_t skip = 0) const noexcept(noexcept(*std::declval<std::ranges::iterator_t<std::remove_cvref_t<Range>>>() = std::declval<T&&>()))
		requires (requires (std::ranges::iterator_t<std::remove_cvref_t<Range>> it, const T& elem) {*it = elem;}) {
		using value_t = std::ranges::range_value_t<std::remove_cvref_t<Range>>;
		size_t idx = _get_idx() + skip;

		if (idx >= capacity())
			idx -= capacity();
		T const *pos = _get_elem(idx);
		if constexpr (std::is_trivial_v<T> && std::is_same_v<T, value_t> && std::ranges::contiguous_range<std::remove_cvref_t<Range>>) {
			std::memcpy(std::ranges::data(buf), pos, size * sizeof(T));
		} else {
			std::copy(pos, pos + size, buf);
		}
		return (size);
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
	size_t _do_write_contiguous_nontrivial(Range&& buf, size_t size) noexcept(std::is_nothrow_constructible_v<T, std::ranges::range_value_t<std::remove_cvref_t<Range>>>) {
		using value_t = std::ranges::range_value_t<std::remove_cvref_t<Range>>;
		constexpr auto construct = [](T* where, value_t &v) {
			if constexpr (!std::is_lvalue_reference_v<Range> && !std::ranges::borrowed_range<std::remove_cvref_t<Range>>) {
				return (std::construct_at(where, std::move(v)));
			} else {
				return (std::construct_at(where, v));
			}
		};

		size_t my_end = _get_idx() + _get_size();
		if (my_end >= capacity())
			my_end -= capacity();
		T* pos = _get_elem(my_end);

		auto it = std::ranges::begin(buf);
		if constexpr (Overwrite) {
			assert(_get_size() <= capacity()); // should be a guaranted invariant - if this fails, something else in the implementation went wrong

			/* first: construct in empty places */
			size_t non_overwrite = min(size, capacity() - _get_size());
			size_t i = 0;
			while (i < non_overwrite) {
				construct(pos + i, *it);
				if constexpr (!std::is_nothrow_constructible_v<T, value_t>) {
					/** if constructor can throw, we want to update the size every time */
					_set_size(_get_size() + 1);
				}
				++i;
				it = std::ranges::next(it);
			}
			if constexpr (std::is_nothrow_constructible_v<T, value_t>) {
				/* if constructor can't throw, we can update it at the end  */
				_set_size(_get_size() + non_overwrite);
			}

			/*
			 * second: overwrite existing elements
			 * similarly, if the constructor can throw, we want to update index and size at every loop
			 * otherwise we can update it all at the end
			 * we are only overwriting so the size is not changing
			 */
			if constexpr (std::is_nothrow_constructible_v<T, value_t>) {
				while (i < size) {
					std::destroy_at(pos + i);
					construct(pos + i, *it);
					it = std::ranges::next(it);
					++i;
				}
				auto new_idx = _get_idx() + (size - non_overwrite);
				if (new_idx >= capacity())
					new_idx -= capacity();
				_set_idx(new_idx);
			} else {
				size_t idx = _get_idx();

				while (i < size) {
					std::destroy_at(pos + i);
					construct(pos + i, *it);
					++idx;
					_set_idx(idx);
					it = std::ranges::next(it);
					++i;
				}
				if (idx == capacity())
					_set_idx(0);
			}
		} else {
			for (size_t i = 0; i < size; ++i) {
				construct(pos + i, *it);
				it = std::ranges::next(it);
			}
			_set_size(_get_size() + size);
		}
		return (size);
	}

	template <typename Range>
	size_t _do_write_unchecked(Range&& buf, size_t size) noexcept(std::is_nothrow_constructible_v<T, std::ranges::range_value_t<std::remove_cvref_t<Range>>>) {
		using value_t = std::ranges::range_value_t<std::remove_cvref_t<Range>>;

		if constexpr (std::is_trivial_v<T> && std::is_same_v<T, value_t> && std::ranges::contiguous_range<std::remove_cvref_t<Range>>) {
			size_t my_end = _get_idx() + _get_size();
			if (my_end >= capacity())
				my_end -= capacity();
			T* pos = _get_elem(my_end);
			std::memcpy(pos, std::ranges::data(buf), size * sizeof(T));
			if constexpr (Overwrite) {
				if (size_t new_size = _get_size() + size; new_size > capacity()) {
					size_t new_idx = my_end + size;
					_set_idx(new_idx == capacity() ? 0 : new_idx);
					_set_size(capacity());
				} else {
					_set_size(new_size);
				}
			} else {
				_set_size(_get_size() + size);
			}
			return (size);
		} else {
			return (_do_write_contiguous_nontrivial(std::forward<Range>(buf), size));
		}
	}

	void _destroy(size_t idx, size_t count) {
		if constexpr (std::is_trivial_v<T>) {
			return; // no need to do anything, those are implicit lifetime types
		} else {
			T* as_t = reinterpret_cast<T*>(buffer.data());
			assert(idx <= capacity());
			assert(count <= capacity());
			size_t max_idx = idx + count;
			if (max_idx > capacity()) {
				while (idx < capacity()) {
					std::destroy_at<T>(as_t + idx);
					++idx;
				}
				idx = 0;
				max_idx -= capacity();
			}
			while (idx < max_idx) {
				std::destroy_at<T>(as_t + idx);
				++idx;
			}
		}
	}

	std::array<std::byte, sizeof(T[Capacity])> buffer alignas(T[Capacity]);
};

template <typename T, size_t Capacity, bool Overwrite>
class basic_ring_common : public basic_ring_base<basic_ring_common<T, Capacity, Overwrite>> {
public:
	using basic_ring_base<basic_ring_common>::basic_ring_base;
	using basic_ring_base<basic_ring_common>::operator=;

private:
	friend class basic_ring_base<basic_ring_common>;

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

template <typename T, size_t Capacity, bool Overwrite>
class basic_ring : public basic_ring_common<T, Capacity, Overwrite> {
public:
	using basic_ring_common<T, Capacity, Overwrite>::basic_ring_common;
	using basic_ring_common<T, Capacity, Overwrite>::operator=;
};

template <typename T, size_t Capacity, bool Overwrite>
requires (sizeof(T) == 1)
class basic_ring<T, Capacity, Overwrite> : public basic_ring_common<T, Capacity, Overwrite> {
public:
	using basic_ring_common<T, Capacity, Overwrite>::basic_ring_common;
	using basic_ring_common<T, Capacity, Overwrite>::operator=;

	/**
	 * \brief Read data from the buffer.
	 *
	 * \param buf Span of Ts to read into.
	 * \return size_t Number of Ts actually read.
	 */
	size_t read(std::span<T> buf) {
		return (this->extract_range(buf));
	}

	/**
	 * \brief Read data from the buffer.
	 *
	 * \param buf Span of Ts to read into.
	 * \param max_read Maximum size to read.
	 * \return size_t Number of Ts actually read.
	 */
	size_t read(std::span<T> buf, size_t max_read) {
		return (this->extract_range(buf, max_read));
	}

	/**
	 * \brief Read data from the buffer.
	 *
	 * \param buf Span of Ts to read into.
	 * \return size_t Number of Ts actually read.
	 */
	size_t peek(std::span<T> buf) {
		return (this->peek_range(buf));
	}

	/**
	 * \brief Read data from the buffer.
	 *
	 * \param buf Span of Ts to read into.
	 * \param max_read Maximum size to read.
	 * \return size_t Number of Ts actually read.
	 */
	size_t peek(std::span<T> buf, size_t max_read) const {
		return (this->peek_range(buf, max_read));
	}

	size_t write(std::span<T const> buf) {
		return (this->append_range(buf));
	}

	size_t write(std::span<T const> buf, size_t max_write) {
		return (this->append_range(buf, max_write));
	}
};

template <typename T, size_t Capacity>
using ring = basic_ring<T, Capacity, false>;

template <typename T, size_t Capacity>
using ouroboros = basic_ring<T, Capacity, true>;

}

#endif /* OCTAHEDRON_RING_H_ */
