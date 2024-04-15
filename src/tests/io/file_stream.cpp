#include "../../penteract/shared/cube.h"

#include "io/io.h"
#include "tests.h"

#include <io/utf8_stream.h>

using namespace octahedron::tests;

[[maybe_unused]] test &utf8_stream_test = g_io_tests.make_test("utf8_stream", "Utf8 streams", [](test& self) {
	octahedron::file_system fs;
	octahedron::utf8_stream stream(fs.open("testdata/utf8_no_bom.txt", octahedron::open_flags::INPUT));
	char buffer1[33];
	char buffer2[33];
	size_t characters_read1;
	size_t characters_read2;
	size_t total_read = 0;
	auto *tess = openutf8file("testdata/utf8_no_bom.txt", "r");

	do {
		characters_read1 = stream.read_cube(buffer1, 32);
		characters_read2 = tess->read(buffer2, 32);

		buffer1[characters_read1] = 0;
		buffer2[characters_read2] = 0;
		total_read += characters_read1;
		if (memcmp(buffer1, buffer2, max(characters_read1, characters_read2)) != 0)
			self.fail(fmt::format("fail at {}:\n{}\n^ read\nv expected\n{}", total_read, buffer1, buffer2));
	} while (characters_read1 == 32 && characters_read2 == 32);

});
