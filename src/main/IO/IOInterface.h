#ifndef OCTAHEDRON_IO_INTERFACE_H_
#define OCTAHEDRON_IO_INTERFACE_H_

#include <span>

#include "Base.h"

namespace Octahedron
{
  template <typename T>
  struct is_string_view_s
  {
      static inline constexpr bool value = false;
  };

  template <typename T>
  requires(std::ranges::range<std::remove_cvref_t<T>>)
  struct is_string_view_s<T>
  {
      using range_type = std::remove_reference_t<T>;
      using char_type  = std::conditional_t<std::is_const_v<range_type>, const char, char>;
      using value_type =
        std::remove_reference_t<decltype(*std::ranges::begin(std::declval<range_type &>()))>;

      static inline constexpr bool value = std::same_as<value_type, char_type>;
  };

  template <typename T>
  constexpr inline bool is_endian_dependent =
    (std::is_enum_v<std::remove_cvref_t<T>> ||
     std::integral<std::remove_cvref_t<T>>) &&(sizeof(std::remove_cvref_t<T>) > 1);

  template <typename T>
  requires(std::ranges::range<std::remove_reference_t<T>>)
  constexpr inline bool can_memcpy =
    std::ranges::contiguous_range<T> &&
    std::is_scalar_v<std::remove_reference_t<
      decltype(*std::ranges::begin(std::declval<std::add_lvalue_reference_t<T>>()))>>;

  template <typename T>
  constexpr inline bool is_string_view = is_string_view_s<T>::value;

  enum class Endianness
  {
    LITTLE,
    BIG,

    DEFAULT = LITTLE,
    NATIVE  = std::endian::native == std::endian::little ? LITTLE : BIG,
    SWAPPED = 1 - static_cast<int>(NATIVE)
  };

  template <typename T>
  consteval auto byte_mask(size_t n)
  {
    return (T(0xFF) << (n * 8));
  };

  template <Endianness Endian = Endianness::SWAPPED>
  constexpr auto byteswap(auto value)
  requires(
    std::is_integral_v<decltype(value)> &&
    (std::has_unique_object_representations_v<decltype(value)>)
  )
  {
    using T = decltype(value);

    constexpr auto swap = []<size_t... Ns>(T v, std::index_sequence<Ns...>) constexpr
    {
      constexpr auto impl = []<size_t N>(T v_) constexpr
      {
        constexpr auto size = sizeof...(Ns) / 2;

        if constexpr (N < size)
        {
          constexpr auto shift = ((size - N) * 16 - 8);

          return ((v_ & byte_mask<T>(N)) << shift);
        }
        else
        {
          constexpr auto shift = ((N - size) * 16 + 8);

          return ((v_ & byte_mask<T>(N)) >> shift);
        }
      };
      return (impl.template operator()<Ns>(v) | ...);
    };

    if constexpr (Endian == Endianness::NATIVE || sizeof(T) == 1)
      return (value);
    else
      return (swap(value, std::make_index_sequence<sizeof(T)>()));
  }
  template <typename T>
  concept writeable = requires(T &t, std::span<const std::byte> data) {
    {
      t.write(data.data(), data.size())
    } -> std::convertible_to<size_t>;
  };

  template <typename T>
  concept readable = requires(T &t, std::span<std::byte> data) {
    {
      t.read(data.data(), data.size())
    } -> std::convertible_to<size_t>;
  };

  template <typename T>
  struct IOInterface
  {
      template <Endianness Endian = Endianness::DEFAULT>
      bool put(auto value)
      requires(writeable<T> && std::is_enum_v<decltype(value)>);

      template <Endianness Endian = Endianness::DEFAULT>
      bool put(auto value)
      requires(writeable<T> && std::is_integral_v<decltype(value)>);

      bool put(std::string_view str)
      requires(writeable<T>);

      template <Endianness Endian = Endianness::DEFAULT>
      size_t put(const auto &values)
      requires(writeable<T> &&
        std::ranges::range<std::remove_cvref_t<decltype(values)>> &&
        !implicitly_convertible_to<decltype(values), std::string_view> &&
        is_endian_dependent<decltype(*std::ranges::begin(values))>)
      ;

      size_t put(const auto &values)
      requires(writeable<T> &&
        std::ranges::range<std::remove_cvref_t<decltype(values)>> &&
        !implicitly_convertible_to<decltype(values), std::string_view> &&
        !is_endian_dependent<decltype(*std::ranges::begin(values))> &&
        can_memcpy<decltype(values)>)
      ;
  };

  template <typename T>
  template <Endianness Endian>
  bool IOInterface<T>::put(auto value)
  requires(writeable<T> && std::is_enum_v<decltype(value)>)
  {
    using type = std::underlying_type_t<decltype(value)>;

    return (put<value, Endian>(static_cast<type>(value)));
  }

  template <typename T>
  template <Endianness Endian>
  bool IOInterface<T>::put(auto value)
  requires(writeable<T> && std::is_integral_v<decltype(value)>)
  {
    if constexpr (Endian != Endianness::NATIVE)
      value = byteswap<Endian>(value);
    return (
      write(std::bit_cast<std::array<const std::byte, sizeof(value)>>(value)) == sizeof(value)
    );
  }

  template <typename T>
  bool IOInterface<T>::put(std::string_view str)
  requires(writeable<T>)
  {
    return (
      static_cast<T *>(this)->write({reinterpret_cast<const std::byte *>(str.data()), str.size()}
      ) == str.size()
    );
  }

  template <typename T>
  template <Endianness Endian>
  size_t IOInterface<T>::put(const auto &values)
      requires(writeable<T> &&
        std::ranges::range<std::remove_cvref_t<decltype(values)>> &&
        !implicitly_convertible_to<decltype(values), std::string_view> &&
        is_endian_dependent<decltype(*std::ranges::begin(values))>)
  {
    if constexpr (Endian == Endianness::NATIVE && can_memcpy<decltype(values)>)
    {
      constexpr size_t value_size =
        sizeof(std::remove_cvref_t<decltype(*std::ranges::begin(values))>);

      return (
        static_cast<T *>(this)->write(
          {reinterpret_cast<const std::byte *>(&(*std::ranges::begin(values))), std::size(values)}
        ) /
        value_size
      );
    }
    else
    {
      size_t inserted = 0;

      for (auto v : values)
      {
        if (!put(values))
          return (inserted);
        ++inserted;
      }
      return (inserted);
    }
  }

  template <typename T>
  size_t IOInterface<T>::put(const auto &values)
      requires(writeable<T> &&
        std::ranges::range<std::remove_cvref_t<decltype(values)>> &&
        !implicitly_convertible_to<decltype(values), std::string_view> &&
        !is_endian_dependent<decltype(*std::ranges::begin(values))> &&
        can_memcpy<decltype(values)>)
  {
    constexpr size_t value_size =
      sizeof(std::remove_cvref_t<decltype(*std::ranges::begin(values))>);

    return (
      static_cast<T *>(this)->write(
        {reinterpret_cast<const std::byte *>(&(*std::ranges::begin(values))), std::size(values)}
      ) /
      value_size
    );
  }
}

#endif
