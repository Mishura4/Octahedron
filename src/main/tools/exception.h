#ifndef OCTAHEDRON_EXCEPTION_H_
#define OCTAHEDRON_EXCEPTION_H_

#include <algorithm>
#include <array>
#include <ranges>

#include "math.h"
#include "string_literal.h"

namespace octahedron
{
template <typename Base, string_literal Literal>
struct exception : Base {
	const char *what() const final {
		return (Literal.data);
	}
};

template <typename Base>
struct exception<Base, string_literal{""}> : Base {
	static inline constexpr auto buffer_size = 512;

	explicit exception(std::string_view msg) noexcept :
		std::exception() {
		auto size = min(msg.size(), buffer_size);

		std::ranges::copy_n(msg.begin(), size, message.begin());
		message[size - 1] = 0;
	}

	exception(exception &&other) = delete;

	exception(const exception &other) noexcept {
		*this = other;
	}

	exception& operator=(const exception &other) noexcept {
		std::ranges::copy(other.message, message.begin());
		return (*this);
	}

	exception& operator=(exception &&other) = delete;

	const char *what() const final {
		return (message.data());
	}

	std::array<char, buffer_size> message;
};

template <string_literal Message>
struct fatal_exception : exception<std::exception, Message> {
	using base = exception<std::exception, Message>;

	using base::base;
	using base::operator=;
};

fatal_exception(std::string_view msg) -> fatal_exception<"">;
} // namespace octahedron

#endif
