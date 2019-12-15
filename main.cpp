#include "threadpool.hpp"

#include <iostream>
#include <unistd.h>
#include <fstream>


using std::cin;
using std::cout;

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

    arg(T *arr, T *buff, int l, int m, int r) : array(arr), buffer(buff), left(l), middle(m), right(r) {}

    T operator()(T val) {
        *array = val;
        len = 1;
    }

    T *array;
    T *buffer;
    int left;
    int middle;
    int right;
    size_t len;
};

int n;


template<typename T>
void merge(void *args) {

    T *array = ((arg<T> *) args)->array;
    T *buffer = ((arg<T> *) args)->buffer;
    int left = ((arg<T> *) args)->left;
    int middle = ((arg<T> *) args)->middle;
    int right = ((arg<T> *) args)->right;

    int pos_left = 0;
    int pos_right = 0;

    while (left + pos_left < middle && middle + pos_right < right) {
        if (array[left + pos_left] < array[middle + pos_right]) {
            buffer[pos_left + pos_right] = std::move(array[left + pos_left]);
            pos_left++;
        } else {
            buffer[pos_left + pos_right] = std::move(array[middle + pos_right]);
            pos_right++;
        }
    }

    while (left + pos_left < middle) {
        buffer[pos_left + pos_right] = std::move(array[left + pos_left]);
        pos_left++;
    }

    while (middle + pos_right < right) {
        buffer[pos_left + pos_right] = std::move(array[middle + pos_right]);
        pos_right++;
    }

    for (int i = 0; i < pos_left + pos_right; i++)
        array[left + i] = std::move(buffer[i]);
}

template<typename T>
void merge_sort(ThreadPool &tp, std::vector<arg<T> *> &vec_pointer, T *array, T *buffer, int left, int right) {
    if (right - left <= 1) {
        return;
    }

    int middle = left + (right - left) / 2;

    merge_sort(tp, vec_pointer, array, buffer, left, middle);
    merge_sort(tp, vec_pointer, array, buffer, middle, right);
    auto *args = new arg<T>(array, buffer, left, middle, right);
    tp.submit(merge<T>, args);
    vec_pointer.push_back(args);
}


int main() {
    int n;
    std::cin >> n;

    int *arr = new int[n];
    int *buff = new int[n];

    std::ifstream ifile("input");
    ifile >> n;

    for (int i = 0; i < n; i++) {
        ifile >> arr[i];
    }

    ifile.close();


    ThreadPool tp;
    std::vector<arg<int> *> vec_pointer; // keeping track of memory we allocated
    merge_sort(tp, vec_pointer, arr, buff, 0, n);
    tp.finish_work(); // signaling pool, there will be no more tasks, waiting for current tasks to finish
    vec_pointer.clear(); // cleaning argument memory


    std::ofstream ofile("output");
    for (int i = 0; i < n; i++) {
        ofile << arr[i] << " ";
    }
    ofile.close();

    std::cout << std::endl;
    return 0;
}