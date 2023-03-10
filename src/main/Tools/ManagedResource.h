#ifndef OCTAHEDRON_MANAGED_RESOURCE_H_
#define OCTAHEDRON_MANAGED_RESOURCE_H_

#include <type_traits>
#include <concepts>

namespace Octahedron
{
  template <typename T>
  concept trivial_resource = std::is_default_constructible_v<T> && std::is_assignable_v<T &, T>;

  /*;

template <typename T, std::invocable<T> auto Releaser>
requires(
  std::is_default_constructible_v<T> &&
  std::is_assignable_v<T&, T>
)
class ManagedResource<T, Releaser>*/
  template <typename T, auto Releaser>
  class ManagedResource;

  template <trivial_resource T, auto Releaser>
  requires(std::invocable<decltype(Releaser), T &>)
  class ManagedResource<T, Releaser>
  {
    public:
      constexpr static inline bool nothrow_release = noexcept(Releaser(std::declval<T &>()));

      ManagedResource() = default;

      ManagedResource(T resource) noexcept(std::is_nothrow_assignable_v<T &, T>) :
          _resource(resource)
      {
      }

      ManagedResource(const ManagedResource &other) = delete;

      ManagedResource(ManagedResource &&rvalue) noexcept(std::is_nothrow_assignable_v<T &, T &&>) :
          _resource(std::move(rvalue._resource))
      {
        rvalue._resource = T{};
      }

      ~ManagedResource() noexcept(nothrow_release) { release(); }

      ManagedResource &operator=(const ManagedResource &other) = delete;

      ManagedResource &operator=(ManagedResource &&rvalue
      ) noexcept(nothrow_release &&std::is_nothrow_assignable_v<T &, T &&>)
      {
        release();
        _resource        = std::move(rvalue._resource);
        rvalue._resource = T{};
        return (*this);
      }

      const T &operator->() const noexcept { return (_resource); }

      ManagedResource &operator=(const T &ptr
      ) noexcept(nothrow_release &&std::is_nothrow_assignable_v<T, T>)
      {
        release();
        _resource = ptr;
        return (*this);
      }

      operator T &() noexcept { return (get()); }

      operator const T &() const noexcept { return (get()); }

      T &get() noexcept { return (_resource); }

      const T &get() const noexcept { return (_resource); }

      operator bool() const noexcept
      {
        if constexpr (std::convertible_to<T, bool>)
        {
          return (static_cast<bool>(_resource));
        }
        else if constexpr (std::equality_comparable_with<T, bool>)
        {
          return (_resource == true);
        }
        else if constexpr (std::equality_comparable<T>)
        {
          return (_resource == T{});
        }
        else
        {
          static_assert(!std::same_as<T, T>, "type does not have a trivial \"is empty\" semantic");
        }
      }

      void release() noexcept(nothrow_release)
      {
        if (*this)
          Releaser(_resource);
      }

    private:
      T _resource{};
  };

  template <std::invocable auto Releaser>
  class ManagedResource<nullptr_t, Releaser>
  {
    public:
      ManagedResource() = default;

      template <typename T>
      requires(std::convertible_to<std::remove_cvref_t<T>, bool>)
      ManagedResource(T &&value) noexcept : _owned{static_cast<bool>(value)}
      {
      }

      ManagedResource(const ManagedResource &other) = delete;
      ManagedResource(ManagedResource &&other)      = delete;

      ~ManagedResource()
      {
        if (_owned)
          Releaser();
      }

      bool hasResource() const { return (_owned); }

      void hasResource(bool value) { _owned = value; }

      operator bool() const { return (hasResource()); }

      ManagedResource &operator=(ManagedResource &&other)      = delete;
      ManagedResource &operator=(const ManagedResource &other) = delete;

    private:
      bool _owned{false};
  };
}

#endif
