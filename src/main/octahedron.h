#ifndef OCTAHEDRON_H_
#define OCTAHEDRON_H_

#include <string_view>

#include "base.h"
#include "io/logger.h"
#include "tools/protected_ptr.h"

namespace std::filesystem {};

namespace octahedron
{

class engine;
class game_engine;

// The main way a game will access the non-graphics part of the engine
inline protected_ptr<engine> g_engine;

// The main way a game will access the whole engine
// Is nullptr if the engine is built without graphics, see `engine` instead
inline protected_ptr<game_engine> g_game_engine;

template <size_t N, std::invocable Generator>
std::array<std::invoke_result_t<Generator>, N> generate(Generator generator) {
	using return_type = std::array<std::invoke_result_t<Generator>, N>;

	constexpr auto               impl =
		[]<size_t... Ns>(Generator impl_generator, std::index_sequence<Ns...>) -> return_type {
		constexpr auto gen = []<size_t N>(Generator generator_) {
			return (generator_());
		};

		return {gen.template operator()<Ns>(impl_generator)...};
	};
	return (impl(generator, std::make_index_sequence<N>()));
}

bool is_log_enabled(bit_set<log_level> logLevel) noexcept;

void log(bit_set<log_level> level, std::string_view line);

template <typename... Args> requires(sizeof...(Args) > 0)
void log(bit_set<log_level> level, fmt::format_string<to_loggable<Args>...> fmt, Args &&... args) {
	if (!is_log_enabled(level))
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

}; // namespace octahedron

#endif
