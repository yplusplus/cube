#include "time_util.h"

#include <sys/time.h>

namespace cube {

time_t TimeUtil::CurrentTime() {
    return ::time(0);
}

int64_t TimeUtil::CurrentTimeMillis() {
    struct timeval tv;
    gettimeofday(&tv, NULL);
    int64_t res = static_cast<int64_t>(tv.tv_sec) * 1000 + 
        static_cast<int64_t>(tv.tv_usec) / 1000;
    return res;
}

}
