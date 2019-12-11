#include "threadpool.h"

#include <iostream>
#include <unistd.h>
#include <zconf.h>

int main() {
    int numCPU = sysconf(_SC_NPROCESSORS_ONLN);
    std::cout << "Hello, World!" << std::endl;
    return 0;
}