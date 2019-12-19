#pragma once

#include "thread_safe_queue.hpp"
#include "thread_joiner.hpp"

#include <zconf.h>
#include <vector>
#include <unistd.h>
#include <functional>
#include <atomic>
#include <thread>
#include <condition_variable>
#include <iostream>
#include <sstream>

typedef void (*function_pointer)(void *);

class ThreadPool {
public:
    ThreadPool() : done(false), soft_finish(false), joiner(threads) {
        unsigned const thread_count = 64;
        num_undone = thread_count;
        try {
            for (unsigned i = 0; i < thread_count; ++i) {
                threads.emplace_back(std::thread(&ThreadPool::worker_thread, this));
            }
        }
        catch (...) {
            done = true;
            throw;
        }
    }

    void submit(function_pointer f, void *args) {
        work_queue.push(std::make_pair(f, args));
    }

    void abort_work() { // exit as soon as possible
        done = true;
        std::unique_lock<std::mutex> lck(mut);
        finished.wait(lck);
    }

    void finish_work() { // exit as all work finishes
        soft_finish = true;
        std::unique_lock<std::mutex> lck(mut);
        finished.wait(lck, [this]() { return this->ready; });
    }

private:
    std::atomic_bool done; // flag not to wait or take any more tasks
    std::atomic_bool soft_finish; // flag that there will be no more new threads
    std::atomic_int num_undone; // keeping track of threads still in work
    bool ready = false;

    ThreadSafeQueue<std::pair<function_pointer, void *>> work_queue;
    std::vector<std::thread> threads;
    std::condition_variable finished;
    std::mutex mut;

    void worker_thread() {
        while (!done) {
            std::pair<function_pointer, void *> task;
            int a = 0;
            if (work_queue.try_pop(task)) {
                task.first(task.second);
            } else {
                if (soft_finish) {
                    done = true;
                } else {
                    std::this_thread::yield();
                }
            }
        }
        num_undone--;
        if (done && !num_undone) {
            std::unique_lock<std::mutex> lck(mut);
            ready = true;
            finished.notify_all();
        }
    }

    join_threads joiner;
};