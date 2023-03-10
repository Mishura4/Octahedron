#ifndef OCTAHEDRON_MATH_H_
#define OCTAHEDRON_MATH_H_

#include <type_traits>
#include <utility>

namespace Octahedron
{
  template <class T>
  constexpr auto abs(T &&a)
  {
    if constexpr (std::unsigned_integral<T>)
      return (std::forward<T>(a));
    else
      return (a < 0 ? -a : a);
  }

  template <class T, class U>
  requires(!std::is_same_v<T, U>)
  constexpr auto max(T &&a, U &&b)
  {
    using type = std::common_type<T, U>::type;

    return (a > b ? static_cast<type>(a) : static_cast<type>(b));
  }

  template <class T, class U>
  requires(std::is_same_v<T, U>)
  constexpr auto max(T &&a, U &&b)
  {
    return (a > b ? a : b);
  }

  template <class T, class U, class V, class... Args>
  constexpr auto max(T &&a, U &&b, V &&c, Args &&...args)
  {
    return (max(
      max(std::forward<T>(a), std::forward<U>(b)),
      std::forward<V>(c),
      std::forward<Args>(args)...
    ));
  }

  template <class T, class U>
  requires(!std::is_same_v<T, U>)
  constexpr auto min(T &&a, U &&b)
  {
    return (static_cast<std::common_type_t<T, U>>(a < b ? a : b));
  }

  template <class T, class U>
  requires(std::is_same_v<T, U>)
  constexpr auto min(T &&a, U &&b)
  {
    using type = std::common_type<T, U>::type;

    return (a < b ? static_cast<type>(a) : static_cast<type>(b));
  }

  template <class T, class U, class V, class... Args>
  constexpr auto min(T &&a, U &&b, V &&c, Args &&...args)
  {
    return (min(
      min(std::forward<T>(a), std::forward<U>(b)),
      std::forward<V>(c),
      std::forward<Args>(args)...
    ));
  }
}

#endif
