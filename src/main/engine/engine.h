#ifndef OCTAHEDRON_ENGINE_H_
#define OCTAHEDRON_ENGINE_H_

#include <array>
#include <chrono>
#include <filesystem>
#include <fstream>
#include <random>
#include <span>
#include <thread>

#include "octahedron.h"

#include "../io/file_stream.h"
#include "../io/file_system.h"
#include "../io/logger.h"
#include "../tools/protected_ptr.h"

namespace octahedron
{

class engine {
public:
	template <typename T> requires requires { T::now(); }
	class clock {
	public:
		using timestamp = typename T::time_point;
		using duration = microseconds;
		using type = T;

		struct tick {
			using clock = engine::clock<T>;

			timestamp time;
			duration  real_diff;
			duration  clamped_diff;
		};

		const tick& update(duration min_duration, duration max_duration);
		const tick& update(duration min_duration);
		const tick& update() noexcept;

		const tick&      get_last_tick() const noexcept;
		const timestamp& get_start_time() const noexcept;

	private:
		const timestamp _start_time{T::now()};
		tick            _last_tick{_start_time, duration::zero(), duration::zero()};
	};

	using game_clock = clock<std::chrono::steady_clock>;
	using wall_clock = clock<std::chrono::system_clock>;

	struct parameter {
		std::string_view name;
		std::string_view value;
	};

	engine(int ac, const char *const argv[]);
	engine(const engine &) = delete;
	engine(engine &&) = delete;

	engine& operator=(const engine &) = delete;
	engine& operator=(engine &&) = delete;

	const game_clock& get_game_clock() const noexcept;
	const wall_clock& get_wall_clock() const noexcept;

	bool is_log_enabled(bit_set<log_level> logLevel) const noexcept;

	void seedRNG();

	enum class state {
		none     = 0,
		starting = 1,
		running  = 1
	};

	void set_log_file(std::string_view file);

	void log(bit_set<log_level> level, std::string_view line);

	template <typename... Args> requires(sizeof...(Args) > 0)
	void log(
		bit_set<log_level>                       level,
		fmt::format_string<to_loggable<Args>...> fmt,
		Args &&...                               args
	) {
		if (!_logger.is_log_enabled(log_level{level}))
			return;
		auto line = fmt::format(fmt, loggable_helper<Args>::get(std::forward<Args>(args))...);

		log(level, line);
	}

	const file_system& get_file_system() const noexcept;
	file_system&       get_file_system() noexcept;

private:
	void   _parse_args(std::span<const char*const> args);
	size_t _get_seed_base() const noexcept;

	logger<std::unique_ptr<file_stream>>                     _log_file{nullptr};
	logger<std::fstream>                                     _debug_log_file{std::fstream{}};
	logger_system<decltype(_log_file), logger<std::fstream>> _logger{_log_file, _debug_log_file};

	game_clock _game_clock{};
	wall_clock _day_clock{};

	std::mt19937    _mt32{static_cast<unsigned int>(_get_seed_base())};
	std::mt19937_64 _mt64{_get_seed_base()};

	struct well_rng_512 {
		std::array<uint16, 16> state{};
		uint64                 index{0};

		uint64 operator()();
	};

	well_rng_512 _rng{
		.state{
			generate<16>(
				[this]() {
					return (static_cast<uint16>(this->_mt64()));
				}
			)
		}
	};

	state _state{state::none};

	file_system _filesystem;
};

template <typename T> requires requires { T::now(); }
auto engine::clock<T>::update(duration min_duration, duration max_duration) -> const tick& {
	timestamp now = T::now();
	duration  real_diff = now - _last_tick.time;

	if (real_diff < min_duration) {
		std::this_thread::sleep_for(min_duration - real_diff);
		now = T::now();
		real_diff = now - _last_tick.time;
	}
	_last_tick = {
		.time = now,
		.real_diff = real_diff,
		.clamped_diff = (max_duration > real_diff ? max_duration : real_diff)
	};
	return (_last_tick);
}

template <typename T> requires requires { T::now(); }
auto engine::clock<T>::update(duration min_duration) -> const tick& {
	timestamp now = T::now();
	duration  real_diff = now - _last_tick.time;

	if (real_diff < min_duration) {
		std::this_thread::sleep_for(min_duration - real_diff);
		now = T::now();
		real_diff = now - _last_tick.time;
	}
	_last_tick = {.time = now, .real_diff = real_diff, .clamped_diff = real_diff};
	return (_last_tick);
}

template <typename T> requires requires { T::now(); }
auto engine::clock<T>::update() noexcept -> const tick& {
	timestamp now = T::now();
	duration  real_diff = now - _last_tick.time;

	_last_tick = {.time = now, .real_diff = real_diff, .clamped_diff = real_diff};
	return (_last_tick);
}

template <typename T> requires requires { T::now(); }
auto engine::clock<T>::get_last_tick() const noexcept -> const tick& {
	return (_last_tick);
}

template <typename T> requires requires { T::now(); }
auto engine::clock<T>::get_start_time() const noexcept -> const timestamp& {
	return (_start_time);
}

} // namespace octahedron

#endif
