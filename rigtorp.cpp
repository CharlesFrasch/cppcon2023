#include "bench.hpp"
#include "rigtorp.hpp"


template<typename ValueT>
struct isRigtorp<rigtorp::SPSCQueue<ValueT>> : std::true_type {};

int main(int argc, char* argv[]) {
    bench<rigtorp::SPSCQueue>("rigtorp", argc, argv);
}
