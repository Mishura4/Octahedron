#include "Engine.h"
#include "Engine.h"
#include "Engine.h"
#include "Octahedron.h"

#include <regex>
#include <variant>

#include "Engine/Engine.h"
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

  constexpr inline Predicate isWhitespace = [](auto &&c)
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
  auto countArgs(std::span<std::string_view> args, ConsoleArgument &argument)
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

  template <std::ranges::range R, typename... Fields>
  size_t parseArgument(R args, Registry<Fields...> &registry)
  {
    using registry_type = Registry<Fields...>;

    constexpr auto impl =
      []<size_t... Ns>(R args, registry_type &registry, std::index_sequence<Ns...>)
    {
      size_t ret{1};

      ((registry.get<arg_field<registry_type, Ns>::key>().compare(args.front()) &&
        (ret = countArgs<Ns>(args, registry.get<arg_field<registry_type, Ns>::key>()), true)) ||
       ...);
      return (ret);
    };

    return {impl(args, registry, std::make_index_sequence<registry_type::key_list::size>())};
  }
}  // namespace

void Engine::setLogFile(std::string_view file)
{
  auto path = _filesystem.findFile(file, FileSystem::SearchMode::CREATE);

  if (!path)
  {
    _logger.log(LogLevel::ERROR, "invalid log file path: {}", CLOSURE(path->string()));
    return;
  }
  _log_file.target.open(*path, std::ios::out | std::ios::trunc);
  if (!_log_file.target.good())
    _logger.log(LogLevel::ERROR, "failed to set log file to {}", CLOSURE(path->string()));
  else
    _logger.log(LogLevel::BASIC, "log file set to {}", CLOSURE(path->string()));
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
  for (auto it = _args.begin(); it != _args.end();)
  {
    ++it;
    auto args_parsed = parseArgument(std::ranges::subrange(it, _args.end()), parameters);

    std::advance(it, args_parsed);
  }

  if (!parameters.get<"HOMEDIR">().value.empty())
    _filesystem.setHomeDir(parameters.get<"HOMEDIR">().value);
  if (!parameters.get<"LOGFILE">().value.empty())
    setLogFile(parameters.get<"LOGFILE">().value);

  namespace stdr = std::ranges;

  const auto packageDirs = parameters.get<"PACKAGEDIRS">().value;
  for (auto it = packageDirs.begin(); it != packageDirs.end();)
  {
    auto begin = stdr::find_if(stdr::subrange(it, packageDirs.end()), !isWhitespace);
    auto end   = stdr::find_if(stdr::subrange(begin, packageDirs.end()), isWhitespace);
    it         = end;
    _filesystem.addPackageDir({begin, end});
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