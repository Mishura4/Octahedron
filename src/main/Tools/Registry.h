#ifndef OCTAHEDRON_REGISTRY_H_
#define OCTAHEDRON_REGISTRY_H_

#include "StringLiteral.h"
#include "TypeList.h"
#include "ValueList.h"
#include "VariableRef.h"

#include <any>
#include <typeinfo>
#include <typeindex>
#include <map>
#include <optional>

namespace Octahedron
{

  namespace _
  {
    // intellisense doesn't seem to like template <auto> constraint overloads, so it requires this to correctly resolve them
    template <typename T>
    concept intellisense_literal_fix = !Octahedron::is_string_literal_s<T>::value && !std::is_same_v<T, const char *>;


    template <typename T>
    struct field_runtime_key_type_s
    {
      using type = std::conditional_t<(sizeof(T) > (sizeof(void *) * 4)), Octahedron::VariableRef<const T>, T>;
    };

    template <typename CharT, size_t N>
    struct field_runtime_key_type_s<Octahedron::StringLiteral<CharT, N>>
    {
      using type = std::basic_string_view<CharT>;
    };

    template <typename CharT, size_t N>
    struct field_runtime_key_type_s<const Octahedron::StringLiteral<CharT, N>>
    {
      using type = std::basic_string_view<CharT>;
    };

    template <auto KEY, typename T>
    struct registry_field
    {
      using value_type = T;
      using key_type = decltype(KEY);

      static constexpr key_type key{KEY};

      template <typename U>
        requires (std::equality_comparable_with<T, const U &>)
      constexpr bool operator==(const U &other) const
      {
        return (value == other);
      }

      template <typename U>
        requires (std::three_way_comparable_with<T, const U &>)
      constexpr auto operator<=>(const U &other) const
      {
        return (value <=> other);
      }

      template <typename U>
        requires (std::assignable_from<T &, const U &>)
      constexpr registry_field &operator=(const U &other) noexcept (std::is_nothrow_assignable_v<T &, const U &>)
      {
        value = other;
        return (*this);
      }

      template <typename U>
        requires (std::assignable_from<T &, U &&>)
      constexpr registry_field &operator=(U &&other) noexcept (std::is_nothrow_assignable_v<T &, U &&>)
      {
        value = other;
        return (*this);
      }

      constexpr operator T &()
      {
        return (value);
      }

      constexpr operator const T &() const
      {
        return (value);
      }

      constexpr auto make_runtime_key() const
      {
        return (typename field_runtime_key_type_s<key_type>::type{key});
      }

      template <decltype(KEY) V> requires (KEY == V)
        constexpr T &get() noexcept { return (value); }

      template <decltype(KEY) V> requires (KEY == V)
        constexpr T const &get() const noexcept { return (value); }

      template<auto K>
      constexpr static auto _has_key()
      {
        if constexpr (std::equality_comparable_with<decltype(K), decltype(KEY)>)
          return (K == KEY);
        else
          return (false);
      }

      T value;
    };

    struct registry_default_ctor_helper_s
    {

    };

    template <typename T>
    concept registry_default_ctor_helper = std::is_same_v<T, registry_default_ctor_helper_s>;
  }  // namespace _

  template <typename T>
  constexpr auto registry_key_get = T::key;

  template <typename T>
  using field_value_type = T::value_type;

  template <typename... Fields>
  struct Registry : private Fields...
  {
    using Fields::get...;

    template <typename T = Octahedron::type_list<Fields...>>
    requires (sizeof...(Fields) > 0 && (std::is_default_constructible_v<Fields> && ...))
    constexpr Registry() :
      Fields{}...
    {
      static_assert(!Octahedron::has_duplicate_values<true, Fields::key...>, "cannot have duplicate keys");
    }

    constexpr Registry(Fields&&... values) noexcept((std::is_nothrow_copy_constructible_v<Fields> && ...)) :
      Fields{values}...
    {
      static_assert(!Octahedron::has_duplicate_values<true, Fields::key...>, "cannot have duplicate keys");
    }

