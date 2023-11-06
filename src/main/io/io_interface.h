#ifndef OCTAHEDRON_IO_INTERFACE_H_
#define OCTAHEDRON_IO_INTERFACE_H_

#include <span>

#include <boost/pfr.hpp>

#include "base.h"

namespace octahedron
{
template <typename T>
struct is_string_view_s {
	static inline constexpr bool value = false;
};

template <typename T> requires(std::ranges::range<std::remove_cvref_t<T>>)
struct is_string_view_s<T> {
	using range_type = std::remove_reference_t<T>;
	using char_type = std::conditional_t<std::is_const_v<range_type>, const char, char>;
	using value_type =
	std::remove_reference_t<decltype(*std::ranges::begin(std::declval<range_type&>()))>;

	static inline constexpr bool value = std::same_as<value_type, char_type>;
};

template <typename T>
constexpr inline bool is_endian_dependent =
	(std::is_scalar_v<T>) && (sizeof(T) > 1);

template <typename T> requires(std::ranges::range<std::remove_reference_t<T>>)
constexpr inline bool is_contiguous_data =
	std::ranges::contiguous_range<T> &&
	std::is_scalar_v<std::remove_reference_t<
		decltype(*std::ranges::begin(std::declval<std::add_lvalue_reference_t<T>>()))>>;

template <typename T>
concept byteoid = sizeof(T) == 1 && alignof(T) == 1;

template <typename T>
concept bytes = std::ranges::range<std::remove_reference_t<T>> &&
								byteoid<std::remove_reference_t<decltype(std::begin(std::declval<T&>()))>>;

template <typename T>
constexpr inline bool is_string_view = is_string_view_s<T>::value;

enum class endianness {
	little,
	big,

	native  = std::endian::native == std::endian::little ? little : big,
	swapped = std::endian::native == std::endian::little ? big : little
};

template <size_t Size>
struct integer_s;

template <>
struct integer_s<1> {
	using type = int8_t;
};

template <>
struct integer_s<2> {
	using type = int16_t;
};

template <>
struct integer_s<4> {
	using type = int32_t;
};

template <>
struct integer_s<8> {
	using type = int64_t;
};

template <size_t Size>
using integer = typename integer_s<Size>::type;

template <typename T>
[[nodiscard]] consteval auto byte_mask(size_t n) noexcept {
	return (T{0xFF} << (n * 8));
};

template <endianness Endian = endianness::swapped>
[[nodiscard]] constexpr auto byteswap(auto value) noexcept requires(
	std::is_integral_v<decltype(value)> &&
	std::has_unique_object_representations_v<decltype(value)>
) {
	using T = decltype(value);

	constexpr auto swap = []<size_t... Ns>(T v, std::index_sequence<Ns...>) constexpr noexcept {
		constexpr auto impl = []<size_t N>(T v_) constexpr noexcept {
			if constexpr (constexpr auto size = sizeof...(Ns) / 2; N < size) {
				constexpr auto shift = ((size - N) * 16 - 8);

				return ((v_ & byte_mask<T>(N)) << shift);
			} else {
				constexpr auto shift = ((N - size) * 16 + 8);

				return ((v_ & byte_mask<T>(N)) >> shift);
			}
		};
		return (impl.template operator()<Ns>(v) | ...);
	};

	if constexpr (Endian == endianness::native || sizeof(T) == 1)
		return (value);
	else
		return (swap(value, std::make_index_sequence<sizeof(T)>()));
}

template <typename T>
concept writeable = requires(T &t, std::span<const std::byte> data) {
	{
		t.write(data.data(), data.size())
	} -> std::convertible_to<size_t>;
};

template <typename T>
concept readable = requires(T &t, std::span<std::byte> data) {
	{
		t.read(data.data(), data.size())
	} -> std::convertible_to<size_t>;
};

template <typename T>
struct io_helper;

/*template <typename T>
struct pfr_helper;

template <typename T>
requires(std::is_scalar_v<T>)
struct pfr_helper<T>
{
		constexpr static inline bool valid = true;
};

template <typename T>
struct pfr_helper
{
		template <size_t... Ns>
		consteval static bool is_pfr_valid_fun(std::index_sequence<Ns...>)
		{
			constexpr auto impl = []<size_t N>() consteval -> bool
			{
				if constexpr (requires { std::declval<boost::pfr::tuple_element_t<N, T>>(); })
				{
					using type = boost::pfr::tuple_element_t<N, T>;

					return (pfr_helper<type>::valid);
				}
				else
					return (false);
			};

			return (impl.template operator()<Ns>() && ...);
		}

		consteval static bool is_pfr_valid_fun()
		{
			if constexpr (requires {
											{
												boost::pfr::tuple_size_v<T>
											} -> std::convertible_to<size_t>;
										})
			{
				return (is_pfr_valid_fun(std::make_index_sequence<boost::pfr::tuple_size_v<T>>()));
			}
			else
				return (false);
		}

		constexpr static inline bool valid = is_pfr_valid_fun();
};*/

struct io_result {
	constexpr operator bool() const {
		return (value == expected_size);
	}

