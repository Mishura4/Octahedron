#ifndef OCTAHEDRON_RAWFILESTREAM_H_
#define OCTAHEDRON_RAWFILESTREAM_H_

#include "file_stream.h"
#include "../base.h"

namespace octahedron
{

template <auto Action = std::nullptr_t{}>
class OnDestroy;

template <auto Action> requires(std::invocable<std::remove_cvref_t<decltype(Action)>>)
class OnDestroy<Action> {
	~OnDestroy() noexcept(noexcept(std::invoke(Action))) {
		std::invoke(Action);
	}
};

template <>
class OnDestroy<std::nullptr_t{}> {
private:
	std::function<void()> _action{nullptr};

public:
	template <typename T> requires(std::invocable<std::remove_cvref_t<T>> &&
																 std::is_nothrow_invocable_v<std::remove_cvref_t<T>>)
	OnDestroy(T &&action) :
		_action(action) { }

	OnDestroy() = default;

	OnDestroy(const OnDestroy &) = delete;

	OnDestroy(OnDestroy &&) = default;

	OnDestroy(nullptr_t) :
		_action{nullptr} {}

	template <typename T> requires(std::invocable<std::remove_cvref_t<T>> &&
																 std::is_nothrow_invocable_v<std::remove_cvref_t<T>>)
	OnDestroy& operator=(T &&action) {
		_action = std::forward<T>(action);
		return (*this);
	}

	OnDestroy& operator=(std::nullptr_t) {
		_action = nullptr;
		return (*this);
	}

	OnDestroy& operator=(const OnDestroy &) = delete;

	OnDestroy& operator=(OnDestroy &&) = default;

	~OnDestroy() noexcept {
		if (_action)
			_action();
	}
};

template <typename T> requires(std::invocable<std::remove_cvref_t<T>>)
OnDestroy(T t) -> OnDestroy<std::nullptr_t{}>;

OnDestroy(std::nullptr_t t) -> OnDestroy<std::nullptr_t{}>;

class raw_file_stream final : public file_stream {
public:
	using io_stream::read;
	using io_stream::write;

	~raw_file_stream() override = default;
	raw_file_stream(const raw_file_stream &) = delete;

	bool                 flush() override;
	bool                 eof() override;
	[[nodiscard]] size_t tell() const override;
	bool                 seek(ssize_t pos, int whence) override;

	size_t read(std::span<std::byte> buf) override;
	size_t write(std::span<const std::byte> buf) override;

	std::optional<std::string> get_next_line(size_t max_size) override;

protected:
	raw_file_stream() = default;
	raw_file_stream(raw_file_stream &&) = default;

private:
	friend std::unique_ptr<file_stream> file_system::open(
		std::string_view    path,
		bit_set<open_flags> mode
	);

	static std::unique_ptr<raw_file_stream> _open(
		const stdfs::path & path,
		bit_set<open_flags> mode
	);

	static constexpr inline auto CLOSE_FN = [](std::FILE *p) noexcept {
		::fclose(p);
	};
	using FileResource = managed_resource<std::FILE*, CLOSE_FN>;

	OnDestroy<>  _onDestroy{nullptr};
	FileResource _file{nullptr};
};

} // namespace octahedron

#endif /* OCTAHEDRON_RAWFILESTREAM_H_ */
