#ifndef OCTAHEDRON_MANAGED_RESOURCE_H_
#define OCTAHEDRON_MANAGED_RESOURCE_H_

#include <concepts>
#include <type_traits>

namespace octahedron
{
template <typename T>
concept trivial_resource = std::is_default_constructible_v<T> && std::is_assignable_v<T&, T>;

/*;

template <typename T, std::invocable<T> auto Releaser>
requires(
std::is_default_constructible_v<T> &&
std::is_assignable_v<T&, T>
)
class ManagedResource<T, Releaser>*/
template <typename T, auto Releaser>
class managed_resource;

template <trivial_resource T, auto Releaser> requires(std::invocable<decltype(Releaser), T&>)
class managed_resource<T, Releaser> {
public:
	constexpr static inline bool nothrow_release = noexcept(Releaser(std::declval<T&>()));

	managed_resource() = default;

	managed_resource(T resource) noexcept(std::is_nothrow_assignable_v<T&, T>) :
		_resource(resource) { }

	managed_resource(const managed_resource &other) = delete;

	managed_resource(managed_resource &&rvalue) noexcept(std::is_nothrow_assignable_v<T&, T&&>) :
		_resource(std::move(rvalue._resource)) {
		rvalue._resource = T{};
	}

	~managed_resource() noexcept(nothrow_release) {
		release();
	}

	managed_resource& operator=(const managed_resource &other) = delete;

	managed_resource& operator=(
		managed_resource &&rvalue
	) noexcept(nothrow_release && std::is_nothrow_assignable_v<T&, T&&>) {
		release();
		_resource = std::move(rvalue._resource);
		rvalue._resource = T{};
		return (*this);
	}

	const T& operator->() const noexcept {
		return (_resource);
	}

	managed_resource& operator=(
		const T &ptr
	) noexcept(nothrow_release && std::is_nothrow_assignable_v<T, T>) {
		release();
		_resource = ptr;
		return (*this);
	}

	operator T&() noexcept {
		return (get());
	}

	operator const T&() const noexcept {
		return (get());
	}

	T& get() noexcept {
		return (_resource);
	}

	const T& get() const noexcept {
		return (_resource);
	}

	operator bool() const noexcept {
		if constexpr (std::convertible_to<T, bool>) {
			return (static_cast<bool>(_resource));
		} else if constexpr (std::equality_comparable_with<T, bool>) {
			return (_resource == true);
		} else if constexpr (std::equality_comparable<T>) {
			return (_resource == T{});
		} else {
			static_assert(!std::same_as<T, T>, "type does not have a trivial \"is empty\" semantic");
		}
	}

	void release() noexcept(nothrow_release) {
		if (*this)
			Releaser(_resource);
	}

private:
	T _resource{};
};

template <std::invocable auto Releaser>
class managed_resource<nullptr_t, Releaser> {
public:
	managed_resource() = default;

	template <typename T> requires(std::convertible_to<std::remove_cvref_t<T>, bool>)
	managed_resource(T &&value) noexcept :
		_owned{static_cast<bool>(value)} { }

	managed_resource(const managed_resource &other) = delete;
	managed_resource(managed_resource &&other) = delete;

	~managed_resource() {
		if (_owned)
			Releaser();
	}

	bool hasResource() const {
		return (_owned);
	}

	void hasResource(bool value) {
		_owned = value;
	}

	operator bool() const {
		return (hasResource());
	}

	managed_resource& operator=(managed_resource &&other) = delete;
	managed_resource& operator=(const managed_resource &other) = delete;

private:
	bool _owned{false};
};
}

#endif