	size_t value{0};
	size_t expected_size{0};
};

constexpr io_result& operator+=(io_result &a, io_result b) noexcept {
	a.value += b.value;
	a.expected_size += b.expected_size;
	return (a);
}

constexpr io_result operator+(io_result a, io_result b) noexcept {
	return {a.value + b.value, a.expected_size + b.expected_size};
}

/* template <typename T>
constexpr bool is_pfr_valid =
	(std::is_aggregate_v<T> || std::is_scalar_v<T>) && pfr_helper<T>::valid;

constexpr auto teststest = is_pfr_valid<test>;

using tyestt = boost::pfr::tuple_element_t<0, test>;

static_assert(pfr_helper<test>::is_pfr_valid_fun(std::make_index_sequence<boost::pfr::tuple_size_v<test>>()) == true);
static_assert(is_pfr_valid<invalid> == false);*/

template <typename T>
struct io_write_interface {
	io_write_interface() {
		static_assert(writeable<T>);
	}

	io_result put(std::string_view str) {
		auto data = reinterpret_cast<const std::byte*>(str.data());

		return {static_cast<T*>(this)->write(data, str.size()), str.size() * sizeof(str[0])};
	}

	template <typename U, endianness Endian = {}> requires(
		std::is_scalar_v<std::remove_reference_t<U>> &&
		!std::is_enum_v<std::remove_reference_t<U>> &&
		!std::is_pointer_v<std::remove_reference_t<U>>)
	io_result put(U value) {
		auto data = reinterpret_cast<const std::byte*>(&value);

		if constexpr (Endian != endianness::native)
			value = byteswap<Endian>(value);
		return {static_cast<T*>(this)->write(data, sizeof(value)), sizeof(value)};
	}

	template <typename U, endianness Endian = {}> requires(std::is_enum_v<std::remove_reference_t<U>>)
	io_result put(U value) {
		using type = std::underlying_type_t<decltype(value)>;

		return (put<type, Endian>(static_cast<type>(value)));
	}

	template <typename U, endianness Endian = {}> requires(
		std::ranges::range<std::remove_cvref_t<U>> &&
		!implicitly_convertible_to<U, std::string_view> &&
		is_endian_dependent<std::ranges::range_value_t<U>>)
	io_result put(const U &values) {
		using range_type = std::remove_cvref<U>;
		using value_type = std::ranges::range_value_t<U>;

		if constexpr (Endian == endianness::native && is_contiguous_data<U>) {
			constexpr size_t value_size = sizeof(value_type);
			auto             data = reinterpret_cast<const std::byte*>(&(*std::ranges::begin(values)));
			size_t           size = std::size(values) * value_size;

			return {static_cast<T*>(this)->write({data, size}), size};
		} else {
			io_result total{0, 0};

			for (auto v : values) {
				io_result inserted = put(values);

				total += inserted;
				if (!inserted)
					return (total);
			}
			return (total);
		}
	}

	template <typename U> requires(
		std::ranges::range<std::remove_cvref_t<U>> &&
		!implicitly_convertible_to<U, std::string_view> &&
		!is_endian_dependent<std::ranges::range_value_t<U>> && is_contiguous_data<U>)
	io_result put(const U &values) {
		using value_type = std::remove_cvref_t<decltype(*std::ranges::begin(values))>;
		constexpr size_t value_size = sizeof(value_type);
		auto             data = reinterpret_cast<const std::byte*>(&(*std::ranges::begin(values)));
		size_t           size = std::size(values) * value_size;

		return {static_cast<T*>(this)->write({data, size}), size};
	}

