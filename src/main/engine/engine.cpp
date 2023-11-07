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

template <size_t N>
auto count_args(std::span<std::string_view> args, console_argument &argument) {
	uint32 args_parsed{0};

	if (args[0][0] == '-' && args[0].size() > 1 && args[0][1] != '-') {
		args[0] = args[0].substr(2);
	} else {
		args = args.subspan(1);
		++args_parsed;
		for (std::string_view arg : args) {
			if (arg.starts_with('-'))
				break;
			++args_parsed;
		}
	}
	argument.value = {args.empty() ? std::string_view{} : args.front()};
	return (args_parsed);
}

template <typename T, size_t N>
using arg_field = typename T::field_type_list::template at<N>;

template <typename R, typename... Fields> requires(std::ranges::range<std::remove_reference_t<R>>)
size_t parse_argument(R &&args, registry<Fields...> &reg) {
	using registry_type = registry<Fields...>;

	constexpr auto       impl0 =
		[]<size_t... Ns>(R args, registry_type &reg, std::index_sequence<Ns...>) {
		size_t         ret{1};
		constexpr auto impl1 = []<size_t N>(R args, registry_type &reg, size_t &nargs) -> bool {
			constexpr auto key = arg_field<registry_type, N>::key;

			if (auto &field = reg.template get<key>(); field.compare(args.front())) {
				nargs = count_args<N>(args, field);
				return (true);
			}
			return (false);
		};

		(impl1.template operator()<Ns>(args, reg, ret) || ...);
		return (ret);
	};

	return {impl0(args, reg, std::make_index_sequence<registry_type::key_list::size>())};
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

engine::engine(int argc, const char *const argv[]) {
	if (g_engine != nullptr)
		throw exception("Another instance of the engine is already running");

	g_engine = this;

	namespace v = std::views;

	registry parameters = {
		field<"LOGFILE">(console_argument{"g"sv, "logfile"sv}),
		field<"HOMEDIR">(console_argument{"u"sv, "home"sv}),
		field<"PACKAGEDIRS">(console_argument{"k"sv, "packagedirs"sv})
	};

	std::vector<std::string_view> _args{argv, argv + argc};
	size_t                        args_parsed = 1;
	while (args_parsed < argc) {
		size_t n = parse_argument(_args | v::drop(args_parsed), parameters);

		args_parsed += n;
	}

	if (!parameters.get<"HOMEDIR">().value.empty())
		_filesystem.set_home_dir(parameters.get<"HOMEDIR">().value);
	if (!parameters.get<"LOGFILE">().value.empty())
		set_log_file(parameters.get<"LOGFILE">().value);

	namespace stdr = std::ranges;

	std::string_view package_dirs = parameters.get<"PACKAGEDIRS">().value;
	constexpr char   escape_delimiter = 0;
	constexpr auto   is_dir = [](auto c) noexcept -> bool {
		if (isWhitespace(c) && escape_delimiter == 0)
			return (false);
		/*
		if (c == escape_delimiter)
			escape_delimiter = 0;
			*/
		return (true);
	};

	while (!package_dirs.empty()) {
		auto end = std::ranges::find_if_not(package_dirs, is_dir);
		auto range = package_dirs | v::take(end - package_dirs.begin());

		_filesystem.add_package_dir(range);
		auto next = std::ranges::find_if_not(end, package_dirs.end(), isWhitespace);
		package_dirs = package_dirs | v::drop(next - package_dirs.begin());
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
