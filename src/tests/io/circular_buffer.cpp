#include "io/io.h"

#include <tools/ring.h>
#include <array>

using namespace octahedron::tests;

namespace {

[[maybe_unused]] test& ring_char_test = g_io_tests.make_test("ring<char>", "Circular buffer with trivial type", [](test &self) {
	octahedron::ring<char, 16> ring;
	std::array<char, 32> buf;

	if (ring.write("abcedf", 6) != 6) {
		self.fail("failed to write [0-6( characters");
	}
	if (ring.write(":)", 2) != 2) {
		self.fail("failed to write [6-8( characters");
	}
	if (ring.read(buf, 3) < 3 || std::strncmp(buf.data(), "abc", 3) != 0) {
		self.fail("failed to read [0-3(");
	}
	if (ring.read(buf, 3) < 3 || std::strncmp(buf.data(), "edf", 3) != 0) {
		self.fail("failed to read [3-6(");
	}
	if (ring.read(buf, 3) != 2 || std::strncmp(buf.data(), ":)", 2) != 0) {
		self.fail("failed to read [6-8(");
	}
	if (ring.size() != 0) {
		self.fail("size() failed to give 0 after writing and reading 8");
	}

	if (ring.write("octahedron", 10) != 10) {
		self.fail("failed to write wrap-around [8-2(");
	}
	if (ring.read(buf, 10) != 10 || std::strncmp(buf.data(), "octahedron", 10) != 0) {
		self.fail("failed to read wrap-around [8-2(");
	}

	if (ring.size() != 0) {
		self.fail("size() failed to give 0 after writing and reading wrap-around 10");
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
	ring.reset();
	if (!ring.empty()) {
		self.fail("empty() failed to return true after reset()");
	}
	c = 'A';
	for (size_t i = 0; i < ring.capacity(); ++i) {
		c = ring.push_back(c);
		++c;
	};
});

struct lifetime_tracker {
	bool has_reference = false;
};

struct ref {
	lifetime_tracker &var;
	bool valid = true;

	explicit ref(lifetime_tracker &v) : var{v} {
		if (v.has_reference) {
			throw test_failure_exception("failed to destroy object");
		}
		v.has_reference = true;
	}

	ref(ref &&rhs) noexcept : var{rhs.var} {
		rhs.valid = false;
	}

	~ref() {
		if (valid)
			var.has_reference = false;
	}
};

[[maybe_unused]] test& ring_class_test = g_io_tests.make_test("ring<class>", "Circular buffer with class type", [](test &) {
	std::array<lifetime_tracker, 16> data;

	{
		octahedron::ring<ref, 16> ring;

		ring.push_back(ref{data[0]});
	}
	if (data[0].has_reference)
		throw test_failure_exception("failed to destroy object");
	{
		octahedron::ring<ref, 16> ring;

		ring.push_back(ref{data[0]});
		ring.push_back(ref{data[1]});
		ring.pop_front();
		if (data[0].has_reference)
			throw test_failure_exception("failed to destroy object");
	}
});
}