	template <typename U, endianness Endian = {}> requires(
		!std::is_scalar_v<std::remove_reference_t<U>> &&
		!std::ranges::range<std::remove_cvref_t<U>>)
	io_result                             put(const U &value) {
		if constexpr (requires(io_helper<U> t) { t.put(this, std::as_const(value)); }) {
			return (io_helper<U>().put(this, value));
		}
		// vvv IF THIS FAILS --- SPECIALIZE THIS ^^^
		else if constexpr (std::is_scalar_v<U> || std::is_aggregate_v<U>) {
			constexpr auto num_fields = boost::pfr::tuple_size_v<U>;

			static_assert(num_fields > 0, "Cannot serialize an empty object");

			constexpr auto        impl = []<size_t... Ns>(
				io_write_interface *self,
				const U &           v,
				std::index_sequence<Ns...>
			) -> io_result {
				constexpr auto put_ = []<size_t N>(io_write_interface *self, const U &v_) -> io_result {
					using field_type = boost::pfr::tuple_element_t<N, U>;

					// const_cast here is a workaround for breaking in boost::pfr::add_cv_like
					const auto &field = boost::pfr::get<N>(const_cast<U&>(v_));

					if constexpr (is_endian_dependent<field_type>)
						return (self->template put<Endian>(field));
					else
						return (self->put(field));
				};
				io_result ret{0, 0};

				((ret += (put_.template operator()<Ns>(self, v))), ...);
				return (ret);
			};

			return (impl(this, value, std::make_index_sequence<num_fields>()));
		} else {
			static_assert(
				std::is_scalar_v<U> || std::is_aggregate_v<U>,
				"type must be either scalar and aggregate for implicit serialization via boost::pfr "
				"or octahedron::IOHelper<U>().get(IOReadable*, U&) must be valid"
			);
			return {};
		}
	}

	template <endianness Endian = {}, typename... Us> requires(
		sizeof...(Us) > 1 && (!std::is_pointer_v<std::remove_reference_t<Us>> && ...))
	io_result put_all(Us &&... values) {
		constexpr auto impl = []<typename T0>(io_write_interface *self, T0 &&v) -> io_result {
			using type = std::remove_reference_t<T0>;
			if constexpr (is_endian_dependent<type>)
				return (self->template put<Endian>(v));
			else
				return (self->put(v));
		};
		io_result ret{0, 0};
		((ret += (impl(this, values))), ...);
		return (ret);
	}

	template <endianness Endian, typename U> requires(
		is_endian_dependent<std::remove_reference_t<U>> &&
		sizeof(T) < sizeof(void*) * 4)
	auto put(U v) {
		return (put<U, Endian>(std::forward<U>(v)));
	}

	template <endianness Endian, typename U> requires(
		is_endian_dependent<std::remove_reference_t<U>> &&
		sizeof(T) > sizeof(void*) * 4)
	auto put(const U &v) {
		return (put<const U&, Endian>(v));
	}

	template <endianness Endian = {}>
	io_result put(const auto *value, auto num) {
		auto data = reinterpret_cast<const std::byte*>(value);

		return {static_cast<T*>(this)->write(data, num * sizeof(*value)), num * sizeof(*value)};
	}

	template <typename U> requires(std::is_scalar_v<U> && !std::same_as<U, std::byte>)
	size_t write(std::span<const U> values) {
		auto *data = reinterpret_cast<const std::byte*>(values.data());

		return (static_cast<T*>(this)->write(std::span{data, values.size() * sizeof(U)}) / sizeof(U));
	}

	template <typename U> requires(std::is_scalar_v<U>)
	size_t write(const U *values, size_t size) {
		auto *data = reinterpret_cast<const std::byte*>(values);

		return (static_cast<T*>(this)->write(std::span{data, size * sizeof(U)}) / sizeof(U));
	}
};

template <typename T>
struct io_read_interface {
	template <typename U, endianness Endian = {}> requires(std::is_enum_v<U>)
	io_result get(U &value) {
		std::underlying_type_t v;
		io_result              ret = get<Endian>(v);

		value = static_cast<std::remove_reference_t<decltype(value)>>(v);
		return (ret);
	}

	template <typename U, endianness Endian = {}> requires(!std::is_enum_v<U> &&
																												 is_endian_dependent<U>)
	io_result get(U &value) {
		auto      data = reinterpret_cast<std::byte*>(&value);
		io_result ret{static_cast<T*>(this)->read(data, sizeof(value)), sizeof(value)};

		if (!ret)
			return (ret);
		if constexpr (Endian != endianness::native)
			value = byteswap<Endian>(value);
		return (ret);
	}

