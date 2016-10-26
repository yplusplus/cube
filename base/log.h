#ifndef __CUBE_LOG_H__
#define __CUBE_LOG_H__

namespace cube {

enum LogLevel {
    LOG_LEVEL_DEBUG,
    LOG_LEVEL_TRACE,
    LOG_LEVEL_WARN,
    LOG_LEVEL_ERROR,
    LOG_LEVEL_INFO,
};

void SetLogLevel(LogLevel log_level);

#ifndef NDEBUG

#define LOG(format, args...) \
    do { \
        printf("%s|%d|%s|" format "\n", __FILE__, __LINE__, __FUNCTION__ , ##args); \
    } while(false)

#else

#define LOG(format, args...) 

#endif

#define LOG_DEBUG(format, args...) LOG("DEBUG|" format, ##args)
#define LOG_TRACE(format, args...) LOG("TRACE|" format, ##args)
#define LOG_WARN(format, args...)  LOG("WARN|" format, ##args)
#define LOG_ERROR(format, args...) LOG("ERROR|" format, ##args)
#define LOG_INFO(format, args...)  LOG("INFO|" format, ##args)

}

#endif
