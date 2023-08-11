//
// Created by Shawn Zhao on 2023/8/10.
//

#ifndef COMMIT_THREAD_H
#define COMMIT_THREAD_H
#include <vector>
#include <queue>
#include <thread>
#include <atomic>
#include <condition_variable>
#include <future>
#include <functional>
#include <stdexcept>
#include <list>
using std::thread;
using std::max;
using std::min;

class ThreadPool {
    using Task = std::function<void()>;
};

template <typename T>
std::list<T> sequential_quick_sort(std::list<T> &input) {
    if (input.empty()) {
        return input;
    }
    std::list<T> result;
    result.splice(result.begin(), input, input.begin());
    T const& pivot = *result.begin(); // 常量引用
    auto divide_point = std::partition(input.begin(), input.end(),
                                       [&](T const& t) { t < pivot; });
    std::list<T> lower_part;
    lower_part.splice(lower_part.end(), input, input.begin(), divide_point);
    std::future<std::list<T>> new_lower(
            std::async(sequential_quick_sort<T>, std::move(lower_part)));
    auto new_higher(sequential_quick_sort(std::move(input)));
    result.splice(result.begin(), new_lower.get());
    result.splice(result.end(), new_higher);
    return result;
}

#endif //COMMIT_THREAD_H
