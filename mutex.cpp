#include "Mutex.hpp"
#include "bench.hpp"

int main(int argc, char* argv[]) {
    bench<Mutex>("Mutex", argc, argv);
}
