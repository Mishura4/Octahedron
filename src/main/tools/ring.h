#ifndef OCTAHEDRON_RING_H_
#define OCTAHEDRON_RING_H_

#include <utility>
#include <cstddef>
#include <span>
#include <cstring>
#include <memory>

#include <tools/math.h>

namespace octahedron {

template <class T, class U>
[[msvc::intrinsic]] [[nodiscard]] constexpr auto&& forward_like(U &&x) noexcept {
	constexpr bool is_adding_const = std::is_const_v<std::remove_reference_t<T>>;
	if constexpr (std::is_lvalue_reference_v<T&&>) {
		if constexpr (is_adding_const)
			return std::as_const(x);
		else
			return static_cast<U&>(x);
	} else {
		if constexpr (is_adding_const)
			return std::move(std::as_const(x));
		else
			return std::move(x);
	}
}

template <bool Condition, typename Trait>
struct conditionally_apply {

};

template <template <typename> typename Trait, typename T>
struct conditionally_apply<false, Trait<T>> {
	using type = T;
};

template <template <typename> typename Trait, typename T>
struct conditionally_apply<true, Trait<T>> {
	using type = typename Trait<T>::type;
};

template <bool Condition, typename T>
using conditionally_apply_t = typename conditionally_apply<Condition, T>::type;

namespace detail {

template <typename T>
class basic_ring_base;

template <template <typename, size_t, bool> typename Derived, typename T, size_t Capacity, bool Overwrite>
class basic_ring_base<Derived<T, Capacity, Overwrite>> {
	using self = Derived<T, Capacity, Overwrite>;

public:
	template <typename... Args>
	inline static constexpr bool nothrow_insert = (!Overwrite || std::is_nothrow_destructible_v<T>) && std::is_nothrow_constructible_v<T, Args...>;

	inline static constexpr bool nothrow_extract = (!Overwrite || std::is_nothrow_destructible_v<T>) && std::is_nothrow_move_assignable_v<T>;

	inline static constexpr bool nothrow_peek = (!Overwrite || std::is_nothrow_destructible_v<T>) && std::is_nothrow_copy_assignable_v<T>;

	consteval static size_t capacity() noexcept {
		return Capacity;
	}

	using value_type = T;

	~basic_ring_base() noexcept(std::is_nothrow_destructible_v<T>) {
		_destroy(_get_idx(), _get_size());
	}

	template <bool Const, bool RValue>
	struct iterator {
		using value_type = conditionally_apply_t<Const, std::add_const<T>>;
		using reference_type = std::conditional_t<RValue, std::add_rvalue_reference_t<value_type>, std::add_lvalue_reference_t<value_type>>;

		std::conditional_t<Const, std::add_const_t<basic_ring_base>, basic_ring_base> *container = nullptr;
		size_t idx{0};

		constexpr iterator &operator++() noexcept {
			++idx;
			return *this;
		}

		constexpr iterator operator++(int) noexcept {
			iterator ret{idx};
			++idx;
			return {ret};
		}

		constexpr iterator &operator--() noexcept {
			assert(idx > 0);
			--idx;
			return *this;
		}

		constexpr iterator operator--(int) noexcept {
			assert(idx > 0);
			iterator ret{idx};
			--idx;
			return {ret};
		}

		constexpr auto operator*() const noexcept -> reference_type {
			return (*container)[idx];
		}

		template <bool C1, bool C2, bool R1, bool R2>
		friend constexpr bool operator==(const iterator<C1, R1> &a, const iterator<C2, R2> &b) noexcept {
			assert(a.container == b.container);
			return (a.idx == b.idx);
		}
	};

	constexpr T &operator[](size_t idx) noexcept {
		assert(idx < size());

		idx += _get_idx();
		if (idx >= capacity())
			idx -= capacity();
		return (*_get_elem(idx));
	}

	constexpr const T &operator[](size_t idx) const noexcept {
		assert(idx < size());

		idx += _get_idx();
		if (idx >= capacity())
			idx -= capacity();
		return (*_get_elem(idx));
	}

	constexpr iterator<false, false> begin() & noexcept {
		return {this, 0};
	}

	constexpr iterator<true, false> begin() const & noexcept {
		return {this, 0};
	}

	constexpr iterator<false, true> begin() && noexcept {
		return {this, 0};
	}

	constexpr iterator<true, true> begin() const && noexcept {
		return {this, 0};
	}

	constexpr iterator<false, false> end() noexcept {
		return {this, _get_size()};
	}

	constexpr iterator<true, false> end() const noexcept {
		return {this, _get_size()};
	}

