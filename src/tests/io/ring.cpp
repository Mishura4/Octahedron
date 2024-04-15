#include "io/io.h"

#include <tools/ring.h>
#include <array>

using namespace octahedron::tests;

namespace {

struct lifetime_tracker {
	int times_created = 0;
	int times_destroyed = 0;

	bool pass() const {
		return (times_created == times_destroyed);
	}
};

struct ref {
	lifetime_tracker *t = nullptr;

	ref() = default;
	ref(lifetime_tracker &tracker) : t{&tracker} {
		tracker.times_created++;
	};
	ref(const ref&) = delete;
	ref(ref&& r) noexcept : t{std::exchange(r.t, nullptr)} {}

	~ref() {
		if (t)
			t->times_destroyed++;
	}

	ref &operator=(const ref&) = delete;
	ref &operator=(ref&& r) noexcept {
		t = std::exchange(r.t, nullptr);
		return (*this);
	}
};

[[maybe_unused]] test& ring_char_test = g_io_tests.make_test("ring<char>", "Circular buffer with trivial type", [](test &self) {
	octahedron::ring<char, 16> ring;
	std::array<char, 32> buf;

	if (ring.write("abcdef", 6) != 6) {
		self.fail("failed to write [0-6( characters");
	}
	if (ring.write(":)") != 2) {
		self.fail("failed to write [6-8( characters");
	}
	if (ring.read(buf, 3) < 3 || std::strncmp(buf.data(), "abc", 3) != 0) {
		self.fail("failed to read [0-3(");
	}
	if (ring.read(buf, 3) < 3 || std::strncmp(buf.data(), "def", 3) != 0) {
		self.fail("failed to read [3-6(");
	}
	if (ring.read(buf, 3) != 2 || std::strncmp(buf.data(), ":)", 2) != 0) {
		self.fail("failed to read [6-8(");
	}
	if (ring.size() != 0) {
		self.fail("size() failed to give 0 after writing and reading 8");
	}

	ring.write("abcdef:)");
	ring.read(buf, 6);
	if (ring.write("octahedron", 10) != 10) {
		self.fail("failed to write wrap-around [8-2(");
	}
	if (ring.read(buf, 12) != 12 || std::strncmp(buf.data(), ":)octahedron", 12) != 0) {
		self.fail("failed to read wrap-around [6-2(");
	}

	if (ring.size() != 0) {
		self.fail("size() failed to give 0 after writing and reading wrap-around 12");
	}
	if (ring.write("sod hype, sod hype", 18) != ring.capacity()) {
		self.fail("writing > capacity failed to write only capacity");
	}
	if (ring.write("this should not work") != 0) {
		self.fail("writing at full size failed to write 0");
	}
	if (ring.read(buf, 18) != ring.capacity() || std::strncmp(buf.data(), "sod hype, sod hype", ring.capacity()) != 0) {
		self.fail("reading > capacity failed to read only capacity");
	}

	char c = 'a';
	for (size_t i = 0; i < ring.capacity(); ++i) {
		if (c != ring.push_back(c)) {
			self.fail("push_back failed");
		}
		++c;
	}
	if (ring.size() != ring.capacity()) {
		self.fail("push_back failed");
	}

	c = 'a';
	for (char c2 : ring) {
		if (c2 != c) {
			self.fail("iterator failed");
		}
		++c;
	}

	c = 'a';
	for (size_t i = 0; i < ring.capacity(); ++i) {
		if (c != ring.front()) {
			self.fail("pop_front failed");
		}
		++c;
		ring.pop_front();
	}
	c = 'a';
	for (size_t i = 0; i < ring.capacity(); ++i) {
		c = ring.push_back(c);
		++c;
	};
	ring.clear();
	if (!ring.empty()) {
		self.fail("empty() failed to return true after reset()");
	}
	c = 'A';
	for (size_t i = 0; i < ring.capacity(); ++i) {
		c = ring.push_back(c);
		++c;
	};
});

[[maybe_unused]] test& ouroboros_char_test = g_io_tests.make_test("ouroboros<char>", "Overwriting circular buffer with trivial type", [](test &self) {
	octahedron::ouroboros<char, 16> ouroboros;
	std::array<char, 32> buf;

	if (ouroboros.write("abcedf", 6) != 6) {
		self.fail("failed to write [0-6( characters");
	}
	if (ouroboros.write(":)", 2) != 2) {
		self.fail("failed to write [6-8( characters");
	}
	if (ouroboros.read(buf, 3) < 3 || std::strncmp(buf.data(), "abc", 3) != 0) {
		self.fail("failed to read [0-3(");
	}
	if (ouroboros.read(buf, 3) < 3 || std::strncmp(buf.data(), "edf", 3) != 0) {
		self.fail("failed to read [3-6(");
	}
	if (ouroboros.read(buf, 3) != 2 || std::strncmp(buf.data(), ":)", 2) != 0) {
		self.fail("failed to read [6-8(");
	}
	if (ouroboros.size() != 0) {
		self.fail("size() failed to give 0 after writing and reading 8");
	}

	if (ouroboros.write("octahedron", 10) != 10) {
		self.fail("failed to write wrap-around [8-2(");
	}
	if (ouroboros.read(buf, 10) != 10 || std::strncmp(buf.data(), "octahedron", 10) != 0) {
		self.fail("failed to read wrap-around [8-2(");
	}

	if (ouroboros.size() != 0) {
		self.fail("size() failed to give 0 after writing and reading wrap-around 10");
	}
	if (ouroboros.write("sod hype, sod hype", 18) != 18) {
		self.fail("writing > capacity failed to write all bytes");
	}
	if (ouroboros.size() != ouroboros.capacity()) {
		self.fail("writing > capacity failed to keep size at capacity");
	}
	if (auto size = ouroboros.peek(buf, ouroboros.size()); size != ouroboros.capacity() || strncmp(buf.data(), "d hype, sod hype", size) != 0) {
		self.fail("writing > capacity failed to overwrite or set start properly");
	}
	if (ouroboros.write("this should work", 16) != 16) {
		self.fail("writing at full size failed to write");
	}
	if (ouroboros.read(buf, 18) != ouroboros.capacity() || std::strncmp(buf.data(), "this should work", ouroboros.capacity()) != 0) {
		self.fail("reading > capacity failed to read only capacity");
	}

	char c = 'a';
	for (size_t i = 0; i < 25; ++i) {
		if (c != ouroboros.push_back(c)) {
			self.fail("push_back failed");
		}
		++c;
	}
	if (ouroboros.size() != ouroboros.capacity()) {
		self.fail("push_back failed");
	}

	c = 'a' + (25 - ouroboros.capacity());
	for (char c2 : ouroboros) {
		if (c2 != c) {
			self.fail("iterator or push_back failed");
		}
		++c;
	}

	c = 'a' + (25 - ouroboros.capacity());
	for (size_t i = 0; i < ouroboros.capacity(); ++i) {
		if (c != ouroboros.front()) {
			self.fail("pop_front failed");
		}
		++c;
		ouroboros.pop_front();
	}
	c = 'a';
	for (size_t i = 0; i < 25; ++i) {
		c = ouroboros.push_back(c);
		++c;
	};
	ouroboros.clear();
	if (!ouroboros.empty()) {
		self.fail("empty() failed to return true after reset()");
	}
	c = 'A';
	for (size_t i = 0; i < ouroboros.capacity(); ++i) {
		c = ouroboros.push_back(c);
		++c;
	};
});

[[maybe_unused]] test& ring_class_test = g_io_tests.make_test("ring<class>", "Circular buffer with class type", [](test &self) {
	std::array<lifetime_tracker, 16> data;

	{
		octahedron::ring<ref, 16> ring;

		ring.push_back(ref{data[0]});
	}
	if (!data[0].pass())
		self.fail("failed to destroy object");

	{
		octahedron::ring<ref, 16> ring;

		ring.push_back(ref{data[0]});
		ring.push_back(ref{data[1]});
		ring.pop_front();
		if (!data[0].pass())
			self.fail("failed to destroy object");
	}

	for (size_t i = 0; i < data.size(); ++i) {
		if (!data[i].pass()) {
			self.fail(fmt::format("failed to destroy object {}", i));
		}
	}

	{
		octahedron::ring<ref, 16> ring;

		ring.push_back(ref{data[0]});
		ring.push_back(ref{data[1]});
		ring.pop_front();
		ring.pop_front();
		if (!data[0].pass() || !data[1].pass())
			self.fail("failed to destroy object");
		for (size_t i = 0; i < data.size(); ++i) {
			if (!data[i].pass())
				self.fail("failed to create wrap-around object");
		}
	}

	for (size_t i = 0; i < data.size(); ++i) {
		if (!data[i].pass()) {
			self.fail(fmt::format("failed to destroy wrap-around object {}", i));
		}
	}

	{
		octahedron::ring<ref, 16> ring;

		ring.push_back(ref{data[0]});
		ring.push_back(ref{data[1]});
		ring.pop_front();
		ring.pop_front();

		{
			std::array<ref, 16> arr;

			std::ranges::generate(arr, [&data, i = 0]() mutable -> ref {
				int idx = i;

				++i;
				return {data[(idx + 2) % data.size()]};
			});
			ring.append_range(std::move(arr));
			for (size_t i = 0; i < data.size(); ++i) {
				if (data[i].pass())
					self.fail("failed to create wrap-around range");
			}
			std::ranges::generate(arr, [&data, i = 0]() mutable -> ref {
				int idx = i;

				++i;
				return {data[(idx + 2) % data.size()]};
			});
			if (ring.append_range(std::move(arr)) != 0)
				self.fail("append_range on a full ring failed to add 0");
		}
	}

	for (size_t i = 0; i < data.size(); ++i) {
		if (!data[i].pass()) {
			self.fail(fmt::format("failed to destroy object {}", i));
		}
	}
});

[[maybe_unused]] test& ouroboros_class_test = g_io_tests.make_test("ouroboros<class>", "Overwriting circular buffer with class type", [](test &self) {
	std::array<lifetime_tracker, 16> data;

	{
		octahedron::ouroboros<ref, 16> ouroboros;

		ouroboros.push_back(ref{data[0]});
	}
	if (!data[0].pass())
		self.fail("failed to destroy object");

	{
		octahedron::ouroboros<ref, 16> ouroboros;

		ouroboros.push_back(ref{data[0]});
		ouroboros.push_back(ref{data[1]});
		ouroboros.pop_front();
		if (!data[0].pass())
			self.fail("failed to destroy object");
	}

	for (size_t i = 0; i < data.size(); ++i) {
		if (!data[i].pass()) {
			self.fail(fmt::format("failed to destroy object {}", i));
		}
	}

	{
		octahedron::ouroboros<ref, 16> ouroboros;

		ouroboros.push_back(ref{data[0]});
		ouroboros.push_back(ref{data[1]});
		ouroboros.pop_front();
		ouroboros.pop_front();
		if (!data[0].pass() || !data[1].pass())
			self.fail("failed to destroy object");
		for (size_t i = 0; i < data.size(); ++i) {
			if (!data[i].pass())
				self.fail("failed to create wrap-around object");
		}
	}

	for (size_t i = 0; i < data.size(); ++i) {
		if (!data[i].pass()) {
			self.fail(fmt::format("failed to destroy wrap-around object {}", i));
		}
	}

	{
		octahedron::ouroboros<ref, 16> ouroboros;

		ouroboros.push_back(ref{data[0]});
		ouroboros.push_back(ref{data[1]});
		ouroboros.pop_front();
		ouroboros.pop_front();

		{
			std::array<ref, 25> arr;

			std::ranges::generate(arr, [&data, i = 0]() mutable -> ref {
				int idx = i;

				++i;
				return {data[(idx + 2) % data.size()]};
			});
			ouroboros.append_range(std::move(arr));
			std::ranges::generate(arr, [&data, i = 0]() mutable -> ref {
				int idx = i;

				++i;
				return {data[(idx + 2) % data.size()]};
			});
			if (ouroboros.append_range(std::move(arr)) != arr.size())
				self.fail("append_range on a full ouroboros failed");
		}

		{
			octahedron::ouroboros ouroboros2 = std::move(ouroboros);
			octahedron::ouroboros<ref, 16> ouroboros3;

			ouroboros3 = std::move(ouroboros2);
		}

		for (size_t i = 0; i < data.size(); ++i) {
			if (!data[i].pass()) {
				self.fail(fmt::format("failed to destroy object {}", i));
			}
		}
	}

	for (size_t i = 0; i < data.size(); ++i) {
		if (!data[i].pass()) {
			self.fail(fmt::format("failed to destroy object {}", i));
		}
	}
});

}


