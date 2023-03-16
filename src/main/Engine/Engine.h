#ifndef OCTAHEDRON_ENGINE_H_
#define OCTAHEDRON_ENGINE_H_

#include <array>
#include <chrono>
#include <filesystem>
#include <fstream>
#include <random>
#include <span>

#include "Octahedron.h"

#include "../IO/Logger.h"
#include "../Tools/ProtectedPtr.h"
#include "../IO/FileSystem.h"
#include "../IO/FileStream.h"

namespace Octahedron
{
  class Engine
  {
    public:
      template <typename T>
      requires requires { T::now(); }
      class Clock
      {
        public:
          using Timestamp = typename T::time_point;
          using Duration  = microseconds;
          using Type      = T;

          struct Tick
          {
              using Clock = Engine::Clock<T>;

              Timestamp time;
              Duration real_diff;
              Duration clamped_diff;
          };

          const Tick &update(Duration min_duration, Duration max_duration);
          const Tick &update(Duration min_duration);
          const Tick &update() noexcept;

          const Tick &lastTick() const noexcept;
          const Timestamp &startTimestamp() const noexcept;

        private:
          const Timestamp _start_time{T::now()};
          Tick _last_tick{_start_time, Duration::zero(), Duration::zero()};
      };

      using GameClock = Clock<std::chrono::steady_clock>;
      using DayClock  = Clock<std::chrono::system_clock>;

      struct Parameter
      {
          std::string_view parameter;
          std::string_view value;
      };

      Engine(int ac, const char *const argv[]);
      Engine(const Engine &) = delete;
      Engine(Engine &&)      = delete;

      Engine &operator=(const Engine &) = delete;
      Engine &operator=(Engine &&)      = delete;

      const GameClock &gameClock() const noexcept;
      const DayClock &dayClock() const noexcept;

      bool isLogEnabled(BitSet<LogLevel> logLevel) const noexcept;

      void seedRNG();

      enum class State
      {
        NONE     = 0,
        STARTING = 1,
        RUNNING  = 1
      };

      void setLogFile(std::string_view file);

      void log(BitSet<LogLevel> level, std::string_view line);

      template <typename... Args>
      requires(sizeof...(Args) > 0)
      void log(BitSet<LogLevel> level, fmt::format_string<to_loggable<Args>...> fmt, Args &&...args)
      {
        if (!_logger.isLogEnabled(LogLevel{level}))
          return;
        auto line = fmt::format(fmt, loggable_helper<Args>::get(args)...);

        log(level, line);
      }

      auto fileSystem() const noexcept -> const FileSystem &;
      auto fileSystem() noexcept -> FileSystem &;

    private:
      void _parseArgs(std::span<const char *const> args);
      size_t _getSeedBase() const noexcept;

      Logger<std::unique_ptr<FileStream>> _log_file{nullptr};
      Logger<std::fstream> _debug_log_file{std::fstream{}};
      LoggerSystem<decltype(_log_file), Logger<std::fstream>> _logger{_log_file, _debug_log_file};

      GameClock _game_clock{};
      DayClock _day_clock{};

      std::mt19937 _mt32{uint32(_getSeedBase())};
      std::mt19937_64 _mt64{_getSeedBase()};

      struct WellRNG512
      {
          std::array<uint16, 16> state{};
          uint64 index{0};

          uint64 operator()();
      };

      WellRNG512 _wellRNG512{
        .state{generate<16>([this]() { return (narrow_cast<uint16>(this->_mt64())); })}};

      State _state{State::NONE};

      FileSystem _filesystem;
  };

  template <typename T>
  requires requires { T::now(); }
  auto Engine::Clock<T>::update(Duration min_duration, Duration max_duration) -> const Tick &
  {
    Timestamp now      = T::now();
    Duration real_diff = now - _last_tick.time;

    if (real_diff < min_duration)
    {
      std::this_thread::sleep_for(min_duration - real_diff);
      now       = T::now();
      real_diff = now - _last_tick.time;
    }
    _last_tick = {
      .time         = now,
      .real_diff    = real_diff,
      .clamped_diff = (max_duration > real_diff ? max_duration : real_diff)};
    return (_last_tick);
  }

  template <typename T>
  requires requires { T::now(); }
  auto Engine::Clock<T>::update(Duration min_duration) -> const Tick &
  {
    Timestamp now      = T::now();
    Duration real_diff = now - _last_tick.time;

    if (real_diff < min_duration)
    {
      std::this_thread::sleep_for(min_duration - real_diff);
      now       = T::now();
      real_diff = now - _last_tick.time;
    }
    _last_tick = {.time = now, .real_diff = real_diff, .clamped_diff = real_diff};
    return (_last_tick);
  }

  template <typename T>
  requires requires { T::now(); }
  auto Engine::Clock<T>::update() noexcept -> const Tick &
  {
    Timestamp now      = T::now();
    Duration real_diff = now - _last_tick.time;

    _last_tick = {.time = now, .real_diff = real_diff, .clamped_diff = real_diff};
    return (_last_tick);
  }

  template <typename T>
  requires requires { T::now(); }
  auto Engine::Clock<T>::lastTick() const noexcept -> const Tick &
  {
    return (_last_tick);
  }

  template <typename T>
  requires requires { T::now(); }
  auto Engine::Clock<T>::startTimestamp() const noexcept -> const Timestamp &
  {
    return (_start_time);
  }
}  // namespace Octahedron

#endif
