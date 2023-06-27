#include "Fifo1.hpp"

#include <array>
#include <chrono>
#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <iostream>
#include <stdexcept>
#include <thread>

#include <pthread.h>


void pinThread(int cpu) {
    if (cpu < 0) {
        return;
    }
    ::cpu_set_t cpuset;
    CPU_ZERO(&cpuset);
    CPU_SET(cpu, &cpuset);
    if (::pthread_setaffinity_np(pthread_self(), sizeof(cpu_set_t), &cpuset) == -1) {
        std::perror("pthread_setaffinity_rp");
        std::exit(EXIT_FAILURE);
    }
}

template<typename T>
void bench(char const* name, int cpu1, int cpu2) {
    using ValueType = typename T::ValueType;
    using AryValueType = typename ValueType::value_type;

    constexpr auto queueSize = 131072;
    constexpr auto iters = 1'000'000'000l;

    T q(queueSize);
    auto t = std::thread([&] {
        pinThread(cpu1);
        for (auto i = AryValueType{}; i < iters; ++i) {
            ValueType val;
            while (not q.pop(val)) {
                ;
            }
            if (val[0] != i) {
                std::cerr << "#### throw " << val[0] << " != " << i <<"\n";
                throw std::runtime_error("foo");
            }
        }
    });

    pinThread(cpu2);
    auto start = std::chrono::steady_clock::now();
    for (auto i = AryValueType{}; i < iters; ++i) {
        while (not q.push(ValueType{i})) {
          ;
        }
    }
    while (not q.empty()) {
      ;
    }
    auto stop = std::chrono::steady_clock::now();
    t.join();
    std::cout << name << " " << iters * 1'000'000'000 /
        std::chrono::duration_cast<std::chrono::nanoseconds>(stop - start).count()
        << " ops/s\n";
}

int main(int argc, char* argv[]) {
    int cpu1 = -1;
    int cpu2 = -1;
    if (argc == 3) {
       cpu1 = std::atoi(argv[1]);
       cpu2 = std::atoi(argv[2]);
    }

    using TestType = std::array<long, 25>;
    bench<Fifo1<TestType>>("Fifo1", cpu1, cpu2);
}




