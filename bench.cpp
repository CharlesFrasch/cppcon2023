#include "Fifo4.hpp"
#include "Fifo4a.hpp"
#include "Fifo5.hpp"
#include "rigtorp.hpp"

#include <benchmark/benchmark.h>

#include <iostream>
#include <thread>


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

constexpr auto cpu1 = 1;
constexpr auto cpu2 = 2;

template<typename T>
struct isRigtorp : std::false_type {};

template<typename ValueT>
struct isRigtorp<rigtorp::SPSCQueue<ValueT>> : std::true_type {};


template<template<typename> class FifoT>
void BM_Fifo(benchmark::State& state) {
    using fifo_type = FifoT<std::int_fast64_t>;
    using value_type = typename fifo_type::value_type;

    constexpr auto fifoSize = 131072;
    fifo_type fifo(fifoSize);

    auto t = std::jthread([&] {
        pinThread(cpu1);
        for (auto i = value_type{};; ++i) {
            value_type val;
            if constexpr(isRigtorp<fifo_type>::value) {
                while (!fifo.front());
                benchmark::DoNotOptimize(val = *fifo.front());
                fifo.pop();
            } else {
                while (not fifo.pop(val)) {
                    ;
                }
                benchmark::DoNotOptimize(val);
            }
            if (val == -1) {
                break;
            }

            if (val != i) {
                throw std::runtime_error("invalid value");
            }
        }
    });

    auto value = value_type{};
    pinThread(cpu2);
    for (auto _ : state) {
        if constexpr(isRigtorp<fifo_type>::value) {
            while (not fifo.try_push(value)) {
                ;
            }
        } else {
            while (not fifo.push(value)) {
                ;
            }
        }
        ++value;

        while (not fifo.empty()) {
          ;
        }
    }
    state.counters["ops/sec"] = benchmark::Counter(double(value), benchmark::Counter::kIsRate);
    state.PauseTiming();
    if constexpr(isRigtorp<fifo_type>::value) {
        while(not fifo.try_push(-1)) {}
    } else {
        fifo.push(-1);
    }
}


BENCHMARK_TEMPLATE(BM_Fifo, Fifo4);
BENCHMARK_TEMPLATE(BM_Fifo, Fifo4a);
BENCHMARK_TEMPLATE(BM_Fifo, Fifo5);
BENCHMARK_TEMPLATE(BM_Fifo, rigtorp::SPSCQueue);

BENCHMARK_MAIN();

