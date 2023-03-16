#include "Octahedron.h"

#include "FileSystem.h"

#include <fstream>

using Octahedron::FileSystem;

auto Octahedron::getUserHomeDir() -> std::optional<stdfs::path>
{
#ifdef WIN32
  wchar_t *home_ptr;
  HRESULT ret = SHGetKnownFolderPath(FOLDERID_Documents, NULL, 0, &home_ptr);
  ManagedResource<wchar_t *, [](wchar_t *ptr) { CoTaskMemFree(ptr); }> home{home_ptr};
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

auto Octahedron::replaceHomeToken(std::string_view path) -> std::string
{
  auto home_dir = getUserHomeDir();

  if (!home_dir)
    return (std::string{path});

  constexpr auto find_token = [](std::string_view path)
  {
    auto position = std::ranges::search(path, "$HOME"sv);
    if (!position)
      position = std::ranges::search(path, "~"sv);
    return (position);
  };

  constexpr std::array tokens = {"$HOME"sv, "~"sv};
  std::stringstream ret;

  while (auto position = find_token(path))
  {
    ret << std::string_view{path.begin(), position.begin()};
    ret << home_dir.value().string();
    path = {position.end(), path.end()};
  }
  ret << path;
  return {ret.str()};
}

auto Octahedron::cleanupPath(stdfs::path str) -> stdfs::path
{
  return (str.lexically_normal());
}

auto Octahedron::fullPath(stdfs::path str) -> std::optional<stdfs::path>
{
  std::error_code err{};
  stdfs::path ret{stdfs::weakly_canonical(stdfs::current_path(err) / str.make_preferred(), err)};

  if (err)
    return {std::nullopt};
  return {ret};
}

void FileSystem::setHomeDir(std::string_view dir)
{
  auto path = fullPath(replaceHomeToken(dir));

  if (!path)
  {
    log(LogLevel::ERROR, "invalid home directory path: {}", CLOSURE(path->string()));
    return;
  }
  _home_dir = *path;
  log(LogLevel::BASIC, "home dir set to {}", CLOSURE(path->string()));
}

void FileSystem::addPackageDir(std::string_view file)
{
  auto path = fullPath(replaceHomeToken(file));

  if (!path)
  {
    log(LogLevel::ERROR, "invalid package dir path: {}", CLOSURE(path->string()));
    return;
  }
  _package_dirs.push_back(*path);
}

auto FileSystem::homeDir() const noexcept -> const stdfs::path &
{
  return (_home_dir);
}

auto FileSystem::packageDirs() const noexcept -> std::span<const stdfs::path>
{
  return {_package_dirs};
}

auto FileSystem::isAccessible(std::string_view path, BitSet<OpenFlags> mode) const -> bool
{
  auto path_ = _resolvePath(path, mode);

  if (!path_)
    return (false);
  return (_isAccessible(*path_, mode));
}

#include "FileStream.h"

auto FileSystem::open(
  std::string_view path,
  BitSet<OpenFlags> mode) -> std::unique_ptr<FileStream>
{
  auto _path = _resolvePath(path, mode);

  if (!_path)
    return {nullptr};
  if (mode & (OpenFlags::APPEND | OpenFlags::TRUNCATE))
    _createPath(_path->parent_path());
  return (RawFileStream::_open(*_path, mode));
}

#include <SDL.h>


auto FileSystem::openSDLRWops(std::string_view path,
  BitSet<OpenFlags> mode) ->
  SDL_RWops *
{
  auto path_ = _resolvePath(path, mode);

  if (!path_)
    return (nullptr);

  std::string _mode;

  if (mode & std::ios::out)
  {
    if (mode & std::ios::trunc)
    {
      if (mode & std::ios::in)
        _mode = "w+";
      else
        _mode = "w";
    }
    else if (mode & std::ios::app)
    {
      if (mode & std::ios::in)
        _mode = "a+";
      else
        _mode = "a";
    }
    else
      _mode = "r+";
  }
  else if (mode & std::ios::in)
    _mode = "r";
  if (mode & std::ios::binary)
    _mode += 'b';
  return (SDL_RWFromFile(path_->string().c_str(), _mode.c_str()));
}

auto FileSystem::_isAccessible(const stdfs::path &path, BitSet<OpenFlags> mode) const -> bool
{
  if (mode & std::ios_base::out)
    return (stdfs::exists(path.parent_path()));
  return (stdfs::exists(path));
}

auto FileSystem::_resolvePath(std::string_view file_name, BitSet<OpenFlags> search_mode) const
  -> std::optional<stdfs::path>
{
  // rewrite of tesseract's findfile, preserving logic

  if (file_name.starts_with("../"sv))
    return {std::nullopt};

  std::error_code err;
  const stdfs::path given{stdfs::path(file_name).lexically_normal()};

  constexpr auto is_parent = [](const stdfs::path &given, const stdfs::path &base)
  {
    auto [input, end] = std::ranges::mismatch(given, base);

    return (end == std::end(base));
  };
  if (given.is_absolute())
    return {std::nullopt};
  if (!_home_dir.empty())
  {
    stdfs::path path = stdfs::absolute(_home_dir / given);

    if ((search_mode & OpenFlags::OUTPUT) || _isAccessible(path, search_mode))
      return {path};
  }
  if (search_mode & OpenFlags::OUTPUT)
    return {std::nullopt};
  for (const stdfs::path &dir : packageDirs())
  {
    stdfs::path path = stdfs::absolute(dir / given);

    if (_isAccessible(path, search_mode))
      return {path};
  }
  if (_isAccessible(given, search_mode))
    return {given};
  return {std::nullopt};
}

bool FileSystem::remove(std::string_view path)
{
  auto full_path = _resolvePath(path, OpenFlags::OUTPUT);

  if (!full_path)
    return (false);
  std::error_code err;

  if (stdfs::remove(*full_path, err))
  {
    log(LogLevel::TRACE, "removed path {}", *full_path);
    return (true);
  }
  else
    log(LogLevel::TRACE, "could not remove path {}: {}", *full_path, CLOSURE(err.message()));
  return (false);
}

bool FileSystem::rename(std::string_view old_path, std::string_view new_path)
{
  auto resolved_old = _resolvePath(old_path, OpenFlags::OUTPUT);
  auto resolved_new = _resolvePath(new_path, OpenFlags::OUTPUT);

  if (!resolved_old || !resolved_new)
    return (false);

  std::error_code err;

  stdfs::rename(*resolved_old, *resolved_new, err);
  return (static_cast<bool>(err));
}

bool FileSystem::createPath(std::string_view path)
{
  auto resolved = _resolvePath(path, OpenFlags::OUTPUT);

  if (!resolved)
    return (false);
  return (_createPath(path));
}

bool FileSystem::_createPath(const stdfs::path &path)
{
  if (_isAccessible(path))
    return (true);
  return (stdfs::create_directories(path));
}
