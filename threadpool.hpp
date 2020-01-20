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
    ThreadPool(int tc = (int) std::thread::hardware_concurrency()) : done(false), soft_finish(false), joiner(threads) {
        thread_count = tc;
        num_busy = 0;

        try {
            for (unsigned i = 0; i < thread_count; ++i) {
                threads.emplace_back(std::thread(&ThreadPool::workerThread, this));
            }
        }
        catch (...) {
            done = true;
            throw;
        }
    }

    int submit(function_pointer f, void *args) {
        std::unique_lock<std::mutex> lck(subm_mut);

        if (num_busy < thread_count) { // here we need a mutex
            num_busy++;
            work_queue.push(std::make_pair(f, args));
            return 0;
        }
        return 1;
    }

    int getNumBusy() {
        return num_busy;
    }

    void abortWork() { // exit as soon as possible
        done = true;
        std::unique_lock<std::mutex> lck(mut);
        finished.wait(lck);
    }

    void finishWork() { // exit as all work finishes
        soft_finish = true;
        std::unique_lock<std::mutex> lck(mut);
        finished.wait(lck, [this]() { return this->ready; });
    }

    ~ThreadPool() = default;


private:
    std::atomic_bool done; // flag not to wait or take any more tasks
    std::atomic_bool soft_finish; // flag that there will be no more new threads
    std::atomic_int num_busy; // keeping track of threads still in work
    bool ready = false;

    ThreadSafeQueue<std::pair<function_pointer, void *>> work_queue;
    std::vector<std::thread> threads;
    std::condition_variable finished;
    std::mutex mut;
    unsigned thread_count;

    std::mutex subm_mut;

    void workerThread() {
        while (!done) {
            std::pair<function_pointer, void *> task;
            int a = 0;
            if (work_queue.try_pop(task)) {
                task.first(task.second);
                num_busy--;
            } else {
                if (soft_finish) {
                    done = true;
                } else {
                    std::this_thread::yield();
                }
            }
        }
        if (done && !num_busy) {
            std::unique_lock<std::mutex> lck(mut);
            ready = true;
            finished.notify_all();
        }
    }

    join_threads joiner;
};