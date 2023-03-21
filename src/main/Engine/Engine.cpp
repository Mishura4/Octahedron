#include "Engine.h"
#include "Octahedron.h"

#include <regex>
#include <variant>

#include "Engine/Engine.h"

#include <iostream>

#include "IO/FileSystem.h"
#include "IO/FileStream.h"
#include "IO/Logger.h"
#include "Tools/Exception.h"
#include "Tools/Registry.h"

using namespace Octahedron;

namespace
{
  template <typename T>
  concept console_argument_handler = std::invocable<T, std::span<std::string_view>> &&
                                     std::convertible_to<
                                       std::invoke_result_t<T, std::span<std::string_view>>,
                                       std::variant<uint32, std::string>>;

  struct ConsoleArgument
  {
      std::optional<std::string_view> short_name;
      std::optional<std::string_view> long_name;
      std::string_view value;

      bool compare(std::string_view lhs) const
      {
        if (lhs[0] == '-')
        {
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
  struct arg_helper<Ret (*)(Args...)>
  {
      template <typename Fun>
      requires(std::invocable<Fun, Args...> && std::convertible_to<std::invoke_result_t<Fun, Args...>, bool>)
      static constexpr auto invert()
      {
        return [](Args... _args) { return (!Fun()(_args...)); };
      }
  };

  template <typename Fun>
  struct Predicate : Fun
  {
      using Fun::operator();

      constexpr Predicate(Fun &&) {}

      constexpr const auto &operator!() const
      {
        return inverted;
      };

      static constexpr inline auto inverted = [](auto &&...args) { return (!Fun()(args...)); };
  };

  constexpr inline Predicate isWhitespace = [](auto &&c) constexpr noexcept
  {
    if constexpr (!requires { c == char{}; })
      return (false);

    switch (c)
    {
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
  auto count_args(std::span<std::string_view> args, ConsoleArgument &argument)
  {
    uint32 args_parsed{0};

    if (args[0][0] == '-' && args[0].size() > 1 && args[0][1] != '-')
    {
      args[0] = args[0].substr(2);
    }
    else
    {
      args = args.subspan(1);
      ++args_parsed;
      for (std::string_view arg : args)
      {
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

  template <typename R, typename... Fields>
	requires(std::ranges::range<std::remove_reference_t<R>>)
  size_t parse_argument(R &&args, Registry<Fields...> &registry)
  {
    using registry_type = Registry<Fields...>;

    constexpr auto impl0 =
      []<size_t... Ns>(R args, registry_type &registry, std::index_sequence<Ns...>)
		{
			size_t         ret{1};
			constexpr auto impl1 = []<size_t N>(R args, registry_type & registry, size_t &nargs) -> bool
			{
				constexpr auto key = arg_field<registry_type, N>::key;

				if (auto &field = registry.template get<key>(); field.compare(args.front()))
				{
					nargs = count_args<N>(args, field);
					return (true);
				}
				return (false);
			};

      (impl1.template operator()<Ns>(args, registry, ret) || ...);
      return (ret);
    };

    return {impl0(args, registry, std::make_index_sequence<registry_type::key_list::size>())};
  }
}  // namespace

#include "IO/FileStream.h"

void Engine::setLogFile(std::string_view file)
{
  auto stream = _filesystem.open(file, OpenFlags::OUTPUT | OpenFlags::TRUNCATE);

  if (!stream)
  {
    _logger.log(LogLevel::ERROR, "failed to set log file to {}", file);
    return;
  }
  _log_file.target = std::move(stream);
  _logger.log(LogLevel::BASIC, "log file set to {}", file);
}

void Octahedron::Engine::log(BitSet<LogLevel> level, std::string_view line)
{
  _logger.log(LogLevel{level}, line);
}

Engine::Engine(int argc, const char *const argv[])
{
  if (Octahedron::g_engine != nullptr)
    throw FatalException<"Another instance of the engine is already running">();

  Octahedron::g_engine = this;

  namespace v = std::views;

  Registry parameters = {
    field<"LOGFILE">(ConsoleArgument{"g"sv, "logfile"sv}),
    field<"HOMEDIR">(ConsoleArgument{"u"sv, "home"sv}),
    field<"PACKAGEDIRS">(ConsoleArgument{"k"sv, "packagedirs"sv})};

  std::vector<std::string_view> _args{argv, argv + argc};
	size_t args_parsed = 1;
  while (args_parsed < argc)
  {
    size_t n = parse_argument(_args | v::drop(args_parsed), parameters);

    args_parsed += n;
  }

  if (!parameters.get<"HOMEDIR">().value.empty())
    _filesystem.setHomeDir(parameters.get<"HOMEDIR">().value);
  if (!parameters.get<"LOGFILE">().value.empty())
    setLogFile(parameters.get<"LOGFILE">().value);

	namespace stdr = std::ranges;

	std::string_view package_dirs     = parameters.get<"PACKAGEDIRS">().value;
	char             escape_delimiter = 0;
	constexpr auto             is_dir           = [&](auto c) noexcept -> bool
	{
		if (isWhitespace(c) && escape_delimiter == 0)
      return (false);
		if (c == escape_delimiter)
			escape_delimiter = 0;
		return (true);
	};

	while (!package_dirs.empty())
  {
		auto end = std::ranges::find_if_not(package_dirs, is_dir);
		auto range = package_dirs | v::take(end - package_dirs.begin());

		_filesystem.addPackageDir(range);
		auto next = std::ranges::find_if_not(end, package_dirs.end(), isWhitespace);
		package_dirs = package_dirs | v::drop(next - package_dirs.begin());
  }
}

void Engine::seedRNG()
{
  auto seed = _getSeedBase();

  _mt64 = std::mt19937_64{seed};
  _mt32 = std::mt19937{narrow_cast<uint32>(seed)};

  _wellRNG512 = {.state{generate<16>([this]() { return (narrow_cast<uint16>(this->_mt64())); })}};
}

auto Octahedron::Engine::fileSystem() const noexcept -> const FileSystem &
{
  return (_filesystem);
}

auto Octahedron::Engine::fileSystem() noexcept -> FileSystem &
{
  return (_filesystem);
}

void Engine::_parseArgs(std::span<const char *const> args) {}

size_t Engine::_getSeedBase() const noexcept
{
  auto day_time  = DayClock::Type::now();
  auto game_time = GameClock::Type::now();

  return (day_time.time_since_epoch().count() ^ game_time.time_since_epoch().count());
}

auto Engine::gameClock() const noexcept -> const GameClock &
{
  return (_game_clock);
}

auto Engine::dayClock() const noexcept -> const DayClock &
{
  return (_day_clock);
}

bool Engine::isLogEnabled(BitSet<LogLevel> logLevel) const noexcept
{
  return (_logger.isLogEnabled(logLevel));
}