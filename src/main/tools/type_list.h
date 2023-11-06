#ifndef OCTAHEDRON_TYPE_LIST_H_
#define OCTAHEDRON_TYPE_LIST_H_

#include <concepts>
#include <tuple>
#include <type_traits>
#include <variant>

namespace octahedron
{
template <typename... Args>
struct type_list;

template <typename... Args> requires (sizeof...(Args) > 0)
consteval bool is_common_with();

template <typename T, typename U, typename... Args>
consteval bool is_common_with_fun() {
	if constexpr (!std::common_with<T, U>)
		return (false);
	if constexpr (sizeof...(Args) == 0)
		return (true);
	else
		return (is_common_with<U, Args...>());
}

template <typename... Args> requires (sizeof...(Args) > 0)
consteval bool is_common_with() {
	if constexpr (sizeof...(Args) < 2)
		return (true);
	else
		return (is_common_with_fun<Args...>());
}

namespace _
{
template <typename T, typename U, typename... Args>
struct type_duplicates_helper_s {
	static consteval auto is_duplicate_fun() {
		if constexpr (std::is_same_v<T, U>)
			return (true);
		if constexpr (sizeof...(Args) > 0)
			return (type_duplicates_helper_s<T, Args...>::is_duplicate_fun());
		else
			return (false);
	}

	static consteval auto has_duplicates_fun() {
		if constexpr (sizeof...(Args) > 0) {
			if constexpr (type_duplicates_helper_s<U, Args...>::is_duplicate_fun())
				return (true);
		}
		return (type_duplicates_helper_s<T, U, Args...>::is_duplicate_fun());
	}
};

template <typename... Args>
consteval bool has_duplicate_types_fun() {
	if constexpr (sizeof...(Args) < 2)
		return (false);
	else
		return type_duplicates_helper_s<Args...>::has_duplicates_fun();
};

template <typename... Args>
consteval bool is_duplicate_type_fun() {
	if constexpr (sizeof...(Args) < 2)
		return (false);
	else
		return type_duplicates_helper_s<Args...>::is_duplicate_fun();
};
}

template <typename... Args>
constexpr bool has_duplicate_types = _::has_duplicate_types_fun<Args...>();

template <typename... Args>
constexpr bool is_duplicate_type = _::is_duplicate_type_fun<Args...>();

namespace _
{
template <typename T, typename... Args>
struct tuple_without_duplicates_s;

template <typename T>
struct tuple_without_duplicates_s<T> {
	using type = std::tuple<T>;
};

template <typename T, typename... Args>
struct tuple_without_duplicates_s {
	using type = std::conditional_t<is_duplicate_type<T, Args...>,
																	typename tuple_without_duplicates_s<Args...>::type,
																	decltype(std::tuple_cat(
																		std::declval<std::tuple<T>>(),
																		std::declval<typename tuple_without_duplicates_s<Args
																			...>::type>()
																	))
	>;
};

template <typename T>
struct to_type_list_s;

template <typename... Args>
struct to_type_list_s<std::tuple<Args...>> {
	using type = type_list<Args...>;
};

template <bool has_duplicates, typename... Args>
struct type_list_duplicate_helper {
	using type = typename to_type_list_s<typename _::tuple_without_duplicates_s<Args...>::type>::type;
};

template <typename... Args>
struct type_list_duplicate_helper<false, Args...> {
	using type = type_list<Args...>;
};

template <bool has_common_type, typename... Args>
struct common_type_helper;

template <typename... Args>
struct common_type_helper<true, Args...> {
	using type = std::common_type_t<Args...>;
};

template <typename... Args>
struct common_type_helper<false, Args...> {
	using type = void;
};

template <typename lhs, typename rhs>
struct type_list_cat_helper;

template <typename rhs, typename... LArgs>
struct type_list_cat_helper<type_list<LArgs...>, rhs> {
	template <typename T>
	struct _helper;

	template <typename... RArgs>
	struct _helper<type_list<RArgs...>> {
		using cat = type_list<LArgs..., RArgs...>;
	};

	using type = typename _helper<rhs>::cat;
};
}

template <typename... Args>
struct type_list;

template <typename T, typename... Args>
struct type_list<T, Args...> : type_list<Args...> {
protected:
	template <typename U>
	static consteval size_t type_index_fun() {
		constexpr size_t npos = std::numeric_limits<size_t>::max();

		if constexpr (std::is_same_v<T, U>)
			return (0);
		else if constexpr (constexpr auto pos = type_list<Args...>::template type_index_fun<U>();
			pos == npos)
			return (npos);
		else // constexpr
			return (1 + pos);
	}

public:
	static constexpr size_t size = 1 + sizeof...(Args);

	using as_tuple = std::tuple<T, Args...>;
	using as_variant = std::variant<T, Args...>;

	template <size_t N> requires (N < size)
	using at = std::tuple_element_t<N, as_tuple>;

	static constexpr bool has_common_conversion = is_common_with<T, Args...>();

	using common_conversion = typename _::common_type_helper<has_common_conversion, T, Args...>::type;

	constexpr static bool has_duplicate_types = has_duplicate_types < T, Args
	...
	>;

	using without_duplicate_types = typename _::type_list_duplicate_helper<
		has_duplicate_types, T, Args...>::type;

	template <typename rhs>
	using cat = typename _::type_list_cat_helper<type_list<T, Args...>, rhs>::type;

	template <typename U>
	static constexpr size_t type_index = type_index_fun<U>();

	template <typename U>
	static constexpr bool has_type = (type_index<U> != std::numeric_limits<size_t>::max());
};

template <>
struct type_list<> {
	static constexpr size_t size = 0;

	using as_tuple = std::tuple<>;
	using as_variant = std::variant<>;

	static constexpr bool has_common_conversion = false;

	using common_conversion = void;

	constexpr static bool has_duplicate_types = false;

	using without_duplicate_types = type_list<>;

	template <typename U>
	static consteval size_t type_index_fun() {
		return (std::numeric_limits<size_t>::max());
	}

	template <typename rhs>
	using cat = typename _::type_list_cat_helper<type_list<>, rhs>::type;

	template <typename U>
	static constexpr size_t type_index = std::numeric_limits<size_t>::max();

	template <typename U>
	static constexpr bool has_type = (type_index<U> != std::numeric_limits<size_t>::max());
};

template <typename T>
using to_type_list = typename _::to_type_list_s<T>::type;
}

#endif
