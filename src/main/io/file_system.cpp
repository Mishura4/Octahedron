#include "octahedron.h"

#include "file_system.h"

#include <fstream>

#include <fmt/std.h>

#include <SDL_RWops.h>

#include "gz_file_stream.h"
#include "raw_file_stream.h"

namespace octahedron {

auto get_user_home_dir() -> std::optional<stdfs::path> {
	#ifdef WIN32
	wchar_t *home_ptr;
	HRESULT ret = SHGetKnownFolderPath(FOLDERID_Documents, NULL, 0, &home_ptr);
	managed_resource<wchar_t*, [](wchar_t *ptr) {
		CoTaskMemFree(ptr);
	}> home{home_ptr};
	if (ret != S_OK)
		return {std::nullopt};
	return {home.get()};
	#else
  const char *home = getenv("HOME");
  if (!home || !home[0])
    return {std::nullopt};
  return {home};
	#endif
}

auto replace_home_token(std::string_view path) -> std::string {
	auto home_dir = get_user_home_dir();

	if (!home_dir)
		return (std::string{path});

	constexpr auto find_token = [](std::string_view path) {
		auto position = std::ranges::search(path, "$HOME"sv);
		if (!position)
			position = std::ranges::search(path, "~"sv);
		return (position);
	};

	constexpr std::array tokens = {"$HOME"sv, "~"sv};
	std::stringstream    ret;

	while (auto position = find_token(path)) {
		ret << std::string_view{path.begin(), position.begin()};
		ret << home_dir.value().string();
		path = {position.end(), path.end()};
	}
	ret << path;
	return {ret.str()};
}

auto cleanup_path(stdfs::path str) -> stdfs::path {
	return (str.lexically_normal());
}

auto full_path(stdfs::path str) -> std::optional<stdfs::path> {
	std::error_code err{};
	stdfs::path ret{stdfs::weakly_canonical(stdfs::current_path(err) / str.make_preferred(), err)};

	if (err)
		return {std::nullopt};
	return {ret};
}

void file_system::set_home_dir(std::string_view dir) {
	auto path = full_path(replace_home_token(dir));

	if (!path) {
		log(log_level::ERROR, "invalid home directory path: {}", CLOSURE(path->string()));
		return;
	}
	_home_dir = *path;
	log(log_level::BASIC, "home dir set to {}", CLOSURE(path->string()));
}

void file_system::add_package_dir(std::string_view file) {
	auto path = full_path(replace_home_token(file));

	if (!path) {
		log(log_level::ERROR, "invalid package dir path: {}", CLOSURE(path->string()));
		return;
	}
	_package_dirs.push_back(*path);
}

auto file_system::get_home_dir() const noexcept -> const stdfs::path& {
	return (_home_dir);
}

auto file_system::get_package_dirs() const noexcept -> std::span<const stdfs::path> {
	return {_package_dirs};
}

bool file_system::is_accessible(std::string_view path, bit_set<open_flags> mode) const {
	auto path_ = _resolve_path(path, mode);

	if (!path_)
		return (false);
	return (_is_accessible(*path_, mode));
}

#include "file_stream.h"

auto file_system::open(
	std::string_view    path,
	bit_set<open_flags> mode
) -> std::unique_ptr<file_stream> {
	auto _path = _resolve_path(path, mode);

	if (!_path)
		return {nullptr};
	if (mode & (open_flags::APPEND | open_flags::TRUNCATE))
		_create_folders(_path->parent_path());
	return (raw_file_stream::_open(*_path, mode));
}

auto file_system::open_gz(
	std::string_view    path,
	bit_set<open_flags> mode
) -> std::unique_ptr<file_stream> {
	auto raw_file = open(path, mode);

	if (!raw_file)
		return {nullptr};
	return (gz_file_stream::_from_raw_stream(std::move(raw_file), mode));
}

SDL_RWops *file_system::openSDLRWops(
	std::string_view    path,
	bit_set<open_flags> mode
) {
	auto path_ = _resolve_path(path, mode);

	if (!path_)
		return (nullptr);

	std::string _mode;

	if (mode & std::ios::out) {
		if (mode & std::ios::trunc) {
			if (mode & std::ios::in)
				_mode = "w+";
			else
				_mode = "w";
		} else if (mode & std::ios::app) {
			if (mode & std::ios::in)
				_mode = "a+";
			else
				_mode = "a";
		} else
			_mode = "r+";
	} else if (mode & std::ios::in)
		_mode = "r";
	if (mode & std::ios::binary)
		_mode += 'b';
	return (SDL_RWFromFile(path_->string().c_str(), _mode.c_str()));
}

bool file_system::_is_accessible(const stdfs::path &path, bit_set<open_flags> mode) const {
	if (mode & (open_flags::APPEND | open_flags::TRUNCATE))
		// will create, so we only need the parent dir
		return (stdfs::exists(path.parent_path()));
	return (stdfs::exists(path));
}

std::optional<stdfs::path> file_system::_resolve_path(
	std::string_view    file_name,
	bit_set<open_flags> search_mode
) const {
	// rewrite of tesseract's findfile, with a few changes
	const stdfs::path given{stdfs::path(file_name).lexically_normal()};

	// we outright disable absolute paths or going up
	constexpr auto is_allowed = [](const stdfs::path &p) -> bool {
		constexpr std::string_view forbidden[] = {"../"sv};
		constexpr auto             cmp = [](stdfs::path::value_type a, char b) -> bool {
			return (a == b);
		};

		if (p.is_absolute())
			return (false);
		for (auto test : forbidden) {
			if (std::ranges::mismatch(p.native(), test, cmp).in2 == test.end())
				return (false);
		}
		return (true);
	};

	if (!is_allowed(given))
		return {std::nullopt};
	// home directory has highest priority
	if (!_home_dir.empty()) {
		// if we are creating the file anyway, return the path relative to the home dir
		if (auto path = stdfs::absolute(_home_dir / given);
			search_mode & (open_flags::APPEND | open_flags::TRUNCATE) ||
			_is_accessible(path, search_mode))
			return {path};
	}
	// only the home dir is modifyable
	if (search_mode & open_flags::OUTPUT)
		return {std::nullopt};
	// package directories have second priority
	for (const stdfs::path &dir : get_package_dirs()) {
		if (auto path = stdfs::absolute(dir / given); _is_accessible(path, search_mode))
			return {path};
	}
	// path relative to the working directory has lowest priority
	if (_is_accessible(given, search_mode))
		return {given};
	return {std::nullopt};
}

bool file_system::remove(std::string_view path) {
	auto full_path = _resolve_path(path, open_flags::OUTPUT);

	if (!full_path)
		return (false);
	std::error_code err;

	if (!stdfs::remove(*full_path, err)) {
		log(log_level::TRACE, "could not remove path {}: {}", *full_path, CLOSURE(err.message()));
		return (false);
	}
	log(log_level::TRACE, "removed path {}", *full_path);
	return (true);
}

bool file_system::rename(std::string_view old_path, std::string_view new_path) {
	auto resolved_old = _resolve_path(old_path, open_flags::OUTPUT);
	auto resolved_new = _resolve_path(new_path, open_flags::OUTPUT);

	if (!resolved_old || !resolved_new)
		return (false);

	std::error_code err;

	stdfs::rename(*resolved_old, *resolved_new, err);
	return (static_cast<bool>(err));
}

bool file_system::create_folders(std::string_view path) {
	auto resolved = _resolve_path(path, open_flags::OUTPUT);

	if (!resolved)
		return (false);
	return (_create_folders(path));
}

bool file_system::_create_folders(const stdfs::path &path) {
	if (_is_accessible(path))
		return (true);
	return (stdfs::create_directories(path));
}

}
