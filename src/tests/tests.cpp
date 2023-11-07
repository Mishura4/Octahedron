#include "tests.h"

#include <fmt/color.h>

namespace tests = octahedron::tests;

void tests::test::fail(std::string reason) {
	if (reason.empty()) {
		g_logger->log(log_level::BASIC, "[{}] {}", failure, name);
	}
	else {
		g_logger->log(log_level::BASIC, "[{}] {}: {}", failure, name, reason);
	}
	state = status::failure;
}

void tests::test::skip(){
	if (state == status::not_executed || state == status::started) {
		state = status::skipped;
		g_logger->log(log_level::BASIC, "[{}] {}", skipped, name);
	}
}

void tests::test::success() {
	if (state != status::failure) {
		state = status::success;
		g_logger->log(log_level::BASIC, "[{}] {}", tests::success, name);
	}
}

void tests::test::run() {
	if (state == status::skipped)
		return;
		g_logger->log(log_level::BASIC, "[{}] {}", tests::starting, name);
	state = status::started;
	try {
		fun(*this);
		success();
	} catch (const test_failure_exception &e) {
		g_logger->log(log_level::BASIC, "[{}] {}: {}", failure, name, e.format());
	} catch (const std::exception &e) {
		g_logger->log(log_level::BASIC, "[{}] {}: exception `{}`", failure, name, e.what());
	} catch (...) {
		g_logger->log(log_level::BASIC, "[{}] {}: exception (unknown)", failure, name);
	}
}

auto tests::test::get_name() const noexcept -> const std::string& {
	return (name);
}


auto tests::test::get_description() const noexcept -> const std::string& {
	return (description);
}

auto tests::test::get_status() const noexcept -> status {
	return (state);
}



auto tests::make_test_suite(std::string name) -> test_suite & {
	auto &ret = g_tests.emplace_back();

	ret.name = std::move(name);
	return (ret);
}