	template <typename... Args>
	constexpr T& emplace_back(Args&&... args) noexcept(nothrow_insert<Args...>) {
		size_t idx = _get_idx() + size();

		if (idx >= capacity())
			idx -= capacity();

		T* where = _buffer() + idx;

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

	constexpr T& front() noexcept {
		assert(!empty());

		return (*_get_elem(_get_idx()));
	}

	constexpr T const& front() const noexcept {
		assert(!empty());

		return (*_get_elem(_get_idx()));
	}

	constexpr void pop_front() noexcept {
		assert(!empty());
		size_t idx = _get_idx();

		std::destroy_at(_get_elem(idx));
		++idx;
		if (idx >= capacity())
			idx = 0;
		_set_idx(idx);
		_set_size(size() - 1);
	}

	constexpr T& push_back(std::add_const_t<T>& value) noexcept(nothrow_insert<std::add_const_t<T>&>) requires (std::is_copy_constructible_v<T>) {
		return (emplace_back(value));
	}

	constexpr T& push_back(T&& value) noexcept(nothrow_insert<T&&>) requires (std::is_move_constructible_v<T>) {
		return (emplace_back(std::move(value)));
	}

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

	template <typename Range>
	constexpr size_t peek_range(Range&& rng) const noexcept(nothrow_peek) {
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
	constexpr size_t extract_range(Range&& rng) noexcept(nothrow_extract) {
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

	constexpr size_t size() const noexcept {
		return (_get_size());
	}

	constexpr bool empty() const noexcept {
		return (!(size() > 0));
	}

	constexpr void reset() noexcept(std::is_nothrow_destructible_v<T>) {
		_destroy(_get_idx(), _get_size());
		_set_size(0);
		_set_idx(0);
	}

protected:
	constexpr basic_ring_base() = default;

	constexpr basic_ring_base(const basic_ring_base &) = delete;

	constexpr basic_ring_base(basic_ring_base&&) = delete;

	constexpr T* _get_elem(size_t idx) noexcept {
		return (reinterpret_cast<T*>(_buffer() + idx));
	}

	constexpr T const* _get_elem(size_t idx) const noexcept {
		return (reinterpret_cast<T const*>(_buffer() + idx));
	}

	constexpr size_t _get_idx() const noexcept {
		return (static_cast<self const *>(this)->_idx());
	}

	constexpr size_t _get_size() const noexcept {
		return (static_cast<self const *>(this)->_size());
	}

	constexpr size_t _get_end_idx() const noexcept {
		size_t my_end = _get_idx() + size();
		if (my_end > Capacity)
			my_end -= Capacity;
		return (my_end);
	}

	constexpr void _set_idx(size_t new_idx) noexcept {
		return (static_cast<self *>(this)->_idx(new_idx));
	}

	constexpr void _set_size(size_t new_size) noexcept {
		return (static_cast<self *>(this)->_size(new_size));
	}

	template <typename Range>
	constexpr size_t _do_peek_unchecked(Range&& buf, size_t size, size_t skip = 0) const noexcept(noexcept(*std::declval<std::ranges::iterator_t<std::remove_cvref_t<Range>>>() = std::declval<T&&>()))
		requires (requires (std::ranges::iterator_t<std::remove_cvref_t<Range>> it, const T& elem) {*it = elem;}) {
		using value_t = std::ranges::range_value_t<std::remove_cvref_t<Range>>;
		size_t idx = _get_idx() + skip;

		if (idx >= capacity())
			idx -= capacity();
		size_t i = 0;
		auto   it = std::ranges::begin(buf);
		while (i < size) {
			*it = *_get_elem(i + idx);
			it = std::ranges::next(it);
			++i;
		}
		return (size);
	}

	template <typename Range>
	constexpr size_t _do_read_unchecked(Range&& buf, size_t size) noexcept(noexcept(*std::declval<std::ranges::iterator_t<std::remove_cvref_t<Range>>>() = std::declval<T&&>())) {
		using value_t = std::ranges::range_value_t<std::remove_cvref_t<Range>>;
		size_t i = 0;

		auto   it = std::ranges::begin(buf);
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

	template <typename Range, bool ForceMove>
	inline static constexpr bool _nothrow_insert_range = nothrow_insert<conditionally_apply_t<ForceMove, std::add_rvalue_reference<std::ranges::range_value_t<std::remove_cvref_t<Range>>>>>;

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
		T*   pos = _buffer() + my_end;
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

	template <bool ForceMove = false>
	constexpr size_t _do_write_unchecked(auto* buf, size_t size) noexcept(nothrow_insert<conditionally_apply_t<ForceMove, std::add_rvalue_reference<std::remove_pointer<decltype(buf)>>>>) {
		return (_do_write_unchecked<ForceMove>(std::span{buf, size}, size));
	}

	template <bool ForceMove = false>
	constexpr size_t _do_write_unchecked(auto&& buf, size_t size) noexcept(_nothrow_insert_range<decltype(buf), ForceMove>) {
		using range_t = std::remove_cvref_t<decltype(buf)>;
		using value_t = std::ranges::range_value_t<range_t>;

		if constexpr (std::is_trivial_v<T> && std::is_same_v<T, value_t> && std::ranges::contiguous_range<range_t>) {
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
			constexpr bool move = ForceMove || (!std::is_lvalue_reference_v<decltype(buf)> && !std::ranges::borrowed_range<range_t>);

			return (_do_write_contiguous_nontrivial<move>(std::forward<decltype(buf)>(buf), size));
		}
	}

	constexpr void _destroy(T *obj) noexcept(std::is_nothrow_destructible_v<T>) {
		obj->~T();
	}

	constexpr void _destroy(size_t idx) noexcept(std::is_nothrow_destructible_v<T>) {
		_destroy(_get_elem(idx));
	}

	constexpr void _destroy(size_t idx, size_t count) noexcept(std::is_nothrow_destructible_v<T>) {
		if constexpr (std::is_trivial_v<T>) {
			return; // no need to do anything, those are implicit lifetime types
		} else {
			assert(idx <= capacity());
			assert(count <= capacity());
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

	template <typename T>
	constexpr void _do_init(T &&rhs) {
		using other_value_type = typename std::remove_cvref_t<T>::value_type;
		constexpr size_t other_capacity = rhs.capacity();
		constexpr bool move = !std::is_lvalue_reference_v<T>;
		constexpr auto convert = [](auto begin, auto end) {
			return (std::span{begin, end} | std::views::transform([](auto&& c) -> auto& {
				return (*reinterpret_cast<other_value_type*>(c.data()));
			}));
		};

		size_t other_idx = rhs._get_idx();
		size_t other_sz = rhs._get_size();
		auto other_buf = rhs._buffer();

		if (other_idx + other_sz <= other_capacity) {
			_do_write_unchecked<move>(other_buf, other_sz);
		} else {
			size_t first_pass = _do_write_unchecked<move>(other_buf, (other_capacity - other_idx));

			_do_write_unchecked<move>(other_buf, (other_sz - first_pass));
		}
	}

	T* _buffer() noexcept {
		return (*std::launder(reinterpret_cast<T(*)[Capacity]>(buffer.data())));
	}

	T const* _buffer() const noexcept {
		return (*std::launder(reinterpret_cast<T const(*)[Capacity]>(buffer.data())));
	}

	std::array<std::byte, sizeof(T[Capacity])> buffer alignas(std::array<T, Capacity>);
};

template <typename T, size_t Capacity, bool Overwrite>
class basic_ring_common;

template <typename T, size_t Capacity, bool Overwrite>
class basic_ring_common : public basic_ring_base<basic_ring_common<T, Capacity, Overwrite>> {
public:
	using basic_ring_base<basic_ring_common>::operator=;

	constexpr basic_ring_common() = default;

	constexpr basic_ring_common(basic_ring_common&& rhs) noexcept (std::is_nothrow_move_constructible_v<T>)
		requires(std::is_move_constructible_v<T>) {
		this->_do_init(std::move(rhs));
	}

	constexpr basic_ring_common(const basic_ring_common& rhs) noexcept (std::is_nothrow_copy_constructible_v<T>)
		requires(std::is_copy_constructible_v<T>) {
		this->_do_init(rhs);
	}

	template <typename T_, size_t Capacity_, bool Overwrite_>
	constexpr basic_ring_common(basic_ring_common<T_, Capacity_, Overwrite_>&& rhs)
		noexcept(std::is_nothrow_constructible_v<T, T_&&>)
	requires (std::constructible_from<T, T_&&> && rhs.capacity() <= this->capacity()) {
		this->_do_init(std::move(rhs));
	}

	template <typename T_, size_t Capacity_, bool Overwrite_>
	constexpr basic_ring_common(const basic_ring_common<T_, Capacity_, Overwrite_>& rhs)
		noexcept(std::is_nothrow_constructible_v<T, std::add_const_t<T_>&>)
	requires (std::constructible_from<T, std::add_const_t<T_>&> && rhs.capacity() <= this->capacity()) {
		this->_do_init(rhs);
	}

	template <typename Range>
	requires (std::ranges::range<std::remove_cvref_t<Range>>)
	constexpr basic_ring_common(Range&& range) noexcept(this->template nothrow_insert<std::ranges::range_value_t<std::remove_cvref_t<Range>>>) {
		this->append_range(range);
	}

	template <typename T_, size_t Capacity_, bool Overwrite_>
	constexpr basic_ring_common &operator=(basic_ring_common<T_, Capacity_, Overwrite_>&& rhs)
		noexcept(std::is_nothrow_constructible_v<T, T_&&>)
	requires (std::constructible_from<T, T_&&> && rhs.capacity() <= this->capacity()) {
		this->_do_init(std::move(rhs));
		return (*this);
	}

	template <typename T_, size_t Capacity_, bool Overwrite_>
	constexpr basic_ring_common &operator=(const basic_ring_common<T_, Capacity_, Overwrite_>& rhs)
		noexcept(std::is_nothrow_constructible_v<T, std::add_const_t<T_>&>)
	requires (std::constructible_from<T, std::add_const_t<T_>&> && rhs.capacity() <= this->capacity()) {
		this->_do_init(rhs);
		return (*this);
	}

private:
	friend class basic_ring_base<basic_ring_common>;

	constexpr size_t _size() const noexcept {
		return (stored_size);
	}

	constexpr size_t _idx() const noexcept {
		return (start);
	}

	constexpr void _size(size_t new_size) noexcept {
		stored_size = new_size;
	}

	constexpr void _idx(size_t new_idx) noexcept {
		start = new_idx;
	}

	size_t stored_size{0};
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
