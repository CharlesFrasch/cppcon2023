#include "Fifo2.hpp"
#include "Fifo3.hpp"
#include "Fifo4.hpp"
#include "Fifo4a.hpp"
#include "Fifo4b.hpp"
#include "Fifo5.hpp"
#include "Fifo5a.hpp"
#include "Mutex.hpp"
#include "rigtorp.hpp"
#include <boost/lockfree/spsc_queue.hpp>   // boost 1.74.0

#include "bench.hpp"

#include <iostream>


template<typename ValueT>
struct isRigtorp<rigtorp::SPSCQueue<ValueT>> : std::true_type {};


// Configuring fixed size to match Fifo5's fixed size
template<typename T>
using boost_spsc_queue = boost::lockfree::spsc_queue<T, boost::lockfree::fixed_sized<true>>;


template<typename ValueT>
void once(long iters, int cpu1, int cpu2) {
    std::cout <<
        // Bench<Fifo2<ValueT>>{}(iters, cpu1, cpu2) << "," <<
        // Bench<Mutex<ValueT>>{}(iters, cpu1, cpu2) <<

        Bench<Fifo3<ValueT>>{}(iters, cpu1, cpu2) << "," << std::flush <<
        Bench<Fifo4<ValueT>>{}(iters, cpu1, cpu2) << "," << std::flush <<
        Bench<Fifo4a<ValueT>>{}(iters, cpu1, cpu2) << "," << std::flush <<
        Bench<Fifo4b<ValueT>>{}(iters, cpu1, cpu2) << "," << std::flush <<
        Bench<Fifo5<ValueT>>{}(iters, cpu1, cpu2) << "," << std::flush <<
        Bench<Fifo5a<ValueT>>{}(iters, cpu1, cpu2) << "," << std::flush <<
        Bench<rigtorp::SPSCQueue<ValueT>>{}(iters, cpu1, cpu2) << "," << std::flush <<
        Bench<boost_spsc_queue<ValueT>>{}(iters, cpu1, cpu2) << std::flush <<
        "\n";
}

int main(int argc, char* argv[]) {
    constexpr auto cpu1 = 1;
    constexpr auto cpu2 = 2;
    constexpr auto iters = 400'000'000l;

    auto reps = 10;
    if (argc == 2) {
       reps = std::atoi(argv[1]);
    }

    using value_type = std::int64_t;

    std::cout << "Fifo3,Fifo4,Fifo4a,Fifo4b,Fifo5,Fifo5a,rigtorp,boost_spsc_queue" << std::endl;
    // std::cout << "Fifo2,Mutex\n";
    for (auto rep = 0; rep < reps; ++rep) {
        once<value_type>(iters, cpu1, cpu2);
    }
}
