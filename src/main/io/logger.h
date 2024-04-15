#ifndef OCTAHEDRON_LOGGER_H_
#define OCTAHEDRON_LOGGER_H_

#include <functional>
#include <ostream>
#include <ranges>
#include <type_traits>
#include <vector>

#include <fmt/format.h>
#include <fmt/std.h>

#include "../tools/managed_resource.h"

#include "io_interface.h"
#include "../tools/type_list.h"

#ifdef ERROR
#  undef ERROR
#endif

namespace octahedron
{

namespace _
{
enum log_level : uint32 {
	NONE  = octahedron::bitflag<log_level>(0),
	BASIC = octahedron::bitflag<log_level>(1),
	INFO  = octahedron::bitflag<log_level>(2),
	WARN  = octahedron::bitflag<log_level>(3),
	ERROR = octahedron::bitflag<log_level>(4),
	DEBUG = octahedron::bitflag<log_level>(5),
	TRACE = octahedron::bitflag<log_level>(6),
	ALL   = octahedron::bitflag<log_level>(uint32(-1))
};
}

using log_level = _::log_level;

template <typename T>
struct logger_base;

template <typename T>
concept logger_type = requires(T &t) {
	t.write(log_level::BASIC, ""sv);
	log_level{t.level};
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
struct loggable_helper : std::identity {
	T&& operator()(T&& v) const noexcept requires(!std::is_reference_v<T> || std::is_rvalue_reference_v<T>) {
		return static_cast<T&&>(v);
	}

	T& operator()(T& v) const noexcept requires(std::is_lvalue_reference_v<T>) {
		return v;
	}

	using type = T;
};

template <typename T> requires(std::invocable<T>)
struct loggable_helper<T> {
	using type = std::invoke_result_t<T>;

	template <typename U> requires(std::same_as<std::remove_cvref_t<U>, std::remove_cvref_t<T>>)
	constexpr std::invoke_result_t<T> operator()(U&& u) noexcept(std::is_nothrow_invocable_v<U>) {
		return (std::invoke(std::forward<U>(u)));
	}
};

template <typename T>
using to_loggable = typename loggable_helper<T>::type;

template <typename T>
struct logger_base {
	using affix_generator = std::function<std::string(log_level, std::string_view)>;
	std::underlying_type_t<log_level> level{log_level::ALL};
	affix_generator                   prefix_generator{make_empty<std::string>};
	affix_generator                   suffix_generator{make_empty<std::string>};

	void log(log_level level, std::string_view str) {
		auto &self = *static_cast<T*>(this);

		if (!(level & self.level))
			return;
		self.write(level, str);
	}

	template <typename... Args> requires(sizeof...(Args) > 0)
	void log(log_level level, fmt::format_string<to_loggable<Args>...> fmt, Args &&... args) {
		auto &self = *static_cast<T*>(this);

		if (!(level & self.level))
			return;
		self.write(level, fmt::format(fmt, loggable_helper<Args>{}(std::forward<Args>(args))...));
	}
};

template <typename T>
struct logger;

template <typename T> requires(!std::is_scalar_v<T>)
logger(T &target, auto...) -> logger<std::conditional_t<std::is_scalar_v<T>, T, T&>>;

template <typename T>
logger(T target, auto...) -> logger<T>;

template <typename T> requires(std::is_convertible_v<T&, std::ostream&>)
struct logger<T&> : public logger_base<logger<T&>> {
	using base = logger_base<logger<T&>>;
	using base::prefix_generator;
	using base::suffix_generator;

	logger(std::ostream &target_, auto &&... args) :
		base{args...},
		target(target_) {}

	logger(const logger &) = delete;
	logger(logger &&) = default;

	std::ostream &target;

	void write(log_level level, std::string_view msg) {
		if (prefix_generator)
			target << prefix_generator(level, msg);
		target << msg;
		if (suffix_generator)
			target << suffix_generator(level, msg);
		target << '\n';
	}
};

template <typename T> requires(std::is_convertible_v<T&, std::ostream&>)
struct logger<T> : public logger_base<logger<T>> {
	using base = logger_base<logger<T>>;
	using base::prefix_generator;
	using base::suffix_generator;

	T target;

	logger(T &&target_, auto &&... args) :
		base{args...},
		target{std::forward<T>(target_)} {}

	logger(const logger &) = delete;
	logger(logger &&) = default;

	void write(log_level level, std::string_view msg) {
		if (prefix_generator)
			target << prefix_generator(level, msg);
		target << msg;
		if (suffix_generator)
			target << suffix_generator(level, msg);
		target << '\n';
	}
};

template <typename T> requires(writeable<T>)
struct logger<std::unique_ptr<T>> : logger_base<logger<std::unique_ptr<T>>> {
	using base = logger_base<logger<std::unique_ptr<T>>>;
	using base::prefix_generator;
	using base::suffix_generator;

	std::unique_ptr<T> target;

	logger(std::unique_ptr<T> &&target_, auto &&... args) :
		base(args...),
		target(std::move(target_)) { }

	logger(const logger &) = delete;
	logger(logger &&) = default;

	void write(log_level level, std::string_view msg) {
		auto data = reinterpret_cast<const std::byte*>(msg.data());

		if (!target)
			return;
		if (prefix_generator) {
			std::string prefix = prefix_generator(level, msg);

			target->write(reinterpret_cast<const std::byte*>(prefix.c_str()), prefix.size());
		}
		target->write(data, msg.size());
		if (suffix_generator) {
			std::string suffix = suffix_generator(level, msg);

			target->write(reinterpret_cast<const std::byte*>(suffix.c_str()), suffix.size());
		}
		target->write("\n", 1);
		if constexpr (requires(T &t) { t.flush(); }) {
			target->flush();
		}
	}
};

template <>
struct logger<std::FILE*> : public logger_base<logger<std::FILE*>> {
	using base = logger_base<logger<std::FILE*>>;
	using base::prefix_generator;
	using base::suffix_generator;

	std::FILE *target;

	logger(std::FILE *target_, auto &&... args) :
		logger_base{args...},
		target(target_) {}

	logger(const logger &) = delete;
	logger(logger &&) = default;

	void write(log_level level, std::string_view msg) {
		if (prefix_generator)
			_write(prefix_generator(level, msg));
		_write(msg);
		if (suffix_generator)
			_write(suffix_generator(level, msg));
		_write('\n');
	}

private:
	std::size_t _write(std::string_view str) {
		return (std::fwrite(str.data(), sizeof(char), str.size(), target));
	}

	std::size_t _write(char c) {
		return (std::fwrite(&c, sizeof(char), 1, target));
	}
};

template <typename T>
struct is_managed_file_s {
	static constexpr inline bool value = false;
};

template <std::invocable<std::FILE*> auto Releaser>
struct is_managed_file_s<managed_resource<std::FILE*, Releaser>> {
	static constexpr inline bool value = true;
};

template <typename T>
constexpr inline bool is_managed_file = is_managed_file_s<T>::value;

template <typename T>
concept managed_file = is_managed_file<T>;

template <managed_file T>
struct logger<T> : public logger_base<logger<T>> {
	using base = logger_base<logger<T>>;
	using base::prefix_generator;
	using base::suffix_generator;
	using target_type = T;

	target_type target;

	logger(target_type &&target_, auto &&... args) :
		base{args...},
		target(std::move(target_)) { }

	logger(const logger &) = delete;
	logger(logger &&) = default;

	void write(log_level level, std::string_view msg) {
		if (prefix_generator)
			_write(prefix_generator(level, msg));
		_write(msg);
		if (suffix_generator)
			_write(suffix_generator(level, msg));
		_write('\n');
	}

private:
	std::size_t _write(std::string_view str) {
		return (std::fwrite(str.data(), sizeof(char), str.size(), target.get()));
	}

	std::size_t _write(char c) {
		return (std::fwrite(&c, sizeof(char), 1, target.get()));
	}
};

template <logger_type... Loggers>
class logger_system {
public:
	template <logger_type... LoggersArgs>
	explicit constexpr logger_system(LoggersArgs &... args) {
		(addLogger(args), ...);
	}

	template <logger_type T>
	constexpr bool addLogger(T &logger) {
		constexpr size_t tuplePos = _findLoggerVectorPos<T, 0>();

		if constexpr (tuplePos == std::numeric_limits<size_t>::max()) {
			static_assert(tuplePos != std::numeric_limits<size_t>::max(), "unsupported logger type");
			return (false);
		} else {
			_collectiveLogLevel |= static_cast<log_level>(logger.level);
			std::get<tuplePos>(_loggers).emplace_back(&logger);
			return (true);
		}
	}

	template <typename... Args> requires(sizeof...(Args) > 0)
	void log(log_level level, std::format_string<to_loggable<Args>...> fmt, Args&&... args) {
		if (!((_collectiveLogLevel & level) == level))
			return;
		auto line = std::format(fmt, loggable_helper<Args>{}(std::forward<Args>(args))...);

		_log(level, line, _idx_seq);
	}

	void log(log_level level, std::string_view line) {
		_log(level, line, _idx_seq);
	}

	bool is_log_enabled(log_level level) const {
		return {(_collectiveLogLevel & level) == level};
	}

private:
	using LoggerList = std::tuple<std::vector<Loggers*>...>;

	constexpr static inline auto _idx_seq =
		std::make_index_sequence<std::tuple_size_v<LoggerList>>();

	template <size_t... Ns>
	void _log(log_level level, std::string_view line, std::index_sequence<Ns...>) {
		constexpr auto             impl =
			[]<size_t N>(LoggerList &loggers, log_level level, std::string_view line) {
			for (auto logger : std::get<N>(loggers))
				logger->log(level, line);
		};

		(impl.template operator()<Ns>(_loggers, level, line), ...);
	}

	template <logger_type T, size_t N>
	static constexpr size_t _findLoggerVectorPos() {
		if constexpr (std::tuple_size_v<LoggerList> > N) {
			if constexpr (std::is_same_v<std::vector<T*>, std::tuple_element_t<N, LoggerList>>)
				return (N);
			else
				return (_findLoggerVectorPos<T, N + 1>());
		} else
			return (std::numeric_limits<size_t>::max());
	}

	log_level  _collectiveLogLevel{log_level::NONE};
	LoggerList _loggers;
};
} // namespace octahedron

//template <typename T>
//struct ::fmt::formatter<octahedron::Formatter<T>>
//{
//		template <typename ParseContext>
//		constexpr auto parse(ParseContext &ctx)
//		{
//			return (_formatter.parse(ctx));
//		}
//
//		template <typename FormatContext>
//		auto format(const octahedron::Formatter<T> &me, FormatContext &ctx)
//		{
//			return (_formatter.format(me, ctx));
//		}
//
//		octahedron::Formatter<T> _formatter{};
//};

#endif
