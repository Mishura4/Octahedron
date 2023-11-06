#ifndef OCTAHEDRON_STRING_LITERAL_H_
#define OCTAHEDRON_STRING_LITERAL_H_

#include <algorithm>
#include <utility>

namespace octahedron
{

template <std::size_t N>
struct u8string_literal;

template <typename CharT, size_t N>
struct basic_string_literal {
	constexpr inline static size_t size{N};
	using char_type = CharT;

	template <typename T> requires (std::is_pointer_v<T> && sizeof(std::remove_pointer_t<T>) <= sizeof
																	(CharT))
	constexpr basic_string_literal(T str) {
		std::ranges::copy_n(str, N, data);
		data[N] = 0;
	}

	template <typename OtherChar, size_t... Is> requires (sizeof(CharT) >= sizeof(OtherChar))
	constexpr basic_string_literal(const OtherChar *str, std::index_sequence<Is...>) :
		data{str[Is]...} { }

	template <typename OtherChar> requires (sizeof(CharT) >= sizeof(OtherChar))
	constexpr basic_string_literal(const OtherChar (&arr)[N + 1]) :
		basic_string_literal{arr, std::make_index_sequence<N>()} { }

	template <typename OtherChar> requires (sizeof(CharT) >= sizeof(OtherChar))
	constexpr basic_string_literal(const basic_string_literal<OtherChar, N> &rhs) :
		basic_string_literal{rhs.data, std::make_index_sequence<N>()} { }

	template <typename CharT2, size_t N2>
	constexpr bool operator==(const basic_string_literal<CharT2, N2> &other) const {
		if constexpr (N2 != N)
			return (false);
		else {
			for (size_t i = 0; i < N; ++i) {
				if (data[i] != other.data[i])
					return (false);
			}
			return (true);
		}
	}

	template <typename CharT2, size_t N2>
	constexpr bool strict_equals(const basic_string_literal<CharT2, N2> &other) const {
		if constexpr (!std::same_as<CharT, CharT2>)
			return (false);
		else
			return (operator==(other));
	}

	template <typename CharT2, size_t N2>
	constexpr bool operator<=>(const basic_string_literal<CharT2, N2> &other) const {
		if constexpr (N2 != N)
			return (false);
		else
			return (std::ranges::lexicographical_compare(data, other.data));
	}

	constexpr operator std::basic_string_view<CharT>() const {
		return {data, N};
	}

	constexpr operator std::basic_string<CharT>() const {
		return {data, N};
	}

	friend constexpr auto format_as(const basic_string_literal &t) noexcept {
		return std::basic_string_view<CharT>{t};
	}

	CharT data[N + 1];
};

template <typename CharT, std::size_t N>
basic_string_literal(const CharT (&)[N]) -> basic_string_literal<CharT, N - 1>;

template <std::size_t N>
struct string_literal : basic_string_literal<char, N> {
	using base = basic_string_literal<char, N>;

	template <typename OtherChar> requires (sizeof(char) >= sizeof(OtherChar))
	consteval string_literal(const basic_string_literal<OtherChar, N> &rhs) :
		string_literal{rhs.data, std::make_index_sequence<N>()} { }

	using typename base::char_type;
	using base::base;
	using base::operator<=>;
	using base::operator==;

	constexpr operator base() const {
		return {*this};
	}
};

template <typename CharT, std::size_t N>
string_literal(const CharT (&)[N]) -> string_literal<N - 1>;

template <typename CharT, std::size_t N>
string_literal(const basic_string_literal<CharT, N> &) -> string_literal<N>;

template <std::size_t N>
basic_string_literal(const string_literal<N> &) -> basic_string_literal<char, N>;

template <std::size_t N>
struct u8string_literal : basic_string_literal<char8_t, N> {
	using base = basic_string_literal<char8_t, N>;

	using typename base::char_type;
	using base::base;
	using base::operator<=>;
	using base::operator==;

	constexpr operator base() const {
		return {*this};
	}
};

template <typename CharT, std::size_t N>
u8string_literal(const CharT (&)[N]) -> u8string_literal<N - 1>;

template <typename CharT, std::size_t N>
u8string_literal(const basic_string_literal<CharT, N> &) -> u8string_literal<N>;

template <std::size_t N>
basic_string_literal(const u8string_literal<N> &) -> basic_string_literal<char8_t, N>;

template <typename T>
constexpr bool is_string_literal = false;

template <template <size_t> typename T, size_t N> requires (requires { typename T<N>::char_type; })
constexpr bool is_string_literal<T<N>> = std::is_base_of_v<
	basic_string_literal<typename T<N>::char_type, N>, T<N>>;

template <template <typename, size_t> typename T, typename CharT, size_t N> requires (requires {
	typename T<CharT, N>::char_type;
})
constexpr bool is_string_literal<T<CharT, N>> = std::is_base_of_v<
	basic_string_literal<CharT, N>, T<CharT, N>>;

template <typename T>
constexpr bool is_string_literal<const T> = is_string_literal<T>;

namespace _
{

struct literal_concat {
	template <typename T>
	constexpr static size_t get_size() {
		using type = std::remove_reference_t<T>;

		if constexpr (is_string_literal<type>)
			return (type::size);
		else if (std::is_array_v<type>)
			return (std::extent_v<type> - 1);
	};

	template <typename T>
	constexpr static decltype(auto) convert(T &&v) {
		using type = std::remove_reference_t<T>;

		if constexpr (is_string_literal<type>)
			return (v);
		else if (std::is_array_v<type>)
			return basic_string_literal{v};
	};

	template <typename CharT, size_t N>
	constexpr static auto insert(const basic_string_literal<CharT, N> &lit, auto it) {
		return (std::copy_n(lit.data, N, it));
	};

	template <typename... Ts>
	constexpr static auto concat(Ts &&... literals) {
		constexpr size_t size = {(0 + ... + get_size<Ts>())};

		char data[size + 1];
		auto it = std::begin(data);
		((it = insert(convert(literals), it)), ...);
		data[size] = 0;
		return (basic_string_literal{data});
	}
};

};

template <typename... Ts> requires (sizeof...(Ts) > 0 && ((
																			std::is_array_v<std::remove_reference_t<Ts>> ||
																			is_string_literal<std::remove_reference_t<Ts>>) && ...))
constexpr auto literal_concat(Ts &&... literals) {
	return (_::literal_concat::concat(std::forward<Ts>(literals)...));
}

namespace literals
{

template <basic_string_literal Input>
consteval auto operator""_sl() {
	return Input;
}

} // namespace literals

} // namespace octahedron

#endif
