#include "engine.h"
#include "octahedron.h"

#include <regex>
#include <variant>

#include "engine/engine.h"

#include <iostream>

#include "io/file_stream.h"
#include "io/file_system.h"
#include "io/logger.h"
#include "tools/exception.h"
#include "tools/registry.h"

using namespace octahedron;

namespace
{
template <typename T>
concept console_argument_handler = std::invocable<T, std::span<std::string_view>> &&
																	 std::convertible_to<
																		 std::invoke_result_t<T, std::span<std::string_view>>,
																		 std::variant<uint32, std::string>>;

struct console_argument {
	std::optional<std::string_view> short_name;
	std::optional<std::string_view> long_name;
	std::string_view                value;

	bool compare(std::string_view lhs) const {
		if (lhs[0] == '-') {
			if (lhs.size() > 1 && lhs[1] == '-')
				return (long_name && lhs.substr(2) == long_name.value());
			return (short_name && lhs.substr(1).starts_with(short_name.value()));
		}
		return (false);
	}
};

template <typename T>
struct arg_helper;

template <typename Ret, typename... Args>
struct arg_helper<Ret (*)(Args...)> {
	template <typename Fun> requires(std::invocable<Fun, Args...> && std::convertible_to<
																		 std::invoke_result_t<Fun, Args...>, bool>)
	static constexpr auto invert() {
		return [](Args...   _args) {
			return (!Fun()(_args...));
		};
	}
};

template <typename Fun>
struct Predicate : Fun {
	using Fun::operator();

	constexpr Predicate(Fun &&) {}

	constexpr const auto& operator!() const {
		return inverted;
	};

	static constexpr inline auto inverted = [](auto &&... args) {
		return (!Fun()(args...));
	};
};

constexpr inline Predicate isWhitespace = [](auto &&c) constexpr noexcept {
	if constexpr (!requires { c == char{}; })
		return (false);

	switch (c) {
		default:
			return (false);

		case ' ':
		case '\t':
		case '\n':
		case '\r':
			return (true);
	}
};



enum class opt_argument_type {
	/**
	 * @brief Option takes no value
	 */
	none,

	/**
	 * @brief Option has a required value
	 */
	required,

	/**
	 * @brief Option has an optional value
	 */
	optional,

	/**
	 * @brief Option has a multiple values
	 */
	multiple
};

/**
 * @brief Represents a command line option passed to the program
 */
struct cmd_option {
	/**
	 * @brief List of options the program uses
	 */
	enum options {
		output_file,
		max
	};

	/**
	 * @brief Short name of the option without the leading dash, e.g. `o` for output
	 */
	char short_opt = {};

	/**
	 * @brief Long name of the option without the leading dashes, e.g. `output` for output
	 */
	std::string_view long_opt = {};

	/**
	 * @brief Whether the option expects an argument or not
	 */
	opt_argument_type argument;

