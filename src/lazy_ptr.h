#pragma once

#include <chrono>
#include <future>
#include <functional>

#ifdef DEBUG
#include <iostream>
#include "printutils.h"
#include "stack_trace.h"
#endif

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
	// lazy_ptr(const lazy_ptr &x)
	// {
	// 	sp_ = x.sp_;
	// 	sf_ = x.sf_;
	// }
	lazy_ptr(const shared_ptr_t &x) { reset(x); }
	lazy_ptr(T *x) { reset(x); }

	void reset(const shared_future_t &x) { sf_ = x; }
	void reset(const shared_ptr_t &x) {
    sp_ = x;
    sf_ = future_value(x);
  }
	void reset(T *x = nullptr) { reset(shared_ptr_t(x)); }

	template <class O>
	typename std::enable_if<std::is_convertible<O *, T *>::value, void>::type reset(
			const std::shared_ptr<O> &x)
	{
		reset(shared_ptr_t(x));
	}

	shared_ptr_t get_shared_ptr() const
	{
		// #ifdef DEBUG
		// 		if (!got_shared_ptr_) {
		// 			printStackTrace("Blocking call on get_shared_ptr");
		// 			got_shared_ptr_ = true;
		// 		}
		// #endif
		return sp_ ? sp_ : sf_.get();
	}
	T *get() const { return get_shared_ptr().get(); }

	template <class O>
	typename std::enable_if<std::is_convertible<O *, T *>::value, lazy_ptr &>::type operator=(
			const lazy_ptr<O> &x)
	{
		sf_ =
				std::async(std::launch::async, [x]() -> std::shared_ptr<T> { return x.get_shared_ptr(); });
		sp_ = x.sp_;
		return *this;
	}

	template <class O>
	typename std::enable_if<std::is_convertible<O *, T *>::value, void>::type reset(
			const lazy_ptr<O> &x)
	{
    *this = x;
	}

	template <class O>
	typename std::enable_if<std::is_convertible<O *, T *>::value, lazy_ptr &>::type operator=(
			const std::shared_ptr<O> &x)
	{
		reset(x);
		return *this;
	}

	operator bool() const { return get_shared_ptr().get(); }
	operator shared_ptr_t() const { return get_shared_ptr(); }
	T &operator*() const { return *get_shared_ptr(); }
	T *operator->() const { return get_shared_ptr().get(); }

// private:
	shared_ptr_t sp_;
	shared_future_t sf_;

#ifdef DEBUG
	mutable bool got_shared_ptr_;
#endif
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
lazy_ptr<T> lazy_ptr_op(const std::function<T *()> &f, const std::string &description)
{
// #ifndef DEBUG
	return std::shared_future<std::shared_ptr<T>>(
			std::async(std::launch::async, [=] {
#ifdef DEBUG
        LOG(message_group::Echo, Location::NONE, "", "Async: %1$s", description);
#endif
        return std::shared_ptr<T>(f());
      }));
// #else
// 	auto scheduled = std::chrono::system_clock::now();
// 	return std::shared_future<std::shared_ptr<T>>(std::async(std::launch::async, [f, description,
// 																																								scheduled] {
// 		auto start = std::chrono::system_clock::now();
// 		int start_delay_millis =
// 				std::chrono::duration_cast<std::chrono::milliseconds>(start - scheduled).count();

// 		LOG(message_group::Echo, Location::NONE, "", "[Async: %1$s]: Started after %2$dms", description,
// 				start_delay_millis);
// 		std::cerr << "[Async: " << description << "]: Started after " << start_delay_millis << "ms\n";

// 		T *result = f();
// 		auto end = std::chrono::system_clock::now();

// 		int execution_millis =
// 				std::chrono::duration_cast<std::chrono::milliseconds>(end - start).count();

// 		LOG(message_group::Echo, Location::NONE, "", "[Async: %1$s]: Execution took %2$dms",
// 				description, execution_millis);
// 		std::cerr << "[Async: " << description << "]: Execution took " << execution_millis << "ms\n";

// 		return std::shared_ptr<T>(result);
// 	}));
// #endif
}
