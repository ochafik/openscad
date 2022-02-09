#pragma once

#include <deque>
#include <future>
#include <mutex>
#include <vector>

template <typename T, typename Comparator>
class parallel_reduction
{
public:
  parallel_reduction(const std::vector<T>& items) :
    // Build the queue in linear time (don't add items one by one!).
    values(items.begin(), items.end()),
    pending_task_count(0),
    max_task_count(0) {}

  std::mutex mutex;
  std::priority_queue<T, std::vector<T>, Comparator> values;

  size_t pending_task_count;
  size_t max_task_count;

  std::promise<T> promise;
  std::function<T(T, T)> f;
  std::vector<std::shared_future<void>> futures_retained_to_keep_them_from_hanging;
};

template <typename T, typename Comparator>
void parallel_reduction_schedule(const std::shared_ptr<parallel_reduction<T, Comparator>> &reduction)
{
  LOG(message_group::None, Location::NONE, "", "schedule called(values: %1$d, tasks: %2$d)", reduction->values.size(), reduction->pending_task_count);

  while (reduction->values.size() > 1 && reduction->pending_task_count < reduction->max_task_count) {
    auto a = reduction->values.top();
    reduction->values.pop();

    auto b = reduction->values.top();
    reduction->values.pop();

    assert(Comparator()(a, b));

    reduction->pending_task_count++;

    // Store the returned future to avoid weird hangs:
    // https://stackoverflow.com/questions/36920579/how-to-use-stdasync-to-call-a-function-in-a-mutex-protected-loop
    reduction->futures_retained_to_keep_them_from_hanging.emplace_back(
        std::shared_future<void>(std::async(std::launch::async, [reduction, a, b]() -> void {
          auto value = reduction->f(a, b);

          std::lock_guard<std::mutex> lock(reduction->mutex);

          reduction->pending_task_count--;
          reduction->values.push(value);
          parallel_reduction_schedule(reduction);
        })));
  }

  if (reduction->pending_task_count == 0 && reduction->values.size() == 1) {
    reduction->promise.set_value(reduction->values.top());
  }
}

/*! Will reduce the items, using as many threads as the architecture has cores, using intermediate
 * results as soon as they are available to keep the processor as busy as possible. */
template <typename T, typename Comparator>
//std::future<T> parallel_reduce(const std::vector<T> &items, const std::function<T(T, T)> &f)
T parallel_reduce(const std::vector<T> &items, const std::function<T(T, T)> &f)
{
  LOG(message_group::None, Location::NONE, "", "parallel_reduce(%1$d items)", items.size());
  auto reduction = std::make_shared<parallel_reduction<T, Comparator>>(items);
  reduction->max_task_count = std::thread::hardware_concurrency();
  reduction->f = f;

  {
    std::lock_guard<std::mutex> lock(reduction->mutex);
    parallel_reduction_schedule(reduction);
  }

  return reduction->promise.get_future().get();
}
