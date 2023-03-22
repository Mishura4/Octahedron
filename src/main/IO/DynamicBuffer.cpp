#include "Serializer.h"

using namespace Octahedron;

void foo()
{
	uint8 *buf{new uint8[512]};

	Serializer toto(buf, 512);
}