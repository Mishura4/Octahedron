#ifndef OCTAHEDRON_PROTECTED_PTR_
#define OCTAHEDRON_PROTECTED_PTR_

namespace octahedron
{
template <typename T>
class protected_ptr {
public:
	friend T;

	protected_ptr() = default;

	explicit protected_ptr(T *ptr) :
		_ptr(ptr) {}

	protected_ptr(const protected_ptr &other) = default;

	T *operator->() const {
		return (_ptr);
	}

	T *get() const {
		return (_ptr);
	}

	auto operator<=>(const protected_ptr &other) const = default;

	auto operator==(T *ptr) const {
		return (_ptr == ptr);
	}

	operator bool() const {
		return (_ptr);
	}

private:
	protected_ptr& operator=(T *ptr) {
		_ptr = ptr;
		return (*this);
	}

	protected_ptr& operator=(const protected_ptr &other) = default;

	T *_ptr{nullptr};
};
} // namespace octahedron

#endif
