#ifndef OCTAHEDRON_MATH_H_
#define OCTAHEDRON_MATH_H_

#include <type_traits>
#include <utility>

namespace octahedron
{
template <class T>
constexpr auto abs(T &&a) {
	if constexpr (std::unsigned_integral<T>)
		return (std::forward<T>(a));
	else
		return (a < 0 ? -a : a);
}

template <class T, class U> requires(!std::is_same_v<
	std::remove_reference_t<T>, std::remove_reference_t<U>>)
constexpr auto max(T &&a, U &&b) {
	using t = std::remove_reference_t<T>;
	using u = std::remove_reference_t<U>;

	if constexpr (std::is_integral_v<t> && std::is_integral_v<u>) {
		constexpr bool needs_unsigned = std::is_unsigned_v<t> != std::is_unsigned_v<u>;
		using smaller_type = std::conditional_t<(sizeof(t) > sizeof(u)), u, t>;
		using final_type =
			std::conditional_t<needs_unsigned, std::make_unsigned_t<smaller_type>, smaller_type>;

		auto a2 = static_cast<final_type>(a & std::numeric_limits<final_type>::max());
		auto b2 = static_cast<final_type>(b & std::numeric_limits<final_type>::max());

		return (a2 > b2 ? a2 : b2);
	} else {
		using type = std::common_type_t<T, U>;

		return (a > b ? static_cast<type>(a) : static_cast<type>(b));
	}
}

template <class T, class U> requires(std::is_same_v<
	std::remove_reference_t<T>, std::remove_reference_t<U>>)
constexpr auto max(T &&a, U &&b) {
	return (a > b ? a : b);
}

template <class T, class U, class V, class... Args>
constexpr auto max(T &&a, U &&b, V &&c, Args &&... args) {
	return (max(
		max(std::forward<T>(a), std::forward<U>(b)),
		std::forward<V>(c),
		std::forward<Args>(args)...
	));
}

template <class T, class U> requires(!std::is_same_v<
	std::remove_reference_t<T>, std::remove_reference_t<U>>)
constexpr auto min(T &&a, U &&b) {
	using t = std::remove_reference_t<T>;
	using u = std::remove_reference_t<U>;

	if constexpr (std::is_integral_v<t> && std::is_integral_v<u>) {
		constexpr bool needs_signed = std::is_unsigned_v<t> != std::is_unsigned_v<u>;
		using smaller_type = std::conditional_t<(sizeof(t) > sizeof(u)), u, t>;
		using final_type =
			std::conditional_t<needs_signed, std::make_signed_t<smaller_type>, smaller_type>;

		auto a2 = static_cast<final_type>(a & std::numeric_limits<final_type>::max());
		auto b2 = static_cast<final_type>(b & std::numeric_limits<final_type>::max());

		return (a2 < b2 ? a2 : b2);
	} else {
		using type = std::common_type_t<T, U>;

		return (a > b ? static_cast<type>(a) : static_cast<type>(b));
	}
}

template <class T, class U> requires(std::is_same_v<
	std::remove_reference_t<T>, std::remove_reference_t<U>>)
constexpr auto min(T &&a, U &&b) {
	using type = std::common_type<T, U>::type;

	return (a < b ? static_cast<type>(a) : static_cast<type>(b));
}

template <class T, class U, class V, class... Args>
constexpr auto min(T &&a, U &&b, V &&c, Args &&... args) {
	return (min(
		min(std::forward<T>(a), std::forward<U>(b)),
		std::forward<V>(c),
		std::forward<Args>(args)...
	));
}
}

#endif
