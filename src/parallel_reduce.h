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

#include <deque>
#include <future>
#include <mutex>
#include <vector>

template <class T>
class parallel_reduction
{
public:
	std::mutex mutex;

	std::deque<T> values;
	size_t value_count;
	size_t task_count;
	size_t max_task_count;
	std::promise<T> promise;
	std::function<T(T, T)> f;
	std::vector<std::shared_future<void>> futures;
};

template <class T>
void parallel_reduction_schedule(const std::shared_ptr<parallel_reduction<T>> &reduction)
{
	while (reduction->value_count > 1 && reduction->task_count < reduction->max_task_count) {
		auto a = reduction->values.front();
		reduction->values.pop_front();

    // // Alternate front and back in case the values are sorted by complexity, to balance the leaves?
		// auto b = reduction->values.back();
		// reduction->values.pop_back();

		auto b = reduction->values.front();
		reduction->values.pop_front();

		reduction->value_count -= 2;
		reduction->task_count++;

		// Store the returned future to avoid weird hangs:
		// https://stackoverflow.com/questions/36920579/how-to-use-stdasync-to-call-a-function-in-a-mutex-protected-loop
		reduction->futures.emplace_back(
				std::shared_future<void>(std::async(std::launch::async, [reduction, a, b]() -> void {
					auto value = reduction->f(a, b);

					std::lock_guard<std::mutex> lock(reduction->mutex);
					reduction->task_count--;
					reduction->values.push_back(value);
					reduction->value_count++;
					parallel_reduction_schedule(reduction);
				})));
	}

	if (reduction->task_count == 0 && reduction->value_count == 1) {
		reduction->promise.set_value(reduction->values[0]);
	}
}

/*! Will reduce the items, using as many threads as the architecture has cores, using intermediate
 * results as soon as they are available to keep the processor as busy as possible. */
template <class T>
std::future<T> parallel_reduce(const std::vector<T> &items, std::function<T(T, T)> f)
{
	auto reduction = std::make_shared<parallel_reduction<T>>();
	reduction->values.insert(reduction->values.end(), items.begin(), items.end());
	reduction->value_count = items.size();
	reduction->max_task_count = std::thread::hardware_concurrency();
	reduction->f = f;

	{
		std::lock_guard<std::mutex> lock(reduction->mutex);
		parallel_reduction_schedule(reduction);
	}

	return reduction->promise.get_future();
}
