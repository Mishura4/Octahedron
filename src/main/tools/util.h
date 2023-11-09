#ifndef OCTAHEDRON_TOOLS_UTIL_H_
#define OCTAHEDRON_TOOLS_UTIL_H_

namespace octahedron {

template <class From, class To>
struct mimic_reference {
	using type = std::remove_reference_t<To>;
};

template <class From, class To>
struct mimic_reference<From&, To> {
	using type = std::add_lvalue_reference_t<std::remove_reference_t<To>>;
};

template <class From, class To>
struct mimic_reference<From&&, To> {
	using type = std::add_rvalue_reference_t<std::remove_reference_t<To>>;
};

template <class From, class To>
struct mimic_reference<From const&, To> {
	using type = std::add_lvalue_reference_t<std::remove_reference_t<To>>;
};

template <class From, class To>
struct mimic_reference<From const&&, To> {
	using type = std::add_rvalue_reference_t<std::remove_reference_t<To>>;
};

template <class From, class To>
using mimic_reference_t = typename mimic_reference<From, To>::type;

static_assert(std::same_as<mimic_reference_t<int, char>, char>);
static_assert(std::same_as<mimic_reference_t<int&, char>, char&>);
static_assert(std::same_as<mimic_reference_t<int const&, char>, char&>);
static_assert(std::same_as<mimic_reference_t<int&&, char>, char&&>);
static_assert(std::same_as<mimic_reference_t<int const&&, char>, char&&>);

static_assert(std::same_as<mimic_reference_t<int, char&>, char>);
static_assert(std::same_as<mimic_reference_t<int, char&&>, char>);
static_assert(std::same_as<mimic_reference_t<int&&, char&>, char&&>);
static_assert(std::same_as<mimic_reference_t<int&, char&&>, char&>);

template <class From, class To>
requires (!std::is_reference_v<To>)
struct mimic_cv {
	using type = std::remove_cv_t<To>;
};

template <class From, class To>
requires (!std::is_reference_v<To>)
struct mimic_cv<const From, To> {
	using type = std::add_const_t<std::remove_cv_t<To>>;
};

template <class From, class To>
requires (!std::is_reference_v<To>)
struct mimic_cv<volatile From, To> {
	using type = std::add_volatile_t<std::remove_cv_t<To>>;
};

template <class From, class To>
requires (!std::is_reference_v<To>)
struct mimic_cv<const volatile From, To> {
	using type = std::add_cv_t<std::remove_cv_t<To>>;
};

template <class From, class To>
requires (!std::is_reference_v<To>)
using mimic_cv_t = typename mimic_cv<From, To>::type;

static_assert(std::same_as<mimic_cv_t<int, char>, char>);
static_assert(std::same_as<mimic_cv_t<const int, char>, const char>);
static_assert(std::same_as<mimic_cv_t<volatile int, char>, volatile char>);
static_assert(std::same_as<mimic_cv_t<const volatile int, char>, const volatile char>);

static_assert(std::same_as<mimic_cv_t<int&, char>, char>);
static_assert(std::same_as<mimic_cv_t<const int&, char>, char>);
static_assert(std::same_as<mimic_cv_t<volatile int&, char>, char>);
static_assert(std::same_as<mimic_cv_t<const volatile int&, char>, char>);

template <class From, class To>
struct mimic_cvref {
	using type = mimic_reference_t<From, mimic_cv_t<std::remove_reference_t<From>, std::remove_reference_t<To>>>;
};

template <class From, class To>
using mimic_cvref_t = typename mimic_cvref<From, To>::type;

static_assert(std::same_as<mimic_cvref_t<int, char>, char>);
static_assert(std::same_as<mimic_cvref_t<const int, char>, const char>);
static_assert(std::same_as<mimic_cvref_t<volatile int, char>, volatile char>);
static_assert(std::same_as<mimic_cvref_t<const volatile int, char>, const volatile char>);

static_assert(std::same_as<mimic_cvref_t<int, const volatile char&>, char>);
static_assert(std::same_as<mimic_cvref_t<const int, volatile char&>, const char>);
static_assert(std::same_as<mimic_cvref_t<volatile int, const char&>, volatile char>);
static_assert(std::same_as<mimic_cvref_t<const volatile int, char&>, const volatile char>);

static_assert(std::same_as<mimic_cvref_t<int&, char>, char&>);
static_assert(std::same_as<mimic_cvref_t<const int&, char>, const char&>);
static_assert(std::same_as<mimic_cvref_t<volatile int&, char>, volatile char&>);
static_assert(std::same_as<mimic_cvref_t<const volatile int&, char>, const volatile char&>);

template <class From, class To>
struct common_cv_impl {
	using common_const = std::conditional_t<std::is_const_v<From> || std::is_const_v<To>, std::add_const_t<To>, To>;
	using common_volatile = std::conditional_t<std::is_volatile_v<From> || std::is_volatile_v<To>, std::add_volatile_t<To>, To>;
	using type = std::conditional_t<std::is_volatile_v<From> || std::is_volatile_v<To>, std::add_volatile_t<common_const>, common_const>;
};

template <class From, class To>
requires (!std::is_reference_v<To>)
struct common_cv {
	using type = typename common_cv_impl<From, To>::type;
};

template <class From, class To>
requires (!std::is_reference_v<To>)
using common_cv_t = typename common_cv<From, To>::type;

static_assert(std::same_as<common_cv_t<int, char>, char>);
static_assert(std::same_as<common_cv_t<const int, char>, const char>);
static_assert(std::same_as<common_cv_t<volatile int, char>, volatile char>);
static_assert(std::same_as<common_cv_t<const volatile int, char>, const volatile char>);

template <class From, class To>
struct forward_like_type {
	using type = mimic_cvref_t<From, To>&&;
};

template <class From, class To>
using forward_like_type_t = typename forward_like_type<From, To>::type;

static_assert(std::same_as<forward_like_type_t<int, char>, char&&>);
static_assert(std::same_as<forward_like_type_t<const int, char>, char const&&>);
static_assert(std::same_as<forward_like_type_t<const int&, char>, char const&>);
static_assert(std::same_as<forward_like_type_t<const int&&, char>, char const&&>);

template <class T, class U>
[[msvc::intrinsic]] [[nodiscard]] constexpr auto&& forward_like(U &&x) noexcept {
	return (static_cast<mimic_cvref_t<T, U>&&>(x));
}

template <bool Condition, typename Trait>
struct conditionally_apply {

};

template <template <typename> typename Trait, typename T>
struct conditionally_apply<false, Trait<T>> {
	using type = T;
};

template <template <typename> typename Trait, typename T>
struct conditionally_apply<true, Trait<T>> {
	using type = typename Trait<T>::type;
};

template <bool Condition, typename T>
using conditionally_apply_t = typename conditionally_apply<Condition, T>::type;

template <typename T>
requires (!std::same_as<T, void>)
class observer_ptr {
public:
	using value_type = std::remove_reference_t<T>;
	using reference_type = std::conditional_t<std::is_reference_v<T>, T, std::add_lvalue_reference_t<T>>;
	using pointer_type = std::add_pointer_t<value_type>;

