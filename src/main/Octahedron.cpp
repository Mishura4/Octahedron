#include "Octahedron.h"

#include "Engine/Engine.h"

bool Octahedron::isLogEnabled(BitSet<LogLevel> level) noexcept
{
  return (g_engine->isLogEnabled(level));
}

void Octahedron::log(BitSet<LogLevel> level, std::string_view line)
{
  g_engine->log(level, line);
}
