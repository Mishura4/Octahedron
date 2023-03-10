#ifndef OCTAHEDRON_BASE_H_
#define OCTAHEDRON_BASE_H_

#include <chrono>
#include <cstddef>

namespace Octahedron
{
  using std::chrono::duration_cast;
  using std::chrono::hours;
  using std::chrono::microseconds;
  using std::chrono::milliseconds;
  using std::chrono::minutes;
  using std::chrono::nanoseconds;
  using std::chrono::seconds;

  using int8   = int8_t;
  using int16  = int16_t;
  using int32  = int32_t;
  using int64  = int64_t;
  using uint8  = uint8_t;
  using uint16 = uint16_t;
  using uint32 = uint32_t;
  using uint64 = uint64_t;

  using fint8  = int_fast8_t;
  using fint16 = int_fast16_t;
  using fint32 = int_fast32_t;
  using fint64 = int_fast64_t;

  using fuint8  = uint_fast8_t;
  using fuint16 = uint_fast16_t;
  using fuint32 = uint_fast32_t;
  using fuint64 = uint_fast64_t;

  using std::byte;

  using namespace std::string_view_literals;

  template <typename T, typename U>
  requires(sizeof(T) < sizeof(U) && !std::is_reference_v<T> && std::is_unsigned_v<T> ||
           std::is_signed_v<std::remove_cvref_t<U>>) &&
          requires { std::declval<T &>() = static_cast<T>(std::declval<U>()); }

  [[msvc::intrinsic]] constexpr T narrow_cast(U &&value) noexcept
  {
    return (static_cast<T>(value));
  }

  template <typename T>
  requires(std::is_arithmetic_v<T> && std::is_signed_v<T> && !std::is_reference_v<T>)
  [[msvc::intrinsice]] constexpr auto to_unsigned(T &&value) noexcept
  {
    return static_cast<std::make_unsigned_t<T>>(value);
  }

  template <typename T>
  requires(std::is_arithmetic_v<T> && !std::is_signed_v<T> && !std::is_reference_v<T>)
  [[msvc::intrinsice]] constexpr auto to_signed(T &&value) noexcept
  {
    return static_cast<std::make_unsigned_t<T>>(value);
  }

  template <typename T>
  requires(!std::is_enum_v<std::remove_cvref_t<T>>)
  constexpr T bitflag(T operand) noexcept
  {
    if (operand == 0)
      return T{0};
    if (operand == static_cast<T>(-1))
      return (static_cast<T>(-1));

    T _operand{operand - 1};

    return (T{1 << _operand});
  }

  template <typename T, typename U>
  requires(std::is_enum_v<std::remove_cvref_t<T>>)
  constexpr auto bitflag(U &&operand) noexcept -> std::underlying_type_t<T>
  {
    if (operand == 0)
      return {0};
    if (operand == U(-1))
      return (static_cast<std::underlying_type_t<T>>(-1));

    U _operand{operand - 1};

    return (static_cast<std::underlying_type_t<T>>(U(1) << _operand));
  }

  template <typename T>
  requires(std::is_enum_v<std::remove_cvref_t<T>>)
  constexpr auto decay_cast(T &&value) noexcept -> std::underlying_type_t<std::remove_cvref_t<T>>
  {
    return static_cast<std::underlying_type_t<std::remove_cvref_t<T>>>(value);
  }

  template <typename T>
  requires(std::is_enum_v<std::remove_cvref_t<T>>)
  constexpr T &operator|=(T &lhs, const T &rhs) noexcept
  {
    lhs = T{decay_cast(lhs) | decay_cast(rhs)};
    return (lhs);
  }

  constexpr inline auto noop = [](auto...) constexpr noexcept {};

  template <auto R>
  constexpr inline auto noop_r = [](auto...) constexpr noexcept -> decltype(R) { return (R); };

  template <typename T>
  constexpr inline auto make_empty =
    [](auto &&...) noexcept(std::is_nothrow_default_constructible_v<T>) { return T{}; };

  template <typename T>
  requires(std::is_enum_v<T>)
  struct BitSet
  {
      template <typename... Args>
      requires(std::same_as<T, std::remove_reference_t<Args>> && ...)
      constexpr BitSet(Args &&...values) noexcept : value{(decay_cast(values) & ...)}
      {
      }

      constexpr BitSet operator&(const BitSet &other) const noexcept
      {
        return (T{value & other.value});
      }

      constexpr BitSet &operator&=(const BitSet &other) noexcept
      {
        value &= other.value;
        return (*this);
      }

      constexpr BitSet operator|(const BitSet &other) const noexcept
      {
        return (T{value | other.value});
      }

      constexpr BitSet &operator|=(const BitSet &other) noexcept
      {
        value |= other.value;
        return (*this);
      }

      constexpr BitSet operator^(const BitSet &other) const noexcept
      {
        return (T{value ^ other.value});
      }

      constexpr BitSet &operator^=(const BitSet &other) noexcept
      {
        value ^= other.value;
        return (*this);
      }

      operator bool() const { return (value != 0); }

      [[msvc::intrinsice]] operator T() const { return (T{value}); }

      constexpr BitSet operator~() const noexcept { return (T{~value}); }

      constexpr BitSet(const BitSet &other) noexcept                 = default;
      constexpr auto operator<=>(const BitSet &other) const noexcept = default;

      std::underlying_type_t<T> value;
  };
}  // namespace Octahedron

#endif
