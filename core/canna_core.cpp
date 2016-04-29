#include <sys/time.h>
#include "canna_core.h"

uint64_t canna_gettime()
{
    uint64_t t = 0;

    struct timeval tv;
    gettimeofday(&tv, nullptr);
    t = (uint64_t)tv.tv_sec * 1000;
    t += tv.tv_usec / 1000;

    return t;
}
