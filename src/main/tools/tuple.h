#ifndef OCTAHEDRON_TUPLE_H_
#define OCTAHEDRON_TUPLE_H_

#include <type_traits>
#include <utility>

namespace octahedron {

namespace detail {

template <size_t, typename _T>
struct _tuple_node {
	_T data;
};

}

namespace detail {

template <typename... Args>
struct _tuple_impl;

template <size_t... Ns, typename... Args>
struct _tuple_impl<std::index_sequence<Ns...>, Args...> : _tuple_node<Ns, Args>... {
};

template <size_t N, typename T>
auto get(_tuple_node<N, T>& tuple) -> std::add_lvalue_reference_t<T> {
	return tuple.data;
}

template <size_t N, typename T>
auto get(_tuple_node<N, T> const& tuple) -> std::add_lvalue_reference_t<std::add_const_t<T>> {
	return tuple.data;
}

template <size_t N, typename T>
auto get(_tuple_node<N, T>&& tuple) -> std::add_rvalue_reference_t<T> {
	return static_cast<std::add_rvalue_reference_t<T>>(tuple.data);
}

template <typename T, size_t N>
auto get(_tuple_node<N, T>& tuple) -> std::add_lvalue_reference_t<T> {
	return tuple.data;
}

template <typename T, size_t N>
auto get(_tuple_node<N, T> const& tuple) -> std::add_lvalue_reference_t<std::add_const_t<T>> {
	return tuple.data;
}

template <typename T, size_t N>
auto get(_tuple_node<N, T>&& tuple) -> std::add_rvalue_reference_t<T> {
	return static_cast<std::add_rvalue_reference_t<T>>(tuple.data);
}

}

template <typename... Args>
struct tuple : detail::_tuple_impl<std::make_index_sequence<sizeof...(Args)>, Args...> {
	template <typename... Args_>
	requires(sizeof...(Args) == sizeof...(Args_))
	constexpr tuple& operator=(const tuple<Args_...>& other) noexcept((std::is_nothrow_assignable_v<Args, Args_> && ...)) {
		[]<size_t... Ns>(std::index_sequence<Ns...>, tuple& me, const tuple<Args_...>& othr) noexcept((std::is_nothrow_assignable_v<Args, Args_> && ...)) {
			(((detail::get<Ns>(me) = detail::get<Ns>(othr))), ...);
		}(std::make_index_sequence<sizeof...(Args)>{}, *this, other);
		return *this;
	}

	template <typename... Args_>
	requires(sizeof...(Args) == sizeof...(Args_))
	constexpr tuple& operator=(tuple<Args_...>&& other) noexcept((std::is_nothrow_assignable_v<Args, Args_&&> && ...)) {
		[]<size_t... Ns>(std::index_sequence<Ns...>, tuple& me, tuple<Args_...>&& othr) noexcept((std::is_nothrow_assignable_v<Args, Args_&&> && ...)) {
			(((detail::get<Ns>(me) = detail::get<Ns>(static_cast<decltype(othr)&&>(othr)))), ...);
		}(std::make_index_sequence<sizeof...(Args)>{}, *this, std::move(other));
		return *this;
	}

	template <typename... Args_>
	requires(sizeof...(Args) == sizeof...(Args_))
	constexpr tuple& operator=(const std::tuple<Args_...>& other) noexcept((std::is_nothrow_assignable_v<Args, Args_> && ...)) {
		[]<size_t... Ns>(std::index_sequence<Ns...>, tuple& me, const tuple<Args_...>& othr) noexcept((std::is_nothrow_assignable_v<Args, Args_> && ...)) {
			(((detail::get<Ns>(me) = detail::get<Ns>(othr))), ...);
		}(std::make_index_sequence<sizeof...(Args)>{}, *this, other);
		return *this;
	}

