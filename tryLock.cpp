#include "TryLock.hpp"
#include "bench.hpp"

int main(int argc, char* argv[]) {
    bench<TryLock>("TryLock", argc, argv);
}