	template <typename U> requires(
		std::is_scalar_v<U> &&
		!std::is_enum_v<std::remove_reference_t<U>> &&
		!std::is_pointer_v<U> &&
		!std::ranges::range<U> &&
		!is_endian_dependent<U>)
	io_result get(U &value) {
		auto data = reinterpret_cast<std::byte*>(&value);

		return {static_cast<T*>(this)->read(data, sizeof(value)), sizeof(U)};
	}

	template <typename U, endianness Endian = {}> requires(
		!std::is_scalar_v<U> &&
		!std::ranges::range<U>)
	io_result get(U &value) {
		using type = std::remove_reference_t<decltype(value)>;

		if constexpr (requires(io_helper<type> t) { t.get(this, value); }) {
			return (io_helper<type>().get(this, value));
		}
		// vvv IF THIS FAILS --- SPECIALIZE THIS ^^^
		else if constexpr (std::is_scalar_v<type> || std::is_aggregate_v<type>) {
			constexpr auto num_fields = boost::pfr::tuple_size_v<type>;

			static_assert(num_fields > 0, "Cannot serialize an empty object");

			constexpr auto impl =
				[]<size_t... Ns>(
				io_read_interface *self,
				type &             v,
				std::index_sequence<Ns...>
			) -> io_result {
				constexpr auto       get_ = []<size_t N>(
					io_read_interface *self,
					decltype(value) &  v_
				) -> io_result {
					using field_type = boost::pfr::tuple_element_t<N, type>;

					auto &field = boost::pfr::get<N>(v_);

					if constexpr (is_endian_dependent<field_type>)
						return (self->template get<Endian>(field));
					else
						return (self->get(field));
				};
				io_result ret{0, 0};
				((ret += (get_.template operator()<Ns>(self, v))), ...);
				return (ret);
			};

			return (impl(this, value, std::make_index_sequence<num_fields>()));
		} else {
			static_assert(
				std::is_scalar_v<type> || std::is_aggregate_v<type>,
				"type must be either scalar and aggregate for implicit serialization via boost::pfr "
				"or octahedron::IOHelper<U>().put(IOReadable*, const U&) must be valid"
			);
			return {};
		}
	}

	template <typename U, endianness Endian = {}> requires(std::ranges::range<U> &&
																												 is_endian_dependent<
																													 std::ranges::range_value_t<U>>)
	io_result get(U &values) {
		using value_type = std::ranges::range_value_t<std::remove_reference_t<decltype(values)>>;
		constexpr auto value_size = sizeof(value_type);

		if constexpr (Endian == endianness::native && is_contiguous_data<decltype(values)>) {
			auto   data = reinterpret_cast<std::byte*>(&(*std::ranges::begin(values)));
			size_t size = std::size(values) * value_size;

			return {static_cast<T*>(this)->read(data, size), size};
		} else {
			io_result total{0, 0};

			for (auto &v : values) {
				io_result inserted = get<Endian>(v);

				total += inserted;
				if (!inserted)
					return (total);
			}
			return (total);
		}
	}

	template <typename U> requires(
		std::ranges::range<U> &&
		!is_endian_dependent<std::ranges::range_value_t<U>>)
	io_result get(U &values) {
		using value_type = std::ranges::range_value_t<std::remove_reference_t<decltype(values)>>;
		constexpr auto value_size = sizeof(value_type);

		if constexpr (is_contiguous_data<decltype(values)>) {
			auto   data = reinterpret_cast<std::byte*>(&(*std::ranges::begin(values)));
			size_t size = std::size(values) * value_size;

			return {static_cast<T*>(this)->read(data, size), size};
		} else {
			io_result total{0, 0};

			for (auto &v : values) {
				io_result inserted = get(v);

				total += inserted;
				if (!inserted)
					return (total);
			}
			return (total);
		}
	}

	template <endianness Endian = {}>
	io_result get(auto *value, auto num) {
		io_result total{0, 0};

		for (decltype(num) i = 0; i < num; ++i) {
			io_result inserted = get(value[num]);

			total += inserted;
			if (!inserted)
				return (total);
		}
		return (total);
	}

