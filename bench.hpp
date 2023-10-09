#pragma once

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

template<typename T>
inline __attribute__((always_inline)) void doNotOptimize(T const& value) {
    asm volatile("" : : "r,m" (value) : "memory");
}


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
class Bench
{
public:
    using value_type = typename T::value_type;

    static constexpr auto fifoSize = 131072;

    auto operator()(long iters, int cpu1, int cpu2) {
        using namespace std::chrono_literals;

        auto t = std::jthread([&] {
            pinThread(cpu1);
            // pop warmup
            for (auto i = value_type{}; i < fifoSize; ++i) {
                pop(i);
            }

            // pop benchmark run
            for (auto i = value_type{}; i < iters; ++i) {
                pop(i);
            }
        });

        pinThread(cpu2);
        // push warmup
        for (auto i = value_type{}; i < fifoSize; ++i) {
            push(i);
        }
        waitForEmpty();

        // push benchmark run
        auto start = std::chrono::steady_clock::now();
        for (auto i = value_type{}; i < iters; ++i) {
            push(i);
        }
        waitForEmpty();
        auto stop = std::chrono::steady_clock::now();

        auto delta = stop - start;
        return (iters * 1s)/delta;
    }

private:
    void pop(value_type expected) {
        value_type val;
        if constexpr(isRigtorp<T>::value) {
            while (auto again = not q.front()) {
                doNotOptimize(again);
	    }
            val = *q.front();
            q.pop();
        } else {
            while (auto again = not q.pop(val)) {
                doNotOptimize(again);
	    }
        }
        if (val != expected) {
            throw std::runtime_error("invalid value");
        }
    }

    void push(value_type i) {
        if constexpr(isRigtorp<T>::value) {
            while (auto again = not q.try_push(i)) {
                doNotOptimize(again);
            }
        } else {
            while (auto again = not q.push(i)) {
                doNotOptimize(again);
            }
        }
    }

    void waitForEmpty() {
        while (auto again = not q.empty()) {
            doNotOptimize(again);
        }
    };

private:
    T q{fifoSize};
};


// "legacy" API
template<typename T>
auto bench(char const* name, long iters, int cpu1, int cpu2) {
    return Bench<T>{}(iters, cpu1, cpu2);
}


template<template<typename> class FifoT>
void bench(char const* name, int argc, char* argv[]) {
    int cpu1 = 1;
    int cpu2 = 2;
    if (argc == 3) {
       cpu1 = std::atoi(argv[1]);
       cpu2 = std::atoi(argv[2]);
    }

    constexpr auto iters = 400'000'000l;
    // constexpr auto iters = 100'000'000l;

    using value_type = std::int64_t;

    auto opsPerSec = bench<FifoT<value_type>>(name, iters, cpu1, cpu2);
    std::cout << std::setw(7) << std::left << name << ": "
        << std::setw(10) << std::right << opsPerSec << " ops/s\n";
}