    using key_list = Octahedron::value_list<registry_key_get<Fields>...>;
    using value_type_list = Octahedron::type_list<field_value_type<Fields>...>;
    using field_type_list = Octahedron::type_list<Fields...>;

    template <Octahedron::StringLiteral key>
    consteval bool has_key() const noexcept
    {
      return (Fields::template _has_key<key>() || ...);
    }

    template <auto key>
      requires (_::intellisense_literal_fix<decltype(key)>)
    consteval bool has_key() const noexcept
    {
      return (Fields::template _has_key<key>() || ...);
    }

    constexpr auto static_fields()
    {

      return std::tuple<_::registry_field<Fields::key, typename Fields::value_type &>...>{this->Fields::value...};
    }

    constexpr auto static_fields() const
    {
      return std::tuple<_::registry_field<Fields::key, typename Fields::value_type const &>...>{this->Fields::value...};
    }
  };

  template <typename>
  struct registry_tag
  {
    constexpr static bool is_registry_tag = true;
  };

  namespace _
  {

    template <typename T>
    struct dynamic_registry_storage
    {
      using type = T;

      dynamic_registry_storage(nullptr_t, T *external) :
        storage{nullptr},
        ptr{external}
      {}

      dynamic_registry_storage(T &&value, nullptr_t) :
        storage{std::make_unique<T>(std::forward<T>(value))},
        ptr{storage.get()}
      {}

      std::unique_ptr<T> storage;
      T *ptr;
    };

    template <typename T>
    dynamic_registry_storage(nullptr_t, T *) -> dynamic_registry_storage<T>;

    template <typename T>
    dynamic_registry_storage(T *, T *) -> dynamic_registry_storage<T>;

    template <typename T>
    struct dynamic_registry_value_type_s;

    template <typename... Args>
    struct dynamic_registry_value_type_s<Octahedron::type_list<Args...>>
    {
      using type = std::variant<dynamic_registry_storage<Args>...>;
      using ref_type = Octahedron::VariableRef<Args...>;
    };

    template <typename T>
    struct dynamic_key_type_s;

    template <typename... Args>
    struct dynamic_key_type_s<Octahedron::type_list<Args...>>
    {
      using type = Octahedron::type_list<typename field_runtime_key_type_s<Args>::type...>::without_duplicate_types;
    };

    template <typename T>
    struct is_registry_tag_s
    {
      constexpr static bool value = false;
    };


    template <typename T>
    struct is_registry_tag_s<registry_tag<T>>
    {
      constexpr static bool value = true;
    };

    template <typename T>
    concept is_tag = requires{ std::remove_cvref_t<T>::is_registry_tag; };

    template <typename... Args>
    struct registry_helper;

    template <>
    struct registry_helper<>
    {
      using type = Octahedron::type_list<>;
    };

    template <is_tag T, typename...Args>
    struct registry_helper<T, Args...>
    {
      using type = typename registry_helper<Args...>::type;
    };

    template <typename T, typename...Args>
    struct registry_helper<T, Args...>
    {
      using type = typename Octahedron::type_list<T>::template cat<typename registry_helper<Args...>::type>;
    };

    template <typename T>
    struct registry_list_helper;

    template <typename T, size_t N>
    struct variant_helper_s
    {
      using type = typename T::as_variant;
    };

    template <typename T>
    struct variant_helper_s<T, 1>
    {
      using type = T;
    };

    template <typename... Args>
    struct registry_list_helper<Octahedron::type_list<Args...>>
    {
      using field_list = Octahedron::type_list<Args...>;
      using key_list = Octahedron::value_list<registry_key_get<Args>...>;
      using key_type_list = key_list::type_list;
      using value_type_list = typename Octahedron::type_list<typename Args::value_type...>::without_duplicate_types;

