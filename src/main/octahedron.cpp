#include "octahedron.h"

#include "engine/engine.h"

bool octahedron::is_log_enabled(bit_set<log_level> level) noexcept {
	return (g_engine->is_log_enabled(level));
}

void octahedron::log(bit_set<log_level> level, std::string_view line) {
	g_engine->log(level, line);
}
