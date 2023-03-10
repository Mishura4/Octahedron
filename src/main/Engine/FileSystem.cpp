#include "Octahedron.h"

#include "FileSystem.h"

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

auto FileSystem::isAccessible(const stdfs::path &path, BitSet<SearchMode> mode) const -> bool
{
  if (mode & SearchMode::CREATE)
    return (stdfs::exists(path.parent_path()));
  return (stdfs::exists(path));
}

auto FileSystem::findFile(std::string_view file_name, BitSet<SearchMode> search_mode) const
  -> std::optional<stdfs::path>
{
  // rewrite of tesseract's findfile, preserving logic
  std::error_code err;
  const stdfs::path given{stdfs::path(file_name).lexically_normal()};

  auto is_parent = [](stdfs::path given)
  {
    constexpr stdfs::path::value_type parent[] = {'.', '.', '/'};

    auto [input, end] = std::ranges::mismatch(given.native(), parent);

    return (end == std::end(parent));
  };

  if (given.is_absolute() || is_parent(given))
  {
    log(LogLevel::ERROR, "attempt to access inaccessible directory {}", given.string());
    return {std::nullopt};
  }

  if (!(search_mode & SearchMode::PACKAGE_ONLY))
  {
    if (!_home_dir.empty())
    {
      stdfs::path path = stdfs::weakly_canonical(_home_dir / given);

      if (isAccessible(path, search_mode))
        return {path};
      if (search_mode & SearchMode::CREATE)
      {
        // TODO: move this logic somewhere else? we are not "finding a file", this is related to opening
        stdfs::create_directories(path.parent_path());
        return {path};
      }
    }
  }
  if (search_mode & SearchMode::CREATE)
    return {std::nullopt};
  for (const stdfs::path &dir : packageDirs())
  {
    stdfs::path path = stdfs::weakly_canonical(dir / given);

    if (isAccessible(path, search_mode))
      return {path};
  }
  if (isAccessible(given, search_mode))
    return {given};
  return {std::nullopt};
}

bool FileSystem::remove(std::string_view path)
{
  auto full_path = findFile(path, SearchMode::CREATE);

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
  auto resolved_old = findFile(old_path, SearchMode::CREATE);
  auto resolved_new = findFile(new_path, SearchMode::CREATE);

  if (!resolved_old || !resolved_new)
    return (false);

  std::error_code err;

  stdfs::rename(*resolved_old, *resolved_new, err);
  return (static_cast<bool>(err));
}

bool FileSystem::createPath(std::string_view path)
{
  auto resolved = findFile(path, SearchMode::CREATE);

  if (!resolved)
    return (false);
  if (isAccessible(path, SearchMode::CREATE))
    return (true);
  return (stdfs::create_directories(*resolved));
}