      using dynamic_key_type_list = typename dynamic_key_type_s<key_type_list>::type;
      using dynamic_key_type = std::conditional_t<(dynamic_key_type_list::size > 1), typename dynamic_key_type_list::as_variant, typename dynamic_key_type_list::template at<0>>;
      using value_type = typename dynamic_registry_value_type_s<value_type_list>::type;
      using interface_type = typename dynamic_registry_value_type_s<value_type_list>::ref_type;
    };

    template <typename... Args>
    struct visitor : Args...
    {
      using Args::operator()...;
    };

    template <typename... Args>
    visitor(Args...) -> visitor<Args...>;

    template <typename R, typename T>
    constexpr auto _visitor = [](T &arg) -> R { return (*arg.ptr); };

    template <typename T>
    struct variant_extractor_s;

    template <typename... Args>
    struct variant_extractor_s<std::variant<Args...>> // all Args are of the form dynamic_registry_storage<T>
    {
      using return_type = Octahedron::VariableRef<typename Args::type...>;

      static constexpr visitor visitors = {
        _visitor<return_type, Args>...
      };

      using _variant_type = std::variant<Args...>;

      using type = typename Octahedron::VariableRef<typename Args::type...>;
    };
  };

  template <typename MapType, typename... StaticFields>
  struct DynamicRegistry : public Registry<StaticFields...>
  {
  public:
    using base = Registry<StaticFields...>;

    MapType _map;

    using mapped_type = typename MapType::mapped_type;
    using key_type = typename MapType::key_type;
    using value_type = typename MapType::value_type;
    using interface_type = _::variant_extractor_s<mapped_type>::return_type;

  public:
    using base::get;

    DynamicRegistry(StaticFields&&... fields) :
      base(std::forward<StaticFields>(fields)...),
      _map{}
    {
      (_map.emplace(std::make_pair(key_type{fields.make_runtime_key()}, _::dynamic_registry_storage(nullptr, &base::template get<registry_key_get<StaticFields>>()))), ...);
    }

    template <_::is_tag Tag>
    DynamicRegistry(Tag &&, StaticFields&&... fields) :
      base(std::forward<StaticFields>(fields)...),
      _map{}
    {
      (_map.emplace(std::make_pair(key_type{fields.make_runtime_key()}, _::dynamic_registry_storage(nullptr, &base::template get<registry_key_get<StaticFields>>()))), ...);
    }

    template <typename K, typename V>
    void insert(const K &key, V &&value)
    {
      _map.emplace(std::make_pair(key_type{key}, _::dynamic_registry_storage(std::forward<V>(value), nullptr)));
    }

    template <typename K>
    std::optional<interface_type> get(const K &key)
    {
      auto it = _map.find(key);

      if (it == _map.end())
        return {std::nullopt};
      return (std::visit(_::variant_extractor_s<mapped_type>::visitors, it->second));
    }
  };

  template <typename... Ts>
  DynamicRegistry(Ts...) -> DynamicRegistry<std::map<typename _::registry_list_helper<typename _::registry_helper<Ts...>::type>::dynamic_key_type, typename _::registry_list_helper<typename _::registry_helper<Ts...>::type>::value_type>, Ts...>;

  template <typename MapType, typename... Ts>
  DynamicRegistry(registry_tag<MapType>, Ts...) -> DynamicRegistry<MapType, Ts...>;

  template <Octahedron::StringLiteral key, typename T>
  constexpr auto field(T &&value) -> _::registry_field<key, T>
  {
    return {value};
  }

  template <Octahedron::StringLiteral key, typename T, typename... Args>
  constexpr auto field(Args&&... args) -> _::registry_field<key, T>
  {
    return _::registry_field<key, T>(std::forward<Args>(args)...);
  }

  template <_::intellisense_literal_fix auto key, typename T>
  constexpr auto field(T &&value) -> _::registry_field<key, T>
  {
    return {value};
  }

  template <_::intellisense_literal_fix auto key, typename T, typename... Args>
  constexpr auto field(Args&&... args) -> _::registry_field<key, T>
  {
    return _::registry_field<key, T>(std::forward<Args>(args)...);
  }
}

#endif
