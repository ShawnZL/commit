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
using std::thread;
using std::max;
using std::min;

class ThreadPool {
    using Task = std::function<void()>;

};

#endif //COMMIT_THREAD_H
