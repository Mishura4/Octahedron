#ifndef OCTAHEDRON_PROTECTED_PTR_
#define OCTAHEDRON_PROTECTED_PTR_

namespace Octahedron
{
  template <typename T>
  class ProtectedPtr
  {
    public:
      friend T;

      ProtectedPtr() = default;

      explicit ProtectedPtr(T *ptr) : _ptr(ptr) {}

      ProtectedPtr(const ProtectedPtr &other) = default;

      T *operator->() const { return (_ptr); }

      T *get() const { return (_ptr); }

      auto operator<=>(const ProtectedPtr &other) const = default;

      auto operator==(T *ptr) const { return (_ptr == ptr); }

      operator bool() const { return (_ptr); }

    private:

      ProtectedPtr &operator=(T *ptr)
      {
        _ptr = ptr;
        return (*this);
      }

      ProtectedPtr &operator=(const ProtectedPtr &other) = default;

      T *_ptr{nullptr};
  };
}  // namespace octahedron

#endif