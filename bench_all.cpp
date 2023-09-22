#include "Fifo3.hpp"
#include "Fifo4.hpp"
#include "Fifo4a.hpp"
#include "Fifo5.hpp"
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
        bench<Fifo3<ValueT>>("Fifo3", iters, cpu1, cpu2) << "," <<
        bench<Fifo4<ValueT>>("Fifo4", iters, cpu1, cpu2) << "," <<
        bench<Fifo4a<ValueT>>("Fifo4a", iters, cpu1, cpu2) << "," <<
        bench<Fifo5<ValueT>>("Fifo5", iters, cpu1, cpu2) << "," <<
        bench<rigtorp::SPSCQueue<ValueT>>("rigtorp", iters, cpu1, cpu2) << "," <<
        bench<boost_spsc_queue<ValueT>>("boost::spsc_queue fixed", iters, cpu1, cpu2) <<
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

    std::cout << "Fifo3,Fifo4,Fifo4a,Fifo5,rigtorp,boost_spsc_queue\n";
    for (auto rep = 0; rep < reps; ++rep) {
        once<value_type>(iters, cpu1, cpu2);
    }
}
