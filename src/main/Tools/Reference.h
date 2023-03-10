#ifndef OCTAHEDRON_REFERENCE_H_
#define OCTAHEDRON_REFERENCE_H_

#include <type_traits>

namespace Octahedron
{
  template <typename T>
  struct Reference
  {
      constexpr Reference(T &var) noexcept : ptr(&var) {}

      constexpr auto &get() const noexcept
      {
        assert(ptr);

        return (*ptr);
      }

      operator T &() const noexcept { return (get()); }

      Reference &operator=(const T &other) noexcept(std::is_nothrow_copy_assignable_v<T>)
      requires(!std::is_const_v<T> && std::is_copy_assignable_v<T>)
      {
        assert(ptr);

        *ptr = other;
        return (*this);
      }

      Reference &operator=(const Reference<T> &other) noexcept(std::is_nothrow_copy_assignable_v<T>)
      requires(!std::is_const_v<T> && std::is_copy_assignable_v<T>)
      {
        assert(ptr);

        *ptr = other;
        return (*this);
      }

      T *ptr;
  };
}  // namespace Octahedron

#endif
