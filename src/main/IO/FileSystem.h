#ifndef OCTAHEDRON_FILESYSTEM_H_
#define OCTAHEDRON_FILESYSTEM_H_

#include "Octahedron.h"

#include <array>
#include <filesystem>
#include <optional>
#include <ranges>

#include <fmt/core.h>

#ifdef _WIN32
#  define NOMINMAX

#  include <ShlObj.h>

#  ifdef ERROR
#    undef ERROR
#  endif
#endif

#include "Tools/ManagedResource.h"

struct SDL_RWops;

namespace Octahedron
{
  namespace stdfs = ::std::filesystem;

  constexpr const char filepath_regex[] =
    "^((?:([a-zA-Z]):[\\\\\\/])|((?:[\\\\\\/])))?((?:[a-zA-Z0-9\\.]+[\\\\\\/])*)([\\.a-zA-Z0-9]*)$";

  namespace _
  {
    struct OpenFlags
    {
        enum Values
        {
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
  }  // namespace _

  using OpenFlags = _::OpenFlags::Values;

  /**
   * \brief Wrapper for OS-specific APIs, returns an stdfs::path containing the user's home directory.
   * (e.g. `/home/user/` on unix, `C:\\Users\\user\\Documents\\` on Windows.
   */
  auto getUserHomeDir() -> std::optional<stdfs::path>;

  /**
   * \brief Replaces all instances of `~` or `$HOME` with the value returned by getUserHomeDir().
   * \relates getUserHomeDir()
   */
  auto replaceHomeToken(std::string_view path) -> std::string;

  /**
   * \brief Cleans up a stdfs::path. Concatenates circular paths, replaces all dividers by the one used for the OS.
   */
  auto cleanupPath(stdfs::path str) -> stdfs::path;

  auto fullPath(stdfs::path str) -> std::optional<stdfs::path>;

  class FileStream;

  class FileSystem
  {
    public:
      void setHomeDir(std::string_view dir);
      void addPackageDir(std::string_view dir);

      auto homeDir() const noexcept -> const stdfs::path &;
      auto packageDirs() const noexcept -> std::span<const stdfs::path>;

      bool isAccessible(std::string_view path, BitSet<OpenFlags> mode = OpenFlags::DEFAULT) const;

      bool remove(std::string_view path);
      bool rename(std::string_view old_path, std::string_view new_path);
      bool createPath(std::string_view path);

      auto open(std::string_view path, BitSet<OpenFlags> mode = OpenFlags::DEFAULT)
        -> std::unique_ptr<FileStream>;

      // LEGACY : to be replaced down the line
      auto openSDLRWops(std::string_view path, BitSet<OpenFlags> mode = OpenFlags::DEFAULT)
        -> SDL_RWops *;

      // public for now during API update - will be private soon
      auto
      _resolvePath(std::string_view file_name, BitSet<OpenFlags> mode = OpenFlags::DEFAULT) const
        -> std::optional<stdfs::path>;

    private:
      stdfs::path _home_dir{stdfs::current_path()};
      std::vector<stdfs::path> _package_dirs{};

      bool
      _isAccessible(const stdfs::path &path, BitSet<OpenFlags> mode = OpenFlags::DEFAULT) const;

      bool _createPath(const stdfs::path &path);
  };
}  // namespace Octahedron

#ifdef RELATIVE
#  undef RELATIVE
#endif

template <>
struct ::fmt::formatter<std::filesystem::path>
{
    template <typename ParseContext>
    constexpr auto parse(ParseContext &ctx);

    template <typename FormatContext>
    auto format(std::filesystem::path const &path, FormatContext &ctx);

    enum class Mode
    {
      AS_IS     = 0,
      CANONICAL = 1,
      RELATIVE  = 2
    };

    Mode mode{Mode::AS_IS};
    fmt::formatter<std::string> str_formatter{};
};

template <typename ParseContext>
constexpr auto ::fmt::formatter<std::filesystem::path>::parse(ParseContext &ctx)
{
  auto end = std::ranges::find(ctx, '}');
  auto it  = ctx.begin();

  if (end != it)
  {
    switch (*it)
    {
      case 'c':
        mode = Mode::CANONICAL;
        break;

      case 'r':
        mode = Mode::RELATIVE;
        break;

      default:
        throw fmt::format_error{"invalid argument"};
    }
    ++it;
  }
  ctx.advance_to(it);
  return (str_formatter.parse(ctx));
}

template <typename FormatContext>
auto ::fmt::formatter<std::filesystem::path>::format(
  std::filesystem::path const &path,
  FormatContext &ctx
)
{
  namespace stdfs = std::filesystem;
  std::string str;

  switch (mode)
  {
    using enum Mode;

    case AS_IS:
      str = path.string();
      break;

    case CANONICAL:
      str = stdfs::weakly_canonical(path).string();
      break;

    case RELATIVE:
      str = stdfs::relative(path).string();
      break;

    default:
      throw fmt::format_error{"invalid argument"};
      break;
  }
  return (str_formatter.format(str, ctx));
}

#endif
