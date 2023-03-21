#ifndef OCTAHEDRON_LOGGER_H_
#define OCTAHEDRON_LOGGER_H_

#include <functional>
#include <ostream>
#include <type_traits>
#include <vector>
#include <ranges>

#include <fmt/format.h>

#include "../Octahedron.h"
#include "../Tools/ManagedResource.h"

#include "IOInterface.h"
#include "../Tools/TypeList.h"

#ifdef ERROR
#  undef ERROR
#endif

namespace Octahedron
{
  namespace _
  {
    enum LogLevel : uint32
    {
      NONE  = Octahedron::bitflag<LogLevel>(0),
      BASIC = Octahedron::bitflag<LogLevel>(1),
      INFO  = Octahedron::bitflag<LogLevel>(2),
      WARN  = Octahedron::bitflag<LogLevel>(3),
      ERROR = Octahedron::bitflag<LogLevel>(4),
      DEBUG = Octahedron::bitflag<LogLevel>(5),
      TRACE = Octahedron::bitflag<LogLevel>(6),
      ALL   = Octahedron::bitflag<LogLevel>(uint32(-1))
    };
  }

  using LogLevel = _::LogLevel;

  template <typename T>
  struct LoggerBase;

  template <typename T>
  concept logger_type = requires(T &t) {
    t.write(LogLevel::BASIC, ""sv);
    LogLevel{t.log_level};
	};

  template <typename T>
	struct Formatter;
	/*

	template <typename T>
	struct Formatter
	{
			constexpr Formatter() = default;

			Formatter(const T &val) :
					value(&val)
			{

			}

			Formatter(const T *val) :
				value(val)
			{

			}

			template <typename ParseContext>
			constexpr auto parse(ParseContext &ctx)
			{
				return (_formatter.parse(ctx));
			}

			template <typename FormatContext>
			auto format(const Formatter &me, FormatContext &ctx)
			{
				return (ctx.out());
			}

		  const T *value{nullptr};
			::fmt::formatter<T> _formatter{};
	};*/

  template <typename T>
  struct loggable_helper
  {
      static auto get(const T &u) noexcept
			{
				if constexpr (requires {Formatter<T>::exists;})
					;// todo?
				else
					return (u);
			}

			using type = std::remove_reference_t<decltype(get(std::declval<const T &>()))>;

  };

  template <typename T>
  requires(std::invocable<T>)
  struct loggable_helper<T>
  {
      using type = std::remove_reference_t<std::invoke_result_t<T>>;

      template <typename U>
      requires(std::same_as<std::remove_cvref_t<U>, std::remove_cvref_t<T>>)
      static auto get(U &&u) noexcept(std::is_nothrow_invocable_v<U>)
      {
        return (std::invoke(std::forward<U>(u)));
      }
  };

  template <typename T>
  using to_loggable = typename loggable_helper<T>::type;

  template <typename T>
  struct LoggerBase
  {
      using PrefixGenerator = std::function<std::string(LogLevel, std::string_view)>;
      std::underlying_type_t<LogLevel> log_level{LogLevel::ALL};
      PrefixGenerator prefix_generator{make_empty<std::string>};
      PrefixGenerator suffix_generator{make_empty<std::string>};

      void log(LogLevel level, std::string_view str)
      {
        auto &self = *static_cast<T *>(this);

        if (!(level & self.log_level))
          return;
        self.write(level, str);
      }

      template <typename... Args>
      requires(sizeof...(Args) > 0)
      void log(LogLevel level, fmt::format_string<to_loggable<Args>...> fmt, Args &&...args)
      {
        auto &self = *static_cast<T *>(this);

        if (!(level & self.log_level))
          return;
        self.write(level, fmt::format(fmt, loggable_helper<Args>::get(args)...));
      }
  };

  template <typename T>
  struct Logger;

  template <typename T>
  requires(!std::is_scalar_v<T>)
  Logger(T &target, auto...) -> Logger<std::conditional_t<std::is_scalar_v<T>, T, T &>>;

  template <typename T>
  Logger(T target, auto...) -> Logger<T>;

  template <typename T>
  requires(std::is_convertible_v<T &, std::ostream &>)
  struct Logger<T &> : public LoggerBase<Logger<T &>>
  {
      using base = LoggerBase<Logger<T &>>;
      using base::prefix_generator;
      using base::suffix_generator;

