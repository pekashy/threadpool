#include "threadpool.hpp"

#include <iostream>
#include <unistd.h>
#include <fstream>


using std::cin;
using std::cout;

ThreadPool tp;


template<typename T>
struct arg {
    arg() = default;

    arg(T val) {
        *array = val;
        len = 1;
    }

    arg(T *vals) {
        len = sizeof(vals);
        array = vals;
    }

    arg(T *arr, int l, int c, int d, std::mutex *_m, std::condition_variable *_cond, bool role, bool *_cond_ready)
            : array(arr), low(l),
              cnt(c), dir(d), m(_m),
              cond(_cond),
              blocking(role),
              cond_ready(_cond_ready) {}

    T operator()(T val) {
        *array = val;
        len = 1;
    }

    T *array;
    int low;
    int cnt;
    int dir;
    std::mutex *m;
    std::condition_variable *cond;
    bool *cond_ready; // not to miss the notify
    bool blocking;
    size_t len;
};

int n;


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
    std::stringstream ss;
    ss << std::this_thread::get_id();
    uint64_t id = std::stoull(ss.str());

    int cnt = ((arg<int> *) args)->cnt;
    int *a = ((arg<int> *) args)->array;
    int low = ((arg<int> *) args)->low;
    int dir = ((arg<int> *) args)->dir;
    bool blocking = (((arg<int> *) args))->blocking;
    bool *cond_ready = (((arg<int> *) args))->cond_ready;
    std::mutex *m = ((arg<int> *) args)->m;
    std::condition_variable *cnd = ((arg<int> *) args)->cond;
    //printf("%d Entered %lu %d\n", blocking, id, cnt);

    std::mutex *m1_new = new std::mutex;
    std::condition_variable *c1 = new std::condition_variable;
    int k = cnt / 2;
    // sort in ascending order since dir here is 1
    bool c_ready = false;
    auto args1 = new arg<int>(a, low, k, 1, m1_new, c1, false, &c_ready);
    auto args2 = new arg<int>(a, low + k, k, 0, m1_new, c1, true, &c_ready);

    if (cnt > 1) {
        tp.submit(&bitonicSort, args1);
        bitonicSort(args2);
        bitonicMerge(a, low, cnt, dir);

        if (blocking) {
            std::unique_lock<std::mutex> lck(*m);
            //printf("%d Waiting %lu %d\n", blocking, id, cnt);
            cnd->wait(lck, [cond_ready]() { return *cond_ready; });
            fflush(stdout);
            //printf("%d Passed %lu %d\n", blocking, id, cnt);
            fflush(stdout);
        }
    }
    if (!blocking) {
        std::unique_lock<std::mutex> lck(*m);
        *cond_ready = true;
        cnd->notify_one();
        fflush(stdout);
        //printf("%d Notified %lu %d\n", blocking, id, cnt);
        fflush(stdout);
    }
    else {
        /*free(args1);
        free(args2);
        free(m1_new);
        free(c1);*/
    }
    //printf("%d Finished %lu %d\n", blocking, id, cnt);
    /*free(args1);
    free(args2);
    free(m1_new);
    free(c1);*/

    //cnd->notify_one();
    //sleep(1);
}

int main() {
    int n;

    std::ifstream ifile("input");
    ifile >> n;
    int *arr = new int[n];
    for (int i = 0; i < n; i++) {
        ifile >> arr[i];
    }
    ifile.close();

    //std::vector<a::argint> *> vec_pointer; // keeping track of memory we allocated
    std::mutex m;
    std::condition_variable cnd;
    bool cond_ready = false;
    auto args = new arg<int>(arr, 0, n, 1, &m, &cnd, false, &cond_ready);
    bitonicSort(args);
    //merge_sort(tp, vec_pointer, arr, buff, 0, n);
    //printf("waiting for pool\n");
    tp.finish_work(); // signaling pool, there will be no more tasks, waiting for current tasks to finish
    //vec_pointer.clear(); // cleaning argument memory


    std::ofstream ofile("output");
    for (int i = 0; i < n; i++) {
        ofile << arr[i] << " ";
        cout << arr[i] << " ";
    }

    ofile.close();

    std::cout << std::endl;
    return 0;
}