#pragma once

// #include <atomic>
#include <future>
#include <functional>

template <typename T>
std::future<T> future_value(const T &value)
{
	return std::async(std::launch::async, [value] { return value; });
}

// Future of a shared pointer. Behaves like a shared_ptr to a lazy value.
// Designed to be a drop-in replacement for shared_ptr.
template <typename T>
class lazy_ptr
{
public:
	using shared_ptr_t = std::shared_ptr<T>;
	using shared_future_t = std::shared_future<shared_ptr_t>;

	lazy_ptr() { reset(shared_ptr_t()); }
	lazy_ptr(const shared_future_t &x) { reset(x); }
	lazy_ptr(const shared_ptr_t &x) { reset(x); }
	lazy_ptr(T *x) { reset(x); }

	void reset(const shared_future_t &x)
	{
		sf_ = x;

    // DO NOT SUBMIT!!
		got_shared_ptr_ = true;
    sp_ = x.get();
	}
	void reset(const shared_ptr_t &x) { reset(future_value(x)); }
	void reset(T *x = nullptr) { reset(shared_ptr_t(x)); }

	template <class O>
	typename std::enable_if<std::is_convertible<O *, T *>::value, lazy_ptr &>::type reset(
			const std::shared_ptr<O> &x)
	{
		sf_ = future_value(x);
	}

	// shared_future_t future() const { return sf_; }
	shared_ptr_t get_shared_ptr() const
	{
		if (!got_shared_ptr_) {
			auto sp = sf_.get();
			if (!got_shared_ptr_) {
				got_shared_ptr_ = true;
				sp_ = sp;
			}
		}
		return sp_;
		// return sf_.get();
	}
	T *get() const { return get_shared_ptr().get(); }

	template <class O>
	typename std::enable_if<std::is_convertible<O *, T *>::value, lazy_ptr &>::type operator=(
			const lazy_ptr<O> &x)
	{
		sf_ =
				std::async(std::launch::async, [x]() -> std::shared_ptr<T> { return x.get_shared_ptr(); });
		return *this;
	}

	template <class O>
	typename std::enable_if<std::is_convertible<O *, T *>::value, lazy_ptr &>::type operator=(
			const std::shared_ptr<O> &x)
	{
		sf_ = future_value(shared_ptr_t(x));
		return *this;
	}

	operator bool() const { return get_shared_ptr().get(); }
	operator shared_ptr_t() const { return get_shared_ptr(); }
	operator T *() const { return get_shared_ptr().get(); }
	T &operator*() const { return *get_shared_ptr(); }
	T *operator->() const { return get_shared_ptr().get(); }

private:
	shared_future_t sf_;
	mutable bool got_shared_ptr_;
	mutable shared_ptr_t sp_;
};

// TODO(ochafik): Find a better approach than this horribly inefficient code (spawns a thread that
// blocks!)
template <class B, class A>
lazy_ptr<B> dynamic_pointer_cast(const lazy_ptr<A> &fp)
{
	return std::shared_future<std::shared_ptr<B>>(std::async(
			std::launch::async, [fp]() { return dynamic_pointer_cast<B>(fp.get_shared_ptr()); }));
	// return fp.future().then([](std::shared_ptr<A> p) { return dynamic_pointer_cast<B>(p); });
}

// TODO(ochafik): Find a better approach than this horribly inefficient code (spawns a thread that
// blocks!)
template <class B, class A>
lazy_ptr<B> static_pointer_cast(const lazy_ptr<A> &fp)
{
	return std::shared_future<std::shared_ptr<B>>(std::async(
			std::launch::async, [fp]() { return static_pointer_cast<B>(fp.get_shared_ptr()); }));
	// return fp.future().then([](std::shared_ptr<A> p) { return static_pointer_cast<B>(p); });
}

template <typename T>
lazy_ptr<T> lazy_ptr_op(const std::function<T *()> &f)
{
	return std::shared_future<std::shared_ptr<T>>(
			std::async(std::launch::async, [f] { return std::shared_ptr<T>(f()); }));
}
