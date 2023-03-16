#ifndef OCTAHEDRON_IO_INTERFACE_H_
#define OCTAHEDRON_IO_INTERFACE_H_

#include <span>

#include <boost/pfr.hpp>

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
    using char_type = std::conditional_t<std::is_const_v<range_type>, const char, char>;
    using value_type =
      std::remove_reference_t<decltype(*std::ranges::begin(std::declval<range_type&>()))>;

    static inline constexpr bool value = std::same_as<value_type, char_type>;
  };

  template <typename T>
  constexpr inline bool is_endian_dependent =
    (std::is_scalar_v<T>) &&(sizeof(T) > 1);

  template <typename T>
    requires(std::ranges::range<std::remove_reference_t<T>>)
  constexpr inline bool is_contiguous_data =
    std::ranges::contiguous_range<T> &&
    std::is_scalar_v<std::remove_reference_t<
    decltype(*std::ranges::begin(std::declval<std::add_lvalue_reference_t<T>>()))>>;

  template <typename T>
  concept byteoid = sizeof(T) == 1 && alignof(T) == 1;

  template <typename T>
  concept bytes = std::ranges::range<std::remove_reference_t<T>> &&
    byteoid<std::remove_reference_t<decltype(std::begin(std::declval<T&>()))>>;

  template <typename T>
  constexpr inline bool is_string_view = is_string_view_s<T>::value;

  enum class Endianness
  {
    LITTLE,
    BIG,

    DEFAULT = LITTLE,
    NATIVE = std::endian::native == std::endian::little ? LITTLE : BIG,
    SWAPPED = 1 - static_cast<int>(NATIVE)
  };

  template <size_t Size>
  struct integer_s;

  template <>
  struct integer_s<1>
  {
    using type = int8_t;
  };

  template <>
  struct integer_s<2>
  {
    using type = int16_t;
  };

  template <>
  struct integer_s<4>
  {
    using type = int32_t;
  };

  template <>
  struct integer_s<8>
  {
    using type = int64_t;
  };

  template <size_t Size>
  using integer = typename integer_s<Size>::type;

  template <typename T>
  consteval auto byte_mask(size_t n)
  {
    return (T(0xFF) << (n * 8));
  };

  template <Endianness Endian = Endianness::SWAPPED>
  constexpr auto byteswap(auto value)
  requires(std::is_floating_point_v<decltype(value)>)
  {
    if constexpr (Endian == Endianness::NATIVE)
      return (value);
    else
    {
      union helper
      {
          integer<sizeof(value)> as_int;
          decltype(value) as_float;
      };

      helper v{.as_float = value};

      v.as_int = byteswap<Endian>(v.as_int);
      return (v.as_float);
    }
  }

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

  struct test
  {
      int e;
      char c;
  };

  struct invalid
  {
      char bb;

      union
      {
          int g;
          char b;
      };
  };

  template <typename T>
  struct Serializer;

  /*template <typename T>
  struct pfr_helper;

  template <typename T>
  requires(std::is_scalar_v<T>)
  struct pfr_helper<T>
  {
      constexpr static inline bool valid = true;
  };

  template <typename T>
  struct pfr_helper
  {
      template <size_t... Ns>
      consteval static bool is_pfr_valid_fun(std::index_sequence<Ns...>)
      {
        constexpr auto impl = []<size_t N>() consteval -> bool
        {
          if constexpr (requires { std::declval<boost::pfr::tuple_element_t<N, T>>(); })
          {
            using type = boost::pfr::tuple_element_t<N, T>;

            return (pfr_helper<type>::valid);
          }
          else
            return (false);
        };

        return (impl.template operator()<Ns>() && ...);
      }

      consteval static bool is_pfr_valid_fun()
      {
        if constexpr (requires {
                        {
                          boost::pfr::tuple_size_v<T>
                        } -> std::convertible_to<size_t>;
                      })
        {
          return (is_pfr_valid_fun(std::make_index_sequence<boost::pfr::tuple_size_v<T>>()));
        }
        else
          return (false);
      }

      constexpr static inline bool valid = is_pfr_valid_fun();
  };*/

  template <size_t N>
  struct size_s
  {
      constexpr static inline size_t size = N;

      constexpr auto operator<=>(const size_s&) const = default;

      constexpr operator bool() const { return (value == size); }

      size_t value{0};
  };

  /* template <typename T>
  constexpr bool is_pfr_valid =
    (std::is_aggregate_v<T> || std::is_scalar_v<T>) && pfr_helper<T>::valid;

  constexpr auto teststest = is_pfr_valid<test>;

  using tyestt = boost::pfr::tuple_element_t<0, test>;

  static_assert(pfr_helper<test>::is_pfr_valid_fun(std::make_index_sequence<boost::pfr::tuple_size_v<test>>()) == true);
  static_assert(is_pfr_valid<invalid> == false);*/

  template <typename T>
  struct IOInterface
  {
      size_t put(std::string_view str)
      requires(writeable<T>);

      template <Endianness Endian = Endianness::DEFAULT>
      size_t put(auto value)
      requires(writeable<T> && std::is_enum_v<decltype(value)>);

      template <Endianness Endian = Endianness::DEFAULT>
      size_t put(auto value)
      requires(writeable<T> && std::is_integral_v<decltype(value)>);

      template <Endianness Endian = Endianness::DEFAULT>
      size_t put(auto &&values)
      requires(writeable<T> &&
        std::ranges::range<std::remove_cvref_t<decltype(values)>> &&
        !implicitly_convertible_to<decltype(values), std::string_view> &&
        is_endian_dependent<std::remove_cvref_t<decltype(*std::ranges::begin(values))>>)
      ;

      size_t put(auto &&values)
      requires(writeable<T> &&
        std::ranges::range<std::remove_cvref_t<decltype(values)>> &&
        !implicitly_convertible_to<decltype(values), std::string_view> &&
        !is_endian_dependent<std::remove_cvref_t<decltype(*std::ranges::begin(values))>> &&
        is_contiguous_data<decltype(values)>)
      ;

      template <Endianness Endian = Endianness::DEFAULT>
      size_t put(auto &&value)
      requires(writeable<T> && !std::is_scalar_v<std::remove_reference_t<decltype(value)>> && !std::ranges::range<std::remove_cvref_t<decltype(value)>>)
      ;

      template <Endianness Endian = Endianness::DEFAULT>
      size_t get(auto &value)
      requires(readable<T>&& std::is_enum_v<std::remove_reference_t<decltype(value)>>)
      {
        std::underlying_type_t v;

        if (!get(v))
          return (0);
        value = static_cast<std::remove_reference_t<decltype(value)>>(v);
        return (1);
      }

      template <Endianness Endian = Endianness::DEFAULT>
      size_t get(auto &value)
      requires(readable<T> &&
        !std::is_enum_v<std::remove_reference_t<decltype(value)>> &&
        is_endian_dependent<std::remove_reference_t<decltype(value)>>)
      {
        std::byte *data = reinterpret_cast<std::byte *>(&value);

        if (static_cast<T *>(this)->read(data, sizeof(value)) != sizeof(value))
          return (0);
        if constexpr (Endian != Endianness::NATIVE)
          value = byteswap<Endian>(value);
        return (1);
      }

      size_t get(auto &value)
        requires(readable<T>&&
        std::is_scalar_v<std::remove_reference_t<decltype(value)>> &&
        !std::ranges::range<std::remove_cvref_t<decltype(value)>> &&
        !is_endian_dependent<std::remove_reference_t<decltype(value)>>
        )
      {
        std::byte *data = reinterpret_cast<std::byte *>(&value);

        return (static_cast<T *>(this)->read(data, sizeof(value)) == sizeof(value) ? 1 : 0);
      }

      template <Endianness Endian = Endianness::DEFAULT>
      size_t get(auto &value)
      requires(readable<T> &&
        !std::is_scalar_v<std::remove_reference_t<decltype(value)>> &&
        !std::ranges::range<std::remove_cvref_t<decltype(value)>>)
      {
        using type = std::remove_reference_t<decltype(value)>;

        if constexpr (requires(Serializer<type> t) { t.get(this, value); })
        {
          return (Serializer<type>().get(this, value));
        }
        // vvv IF THIS FAILS --- SPECIALIZE THIS ^^^
        else if constexpr (std::is_scalar_v<type> || std::is_aggregate_v<type>)
        {
          constexpr auto num_fields = boost::pfr::tuple_size_v<type>;

          static_assert(num_fields > 0, "Cannot serialize an empty object");

          constexpr auto impl =
            []<size_t... Ns>(IOInterface<T> *self, type &v, std::index_sequence<Ns...>)
          {
            constexpr auto get_ = []<size_t N>(IOInterface<T> *self, decltype(value) &v_)
            {

              using field_type = boost::pfr::tuple_element_t<N, type>;

              auto &field = boost::pfr::get<N>(v_);

              if constexpr (is_endian_dependent<field_type>)
                return (self->get<Endian>(field));
              else
                return (self->get(field));
            };
            return (size_t{0} + ... + get_.template operator()<Ns>(self, v));
          };

          return (impl(this, value, std::make_index_sequence<num_fields>()));
        }
      }

      template <Endianness Endian = Endianness::DEFAULT>
      size_t get(auto &values)
        requires(writeable<T>&&
        std::ranges::range<std::remove_cvref_t<decltype(values)>>&&
        is_endian_dependent<std::remove_cvref_t<decltype(*std::ranges::begin(values))>>)
      {
        using value_type = std::ranges::range_value_t<std::remove_reference_t<decltype(values)>>;
        constexpr auto value_size = sizeof(value_type);

        if constexpr (Endian == Endianness::NATIVE && is_contiguous_data<decltype(values)>)
        {
          std::byte *data = reinterpret_cast<std::byte *>(&(*std::ranges::begin(values)));
          size_t size     = std::size(values) * value_size;

          return (static_cast<T *>(this)->read(data, size) == size);
        }
        else
        {
          size_t i{0};

          for (auto &v : values)
          {
            if (get<Endian>(v) != sizeof(v))
              return (i);
            ++i;
          }
          return (i);
        }
      }

      size_t get(auto &values)
        requires(writeable<T>&&
        std::ranges::range<std::remove_cvref_t<decltype(values)>> &&
        !is_endian_dependent<std::remove_cvref_t<decltype(*std::ranges::begin(values))>>)
      {
        using value_type = std::ranges::range_value_t<std::remove_reference_t<decltype(values)>>;
        constexpr auto value_size = sizeof(value_type);

        if constexpr (is_contiguous_data<decltype(values)>)
        {
          std::byte *data = reinterpret_cast<std::byte *>(&(*std::ranges::begin(values)));
          size_t size = std::size(values) * value_size;

          return (static_cast<T *>(this)->read(data, size) / value_size);
        }
        else
        {
          size_t i{0};

          for (auto& v : values)
          {
            if (get(v) != sizeof(v))
              return (i);
            ++i;
          }
          return (i);
        }
      }

      template <Endianness Endian = Endianness::DEFAULT>
      auto get(auto *value, auto num) -> decltype(num)
      requires(writeable<T>)
      {
        for (decltype(num) i = 0; i < num; ++i)
        {
          if (!get(value[num]))
            return (i);
        }
        return (num);
      }

      template <Endianness Endian = Endianness::DEFAULT>
      auto get(auto &...values)
      requires(
        writeable<T> && sizeof...(values) > 1 &&
        (!std::is_pointer_v<std::remove_reference_t<decltype(values)>> && ...)
      )
      {
        constexpr auto impl = [](IOInterface<T> *self, auto &v)
        {
          using type = std::remove_reference_t<decltype(v)>;
          if constexpr (is_endian_dependent<type>)
            return (self->get<Endian>(v));
          else
            return (self->get(v));
        };
        return size_s<sizeof...(values)>{(size_t{0} + ... + impl(this, values))};
      }

      template <Endianness Endian = Endianness::DEFAULT>
      auto put(auto &&...values)
      requires(
        writeable<T> && sizeof...(values) > 1 &&
        (!std::is_pointer_v<std::remove_reference_t<decltype(values)>> && ...))
      {
        constexpr auto impl = [](IOInterface<T> *self, auto &&v)
        {
          using type = std::remove_reference_t<decltype(v)>;
          if constexpr (is_endian_dependent<type>)
            return (self->put<Endian>(v));
          else
            return (self->put(v));
        };
        return size_s<sizeof...(values)>{(size_t{0} + ... + impl(this, values))};
      }
  };

  template <typename T>
  template <Endianness Endian>
  size_t IOInterface<T>::put(auto value)
  requires(writeable<T> && std::is_enum_v<decltype(value)>)
  {
    using type = std::underlying_type_t<decltype(value)>;

    return (put<value, Endian>(static_cast<type>(value)));
  }

  template <typename T>
  template <Endianness Endian>
  size_t IOInterface<T>::put(auto value)
  requires(writeable<T> && std::is_integral_v<decltype(value)>)
  {
    auto data = reinterpret_cast<const std::byte *>(&value);

    if constexpr (Endian != Endianness::NATIVE)
      value = byteswap<Endian>(value);
    return (static_cast<T *>(this)->write(data, sizeof(value)) == sizeof(value) ? 1 : 0);
  }

  template <typename T>
  size_t IOInterface<T>::put(std::string_view str)
  requires(writeable<T>)
  {
    auto data = reinterpret_cast<const std::byte *>(str.data());

    return (static_cast<T *>(this)->write(data, str.size()) / sizeof(str[0]));
  }

  template <typename T>
  template <Endianness Endian>
  size_t IOInterface<T>::put(auto &&values)
      requires(writeable<T> &&
        std::ranges::range<std::remove_cvref_t<decltype(values)>> &&
        !implicitly_convertible_to<decltype(values), std::string_view> &&
        is_endian_dependent<std::remove_cvref_t<decltype(*std::ranges::begin(values))>>)
  {
    using value_type = std::remove_cvref_t<decltype(*std::ranges::begin(values))>;

    if constexpr (Endian == Endianness::NATIVE && is_contiguous_data<decltype(values)>)
    {
      constexpr size_t value_size = sizeof(value_type);
      const std::byte *data = reinterpret_cast<const std::byte *>(&(*std::ranges::begin(values)));
      size_t size = std::size(values) * value_size;

      return (static_cast<T *>(this)->write({data, size}) / value_size);
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
  size_t IOInterface<T>::put(auto &&values)
      requires(writeable<T> &&
        std::ranges::range<std::remove_cvref_t<decltype(values)>> &&
        !implicitly_convertible_to<decltype(values), std::string_view> &&
        !is_endian_dependent<std::remove_cvref_t<decltype(*std::ranges::begin(values))>> &&
        is_contiguous_data<decltype(values)>)
  {
    using value_type            = std::remove_cvref_t<decltype(*std::ranges::begin(values))>;
    constexpr size_t value_size = sizeof(value_type);
    const std::byte *data = reinterpret_cast<const std::byte *>(&(*std::ranges::begin(values)));
    size_t size           = std::size(values) * value_size;

    return (static_cast<T *>(this)->write({data, size}) / value_size);
  }

  template <typename T>
  template <Endianness Endian>
  size_t IOInterface<T>::put(auto &&value)
  requires(writeable<T> && !std::is_scalar_v<std::remove_reference_t<decltype(value)>> && !std::ranges::range<std::remove_cvref_t<decltype(value)>>)
  {
    using type = std::remove_cvref_t<decltype(value)>;

    if constexpr (requires(Serializer<type> t) { t.put(this, std::as_const(value)); })
    {
      return (Serializer<type>().put(this, value));
    }
    // vvv IF THIS FAILS --- SPECIALIZE THIS ^^^
    else if constexpr (std::is_scalar_v<type> || std::is_aggregate_v<type>)
    {
      constexpr auto num_fields = boost::pfr::tuple_size_v<type>;

      static_assert(num_fields > 0, "Cannot serialize an empty object");

      constexpr auto impl =
        []<size_t... Ns>(IOInterface<T> *self, const type &v, std::index_sequence<Ns...>) -> size_t
      {
        constexpr auto put_ = []<size_t N>(IOInterface<T> *self, const type &v_) -> size_t
        {
          using field_type = boost::pfr::tuple_element_t<N, type>;

          auto &field = boost::pfr::get<N>(v_);

          if constexpr (is_endian_dependent<field_type>)
            return (self->put<Endian>(field));
          else
            return (self->put(field));
        };
        return (size_t{0} + ... + put_.template operator()<Ns>(self, v));
      };

      return (impl(this, value, std::make_index_sequence<num_fields>()));
    }
  }
}

#endif
