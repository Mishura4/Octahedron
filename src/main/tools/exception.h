#ifndef OCTAHEDRON_EXCEPTION_H_
#define OCTAHEDRON_EXCEPTION_H_

#include <algorithm>
#include <array>
#include <ranges>
#include <source_location>

#include "math.h"
#include "string_literal.h"

namespace octahedron
{

class exception : public std::exception {
public:
	exception() = default;

	exception(std::string &&msg) :
		message{std::move(msg)} {}

	exception(const std::string &msg) :
		message(msg) {}

	exception(const exception &) = default;

	exception(exception &&) = default;

	exception& operator=(const exception &) = default;

	exception& operator=(exception &&) = default;

	char const* what() const override {
		return (message.c_str());
	}

private:
	std::string message;
};

class debug_exception : public exception {
public:
	debug_exception(std::source_location loc = std::source_location::current()) :
		location{loc} {}

	debug_exception(
		const std::string &msg, bool full_msg = false, std::source_location loc = std::source_location::current()
	) : exception{full_msg ? _format(msg.c_str(), loc) : msg}, location{loc}
	{}

	debug_exception(const debug_exception &) = default;

	debug_exception(debug_exception &&) = default;

	debug_exception& operator=(const debug_exception &) = default;

	debug_exception& operator=(debug_exception &&) = default;

	const std::source_location& where() const noexcept {
		return (location);
	}

	std::string format() const {
		return (_format(what(), location));
	}

private:
	static std::string _format(const char *msg, const std::source_location &loc) {
		return (
			fmt::format(
			"{}\n"
			"in {}\n\t"
			"at {}:{} [{}]\n",
			msg,
			loc.function_name(),
			loc.file_name(), loc.line(), loc.column())
		);
	}
	std::source_location location;
};

} // namespace octahedron

#endif
