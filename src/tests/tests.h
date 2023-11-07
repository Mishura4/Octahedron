#ifndef OCTAHEDRON_TESTS_H_
#define OCTAHEDRON_TESTS_H_

#include <vector>
#include <string_view>
#include <string>
#include <concepts>
#include <utility>
#include <functional>
#include <source_location>

#include <io/logger.h>
#include <tools/exception.h>

#include <fmt/color.h>

#include "../main/tools/exception.h"

namespace octahedron::tests {

inline const auto failure = styled("FAILURE", fg(fmt::terminal_color::red));
inline const auto timeout = styled("TIMEOUT", fg(fmt::terminal_color::red));
inline const auto not_executed = styled("NOT EXECUTED", fg(fmt::terminal_color::red));
inline const auto starting = styled("STARTING", fg(fmt::terminal_color::bright_blue));
inline const auto skipped = styled("SKIPPED", fg(fmt::terminal_color::yellow));
inline const auto success = styled("SUCCESS", fg(fmt::terminal_color::green));

class test {
public:
	enum class status {
		not_executed,
		skipped,
		started,
		success,
		failure,
		timeout
	};

	test() = default;

	template <typename Fun>
	test(std::string n, std::string desc, Fun&& function) :
		name{std::move(n)}, description{std::move(desc)}, fun{std::forward<Fun>(function)}
	{}

	test(const test &) = delete;

	test(test&&) = default;

	test& operator=(const test&) = delete;

	test& operator=(test&&) = default;

	~test() = default;

	void fail(std::string reason = {});

	void success();

	void skip();

	void run();

	status get_status() const noexcept;

	const std::string& get_name() const noexcept;

	const std::string& get_description() const noexcept;

private:
	std::string name = {};
	std::string description = {};
	std::function<void(test&)> fun = {};
	status state = status::not_executed;
};

struct test_suite {

	test_suite() = default;

	test_suite(const test &) = delete;

	test_suite(test_suite&&) = default;

	test_suite& operator=(const test_suite&) = delete;

	test_suite& operator=(test_suite&&) = default;

	std::string name;
	std::vector<test> tests;

	template <typename Fun>
	requires (std::is_invocable_r_v<void, Fun, test&>)
	test &make_test(std::string n, std::string desc, Fun fun) {
		return (tests.emplace_back(std::move(n), std::move(desc), std::forward<Fun>(fun)));
	}
};

inline std::vector<test_suite> g_tests;

inline std::optional<logger<std::ostream&>> g_logger;

test_suite &make_test_suite(std::string name);

struct test_failure_exception : debug_exception {
	using debug_exception::debug_exception;
	using debug_exception::operator=;
};

} // namespace octahedron::tests

#endif
