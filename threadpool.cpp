#include "threadpool.h"

ThreadPool::ThreadPool() : done(false), joiner(threads) {
    unsigned const thread_count = std::thread::hardware_concurrency();

    try {
        for (unsigned i = 0; i < thread_count; ++i) {
            threads.emplace_back(std::thread(&ThreadPool::worker_thread,this));
        }
    }
    catch (...) {
        done = true;
        throw;
    }
}

template<typename FunctionType>
void ThreadPool::submit(FunctionType f) {
    work_queue.push(std::function<void()>(f));
}

void ThreadPool::worker_thread() {
    while (!done) {
        std::function<void()> task;
        int a = 0;
        //work_queue.try_pop(a);
        if (work_queue.try_pop(task)) {
            task();
        } else {
            std::this_thread::yield();
        }
    }
}
