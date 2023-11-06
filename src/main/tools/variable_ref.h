#ifndef OCTAHEDRON_VARIABLE_REF_H_
#define OCTAHEDRON_VARIABLE_REF_H_

#include <cassert>
#include <limits>

#include "Reference.h"

namespace octahedron
{

template <typename... PossibleTypes> requires (sizeof...(PossibleTypes) > 0)
struct variable_ref {
	using possible_types = octahedron::type_list<PossibleTypes...>;
	using index_type = std::size_t;

	static constexpr size_t npos = std::numeric_limits<std::size_t>::max();

	template <typename T>
	consteval static index_type index_of() noexcept {
		return (static_cast<index_type>(possible_types::template type_index<T>));
	}

	template <typename T>
	consteval static bool has_alternative() noexcept {
		return (index_of<T>() != npos);
	}

	template <typename T>
	constexpr bool holds_type() const {
		return (index_of<T>() == type_index);
	}

	template <typename T> requires (has_alternative<T>())
	constexpr variable_ref(T &value) noexcept :
		ptr{&value},
		type_index{index_of<T>()} { }

	template <size_t N> requires (N < possible_types::size)
	constexpr possible_types::template at<N>& get() const noexcept {
		assert(type_index == N);

		return (*static_cast<possible_types::template at<N>*>(ptr));
	}

	template <typename T> requires (has_alternative<T>())
	constexpr T& get() const noexcept {
		assert(type_index == index_of<T>());

		return (*static_cast<T*>(ptr));
	}

	template <size_t N> requires (N < possible_types::size)
	constexpr possible_types::template at<N>& at() const {
		if (type_index != N)
			throw std::bad_variant_access();
		return (*static_cast<possible_types::template at<N>*>(ptr));
	}

	template <typename T> requires (has_alternative<T>())
	constexpr T& at() const {
		if (type_index != index_of<T>())
			throw std::bad_variant_access();
		return (*static_cast<T*>(ptr));
	}

	template <size_t N> requires (N < possible_types::size)
	constexpr possible_types::template at<N> *try_get() const noexcept {
		if (type_index != N)
			return (nullptr);
		return (static_cast<possible_types::template at<N>*>(ptr));
	}

	template <typename T> requires (has_alternative<T>())
	constexpr T *try_get() const noexcept {
		if (type_index != index_of<T>())
			return (nullptr);
		return (static_cast<T*>(ptr));
	}

	void *            ptr;
	octahedron::int32 type_index;
};

template <typename T>
struct variable_ref<T> : public Reference<T> {
	using pcossible_types = octahedron::type_list<T>;
	using index_type = octahedron::int32;

	static constexpr size_t npos = std::numeric_limits<std::size_t>::max();

	using Reference<T>::reference;
	using Reference<T>::operator=;
	using Reference<T>::get;

	template <typename U>
	consteval static index_type index_of() noexcept {
		if constexpr (std::is_same_v<T, U>)
			return (0);
		else
			return (npos);
	}

	template <typename U>
	consteval bool holds_type() const {
		return (std::is_same_v<T, U>);
	}

	template <typename U>
	consteval static bool has_alternative() noexcept {
		return (std::is_same_v<T, U>);
	}

	template <size_t N> requires (N == 0)
	constexpr T& get() const noexcept {
		return (Reference<T>::get());
	}

	template <typename U> requires (has_alternative<U>())
	constexpr U& get() const noexcept {
		return (Reference<T>::get());
	}

	template <size_t N> requires (N == 0)
	constexpr T& at() const noexcept {
		return (Reference<T>::get());
	}

	template <typename U> requires (has_alternative<U>())
	constexpr U& at() const noexcept {
		return (Reference<T>::get());
	}

	template <size_t N> requires (N == 0)
	constexpr T *try_get() const noexcept {
		return (&Reference<T>::get());
	}

	template <typename U> requires (has_alternative<U>())
	constexpr U *try_get() const noexcept {
		return (&Reference<T>::get());
	}
};

} // namespace octahedron

#endif
