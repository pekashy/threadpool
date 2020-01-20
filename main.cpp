#include "threadpool.hpp"

#include <iostream>
#include <fstream>
#include <algorithm>
#include <chrono>  // for high_resolution_clock

using std::cin;
using std::cout;

template<typename T>
struct arg {
    arg() = default;

    arg(T *arr, int l, int c, int d, std::mutex *_m, std::condition_variable *_cond, bool role, bool *_cond_ready, ThreadPool& _tp)
            : array(arr), low(l),
              cnt(c), dir(d), m(_m),
              cond(_cond),
              blocking(role),
              cond_ready(_cond_ready),
              tp(_tp) {}

    T operator()(T val) {
        *array = val;
    }

    T *array;
    int low;  // low border for merge
    int cnt;  // cnt = k *2, k is number of iterations
    int dir;  // direction
    std::mutex *m;
    std::condition_variable *cond;  // to sync before merge
    bool *cond_ready;  // not to miss the notify
    bool blocking;  // on each stage of we run one part in current part and one in queue
    ThreadPool &tp;
};

void compAndSwap(int a[], int i, int j, int dir) {
    if (dir == (a[i] > a[j]))
        std::swap(a[i], a[j]);
}

void bitonicMerge(int a[], int low, int cnt, int dir) {
    if (cnt > 1) {
        int k = cnt / 2;
        for (int i = low; i < low + k; i++)
            compAndSwap(a, i, i + k, dir);
        bitonicMerge(a, low, k, dir);
        bitonicMerge(a, low + k, k, dir);
    }
}

void bitonicSort(void *args) {
    int cnt = ((arg<int> *) args)->cnt;
    int *a = ((arg<int> *) args)->array;
    int low = ((arg<int> *) args)->low;
    int dir = ((arg<int> *) args)->dir;
    bool blocking = (((arg<int> *) args))->blocking;
    bool *cond_ready = (((arg<int> *) args))->cond_ready;
    std::mutex *m = ((arg<int> *) args)->m;
    std::condition_variable *cnd = ((arg<int> *) args)->cond;
    ThreadPool& tp = ((arg<int> *) args)->tp;

    auto *m1_new = new std::mutex;
    auto *c1 = new std::condition_variable;
    int k = cnt / 2;
    // sort in ascending order since dir here is 1
    bool c_ready = false;
    auto args1 = new arg<int>(a, low, k, 1, m1_new, c1, false, &c_ready, tp);
    auto args2 = new arg<int>(a, low + k, k, 0, m1_new, c1, true, &c_ready, tp);

    if (cnt > 1) {
        if (tp.submit(&bitonicSort, args1)) { // part 1
            std::sort(a + low, a + low + cnt); // sort to the end in case we are out of threads
        } else {
            bitonicSort(args2);  // part 2
        }
        bitonicMerge(a, low, cnt, dir);
        if (blocking) {  // this part called for part 2
            std::unique_lock<std::mutex> lck(*m); // locks mutex
            cnd->wait(lck, [cond_ready]() { return *cond_ready; });  // cond_ready helps not to miss the notification
        }
    }
    if (!blocking) {  // this - is for part 1
        std::unique_lock<std::mutex> lck(*m);
        *cond_ready = true;  // in case part 2 not got to wait block yet
        cnd->notify_one();  // allow part 2 to pass
    } else { // have some sigsegv issues here
        //free(args1);
        //free(args2);
        //free(m1_new);
        //free(c1);
    }

}

int main(int argc, char** argv) {
    int n;
    int nthreads = 8;
    std::ifstream ifile("input");
    ifile >> n;
    int *arr = new int[n];
    for (int i = 0; i < n; i++) {
        ifile >> arr[i];
    }
    ifile.close();
    if(argc > 1) {
        std::istringstream(argv[1]) >> nthreads;
    }
    ThreadPool tp(nthreads);
    std::mutex m;
    std::condition_variable cnd;
    bool cond_ready = false;
    auto args = new arg<int>(arr, 0, n, 1, &m, &cnd, false, &cond_ready, tp);
    auto start = std::chrono::high_resolution_clock::now();
    bitonicSort(args);
    tp.finishWork(); // signaling pool, there will be no more tasks, waiting for current tasks to finish
    auto finish = std::chrono::high_resolution_clock::now();
    std::chrono::duration<double> elapsed = finish - start;
    cout << elapsed.count() << std::endl;
    /*
    std::ofstream ofile("output");
    for (int i = 0; i < n; i++) {
        ofile << arr[i] << " ";
        cout << arr[i] << " ";
    }*/
    //ofile.close();
    //std::cout << std::endl;
    return 0;
}