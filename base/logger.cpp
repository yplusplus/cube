#include <stdio.h>

#include "logger.h"

namespace cube {

Logger::Logger()
    : m_log_level(LogLevel_Trace) {
}

Logger::Logger(LogLevel log_level)
    : m_log_level(log_level) {
}

Logger::~Logger() {
}

Logger::LogLevel Logger::LoggerLevel() const {
    return m_log_level;
}

void Logger::SetLoggerLevel(LogLevel log_level) {
    m_log_level = log_level;
}

#define M_GENERATE_LOG_FUNCTION(log_level) \
    void Logger::log_level(const char *format, ...) { \
        if (LogLevel_##log_level >= m_log_level) { \
            va_list args; \
            va_start(args, format); \
            Output(format, args); \
            va_end(args); \
        } \
    }

M_GENERATE_LOG_FUNCTION(Debug);
M_GENERATE_LOG_FUNCTION(Trace);
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

// ************************************
// StdoutLogger
StdoutLogger::StdoutLogger()
    : Logger() {
}

StdoutLogger::StdoutLogger(LogLevel log_level)
    : Logger(log_level) {
}

StdoutLogger::~StdoutLogger() {
}

void StdoutLogger::Output(const char *format, va_list args) {
    vprintf(format, args);
}

// ************************************
// EmptyLogger
EmptyLogger::EmptyLogger()
    : Logger() {
}

EmptyLogger::~EmptyLogger() {
}

void EmptyLogger::Output(const char *format, va_list args) {
    // do nothing
}

}

