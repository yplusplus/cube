#include <stdio.h>

#include "logging.h"

namespace cube {

namespace logging {

#define M_GENERATE_LOG_FUNCTION(log_level) \
    void Logger::log_level(const char *format, ...) { \
        if (LogLevel_##log_level >= LoggerLevel()) { \
            va_list args; \
            va_start(args, format); \
            Output(format, args); \
            va_end(args); \
        } \
    }

M_GENERATE_LOG_FUNCTION(Trace);
M_GENERATE_LOG_FUNCTION(Debug);
M_GENERATE_LOG_FUNCTION(Warn);
M_GENERATE_LOG_FUNCTION(Info);
M_GENERATE_LOG_FUNCTION(Error);

#undef M_GENERATE_LOG_FUNCTION

void Logger::Log(const char *format, ...) {
    va_list args;
    va_start(args, format);
    Output(format, args);
    va_end(args);
}

void StdoutLogger::Output(const char *format, va_list args) {
    vprintf(format, args);
}

#ifdef NDEBUG
LoggerPtr g_logger(new EmptyLogger);
#else
LoggerPtr g_logger(new StdoutLogger);
#endif

LogLevel g_logger_level = LogLevel_Trace;

LogLevel LoggerLevel() {
    return g_logger_level;
}

void SetLoggerLevel(LogLevel logger_level) {
    g_logger_level = logger_level;
}

LoggerPtr GetLogger() {
    return g_logger;
}

void SetLogger(LoggerPtr logger) {
    g_logger = logger;
}

}

}

