#ifndef OCTAHEDRON_RAWFILESTREAM_H_
#define OCTAHEDRON_RAWFILESTREAM_H_

#include "../Base.h"
#include "FileStream.h"

namespace Octahedron
{
  template <auto Action = std::nullptr_t{}>
  class OnDestroy;

  template <auto Action>
  requires(std::invocable<std::remove_cvref_t<decltype(Action)>>)
  class OnDestroy<Action>
  {
      ~OnDestroy() noexcept(noexcept(std::invoke(Action))) { std::invoke(Action); }
  };

  template <>
  class OnDestroy<std::nullptr_t{}>
  {
    private:
      std::function<void()> _action{nullptr};

    public:
      template <typename T>
      requires(std::invocable<std::remove_cvref_t<T>> && std::is_nothrow_invocable_v<std::remove_cvref_t<T>>)
      OnDestroy(T &&action) : _action(action)
      {
      }

      OnDestroy() = default;

      OnDestroy(const OnDestroy &) = delete;

      OnDestroy(OnDestroy &&) = default;

      OnDestroy(nullptr_t) : _action{nullptr} {}

      template <typename T>
      requires(std::invocable<std::remove_cvref_t<T>> && std::is_nothrow_invocable_v<std::remove_cvref_t<T>>)
      OnDestroy &operator=(T &&action)
      {
        _action = std::forward<T>(action);
        return (*this);
      }

      OnDestroy &operator=(std::nullptr_t)
      {
        _action = nullptr;
        return (*this);
      }

      OnDestroy &operator=(const OnDestroy &) = delete;

      OnDestroy &operator=(OnDestroy &&) = default;

      ~OnDestroy() noexcept
      {
        if (_action)
          _action();
      }
  };

  template <typename T>
  requires(std::invocable<std::remove_cvref_t<T>>)
  OnDestroy(T t) -> OnDestroy<std::nullptr_t{}>;

  OnDestroy(std::nullptr_t t) -> OnDestroy<std::nullptr_t{}>;

  class RawFileStream : public FileStream
  {
    public:
      using IOStream::read;
      using IOStream::write;

      ~RawFileStream() override						 = default;
			RawFileStream(const RawFileStream &) = delete;

      bool                 flush() override;
      bool                 eof() override;
			[[nodiscard]] size_t tell() const override;
      bool                 seek(ssize_t pos, int whence) override;

      size_t read(std::span<std::byte> buf) override;
      size_t write(std::span<const std::byte> buf) override;

      auto getLine(size_t max_size) -> std::optional<std::string> override;

    protected:
      RawFileStream()                      = default;
      RawFileStream(RawFileStream &&)      = default;

    private:
      friend auto FileSystem::open(std::string_view path, BitSet<OpenFlags> mode)
        -> std::unique_ptr<FileStream>;

      static auto _open(const stdfs::path &path, BitSet<OpenFlags> mode)
        -> std::unique_ptr<RawFileStream>;

      static constexpr inline auto CLOSE_FN = [](std::FILE *p) noexcept { ::fclose(p); };
      using FileResource                  = ManagedResource<std::FILE *, CLOSE_FN>;

      OnDestroy<> _onDestroy{nullptr};
      FileResource _file{nullptr};
  };
}

#endif /* OCTAHEDRON_RAWFILESTREAM_H_ */