      Logger(std::ostream &target_, auto &&...args) : base{args...}, target(target_) {}

      Logger(const Logger &) = delete;
      Logger(Logger &&)      = default;

      std::ostream &target;

      void write(LogLevel level, std::string_view msg)
      {
        if (prefix_generator)
          target << prefix_generator(level, msg);
        target << msg;
        if (suffix_generator)
          target << suffix_generator(level, msg);
        target << '\n';
      }
  };

  template <typename T>
  requires(std::is_convertible_v<T &, std::ostream &>)
  struct Logger<T> : public LoggerBase<Logger<T>>
  {
      using base = LoggerBase<Logger<T>>;
      using base::prefix_generator;
      using base::suffix_generator;

      T target;

      Logger(T &&target_, auto &&...args) : base{args...}, target{std::forward<T>(target_)} {}

      Logger(const Logger &) = delete;
      Logger(Logger &&)      = default;

      void write(LogLevel level, std::string_view msg)
      {
        if (prefix_generator)
          target << prefix_generator(level, msg);
        target << msg;
        if (suffix_generator)
          target << suffix_generator(level, msg);
        target << '\n';
      }
  };

  template <typename T>
  requires(writeable<T>)
  struct Logger<std::unique_ptr<T>> : LoggerBase<Logger<std::unique_ptr<T>>>
  {
      using base = LoggerBase<Logger<std::unique_ptr<T>>>;
      using base::prefix_generator;
      using base::suffix_generator;

      std::unique_ptr<T> target;

      Logger(std::unique_ptr<T>&& target_, auto &&...args) : base(args...), target(std::move(target_))
      {

      }

      Logger(const Logger &) = delete;
      Logger(Logger &&)      = default;

      void write(LogLevel level, std::string_view msg)
      {
        const std::byte *data = reinterpret_cast<const std::byte *>(msg.data());

        if (!target)
          return;
        if (prefix_generator)
        {
          std::string prefix = prefix_generator(level, msg);

          target->write(reinterpret_cast<const std::byte *>(prefix.c_str()), prefix.size());
        }
        target->write(data, msg.size());
        if (suffix_generator)
        {
          std::string suffix = suffix_generator(level, msg);

          target->write(reinterpret_cast<const std::byte *>(suffix.c_str()), suffix.size());
        }
        target->write("\n", 1);
        if constexpr (requires(T & t) { t.flush(); })
        {
          target->flush();
        }
      }
  };

  template <>
  struct Logger<std::FILE *> : public LoggerBase<Logger<std::FILE *>>
  {
      using base = LoggerBase<Logger<std::FILE *>>;
      using base::prefix_generator;
      using base::suffix_generator;

      std::FILE *target;

      Logger(std::FILE *target_, auto &&...args) : LoggerBase{args...}, target(target_) {}

      Logger(const Logger &) = delete;
      Logger(Logger &&)      = default;

      void write(LogLevel level, std::string_view msg)
      {
        if (prefix_generator)
          _write(prefix_generator(level, msg));
        _write(msg);
        if (suffix_generator)
          _write(suffix_generator(level, msg));
        _write('\n');
      }

    private:
      std::size_t _write(std::string_view str)
      {
        return (std::fwrite(str.data(), sizeof(char), str.size(), target));
      }

      std::size_t _write(char c) { return (std::fwrite(&c, sizeof(char), 1, target)); }
  };

  template <typename T>
  struct is_managed_file_s
  {
      static constexpr inline bool value = false;
  };

  template <std::invocable<std::FILE *> auto Releaser>
  struct is_managed_file_s<ManagedResource<std::FILE *, Releaser>>
  {
      static constexpr inline bool value = true;
  };

  template <typename T>
  constexpr inline bool is_managed_file = is_managed_file_s<T>::value;

  template <typename T>
  concept managed_file = is_managed_file<T>;

  template <managed_file T>
  struct Logger<T> : public LoggerBase<Logger<T>>
  {
      using base = LoggerBase<Logger<T>>;
      using base::prefix_generator;
      using base::suffix_generator;
      using target_type = T;

      target_type target;

      Logger(target_type &&target_, auto &&...args) :
          LoggerBase{args...}, target(std::move(target_))
      {
      }