	/**
	 * @brief Value of the option
	 */
	std::vector<std::string_view> values = {};
};

using program_options_array = std::array<cmd_option, cmd_option::max>;


class cmd_option_exception : exception {
public:
	using exception::exception;
	using exception::operator=;
};

auto parse_cmd_options(int ac, char *av[]) {
	auto options = std::to_array<cmd_option>({
		{'g', "logfile", opt_argument_type::required, {}},
		{'u', "home", opt_argument_type::required, {}},
		{'k', "packagedirs", opt_argument_type::multiple, {}}
	});
	for (int i = 1; i < ac; ++i) {
		if (av[i][0] == '-') {
			auto it = options.end();

			if (av[i][1] == '-') { // long option
				const char *equal_chr = std::strchr(av[i] + 2, '=');
				std::string_view opt;

				if (equal_chr == nullptr) {
					opt = av[i] + 2;
				} else {
					opt = {av[i] + 2, equal_chr};
				}
				it = std::ranges::find(options, opt, &cmd_option::long_opt);

				if (it == options.end()) {
					throw cmd_option_exception{std::string{"unknown option "} + av[i]};
				}

				switch (it->argument) {
					using enum opt_argument_type;

					case required:
						if (equal_chr == nullptr) { // option is the next argument
							if (i + 1 >= ac) {
								throw cmd_option_exception{std::string{"option "} + av[i] + " requires an argument"};
							}

							it->values = { av[i + 1] };
							++i;
						} else { // option is in the same argument
							it->values = { equal_chr + 1 };
						}
						break;

					case optional:
						if (equal_chr == nullptr) {
							it->values = {};
						} else {
							it->values = { equal_chr + 1 };
						}
						break;

					case multiple:
						if (equal_chr == nullptr) {
							++i;
							while (i < ac && av[i][0] != '-') {
								it->values.emplace_back(av[i]);
								++i;
							}
						} else {
							const char *begin = equal_chr;
							const char *end;

							while (begin != nullptr) {
								++begin;
								end = std::strrchr(begin, ',');
								it->values.emplace_back(end == nullptr ? begin : std::string_view{begin, end});
								begin = end;
							}
						}


					case none:
						break;
				}
			} else {
				const char *opt = av[i] + 1;
				bool done = false;

				while (!done) {
					it = std::ranges::find(options, *opt, &cmd_option::short_opt);

					if (it == options.end()) {
						throw cmd_option_exception{std::string{"unknown option "} + av[i]};
					}

					switch (it->argument) {
						using enum opt_argument_type;

						case required:
							if (*(opt + 1) == 0) { // value is the next argument
								if (i + 1 >= ac) {
									throw cmd_option_exception{std::string{"option "} + av[i] + " requires an argument"};
								}

								it->values = { av[i + 1] };
								++i;
							} else { // value is in the same argument
								it->values = { opt + 1 };
							}
							done = true;
							break;

						case optional:
							it->values = { opt + 1 };
							done = true;
							break;

						case multiple:
							if (*(opt + 1) == 0) { // values are the next arguments
								++i;
								while (i < ac && av[i][0] != '-') {
									it->values.emplace_back(av[i]);
									++i;
								}
							} else {
								const char *begin = opt + 1;
								const char *end;

								while (begin != nullptr) {
									++begin;
									end = std::strrchr(begin, ',');
									it->values.emplace_back(end == nullptr ? begin : std::string_view{begin, end});
									begin = end;
								}
							}


						case none:
							++opt;
							done = *opt == 0;
							break;
					}
				}
			}
		}
	}
	return options;
}

} // namespace

#include "io/file_stream.h"

void engine::set_log_file(std::string_view file) {
	auto stream = _filesystem.open(file, open_flags::OUTPUT | open_flags::TRUNCATE);

	if (!stream) {
		_logger.log(log_level::ERROR, "failed to set log file to {}", file);
		return;
	}
	_log_file.target = std::move(stream);
	_logger.log(log_level::BASIC, "log file set to {}", file);
}

void engine::log(bit_set<log_level> level, std::string_view line) {
	_logger.log(log_level{level}, line);
}

engine::engine(int argc, char *argv[]) {
	if (g_engine != nullptr)
		throw exception("Another instance of the engine is already running");

	g_engine = this;

	namespace v = std::views;

	auto arguments = parse_cmd_options(argc, argv);

	if (!arguments[1].values.empty())
			_filesystem.set_home_dir(arguments[1].values[0]);
	if (!arguments[0].values.empty())
			set_log_file(arguments[0].values[0]);
	for (std::string_view dir : arguments[1].values) {
		_filesystem.add_package_dir(dir);
	}
}

void engine::seedRNG() {
	auto seed = _get_seed_base();

	_mt64 = std::mt19937_64{seed};
	_mt32 = std::mt19937{narrow_cast<uint32>(seed)};

	_rng = {
		.state{
			generate<16>(
				[this]() {
					return (narrow_cast<uint16>(this->_mt64()));
				}
			)
		}
	};
}

auto engine::get_file_system() const noexcept -> const file_system& {
	return (_filesystem);
}

auto engine::get_file_system() noexcept -> file_system& {
	return (_filesystem);
}

void engine::_parse_args(std::span<const char *const> args) {}

auto engine::_get_seed_base() const noexcept -> size_t {
	auto day_time = wall_clock::type::now();
	auto game_time = game_clock::type::now();

	return (day_time.time_since_epoch().count() ^ game_time.time_since_epoch().count());
}

auto engine::get_game_clock() const noexcept -> const game_clock& {
	return (_game_clock);
}

auto engine::get_wall_clock() const noexcept -> const wall_clock& {
	return (_day_clock);
}

bool engine::is_log_enabled(bit_set<log_level> logLevel) const noexcept {
	return (_logger.is_log_enabled(logLevel));
}
