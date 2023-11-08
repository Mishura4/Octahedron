#include "tests.h"

#include <iostream>
#include <thread>
#include <atomic>
#include <condition_variable>

#include <tools/math.h>

namespace octahedron::tests {

using clock = std::chrono::steady_clock;

using timepoint = std::chrono::time_point<clock>;

constexpr inline auto timeout_duration = std::chrono::seconds{60};

struct dthread : std::thread {
	using thread::thread;
	using thread::operator=;

	~dthread(){
		if (joinable())
			detach();
	}
};

}

namespace tests = octahedron::tests;


int main() {
	using tests::g_logger;
	g_logger.emplace(std::cout);

	tests::timepoint start = tests::clock::now();
	std::atomic max_time = (start + tests::timeout_duration).time_since_epoch().count();
	std::atomic finished = false;
	std::condition_variable cv;

	auto th = tests::dthread([&] {
		for (tests::test_suite &suite : tests::g_tests) {
			for (tests::test &t : suite.tests) {
				if (finished) { // aborted
					return;
				}
				max_time = (tests::clock::now() + tests::timeout_duration).time_since_epoch().count();
				t.run();
			}
		}
		finished = true;
		cv.notify_all();
	});

	std::mutex m;
	std::unique_lock l{m};
	tests::timepoint timeout;

	do {
		timeout = tests::timepoint{tests::clock::duration{max_time.load()}};
		cv.wait_until(l, timeout, [&]() -> bool {
			return finished;
		});
	} while (!finished && tests::clock::now() < timeout);
	if (!finished) {
		finished = true;
		g_logger->log(octahedron::log_level::BASIC, "Timing out suite");
		std::this_thread::sleep_for(std::chrono::seconds(5));
	}

	using octahedron::log_level;

	size_t total_failed = 0;
	size_t total_count = 0;
	for (const tests::test_suite &suite : tests::g_tests) {
		if (suite.tests.size() == 0)
			continue;
		size_t count = 0;
		size_t failed = 0;
		g_logger->log(log_level::BASIC, "\nSuite {}:", suite.name);
		for (const tests::test &t : suite.tests) {
			static constexpr char fmt_str[] = "  {: <20}{: <24}{}";
			using enum tests::test::status;

			++count;
			switch (t.get_status()) {
				case not_executed:
					g_logger->log(log_level::BASIC, fmt_str, tests::not_executed, t.get_name(), t.get_description());
					++failed;
					break;

				case skipped:
					g_logger->log(log_level::BASIC, fmt_str, tests::skipped, t.get_name(), t.get_description());
					break;

				case started:
				case timeout:
					g_logger->log(log_level::BASIC, fmt_str, tests::timeout, t.get_name(), t.get_description());
					++failed;
					break;

				case success:
					g_logger->log(log_level::BASIC, fmt_str, tests::success, t.get_name(), t.get_description());
					break;

				case failure:
					g_logger->log(log_level::BASIC, fmt_str, tests::failure, t.get_name(), t.get_description());
					++failed;
					break;
			}
		}
		g_logger->log(log_level::BASIC, "Completed {}: {:.2f}% success [{}/{}]\n", suite.name, octahedron::ratio{count - failed, count}.get_percent(), count - failed, count);
		total_failed += failed;
		total_count += count;
	}
	if (total_count == 0) {
		g_logger->log(log_level::BASIC, "No tests executed.");
	} else {
		g_logger->log(
			log_level::BASIC,
			"Tests completed: {:.2f}% success [{}/{}]",
			octahedron::ratio{total_count - total_failed, total_count}.get_percent(),
			total_count - total_failed,
			total_count
		);
	}
	return (total_failed);
}
