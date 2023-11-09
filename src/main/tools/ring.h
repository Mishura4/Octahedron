#ifndef OCTAHEDRON_RING_H_
#define OCTAHEDRON_RING_H_

#include <utility>
#include <cstddef>
#include <span>
#include <cstring>
#include <memory>

#include <tools/math.h>
#include <tools/util.h>

namespace octahedron {

/**
 * \brief <a href="https://en.wikipedia.org/wiki/Circular_buffer">Circular buffer</a>.
 *
 * A circular buffer is a FIFO container with a fixed capacity. It does this by having an internal buffer of a certain size, having a start and an end marker which are pushed on IO operations; such operations wrap around the actual start and end of the internal buffer.
 *
 * \tparam T CRTP type
 */
namespace detail {
template <typename T>
class basic_ring_base;

/**
 * \brief <a href="https://en.wikipedia.org/wiki/Circular_buffer">Circular buffer</a>.
 *
 * A circular buffer is a FIFO container with a fixed capacity. It does this by having an internal buffer of a certain size, having a start and an end marker which are pushed on IO operations; such operations wrap around the actual start and end of the internal buffer.
 *
 * \tparam T Type of object stored
 * \tparam Capacity Capacity of the buffer
 * \tparam Overwrite Whether to overwrite data on write past the end, or write 0
 */
template <template <typename, size_t, bool> typename Derived, typename T, size_t Capacity, bool Overwrite>
class basic_ring_base<Derived<T, Capacity, Overwrite>> {
	using self = Derived<T, Capacity, Overwrite>;

public:
	/**
	 * \brief Whether inserting operations using a certain set of arguments are noexcept
	 * \tparam Args Arguments to insert with
	 */
	template <typename... Args>
	inline static constexpr bool nothrow_insert = (!Overwrite || std::is_nothrow_destructible_v<T>) && std::is_nothrow_constructible_v<T, Args...>;

	/**
	 * \brief Whether extracting operations are noexcept
	 */
	inline static constexpr bool nothrow_extract = (!Overwrite || std::is_nothrow_destructible_v<T>) && std::is_nothrow_move_assignable_v<T>;

	/**
	 * \brief Whether non-consuming extracting operations are noexcept
	 */
	inline static constexpr bool nothrow_peek = (!Overwrite || std::is_nothrow_destructible_v<T>) && std::is_nothrow_copy_assignable_v<T>;

	/**
	 * \brief Returns the maximum capacity of the ring container as a constant expression.
	 * \return Capacity
	 */
	consteval static size_t capacity() noexcept {
		return Capacity;
	}

	/**
	 * \brief Type stored in this container
	 */
	using value_type = T;

	using iterator_type = random_access_iterator<Derived<T, Capacity, Overwrite>>;

	/**
	 * \brief Destructor. Destroys any elements still left in the container.
	 * \throw Any Re-throws exceptions thrown by the destroying elements.
	 */
	~basic_ring_base() noexcept(std::is_nothrow_destructible_v<T>) {
		_destroy(_get_idx(), _get_size());
	}

	/**
	 * \brief Random-access operator.
	 * \param idx Index to fetch.
	 * \return Reference to the element at index idx.
	 */
	constexpr T &operator[](size_t idx) noexcept {
		assert(idx < size());

		idx += _get_idx();
		if (idx >= capacity())
			idx -= capacity();
		return (*_get_elem(idx));
	}

	/**
	 * \brief Random-access operator.
	 * \param idx Index to fetch.
	 * \return Reference to the element at index idx.
	 */
	constexpr const T &operator[](size_t idx) const noexcept {
		assert(idx < size());

		idx += _get_idx();
		if (idx >= capacity())
			idx -= capacity();
		return (*_get_elem(idx));
	}

	/**
	 * \brief Returns an interator to the first element.
	 *
	 * If the container is empty, the returned iterator will be equal to the iterator returned by end().
	 */
	constexpr random_access_iterator<Derived<T, Capacity, Overwrite>&> begin() & noexcept {
		return {_as_derived(), 0};
	}

	/**
	 * \brief Returns an interator to the first element.
	 *
	 * If the container is empty, the returned iterator will be equal to the iterator returned by end().
	 */
	constexpr random_access_iterator<Derived<T, Capacity, Overwrite> const&> begin() const & noexcept {
		return {_as_derived(), 0};
	}

	/**
	 * \brief Returns an interator to the first element.
	 *
	 * If the container is empty, the returned iterator will be equal to the iterator returned by end().
	 */
	constexpr random_access_iterator<Derived<T, Capacity, Overwrite>&&> begin() && noexcept {
		return {_as_derived(), 0};
	}