	template <typename... Args_>
	requires(sizeof...(Args) == sizeof...(Args_))
	constexpr tuple& operator=(std::tuple<Args_...>&& other) noexcept((std::is_nothrow_assignable_v<Args, Args_&&> && ...)) {
		[]<size_t... Ns>(std::index_sequence<Ns...>, tuple& me, std::tuple<Args_...>&& othr) noexcept((std::is_nothrow_assignable_v<Args, Args_&&> && ...)) {
			(((detail::get<Ns>(me) = detail::get<Ns>(static_cast<decltype(othr)&&>(othr)))), ...);
		}(std::make_index_sequence<sizeof...(Args)>{}, *this, std::move(other));
		return *this;
	}
};

template <typename T, typename U>
struct tuple<T, U> : detail::_tuple_impl<std::index_sequence<0, 1>, T, U> {
	template <typename T_, typename U_>
	constexpr tuple& operator=(const tuple<T_, U_>& other) noexcept(std::is_nothrow_assignable_v<T, T_> && std::is_nothrow_assignable_v<U, U_>) {
		detail::get<0>(*this) = detail::get<0>(other);
		detail::get<1>(*this) = detail::get<1>(other);
		return *this;
	}

	template <typename T_, typename U_>
	constexpr tuple& operator=(tuple<T_, U_>&& other) noexcept(std::is_nothrow_assignable_v<T, T_&&> && std::is_nothrow_assignable_v<U, U_&&>) {
		detail::get<0>(*this) = std::forward<T_>(detail::get<0>(other));
		detail::get<1>(*this) = std::forward<U_>(detail::get<1>(other));
		return *this;
	}

	template <typename T_, typename U_>
	constexpr tuple& operator=(const std::tuple<T_, U_>& other) noexcept(std::is_nothrow_assignable_v<T, T_> && std::is_nothrow_assignable_v<U, U_>) {
		detail::get<0>(*this) = std::get<0>(other);
		detail::get<1>(*this) = std::get<1>(other);
		return *this;
	}

	template <typename T_, typename U_>
	constexpr tuple& operator=(std::tuple<T_, U_>&& other) noexcept(std::is_nothrow_assignable_v<T, T_&&> && std::is_nothrow_assignable_v<U, U_&&>) {
		detail::get<0>(*this) = std::forward<T_>(std::get<0>(other));
		detail::get<1>(*this) = std::forward<U_>(std::get<1>(other));
		return *this;
	}

	template <typename T_, typename U_>
	constexpr tuple& operator=(const std::pair<T_, U_>& other) noexcept(std::is_nothrow_assignable_v<T, T_> && std::is_nothrow_assignable_v<U, U_>) {
		detail::get<0>(*this) = std::forward<T_>(other.first);
		detail::get<1>(*this) = std::forward<U_>(other.second);
		return *this;
	}

	template <typename T_, typename U_>
	constexpr tuple& operator=(std::pair<T_, U_>&& other) noexcept(std::is_nothrow_assignable_v<T, T_&&> && std::is_nothrow_assignable_v<U, U_&&>) {
		detail::get<0>(*this) = std::forward<T_>(other.first);
		detail::get<1>(*this) = std::forward<U_>(other.second);
		return *this;
	}
};

template <size_t N, typename... Args>
auto get(tuple<Args...>& tuple) -> decltype(auto) {
	return detail::get<N>(tuple);
}

template <size_t N, typename... Args>
auto get(tuple<Args...> const& tuple) -> decltype(auto) {
	return detail::get<N>(tuple);
}

template <size_t N, typename... Args>
auto get(tuple<Args...>&& tuple) -> decltype(auto) {
	return detail::get<N>(static_cast<decltype(tuple)&&>(tuple));
}

template <typename T, typename... Args>
decltype(auto) get(tuple<Args...>& tuple) {
	return detail::get<T>(tuple);
}

template <typename T, typename... Args>
decltype(auto) get(const tuple<Args...>& tuple) {
	return detail::get<T>(tuple);
}

template <typename T, typename... Args>
decltype(auto) get(tuple<Args...>&& tuple) {
	return detail::get<T>(static_cast<decltype(tuple)&&>(tuple));
}

template <typename... Args>
tuple<Args&...> tie(Args&... args) noexcept {
	return {args...};
}

}

#endif 