      Logger(const Logger &) = delete;
      Logger(Logger &&)      = default;

      void write(LogLevel level, std::string_view msg)
      {
        if (prefix_generator)
          _write(prefix_generator(level, msg));
        _write(msg);
        if (suffix_generator)
          _write(suffix_generator(level, msg));
        _write('\n');
      }

    private:
      std::size_t _write(std::string_view str)
      {
        return (std::fwrite(str.data(), sizeof(char), str.size(), target.get()));
      }

      std::size_t _write(char c) { return (std::fwrite(&c, sizeof(char), 1, target.get())); }
  };

  template <logger_type... Loggers>
  class LoggerSystem
  {
    public:
      template <logger_type... LoggersArgs>
      explicit constexpr LoggerSystem(LoggersArgs &...args)
      {
        (addLogger(args), ...);
      }

      template <logger_type T>
      constexpr bool addLogger(T &logger)
      {
        constexpr size_t tuplePos = _findLoggerVectorPos<T, 0>();

        if constexpr (tuplePos == std::numeric_limits<size_t>::max())
        {
          static_assert(tuplePos != std::numeric_limits<size_t>::max(), "unsupported logger type");
          return (false);
        }
        else
        {
          _collectiveLogLevel |= static_cast<LogLevel>(logger.log_level);
          std::get<tuplePos>(_loggers).emplace_back(&logger);
          return (true);
        }
      }

      template <typename... Args>
      requires(sizeof...(Args) > 0)
      void log(LogLevel level, fmt::format_string<to_loggable<Args>...> fmt, Args &&...args)
      {
        if (!((_collectiveLogLevel & level) == level))
          return;
        auto line = fmt::format(fmt, loggable_helper<Args>::get(args)...);

        _log(level, line, _idx_seq);
      }

      void log(LogLevel level, std::string_view line) { _log(level, line, _idx_seq); }

      bool isLogEnabled(LogLevel level) const { return {(_collectiveLogLevel & level) == level}; }

    private:
      using LoggerList = std::tuple<std::vector<Loggers *>...>;

      constexpr static inline auto _idx_seq =
        std::make_index_sequence<std::tuple_size_v<LoggerList>>();

      template <size_t... Ns>
      void _log(LogLevel level, std::string_view line, std::index_sequence<Ns...>)
      {
        constexpr auto impl =
          []<size_t N>(LoggerList &loggers, LogLevel level, std::string_view line)
        {
          for (auto logger : std::get<N>(loggers))
            logger->log(level, line);
        };

        (impl.template operator()<Ns>(_loggers, level, line), ...);
      }

      template <logger_type T, size_t N>
      static constexpr size_t _findLoggerVectorPos()
      {
        if constexpr (std::tuple_size_v < LoggerList >> N)
        {
          if constexpr (std::is_same_v<std::vector<T *>, std::tuple_element_t<N, LoggerList>>)
            return (N);
          else
            return (_findLoggerVectorPos<T, N + 1>());
        }
        else
          return (std::numeric_limits<size_t>::max());
      }

      LogLevel _collectiveLogLevel{LogLevel::NONE};
      LoggerList _loggers;
	};
}  // namespace Octahedron

//template <typename T>
//struct ::fmt::formatter<Octahedron::Formatter<T>>
//{
//		template <typename ParseContext>
//		constexpr auto parse(ParseContext &ctx)
//		{
//			return (_formatter.parse(ctx));
//		}
//
//		template <typename FormatContext>
//		auto format(const Octahedron::Formatter<T> &me, FormatContext &ctx)
//		{
//			return (_formatter.format(me, ctx));
//		}
//
//		Octahedron::Formatter<T> _formatter{};
//};

template <typename T>
requires(std::is_enum_v<T>)
struct ::fmt::formatter<Octahedron::BitSet<T>>
{
	template <typename ParseContext>
	constexpr auto parse(ParseContext &ctx)
	{
		return (_formatter.parse(ctx));
	}

	template <typename FormatContext>
	auto format(const Octahedron::BitSet<T> &me, FormatContext &ctx)
	{
		return (_formatter.format(me.value, ctx));
	}

	::fmt::formatter<std::underlying_type_t<T>> _formatter{};
};

#endif
