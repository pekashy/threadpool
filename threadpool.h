#pragma once
#include "thread_safe_queue.hpp"
#include "thread_joiner.hpp"

#include <zconf.h>
#include <vector>
#include <unistd.h>
#include <functional>
#include <atomic>
#include <thread>

class ThreadPool {
public:
    ThreadPool();
    template<typename FunctionType>
    void submit(FunctionType f);

private:
    std::atomic_bool done;
    ThreadSafeQueue<std::function<void()> > work_queue;
    std::vector<std::thread> threads;
    void worker_thread();
    join_threads joiner;
};