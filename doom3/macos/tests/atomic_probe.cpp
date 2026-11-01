#include <thread>
#include <vector>
#include <cstdio>
#define ID_INLINE inline
#include "splashdamage/idlib/threading/Atomic.h"

int main() {
    volatile int count = 0;
    if (sdAtomic::CompareAndSwap(&count, 1, 10) || count != 0)
        return 1;
    sdAtomicValue<int> counter(0);
    std::vector<std::thread> workers;
    for (int i = 0; i < 4; ++i) {
        workers.emplace_back([&]() {
            for (int j = 0; j < 100000; ++j) {
                int old;
                do {
                    old = __atomic_load_n(&count, __ATOMIC_SEQ_CST);
                } while (!sdAtomic::CompareAndSwap(&count, old, old + 1));
                counter++;
            }
        });
    }
    for (auto &worker : workers)
        worker.join();
    std::printf("CAS counter: %d; atomic value: %d; expected: 400000\n", count, counter.Load());
    return count == 400000 && counter.Load() == 400000 ? 0 : 2;
}
