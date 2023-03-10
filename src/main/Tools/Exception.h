#ifndef OCTAHEDRON_EXCEPTION_H_
#define OCTAHEDRON_EXCEPTION_H_

#include <ranges>
#include <array>
#include <algorithm>

#include "Math.h"
#include "StringLiteral.h"

namespace Octahedron
{
  template <typename Base, StringLiteral Literal>
  struct Exception : public Base
  {
      const char *what() const final { return (Literal.data); }
  };

  template <typename Base>
  struct Exception<Base, StringLiteral{""}> : public Base
  {
      static inline constexpr auto buffer_size = 512;

      explicit Exception(std::string_view msg) noexcept : std::exception()
      {
        auto size = min(msg.size(), buffer_size);

        std::ranges::copy_n(msg.begin(), size, message.begin());
        message[size - 1] = 0;
      }

      Exception(Exception &&other) = delete;

      Exception(const Exception &other) noexcept { *this = other; }

      Exception &operator=(const Exception &other) noexcept
      {
        std::ranges::copy(other.message, message.begin());
        return (*this);
      }

      Exception &operator=(Exception &&other) = delete;

      const char *what() const final { return (message.data()); }

      std::array<char, buffer_size> message;
  };

  template <StringLiteral Message>
  struct FatalException : public Exception<std::exception, Message>
  {
    using Base = Exception<std::exception, Message>;

    using Base::Base;
    using Base::operator=;
  };

  FatalException(std::string_view msg) -> FatalException<"">;
}  // namespace octahedron

#endif
