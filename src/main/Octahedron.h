#ifndef OCTAHEDRON_H_
#define OCTAHEDRON_H_

#include <string_view>

#include "Base.h"
#include "Tools/ProtectedPtr.h"
#include "IO/Logger.h"

namespace std::filesystem
{
};

namespace Octahedron
{
  class Engine;
  class GameEngine;

  // The main way a game will access the non-graphics part of the engine
  inline ProtectedPtr<Engine> g_engine;

  // The main way a game will access the whole engine
  // Is nullptr if the engine is built without graphics, see `engine` instead
  inline ProtectedPtr<GameEngine> g_game_engine;

  template <size_t N, std::invocable Generator>
  auto generate(Generator generator) -> std::array<std::invoke_result_t<Generator>, N>
  {
    using return_type = std::array<std::invoke_result_t<Generator>, N>;

    constexpr auto impl =
      []<size_t... Ns>(Generator impl_generator, std::index_sequence<Ns...>) -> return_type
    {
      constexpr auto gen = []<size_t N>(Generator generator_) { return (generator_()); };

      return {gen.template operator()<Ns>(impl_generator)...};
    };
    return (impl(generator, std::make_index_sequence<N>()));
  }

  bool isLogEnabled(BitSet<LogLevel> logLevel) noexcept;

  void log(BitSet<LogLevel> level, std::string_view line);

  template <typename... Args>
  requires(sizeof...(Args) > 0)
  void log(BitSet<LogLevel> level, fmt::format_string<to_loggable<Args>...> fmt, Args &&...args)
  {
    if (!isLogEnabled(level))
      return;
    //auto line = fmt::format(fmt, (Formatter<std::decay_t<decltype(args)>>(loggable_helper<Args>::get(args)))...);
    auto line = fmt::format(fmt, loggable_helper<Args>::get(args)...);

    log(level, line);
  }

  template <typename T>
  constexpr inline T error_value{};

  template <>
  constexpr inline size_t error_value<size_t> = std::numeric_limits<size_t>::max();

  constexpr inline size_t error_size = error_value<size_t>;

// clang-format off
  #define OPENING_PARENTHESIS (
  #define CLOSING_PARENTHESIS )
  #define LAMBDA_START [&]() { return OPENING_PARENTHESIS
  #define LAMBDA_END CLOSING_PARENTHESIS; }

  #define CLOSURE(a, ...)\
    LAMBDA_START a LAMBDA_END __VA_OPT__(,) __VA_ARGS__
  // clang-format on
};  // namespace Octahedron

#endif