	constexpr observer_ptr() = default;
	constexpr observer_ptr(const observer_ptr&) = default;

	constexpr observer_ptr(pointer_type p) noexcept : ptr{p} {}
	constexpr observer_ptr(const std::unique_ptr<value_type> &p) noexcept : ptr{p.get()} {}
	constexpr explicit observer_ptr(std::add_rvalue_reference<T> obj) noexcept : observer_ptr(&obj) {}

	constexpr observer_ptr& operator=(const observer_ptr&) = default;

	constexpr observer_ptr& operator=(const std::unique_ptr<value_type> &p) noexcept {
		ptr = p.get();
		return (*this);
	}

	constexpr observer_ptr& operator=(pointer_type p) noexcept {
		ptr = p;
		return (*this);
	}

	template <typename U>
	requires (std::same_as<std::remove_cvref_t<T>, std::remove_cvref_t<U>>)
	constexpr friend bool operator==(const observer_ptr& lhs, const observer_ptr<U>& rhs) noexcept {
		return (lhs.get() == rhs.get());
	}

	constexpr explicit operator bool() const noexcept {
		return (ptr);
	}

	constexpr operator pointer_type() const noexcept {
		return (ptr);
	}

	pointer_type get() const noexcept {
		return (ptr);
	}

	reference_type operator*() const noexcept {
		return (static_cast<reference_type>(*ptr));
	}

