/*
 * OpenSCAD (https://www.openscad.org)
 * Copyright 2021 Google LLC
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * version 2 as published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU General Public License for more details.
 */

#pragma once

#include <chrono>
#include <future>
#include <functional>

#ifdef DEBUG
#include <iostream>
#include "printutils.h"
#include "stack_trace.h"

// #define DEBUG_BLOCKING_FUTURE_GET_CALLS
#endif



template <typename T>
std::future<T> future_value(const T &value)
{
	return std::async(std::launch::async, [value] { return value; });
}

// TODO(ochafik): Find a better approach than this horribly inefficient code (spawns a thread that
// blocks!)
template <class A, class B>
std::shared_future<B> heavy_then_(const std::shared_future<A>& a, std::function<B(const A&)> callback) {
  return std::shared_future<B>(std::async(
      std::launch::async, [callback, a]() -> B { return callback(a.get()); }));
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

  template <typename R>
  std::shared_future<R> then(std::function<R(const shared_ptr_t&)> f) const {
    if (sp_) {
      return std::shared_future<R>(future_value<R>(f(sp_)));
    } else {
      return heavy_then_<shared_ptr_t, R>(sf_, f);
    }
  }
  template <typename R>
  lazy_ptr<R> then_lazy(std::function<std::shared_ptr<R>(const shared_ptr_t&)> f) const {
    return then(f);
  }

	template <class O>
	typename std::enable_if<std::is_convertible<O *, T *>::value, void>::type reset(
			const std::shared_ptr<O> &x)
	{
		reset(shared_ptr_t(x));
	}

	shared_ptr_t get_shared_ptr() const
	{
#ifdef DEBUG_BLOCKING_FUTURE_GET_CALLS
    auto start = std::chrono::system_clock::now();
#endif
    auto sp = sp_ ? sp_ : sf_.get();;
#ifdef DEBUG_BLOCKING_FUTURE_GET_CALLS
    if (!got_shared_ptr_) {
      got_shared_ptr_ = true;

  		auto end = std::chrono::system_clock::now();
      int elapsed_millis =
          std::chrono::duration_cast<std::chrono::milliseconds>(end - start).count();

      if (elapsed_millis > 0) {
        printStackTrace((std::ostringstream() << "Call on get_shared_ptr blocked for " << elapsed_millis << "ms").str());
      }
    }
#endif
		return sp;
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

template <class B, class A>
lazy_ptr<B> dynamic_pointer_cast(const lazy_ptr<A> &lp)
{
  if (lp.sp_) return lazy_ptr<B>(dynamic_pointer_cast<B>(lp.sp_));

  return heavy_then_<std::shared_ptr<A>, std::shared_ptr<B>>(lp.sf_, [](const std::shared_ptr<A>& p) { return dynamic_pointer_cast<B>(p); });
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
// #ifndef DEBUG
	return std::shared_future<std::shared_ptr<T>>(
			std::async(std::launch::async, [=] {
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