	template <endianness Endian = {}, typename... Ts> requires(sizeof...(Ts) > 1 &&
																														 (!std::is_pointer_v<Ts> && ...))
	io_result getAll(Ts &... values) {
		constexpr auto impl = []<typename T0>(io_read_interface *self, T0 &v) -> io_result {
			if constexpr (is_endian_dependent<T0>)
				return (self->template get<Endian>(v));
			else
				return (self->get(v));
		};
		io_result ret{0, 0};

		((ret += (impl(this, values))), ...);
		return (ret);
	}

	template <endianness Endian, typename U> requires(is_endian_dependent<U>)
	auto get(U &v) {
		return (get<U, Endian>(v));
	}

	template <typename U, endianness Endian = {}> requires(
		std::ranges::range<U> && is_endian_dependent<std::ranges::range_value_t<U>> && requires(U &u) {
			std::back_inserter(u);
		})
	auto get(U &cont, size_t size) {
		using value_type = std::remove_reference_t<std::ranges::range_value_t<U>>;

		if constexpr (is_contiguous_data<U> && requires(U &u, size_t s) { u.resize(s); }) {
			cont.resize(size);
			return (get(std::data(cont), size));
		} else {
			if constexpr (requires(U &u, size_t s) { u.reserve(s); })
				cont.reserve(size);

			value_type buffer;
			io_result  ret{0, 0};
			auto       inserter = std::back_inserter(cont);

			for (size_t i = 0; i < size; ++i) {
				io_result extracted = get<value_type, Endian>(buffer);

				ret += extracted;
				if (!extracted)
					return (ret);
				inserter = buffer;
			}
			return (ret);
		}
	}

	template <typename U> requires(std::ranges::range<U>)
	auto get(U &cont, size_t size) {
		using value_type = std::remove_reference_t<std::ranges::range_value_t<U>>;

		if constexpr (is_contiguous_data<U> && requires(U &u, size_t s) { u.resize(s); }) {
			cont.resize(size);
			return (get(std::data(cont), size));
		} else {
			if constexpr (requires(U &u, size_t s) { u.reserve(s); })
				cont.reserve(size);

			value_type buffer;
			io_result  ret{0, 0};
			auto       inserter = std::back_inserter(cont);

			for (size_t i = 0; i < size; ++i) {
				io_result extracted = get<value_type>(buffer);

				ret += extracted;
				if (!extracted)
					return (ret);
				inserter = buffer;
			}
			return (ret);
		}
	}

	io_result get(std::string &str) {
		size_t size = 0;
		char   c{0x7f};

		while (c != 0) {
			if (!get(c))
				return {size, size + 1};
			str.push_back(c);
			++size;
		}
		return {size, size};
	}

	template <typename U, endianness Endian = {}> requires(is_endian_dependent<U>)
	U get() {
		U ret;

		if (!get<U, Endian>(ret))
			throw std::runtime_error("could not retrieve value");
		return (ret);
	}

	template <typename U> requires(!is_endian_dependent<U>)
	U get() {
		U ret;

		if (!get<U>(ret))
			throw std::runtime_error("could not retrieve value");
		return (ret);
	}

	template <endianness Endian = {}>
	int get() {
		return (get<int, Endian>());
	}

	std::optional<std::string> get_next_line(size_t max_size = (2uLL << 15)) {
		if constexpr (requires { std::declval<T>()._get_next_line(max_size); }) {
			return (static_cast<T*>(this)->_get_next_line(max_size));
		} else {
			char        c;
			std::string ret;

			for (int i = 0; i < max_size; ++i) {
				if (!get(c)) {
					if (ret.empty())
						return std::nullopt;
					return ret;
				}
				if (c == 0 || c == '\n')
					return (ret);
				ret.push_back(c);
			}
			return (ret);
		}
	}

	template <typename U> requires(std::is_scalar_v<U> && !std::same_as<U, std::byte>)
	size_t read(std::span<U> values) {
		auto *data = reinterpret_cast<std::byte*>(values.data());

		return (static_cast<T*>(this)->read(std::span{data, values.size() * sizeof(U)}) / sizeof(U));
	}

	template <typename U> requires(std::is_scalar_v<U>)
	size_t read(U *values, size_t size) {
		auto *data = reinterpret_cast<std::byte*>(values);

		return (static_cast<T*>(this)->read(std::span{data, size * sizeof(U)}) / sizeof(U));
	}
};

template <typename T>
struct io_interface : io_read_interface<T>, io_write_interface<T> {};

} // namespace octahedron

#endif
