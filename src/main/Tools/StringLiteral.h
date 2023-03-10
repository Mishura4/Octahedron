#ifndef OCTAHEDRON_STRING_LITERAL_H_
#define OCTAHEDRON_STRING_LITERAL_H_

#include <algorithm>
#include <utility>

namespace Octahedron
{
  template <typename CharT, size_t N>
  struct StringLiteral
  {
      constexpr static size_t size{N};

      template <size_t... Is>
      constexpr StringLiteral(const CharT *str, std::index_sequence<Is...>) : data{str[Is]...}
      {
      }

      constexpr StringLiteral(const CharT (&arr)[N + 1]) :
          StringLiteral{arr, std::make_index_sequence<N + 1>()}
      {
      }

      template <typename CharT2, size_t N2>
      constexpr bool operator==(StringLiteral<CharT2, N2> other) const
      {
        if constexpr (N2 != N)
          return (false);
        else
        {
          for (size_t i = 0; i < N; ++i)
          {
            if (data[i] != other.data[i])
              return (false);
          }
          return (true);
        }
      }

      template <typename CharT2, size_t N2>
      constexpr bool operator<=>(StringLiteral<CharT2, N2> other) const
      {
        if constexpr (N2 != N)
          return (false);
        else
          return (std::ranges::lexicographical_compare(data, other.data));
      }

      constexpr operator std::string_view() const { return {data, N}; }

      constexpr operator std::string() const { return {data, N}; }

      char data[N + 1];
  };

  template <typename CharT, std::size_t N>
  StringLiteral(const CharT (&)[N]) -> StringLiteral<CharT, N - 1>;

  template <typename T>
  struct is_string_literal_s
  {
      constexpr static bool value = false;
  };

  template <template <size_t> typename T, size_t N>
  struct is_string_literal_s<T<N>>
  {
      constexpr static bool value = true;
  };

  template <typename T>
  concept string_literal = is_string_literal_s<T>::value;

  namespace _
  {
    template <StringLiteral lhs, StringLiteral rhs, StringLiteral... more>
    struct literal_concat
    {
        consteval static auto insert(auto &src, size_t size, auto &where)
        {
          return (std::copy_n(src, size, where));
        }

        consteval static auto concat()
        {
          constexpr size_t size1    = decltype(lhs)::size;
          constexpr size_t size2    = decltype(rhs)::size;
          constexpr size_t packSize = {(0 + ... + decltype(more)::size)};
          char data[size1 + size2 + packSize + 1];
          auto it{std::begin(data)};

          it = insert(lhs.data, size1, it);
          it = insert(rhs.data, size2, it);
          ((it = insert(more.data, decltype(more)::size, it)), ...);
          data[size1 + size2 + packSize] = 0;
          return (StringLiteral{data});
        }
    };
  }  // namespace _

  template <StringLiteral lhs, StringLiteral rhs, StringLiteral... more>
  constexpr auto literal_concat = []() consteval
  { return (_::literal_concat<lhs, rhs, more...>::concat()); };

  namespace literals
  {
    template <StringLiteral Input>
    consteval auto operator""_sl()
    {
      return Input;
    }
  }  // namespace literals
}  // namespace octahedron

#endif
