#include "bres+client_ext.h"

namespace bres {
    // global instances
    bool quit = false;
}; // namespace bres
//
void
bres::setAffinity(
    int cpu
)
{
    #ifndef __OSX__
    cpu_set_t cpumask;
    CPU_ZERO(&cpumask);
    CPU_SET(cpu, &cpumask);
    if (cpu >= 0) {
        if (pthread_setaffinity_np(pthread_self(), sizeof(cpumask), &cpumask) != 0) {
            PANIC("failed setting thread affinity, errorno: %d", errno);
        }
    }
    #endif
}