	pointer_type operator->() const noexcept {
		return (ptr);
	}

private:
	pointer_type ptr = nullptr;
};

/**
 * \brief Generic random access iterator type.
 * \tparam Container The container this iterator is for. In particular, its CV and reference qualifiers.
 */
template <typename Container>
struct random_access_iterator {
	/**
	 * \brief Alias for the container type, without CV and reference qualifiers.
	 */
	using container_type = std::remove_cvref_t<Container>;

	/**
	 * \brief Value type this iterator represents. Preserves CV qualifiers of the container.
	 */
	using value_type = common_cv_t<container_type, typename container_type::value_type>;

	/**
	 * \brief Reference type this iterator represents, through operator*(). Preserves CV and ref qualifiers of the container.
	 */
	using reference_type = std::conditional_t<std::is_rvalue_reference_v<Container>, std::add_rvalue_reference_t<value_type>, std::add_lvalue_reference_t<value_type>>;

	/**
	 * \brief Container linked to this iterator.
	 */
	observer_ptr<Container> container = {};

	/**
	 * \brief Index of this iterator. Represents the element pointed to, using the container's operator[] semantics.
	 */
	size_t idx{0};

	/**
	 * \brief Increment operator. Moves to the next element.
	 *
	 * \return Self after incrementing.
	 */
	constexpr random_access_iterator &operator++() noexcept {
		++idx;
		return (*this);
	}

	/**
	 * \brief Postfix increment operator. Moves to the next element.
	 *
	 * \return New iterator pointing to the element this iterator pointed to before incrementing.
	 */
	constexpr random_access_iterator operator++(int) noexcept {
		size_t prev = idx;

		++idx;
		return {container, prev};
	}

	/**
	 * \brief Decrement operator. Moves to the previous element.
	 *
	 * \return Self after decrementing.
	 */
	constexpr random_access_iterator &operator--() noexcept {
		assert(idx > 0);
		--idx;
		return *this;
	}

	/**
	 * \brief Postfix decrement operator. Moves to the previous element.
	 *
	 * \return New iterator pointing to the element this iterator pointed to before decrementing.
	 */
	constexpr random_access_iterator operator--(int) noexcept {
		assert(idx > 0);
		size_t prev = idx;

		--idx;
		return {container, prev};
	}

	/**
	 * \brief Dereference operator.
	 *
	 * \return Reference to the pointed element, accessed through the container's operator[] function.
	 */
	constexpr auto operator*() const noexcept -> reference_type {
		return (static_cast<reference_type>((*container)[idx]));
	}

	/**
	 * \brief Access operator.
	 *
	 * \return Pointer to the pointed element, accessed through the container's operator[] function.
	 */
	constexpr auto operator->() const noexcept -> std::add_pointer_t<reference_type> {
		return &((*container)[idx]);
	}

	/**
	 * \brief Bool conversion operator. Returns true if this operator is within the bounds of the container.
	 */
	constexpr explicit operator bool() const noexcept
	requires (requires (size_t my_idx, Container c) { { my_idx < std::ranges::end(c).idx } -> std::convertible_to<bool>; } ) {
		return (idx < std::ranges::end(*container).idx);
	}

	/**
	 * \brief Equality comparison operator.
	 *
	 * \tparam C2 Container type of the right operand.
	 * \param a Left operand of the comparison.
	 * \param b Right operand of the comparison.
	 * \warn If the operands refer to different containers, the behavior is undefined.
	 * \return Whether a and b point to the same element.
	 */
	template <typename C2>
	requires (std::same_as<std::remove_cvref_t<Container>, std::remove_cvref_t<C2>>)
	friend constexpr bool operator==(const random_access_iterator &a, const random_access_iterator<C2> &b) noexcept {
		assert(a.container == b.container);
		return (a.idx == b.idx);
	}
};

}

#endif