	/**
	 * \brief Returns an interator to the first element.
	 *
	 * If the container is empty, the returned iterator will be equal to the iterator returned by end().
	 */
	constexpr random_access_iterator<Derived<T, Capacity, Overwrite> const&&> begin() const && noexcept {
		return {_as_derived(), 0};
	}

	/**
	 * \brief Returns an interator to the end of the container.
	 *
	 * \return An invalid iterator referring to the hypothetical element one past the end of the container.
	 */
	constexpr random_access_iterator<Derived<T, Capacity, Overwrite>> end() noexcept {
		return {_as_derived(), _get_size()};
	}

	/**
	 * \brief Returns an interator to the end of the container.
	 *
	 * \return An invalid iterator referring to the hypothetical element one past the end of the container.
	 */
	constexpr random_access_iterator<Derived<T, Capacity, Overwrite> const> end() const noexcept {
		return {_as_derived(), _get_size()};
	}

	/**
	 * \brief Emplace a value at the end of the container.
	 *
	 * If the container is \see Overwrite "overwriting" and is at capacity, this will overwrite the first element.
	 * If the container is \see Overwrite "not overwriting" and is at capacity, the behavior is undefined.
	 *
	 * \tparam Args Arguments to forward to the constructor of the element.
	 * \param args Arguments to forward to the constructor of the element.
	 * \throw Any Any exceptions thrown by the construction of elements or, if overwriting, the destruction of overwritten elements.
	 * \return A reference to the new element constructed.
	 */
	template <typename... Args>
	constexpr T& emplace_back(Args&&... args) noexcept(nothrow_insert<Args...>) {
		size_t idx = _get_idx() + size();

		if (idx >= capacity())
			idx -= capacity();

		T* where = _array() + idx;

		if constexpr (Overwrite) {
			if (size() == capacity()) {
				_destroy(where);
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

	/**
	 * \brief Access the first element of the container.
	 *
	 * \warn If the container is empty, the behavior is undefined.
	 * \return Reference to the first element of the container.
	 */
	constexpr T& front() noexcept {
		assert(!empty());

		return (*_get_elem(_get_idx()));
	}

	/**
	 * \brief Access the first element of the container.
	 *
	 * \warn If the container is empty, the behavior is undefined.
	 * \return Reference to the first element of the container.
	 */
	constexpr T const& front() const noexcept {
		assert(!empty());

		return (*_get_elem(_get_idx()));
	}

	/**
	 * \brief Remove the first element of the container.
	 *
	 * \warn If the container is empty, the behavior is undefined.
	 * \throw Any Any exceptions thrown by the the destruction of the element.
	 */
	constexpr void pop_front() noexcept(std::is_nothrow_destructible_v<T>) {
		assert(!empty());
		size_t idx = _get_idx();

		_destroy(idx);
		++idx;
		if (idx >= capacity())
			idx = 0;
		_set_idx(idx);
		_set_size(size() - 1);
	}

	/**
	 * \brief Copy a value to the end of the container.
	 *
	 * If the container is \see Overwrite "overwriting" and is at capacity, this will overwrite the first element.
	 * If the container is \see Overwrite "not overwriting" and is at capacity, the behavior is undefined.
	 *
	 * \param value Value to copy to the end of the container.
	 * \throw Any Any exceptions thrown by the construction of elements or, if overwriting, the destruction of overwritten elements.
	 * \return A reference to the new element constructed.
	 */
	constexpr T& push_back(std::add_const_t<T>& value) noexcept(nothrow_insert<std::add_const_t<T>&>) requires (std::is_copy_constructible_v<T>) {
		return (emplace_back(value));
	}

	/**
	 * \brief Move a value to the end of the container.
	 *
	 * If the container is \see Overwrite "overwriting" and is at capacity, this will overwrite the first element.
	 * If the container is \see Overwrite "not overwriting" and is at capacity, the behavior is undefined.
	 *
	 * \param value Value to move to the end of the container.
	 * \throw Any Any exceptions thrown by the construction of elements or, if overwriting, the destruction of overwritten elements.
	 * \return A reference to the new element constructed.
	 */
	constexpr T& push_back(T&& value) noexcept(nothrow_insert<T&&>) requires (std::is_move_constructible_v<T>) {
		return (emplace_back(std::move(value)));
	}

	/**
	 * \brief Try append a range to the end of the container.
	 *
	 * If the container is \see Overwrite "overwriting" and max_count is greater than `capacity() - size()`, this may overwrite elements.
	 *
	 * \param rng Range to append to the end of the container. If the range is not a <a href="https://en.cppreference.com/w/cpp/ranges/borrowed_range>borrowed range</a> and is an rvalue, the move constructor will be used to construct elements.
	 * \param max_count Maximum count of elements to add.
	 * \throw Any Any exceptions thrown by the construction of elements or, if overwriting, the destruction of overwritten elements.
	 * \return The count of elements actually added.
	 */
	template <typename Range>
	constexpr size_t append_range(Range&& rng, size_t max_count) noexcept(nothrow_insert<std::ranges::range_value_t<std::remove_cvref_t<Range>>>) {
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

	/**
	 * \brief Try extract a range from the start of the container.
	 *
	 * \param rng Range to extract the elements into.
	 * \param max_count Maximum count of elements to extract.
	 * \throw Any Any exceptions thrown by moving an element into the range.
	 * \return The count of elements actually extracted.
	 */
	template <typename Range>
	constexpr size_t extract_range(Range&& rng, size_t max_count) noexcept(nothrow_extract) {
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

	/**
	 * \brief Try copy a range from the start of the container.
	 *
	 * \param rng Range to extract the elements into.
	 * \param max_count Maximum count of elements to copy.
	 * \throw Any Any exceptions thrown by copying an element into the range.
	 * \return The count of elements actually extracted.
	 */
	template <typename Range>
	constexpr size_t peek_range(Range&& rng, size_t max_count) const noexcept(nothrow_peek) {
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

	/**
	 * \brief Try copy a range from the start of the container.
	 *
	 * \param rng Range to extract the elements into.
	 * \throw Any Any exceptions thrown by copying an element into the range.
	 * \return The count of elements actually extracted.
	 */
	template <typename Range>
	constexpr size_t peek_range(Range&& rng) const noexcept(nothrow_peek) {
		using range_t = std::remove_cvref_t<Range>;

		if constexpr (std::ranges::sized_range<range_t>) {
			return (peek_range(std::forward<Range>(rng), std::ranges::size(rng)));
		} else {
			auto it = std::ranges::begin(rng);
			size_t i = 0;
			while (it != std::ranges::end(rng)) {
				if (!(i < size()))
					return (i);
				*it = operator[](i);
				++i;
			}
			return (i);
		}
	}

	/**
	 * \brief Try extract a range from the start of the container.
	 *
	 * \param rng Range to extract the elements into.
	 * \throw Any Any exceptions thrown by moving an element into the range.
	 * \return The count of elements actually extracted.
	 */
	template <typename Range>
	constexpr size_t extract_range(Range&& rng) noexcept(nothrow_extract) {
		using range_t = std::remove_cvref_t<Range>;

		if constexpr (std::ranges::sized_range<range_t>) {
			return (extract_range(std::forward<Range>(rng), std::ranges::size(rng)));
		} else {
			auto it = std::ranges::begin(rng);
			size_t i = 0;
			while (it != std::ranges::end(rng)) {
				if (!(i < size()))
					return (i);
				*it = std::move(front());
				pop_front();
				++i;
			}
			return (i);
		}
	}

	/**
	 * \brief Try append a range to the end of the container.
	 *
	 * If the container is \see Overwrite "overwriting" and max_count is greater than `capacity() - size()`, this may overwrite elements.
	 *
	 * \param rng Range to append to the end of the container. If the range is not a <a href="https://en.cppreference.com/w/cpp/ranges/borrowed_range>borrowed range</a> and is an rvalue, the move constructor will be used to construct elements.
	 * \throw Any Any exceptions thrown by the construction of elements or, if overwriting, the destruction of overwritten elements.
	 * \return The count of elements actually added.
	 */
	template <typename Range>
	constexpr size_t append_range(Range&& rng) noexcept(nothrow_insert<std::ranges::range_value_t<std::remove_cvref_t<Range>>>) {
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

	/**
	 * \brief Get the current size of the container, in constant time (O(1)).
	 */
	constexpr size_t size() const noexcept {
		return (_get_size());
	}

	/**
	 * \brief Returns whether the container is empty or not.
	 * \return size() == 0.
	 */
	constexpr bool empty() const noexcept {
		return (size() == 0);
	}

	/**
	 * \brief Clears the container, destroying all elements.
	 *
	 * \throw Any exceptions thrown by destroying an element.
	 * \return The size of the container prior to clearing all elements.
	 */
	constexpr size_t clear() noexcept(std::is_nothrow_destructible_v<T>) {
		size_t ret = size();
		_destroy(_get_idx(), ret);
		_set_size(0);
		_set_idx(0);
		return (ret);
	}

protected:
	/**
	 * \brief Default constructor. Does not initialize any element.
	 */
	constexpr basic_ring_base() = default;

	/**
	 * \brief Base copy constructor is disabled.
	 */
	constexpr basic_ring_base(const basic_ring_base &) = delete;

	/**
	 * \brief Base move constructor is disabled.
	 */
	constexpr basic_ring_base(basic_ring_base&&) = delete;

	/**
	 * \brief Returns a pointer to the element at an absolute index.
	 *
	 * \warn The pointer returned does not necessarily point to a living object of type T.
	 * \param idx Absolute index of the pointer to retrieve.
	 */
	constexpr T* _get_elem(size_t idx) noexcept {
		return (reinterpret_cast<T*>(_array() + idx));
	}

	/**
	 * \brief Returns a pointer to the element at an absolute index.
	 *
	 * \warn The pointer returned does not necessarily point to a living object of type T.
	 * \param idx Absolute index of the pointer to retrieve.
	 */
	constexpr T const* _get_elem(size_t idx) const noexcept {
		return (reinterpret_cast<T const*>(_array() + idx));
	}

	/**
	 * \brief Returns the absolute index at which the virtual container starts.
	 */
	constexpr size_t _get_idx() const noexcept {
		return (static_cast<self const *>(this)->_idx());
	}

	/**
	 * \brief Returns the number of elements in the buffer.
	 */
	constexpr size_t _get_size() const noexcept {
		return (static_cast<self const *>(this)->_size());
	}

	/**
	 * \brief Returns the absolute index at which the virtual container ends.
	 */
	constexpr size_t _get_end_idx() const noexcept {
		size_t my_end = _get_idx() + size();
		if (my_end > Capacity)
			my_end -= Capacity;
		return (my_end);
	}

	/**
	 * \brief Sets the absolute index at which the virtual container starts.
	 * \param new_idx Absolute index to set the start of the virtual container to.
	 */
	constexpr void _set_idx(size_t new_idx) noexcept {
		return (static_cast<self *>(this)->_idx(new_idx));
	}

	/**
	 * \brief Sets the count of elements contained in the buffer.
	 * \param new_size New size of the container.
	 */
	constexpr void _set_size(size_t new_size) noexcept {
		return (static_cast<self *>(this)->_size(new_size));
	}

	/**
	 * \brief Execute a contiguous non-consuming extracting operation.
	 *
	 * \warn This function assumes the entire operation is contiguous and does not wrap around the buffer.
	 * \param buf Range of elements to copy into.
	 * \param size Size of the contiguous copy.
	 * \param skip Number of elements to skip over before copying.
	 * \return size_t Number of elements copied.
	 */
	template <typename Range>
	constexpr size_t _do_peek_unchecked(Range&& buf, size_t size, size_t skip = 0) const noexcept(noexcept(*std::declval<std::ranges::iterator_t<std::remove_cvref_t<Range>>>() = std::declval<T&&>()))
		requires (requires (std::ranges::iterator_t<std::remove_cvref_t<Range>> it, const T& elem) {*it = elem;}) {
		using value_t = std::ranges::range_value_t<std::remove_cvref_t<Range>>;
		size_t idx = _get_idx() + skip;

		if (idx >= capacity())
			idx -= capacity();
		assert(idx + size <= capacity());
		size_t i = 0;
		auto   it = std::ranges::begin(buf);
		while (i < size) {
			*it = *_get_elem(i + idx);
			it = std::ranges::next(it);
			++i;
		}
		return (size);
	}

	/**
	 * \brief Execute a contiguous extracting operation.
	 *
	 * \warn This function assumes the entire operation is contiguous and does not wrap around the buffer.
	 * \param buf Range of elements to copy into.
	 * \param size Size of the contiguous copy.
	 * \return size_t Number of elements copied.
	 */
	template <typename Range>
	constexpr size_t _do_read_unchecked(Range&& buf, size_t size) noexcept(noexcept(*std::declval<std::ranges::iterator_t<std::remove_cvref_t<Range>>>() = std::declval<T&&>())) {
		using value_t = std::ranges::range_value_t<std::remove_cvref_t<Range>>;
		size_t i = 0;

		auto   it = std::ranges::begin(buf);
		assert(_get_idx() + size <= capacity());
		while (i < size) {
			T* elem = _get_elem(_get_idx() + i);
			*it = std::move(*elem);
			std::destroy_at(elem);
			it = std::ranges::next(it);
			++i;
		}
		size_t new_idx = _get_idx() + size;
		_set_size(_get_size() - size);
		_set_idx(new_idx >= capacity() ? 0 : new_idx);
		return (size);
	}

	/**
	 * \brief Whether inserting a range into the container may throw.
	 * \tparam Range Range to insert.
	 * \tparam ForceMove Whether to use the move constructor to insert or not.
	 */
	template <typename Range, bool ForceMove>
	inline static constexpr bool _nothrow_insert_range = nothrow_insert<conditionally_apply_t<ForceMove, std::add_rvalue_reference<std::ranges::range_value_t<std::remove_cvref_t<Range>>>>>;

	/**
	 * \brief Execute a contiguous inserting operation of non-trivial types.
	 *
	 * \warn This function assumes the entire operation is contiguous and does not wrap around the buffer.
	 * \param buf Range of elements to insert.
	 * \param size Size of the contiguous insertion.
	 * \tparam Move Whether to use the move constructor to insert or not.
	 * \return size_t Number of elements copied.
	 */
	template <bool Move = false>
	constexpr size_t _do_write_contiguous_nontrivial(auto&& buf, size_t size) noexcept(_nothrow_insert_range<decltype(buf), Move>) {
		using range_t = std::remove_cvref_t<decltype(buf)>;
		using value_t = std::ranges::range_value_t<range_t>;
		constexpr auto construct = [](T* where, value_t &v) {
			if constexpr (Move) {
				return (std::construct_at(where, std::move(v)));
			} else {
				return (std::construct_at(where, v));
			}
		};

		size_t my_end = _get_idx() + _get_size();
		if (my_end >= capacity())
			my_end -= capacity();
		auto it = std::ranges::begin(buf);
		T*   pos = _array() + my_end;
		assert(my_end + size <= capacity());
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
			if constexpr (std::is_nothrow_constructible_v<T, value_t> && std::is_nothrow_destructible_v<T>) {
				while (i < size) {
					_destroy(pos + i);
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
					_destroy(pos + i);
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

	/**
	 * \brief Execute a contiguous inserting operation.
	 *
	 * \warn This function assumes the entire operation is contiguous and does not wrap around the buffer.
	 * \param buf Range of elements to insert.
	 * \param size Size of the contiguous insertion.
	 * \tparam ForceMove Whether to force using the move constructor to insert or not.
	 * \return size_t Number of elements copied.
	 */
	template <bool ForceMove = false>
	constexpr size_t _do_write_unchecked(auto* buf, size_t size) noexcept(nothrow_insert<conditionally_apply_t<ForceMove, std::add_rvalue_reference<std::remove_pointer<decltype(buf)>>>>) {
		return (_do_write_unchecked<ForceMove>(std::span{buf, size}, size));
	}

	/**
	 * \brief Execute a contiguous inserting operation.
	 *
	 * \warn This function assumes the entire operation is contiguous and does not wrap around the buffer.
	 * \param buf Range of elements to insert.
	 * \param size Size of the contiguous insertion.
	 * \tparam ForceMove Whether to force using the move constructor to insert or not.
	 * \return size_t Number of elements copied.
	 */
	template <bool ForceMove = false>
	constexpr size_t _do_write_unchecked(auto&& buf, size_t size) noexcept(_nothrow_insert_range<decltype(buf), ForceMove>) {
		using range_t = std::remove_cvref_t<decltype(buf)>;
		using value_t = std::ranges::range_value_t<range_t>;

		if constexpr (std::is_trivial_v<T> && std::is_same_v<T, value_t> && std::ranges::contiguous_range<range_t>) {
			size_t my_end = _get_idx() + _get_size();
			if (my_end >= capacity())
				my_end -= capacity();
			assert(my_end + size <= capacity());
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
			constexpr bool move = ForceMove || (!std::is_lvalue_reference_v<decltype(buf)> && !std::ranges::borrowed_range<range_t>);

			return (_do_write_contiguous_nontrivial<move>(std::forward<decltype(buf)>(buf), size));
		}
	}

	/**
	 * \brief Destroy an object contained in this container.
	 *
	 * \warn The behavior is undefined if `obj` is not within the container.
	 * \param obj Pointer to the object to destroy
	 */
	constexpr void _destroy(T *obj) noexcept(std::is_nothrow_destructible_v<T>) {
		obj->~T();
	}

	/**
	 * \brief Destroy an object contained in this container.
	 *
	 * \warn The behavior is undefined if `idx` does not refer to a valid object.
	 * \param idx Absolute index of the object to destroy.
	 */
	constexpr void _destroy(size_t idx) noexcept(std::is_nothrow_destructible_v<T>) {
		_destroy(_get_elem(idx));
	}

	/**
	 * \brief Destroy a potentially wrapping range of elements contained in this container.
	 *
	 * \warn The behavior is undefined if the range `idx + count` does not refer to a valid object.
	 * \param idx Absolute index of the object to destroy.
	 * \param count Number of elements to destroy. The operation will wrap around the buffer if needed.
	 */
	constexpr void _destroy(size_t idx, size_t count) noexcept(std::is_nothrow_destructible_v<T>) {
		if constexpr (std::is_trivial_v<T>) {
			return; // no need to do anything, those are implicit lifetime types
		} else {
			assert(idx <= capacity());
			assert(count <= _get_size());
			size_t max_idx = idx + count;
			if (max_idx > capacity()) {
				while (idx < capacity()) {
					_destroy(idx);
					++idx;
				}
				idx = 0;
				max_idx -= capacity();
			}
			while (idx < max_idx) {
				_destroy(idx);
				++idx;
			}
		}
	}

	/**
	 * \brief Logic for a copy or move constructor, cannot be used in this class due to calling methods of Derived, which is uninitialized at the point of construction.
	 *
	 * \tparam U Other container to copy or move from
	 * \param rhs Other container to copy or move from
	 */
	template <typename U>
	constexpr void _do_init(U &&rhs) {
		using other_value_type = typename std::remove_cvref_t<U>::value_type;
		constexpr size_t other_capacity = rhs.capacity();
		constexpr bool move = !std::is_lvalue_reference_v<U>;
		constexpr auto convert = [](auto begin, auto end) {
			return (std::span{begin, end} | std::views::transform([](auto&& c) -> auto& {
				return (*reinterpret_cast<other_value_type*>(c.data()));
			}));
		};

		size_t other_idx = rhs._get_idx();
		size_t other_sz = rhs._get_size();
		auto other_buf = rhs._array();

		if (other_idx + other_sz <= other_capacity) {
			_do_write_unchecked<move>(other_buf, other_sz);
		} else {
			size_t first_pass = _do_write_unchecked<move>(other_buf, (other_capacity - other_idx));

			_do_write_unchecked<move>(other_buf, (other_sz - first_pass));
		}
	}

	/**
	 * \brief Returns a valid pointer to the start of an array of T and size Capacity, located at the underlying buffer.
	 */
	T* _array() noexcept {
		return (*std::launder(reinterpret_cast<T(*)[Capacity]>(_buffer.data())));
	}

	/**
	 * \brief Returns a valid pointer to the start of an array of T and size Capacity, located at the underlying buffer.
	 */
	T const* _array() const noexcept {
		return (*std::launder(reinterpret_cast<T const(*)[Capacity]>(_buffer.data())));
	}

private:
	/**
	 * \brief Cast `this` to its CRTP class.
	 */
	[[msvc::intrinsic]] self *_as_derived() {
		return (static_cast<self*>(this));
	}

	/**
	 * \brief Cast `this` to its CRTP class.
	 */
	[[msvc::intrinsic]] self const *_as_derived() const {
		return (static_cast<self const *>(this));
	}

	/**
	 * \brief Underlying buffer. **Never** do any operation on this, use _array().
	 */
	std::array<std::byte, sizeof(T[Capacity])> _buffer alignas(std::array<T, Capacity>);
};

/**
 * \brief Basic implementation of basic_ring_base, suitable for a non-threadsafe ring buffer.
 *
 * \tparam T Type of each element
 * \tparam Capacity Number of elements the container can hold
 * \tparam Overwrite Whether to overwrite and push the start of the container when at capacity
 */
template <typename T, size_t Capacity, bool Overwrite>
class basic_ring_common : public basic_ring_base<basic_ring_common<T, Capacity, Overwrite>> {
public:
	using basic_ring_base<basic_ring_common>::operator=;

	/**
	 * \brief Default constructor, initializes an empty ring.
	 */
	constexpr basic_ring_common() = default;

	/**
	 * \brief Move constructor, moves the elements of another ring container into this.
	 *
	 * \param rhs Container to move from, left in an unspecified state.
	 */
	constexpr basic_ring_common(basic_ring_common&& rhs) noexcept (std::is_nothrow_move_constructible_v<T>)
		requires(std::is_move_constructible_v<T>) {
		this->_do_init(std::move(rhs));
	}

	/**
	 * \brief Copy constructor, copies the elements of another ring container into this.
	 *
	 * \param rhs Container to copy elements from.
	 */
	constexpr basic_ring_common(const basic_ring_common& rhs) noexcept (std::is_nothrow_copy_constructible_v<T>)
		requires(std::is_copy_constructible_v<T>) {
		this->_do_init(rhs);
	}

	/**
	 * \brief Generic move constructor, moves the elements of another ring container of lesser or equal capacity into this.
	 *
	 * \param rhs Container to move from, left in an unspecified state.
	 */
	template <typename T_, size_t Capacity_, bool Overwrite_>
	constexpr basic_ring_common(basic_ring_common<T_, Capacity_, Overwrite_>&& rhs)
		noexcept(std::is_nothrow_constructible_v<T, T_&&>)
	requires (std::constructible_from<T, T_&&> && rhs.capacity() <= this->capacity()) {
		this->_do_init(std::move(rhs));
	}

	/**
	 * \brief Generic copy constructor, copies the elements of another ring container of lesser or equal capacity into this.
	 *
	 * \param rhs Container to copy from.
	 */
	template <typename T_, size_t Capacity_, bool Overwrite_>
	constexpr basic_ring_common(const basic_ring_common<T_, Capacity_, Overwrite_>& rhs)
		noexcept(std::is_nothrow_constructible_v<T, std::add_const_t<T_>&>)
	requires (std::constructible_from<T, std::add_const_t<T_>&> && rhs.capacity() <= this->capacity()) {
		this->_do_init(rhs);
	}

	/**
	 * \brief Generic range constructor, equivalent to using `append_range` after default constructino.
	 *
	 * \param range The range to append to this container.
	 */
	template <typename Range>
	requires (std::ranges::range<std::remove_cvref_t<Range>>)
	constexpr basic_ring_common(Range&& range) noexcept(this->template nothrow_insert<std::ranges::range_value_t<std::remove_cvref_t<Range>>>) {
		this->append_range(range);
	}

	/**
	 * \brief Move asignment operator, moves the elements of another ring container into this.
	 *
	 * \param rhs Container to move from, left in an unspecified state.
	 */
	constexpr basic_ring_common &operator=(basic_ring_common&& rhs) noexcept (std::is_nothrow_destructible_v<T> && std::is_nothrow_move_constructible_v<T>)
		requires(std::is_move_constructible_v<T>) {
		this->clear();
		this->_do_init(std::move(rhs));
		return (*this);
	}

	/**
	 * \brief Copy asignment operator, copies the elements of another ring container into this.
	 *
	 * \param rhs Container to copy elements from.
	 */
	constexpr basic_ring_common &operator=(const basic_ring_common& rhs) noexcept (std::is_nothrow_destructible_v<T> && std::is_nothrow_copy_constructible_v<T>)
		requires(std::is_copy_constructible_v<T>) {
		this->clear();
		this->_do_init(rhs);
		return (*this);
	}

	/**
	 * \brief Generic move assignment operator, copies the elements of another ring container of lesser or equal capacity into this.
	 *
	 * \param rhs Container to copy from.
	 */
	template <typename T_, size_t Capacity_, bool Overwrite_>
	constexpr basic_ring_common &operator=(basic_ring_common<T_, Capacity_, Overwrite_>&& rhs)
		noexcept(std::is_nothrow_destructible_v<T> && std::is_nothrow_constructible_v<T, T_&&>)
	requires (std::constructible_from<T, T_&&> && rhs.capacity() <= this->capacity()) {
		this->clear();
		this->_do_init(std::move(rhs));
		return (*this);
	}

	/**
	 * \brief Generic copy assignment operator, copies the elements of another ring container of lesser or equal capacity into this.
	 *
	 * \param rhs Container to copy from.
	 */
	template <typename T_, size_t Capacity_, bool Overwrite_>
	constexpr basic_ring_common &operator=(const basic_ring_common<T_, Capacity_, Overwrite_>& rhs)
		noexcept(std::is_nothrow_destructible_v<T> && std::is_nothrow_constructible_v<T, std::add_const_t<T_>&>)
	requires (std::constructible_from<T, std::add_const_t<T_>&> && rhs.capacity() <= this->capacity()) {
		this->clear();
		this->_do_init(rhs);
		return (*this);
	}

private:
	friend class basic_ring_base<basic_ring_common>;

	/**
	 * \brief Returns the current size of the container.
	 */
	constexpr size_t _size() const noexcept {
		return (stored_size);
	}

	/**
	 * \brief Returns the absolute index of the start of the container.
	 */
	constexpr size_t _idx() const noexcept {
		return (start);
	}

	/**
	 * \brief Set the count of elements in the container. Does not actually create or destroy objects.
	 *
	 * \param new_size New count
	 */
	constexpr void _size(size_t new_size) noexcept {
		stored_size = new_size;
	}

	/**
	 * \brief Set the absolute index of the start of the container. Does not actually create or destroy objects.
	 * \param new_idx New start index
	 */
	constexpr void _idx(size_t new_idx) noexcept {
		start = new_idx;
	}

	/**
	 * \brief Current count of elements in the container.
	 */
	size_t stored_size{0};

	/**
	 * \brief Absolute index at which the container starts.
	 */
	size_t start{0};
};

}

/**
 * \brief <a href="https://en.wikipedia.org/wiki/Circular_buffer">Circular buffer</a>.
 *
 * A circular buffer is a FIFO container with a fixed capacity. It does this by having an internal buffer of a certain size, having a start and an end marker which are pushed on IO operations; such operations wrap around the actual start and end of the internal buffer.
 *
 * \tparam T Type of object stored
 * \tparam Capacity Capacity of the buffer
 * \tparam Overwrite Whether to overwrite data on write past the end, or write 0
 */
template <typename T, size_t Capacity, bool Overwrite>
class basic_ring : public detail::basic_ring_common<T, Capacity, Overwrite> {
public:
	using detail::basic_ring_common<T, Capacity, Overwrite>::basic_ring_common;
	using detail::basic_ring_common<T, Capacity, Overwrite>::operator=;
};

/**
 * \copydoc basic_ring
 */
template <typename T, size_t Capacity, bool Overwrite>
requires (sizeof(T) == 1)
class basic_ring<T, Capacity, Overwrite> : public detail::basic_ring_common<T, Capacity, Overwrite> {
public:
	using detail::basic_ring_common<T, Capacity, Overwrite>::basic_ring_common;
	using detail::basic_ring_common<T, Capacity, Overwrite>::operator=;

	/**
	 * \brief Read data from the buffer.
	 *
	 * \param buf Span of Ts to read into.
	 * \return size_t Number of Ts actually read.
	 */
	constexpr size_t read(std::span<T> buf) noexcept (this->nothrow_extract) {
		return (this->extract_range(buf));
	}

	/**
	 * \brief Read data from the buffer, consuming it.
	 *
	 * \param buf Span of Ts to read into.
	 * \param max_read Maximum size to read.
	 * \return size_t Number of Ts actually read.
	 */
	constexpr size_t read(std::span<T> buf, size_t max_read) noexcept (this->nothrow_extract) {
		return (this->extract_range(buf, max_read));
	}

	/**
	 * \brief Peek data from the buffer without consuming it.
	 *
	 * \param buf Span of Ts to read into.
	 * \return size_t Number of Ts actually read.
	 */
	constexpr size_t peek(std::span<T> buf) const noexcept (this->nothrow_peek) {
		return (this->peek_range(buf));
	}

	/**
	 * \brief Peek data from the buffer without consuming it.
	 *
	 * \param buf Span of Ts to read into.
	 * \param max_read Maximum size to read.
	 * \return size_t Number of Ts actually read.
	 */
	constexpr size_t peek(std::span<T> buf, size_t max_read) const noexcept (this->nothrow_peek) {
		return (this->peek_range(buf, max_read));
	}

	/**
	 * \brief Write data to the buffer.
	 *
	 * If the template parameter Overwrite is set to true, writing past the end of the buffer will overwrite and push the start of the buffer.
	 * Otherwise, nothing will be written.
	 *
	 * \param buf Data to write.
	 * \return size_t Number of elements written.
	 */
	constexpr size_t write(std::span<std::add_const_t<T>> buf) noexcept (this->template nothrow_insert<std::add_const_t<T>&>) {
		return (this->append_range(buf));
	}

	/**
	 * \brief Write data to the buffer.
	 *
	 * If the template parameter Overwrite is set to true, writing past the end of the buffer will overwrite and push the start of the buffer.
	 * Otherwise, nothing will be written.
	 *
	 * \param buf Data to write.
	 * \param max_write Maximum size to write.
	 * \return size_t Number of elements written.
	 */
	constexpr size_t write(std::span<std::add_const_t<T>> buf, size_t max_write) noexcept (this->template nothrow_insert<std::add_const_t<T>&>) {
		return (this->append_range(buf, max_write));
	}
};

/**
 * \brief <a href="https://en.wikipedia.org/wiki/Circular_buffer">Circular buffer</a>.
 *
 * A circular buffer is a FIFO container with a fixed capacity. It does this by having an internal buffer of a certain size, having a start and an end marker which are pushed on IO operations; such operations wrap around the actual start and end of the internal buffer.
 *
 * Operations on a full ring will be a no-op.
 *
 * \tparam T Type of object stored
 * \tparam Capacity Capacity of the buffer
 */
template <typename T, size_t Capacity>
using ring = basic_ring<T, Capacity, false>;

/**
 * \brief <a href="https://en.wikipedia.org/wiki/Circular_buffer">Circular buffer</a>.
 *
 * A circular buffer is a FIFO container with a fixed capacity. It does this by having an internal buffer of a certain size, having a start and an end marker which are pushed on IO operations; such operations wrap around the actual start and end of the internal buffer.
 *
 * Operations on a full ouroboros will overwrite the other end, and push the starting point.
 *
 * \tparam T Type of object stored
 * \tparam Capacity Capacity of the buffer
 */
template <typename T, size_t Capacity>
using ouroboros = basic_ring<T, Capacity, true>;

}

#endif /* OCTAHEDRON_RING_H_ */
