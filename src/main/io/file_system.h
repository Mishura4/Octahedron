#ifndef OCTAHEDRON_FILESYSTEM_H_
#define OCTAHEDRON_FILESYSTEM_H_

#include "octahedron.h"

#include <array>
#include <filesystem>
#include <optional>
#include <ranges>
#include <string>

#include <fmt/core.h>

#ifdef _WIN32
#  define NOMINMAX

#  include <ShlObj.h>

#  ifdef ERROR
#    undef ERROR
#  endif
#endif

#include "tools/managed_resource.h"

struct SDL_RWops;

namespace octahedron
{
namespace stdfs = ::std::filesystem;

constexpr const char filepath_regex[] =
	"^((?:([a-zA-Z]):[\\\\\\/])|((?:[\\\\\\/])))?((?:[a-zA-Z0-9\\.]+[\\\\\\/])*)([\\.a-zA-Z0-9]*)$";

namespace _
{
struct OpenFlags {
	enum Values {
		NONE      = bitflag(0),
		INPUT     = bitflag(1),
		OUTPUT    = bitflag(2),
		APPEND    = bitflag(3),
		TRUNCATE  = bitflag(4),
		BINARY    = bitflag(16),
		TEMPORARY = bitflag(17),

		DEFAULT     = INPUT,
		MASK_CREATE = APPEND | TRUNCATE
	};
};
} // namespace _

using open_flags = _::OpenFlags::Values;

/**
 * \brief Wrapper for OS-specific APIs, returns an stdfs::path containing the user's home directory.
 * (e.g. `/home/user/` on unix, `C:\\Users\\user\\Documents\\` on Windows.
 */
std::optional<stdfs::path> get_user_home_dir();

/**
 * \brief Replaces all instances of `~` or `$HOME` with the value returned by get_user_home_dir().
 * \relates get_user_home_dir()
 */
std::string replace_home_token(std::string_view path);

/**
 * \brief Cleans up a stdfs::path. Concatenates circular paths, replaces all dividers by the one used for the OS.
 */
stdfs::path cleanup_path(stdfs::path str);

std::optional<stdfs::path> full_path(stdfs::path str);

class file_stream;

class file_system {
public:
	void set_home_dir(std::string_view dir);
	void add_package_dir(std::string_view dir);

	const stdfs::path&           get_home_dir() const noexcept;
	std::span<const stdfs::path> get_package_dirs() const noexcept;

	bool is_accessible(std::string_view path, bit_set<open_flags> mode = open_flags::DEFAULT) const;

	bool remove(std::string_view path);
	bool rename(std::string_view old_path, std::string_view new_path);
	bool create_folders(std::string_view path);

	std::unique_ptr<file_stream> open(
		std::string_view    path,
		bit_set<open_flags> mode = open_flags::DEFAULT
	);

	std::unique_ptr<file_stream> open_gz(
		std::string_view    path,
		bit_set<open_flags> mode = open_flags::DEFAULT
	);

	// LEGACY : to be replaced down the line
	SDL_RWops *openSDLRWops(
		std::string_view    path,
		bit_set<open_flags> mode = open_flags::DEFAULT
	);

	// public for now during API update - will be private soon
	std::optional<stdfs::path> _resolve_path(
		std::string_view    file_name,
		bit_set<open_flags> mode = open_flags::DEFAULT
	) const;

private:
	stdfs::path              _home_dir{stdfs::current_path()};
	std::vector<stdfs::path> _package_dirs{};

	bool _is_accessible(
		const stdfs::path & path,
		bit_set<open_flags> mode = open_flags::DEFAULT
	) const;

	bool _create_folders(const stdfs::path &path);
};
} // namespace octahedron

#endif
