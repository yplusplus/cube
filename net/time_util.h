#ifndef __CUBE_TIME_UTIL_H__
#define __CUBE_TIME_UTIL_H__

#include <ctime>
#include <stdint.h>

namespace cube {

class TimeUtil {
    public:

        // get current time in second
        static time_t CurrentTime();

        // get current time in ms
        static int64_t CurrentTimeMillis();

};

}

#endif
