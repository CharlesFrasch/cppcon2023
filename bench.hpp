#pragma once

#include <array>
#include <chrono>
#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <iomanip>
#include <iostream>
#include <stdexcept>
#include <thread>
#include <utility>

#include <pthread.h>


static void pinThread(int cpu) {
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
struct isRigtorp : std::false_type {};

template<typename T>
void bench(char const* name, int cpu1, int cpu2) {
    using value_type = struct alignas(64) : T {};
    using AryValueType = typename value_type::value_type::value_type;

    constexpr auto queueSize = 131072;
    // constexpr auto iters = 200'000'000l;
    constexpr auto iters = 100'000'000l;

    T q(queueSize);
    auto t = std::jthread([&] {
        pinThread(cpu1);
        for (auto i = AryValueType{}; i < iters; ++i) {

            typename value_type::value_type val;
            if constexpr(isRigtorp<T>::value) {
                while (!q.front());
                val = *q.front();
                q.pop();
            } else {
                while (not q.pop(val)) {
                    ;
                }
            }

            if (val[0] != i) {
                throw std::runtime_error("invalid value");
            }
        }
    });

    pinThread(cpu2);
    auto start = std::chrono::steady_clock::now();
    for (auto i = AryValueType{}; i < iters; ++i) {
        if constexpr(isRigtorp<T>::value) {
            while (not q.try_push(value_type{i})) {
                ;
            }

        } else {
            while (not q.push(typename value_type::value_type{i})) {
                ;
            }
        }
    }
    while (not q.empty()) {
      ;
    }
    auto stop = std::chrono::steady_clock::now();
    auto delta = std::chrono::duration_cast<std::chrono::nanoseconds>(stop - start);
    std::cout << name << ": "
        << std::setw(8) << std::right << iters * 1'000'000'000 / delta.count()
        << " ops/s\n";
}

template<template<typename> class FifoT>
void bench(char const* name, int argc, char* argv[]) {
    int cpu1 = -1;
    int cpu2 = -1;
    if (argc == 3) {
       cpu1 = std::atoi(argv[1]);
       cpu2 = std::atoi(argv[2]);
    }
    using value_type = std::array<long, 25>;
    bench<FifoT<value_type>>(name, cpu1, cpu2);
}
