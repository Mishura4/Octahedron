#ifndef OCTAHEDRON_VALUE_LIST_H_
#define OCTAHEDRON_VALUE_LIST_H_

#include <utility>

namespace Octahedron
{
  template <auto... Args>
  struct value_list;

  template <typename... Args>
  struct type_list;

  namespace _
  {
    template <auto... Args>
    struct value_list_helper
    {
      static constexpr size_t size = sizeof...(Args);

      static constexpr std::tuple as_tuple = std::tuple<decltype(Args)...>{Args...};

      using type_list = Octahedron::type_list<decltype(Args)...>;

      template <size_t N>
        requires (N < sizeof...(Args))
      static constexpr auto at = std::get<N>(as_tuple);

      using type = value_list<Args...>;
    };

    template <auto X, auto Y, auto... Vs>
    struct value_duplicates_helper_s
    {
      template <bool exact_type>
      static consteval auto is_duplicate_fun()
      {
        using T = decltype(X);
        using U = decltype(Y);
        constexpr bool is_comparable = (exact_type ? std::is_same_v<T, U> : std::equality_comparable_with<T, U>);

        if constexpr (is_comparable)
          if constexpr (X == Y)
            return (true);
        if constexpr (sizeof...(Vs) > 0)
          return (value_duplicates_helper_s<X, Vs...>::template is_duplicate_fun<true>());
        else
          return (false);
      }

      template <bool exact_type = true>
      static consteval auto has_duplicates_fun()
      {
        if constexpr (sizeof...(Vs) > 0)
          if constexpr (value_duplicates_helper_s<Y, Vs...>::template has_duplicates_fun<exact_type>())
            return (true);
        return (value_duplicates_helper_s<X, Y, Vs...>::template is_duplicate_fun<exact_type>());
      }
    };

    template <bool exact_type, auto... Args>
    consteval bool has_duplicate_values_fun()
    {
      if constexpr (sizeof...(Args) < 2)
        return (false);
      else
        return value_duplicates_helper_s<Args...>::template has_duplicates_fun<exact_type>();
    };

    template <bool exact_type, auto... Args>
    consteval bool is_duplicate_value_fun()
    {
      if constexpr (sizeof...(Args) < 2)
        return (false);
      else
        return value_duplicates_helper_s<Args...>::template is_duplicate_fun<exact_type>();
    };
  }

  template <bool exact_type, auto... Args>
  constexpr bool has_duplicate_values = _::has_duplicate_values_fun<exact_type, Args...>();

  template <bool exact_type, auto... Args>
  constexpr bool is_duplicate_value = _::is_duplicate_value_fun<exact_type, Args...>();

  namespace _
  {
    template <typename lhs, typename rhs, typename... more>
    struct value_list_cat_s;

    template <typename lhs, typename rhs, typename idx_seq>
    struct value_list_cat_helper;

    template <auto T, auto... Args>
    struct value_list_without_duplicates_s;

    template <auto T>
    struct value_list_without_duplicates_s<T>
    {
      using type = value_list_helper<T>;
    };

    template <auto T, auto... Args>
    struct value_list_without_duplicates_s
    {
      using type = std::conditional_t <is_duplicate_value<true, T, Args...>,
        typename value_list_without_duplicates_s<Args...>::type,
        typename value_list_cat_s<value_list<T>, typename value_list_without_duplicates_s<Args...>::type>::type
      >;
    };

    template <typename lhs, typename rhs, size_t... Is>
    struct value_list_cat_helper<lhs, rhs, std::index_sequence<Is...>>
    {
      consteval value_list_cat_helper(std::index_sequence<Is...>) {}

      template <size_t N>
      static consteval auto get()
      {
        if constexpr (N < lhs::size)
          return (lhs::template at<N>);
        else
          return (rhs::template at<N - lhs::size>);
      }

      static consteval auto fun()
      {
        return (value_list_helper<get<Is>()...>{});
      }
    };

    template <typename lhs, typename rhs>
    struct value_list_cat_s<lhs, rhs>
    {
      using type = decltype(value_list_cat_helper<lhs, rhs, std::make_index_sequence<lhs::size + rhs::size>>::fun());
    };

    template <typename lhs, typename rhs, typename... more>
    struct value_list_cat_s
    {
      using type = typename value_list_cat_s<typename value_list_cat_s<lhs, rhs>::type, more...>::type;
    };
  }

  template <auto... Args>
  struct value_list
  {
    static constexpr size_t size = sizeof...(Args);

    static constexpr std::tuple as_tuple = std::tuple<decltype(Args)...>{Args...};

    using type_list = Octahedron::type_list<decltype(Args)...>;

    template <size_t N>
      requires (N < sizeof...(Args))
    static constexpr auto at = std::get<N>(as_tuple);

    constexpr static bool has_duplicate_values = has_duplicate_values<true, Args...>;

    using prune = typename _::value_list_without_duplicates_s<Args...>::type::type;

    template <typename Rhs, typename... More>
    using cat = typename _::value_list_cat_s<value_list<Args...>, Rhs, More...>::type::type;

    template <auto... _Args>
    using append = value_list<Args..., _Args...>;
  };

  template <>
  struct value_list<>
  {
    static constexpr size_t size = 0;

    using type_list = Octahedron::type_list<>;

    constexpr static bool has_duplicate_values = false;

    using prune = value_list<>;

    template <typename Rhs, typename... More>
    using cat = typename _::value_list_cat_s<value_list<>, Rhs, More...>::type::type;

    template <auto... _Args>
    using append = value_list<_Args...>;
  };
  template <typename Lhs, typename Rhs, typename... More>
  using value_list_cat = typename _::value_list_cat_s<Lhs, Rhs, More...>::type::type;

  namespace _
  {
    template <bool has_duplicates, typename T>
    struct value_list_duplicate_helper;

    template <bool has_duplicates, auto... Args>
    struct value_list_duplicate_helper<has_duplicates, value_list<Args...>>
    {
      using type = typename _::value_list_without_duplicates_s<Args...>::type;
    };

    template <auto... Args>
    struct value_list_duplicate_helper<false, value_list<Args...>>
    {
      using type = value_list<Args...>;
    };
  }
}

#endif
