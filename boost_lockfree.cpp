#include "bench.hpp"
#include <boost/lockfree/spsc_queue.hpp>   // boost 1.74.0

// Configuring fixed size to match Fifo5's fixed size
template<typename T>
using boost_spsc_queue = boost::lockfree::spsc_queue<T, boost::lockfree::fixed_sized<true>>;

int main(int argc, char* argv[]) {
    bench<boost_spsc_queue>("boost::spsc_queue fixed", argc, argv);
}
