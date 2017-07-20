#ifndef __CUBE_LOGGER_H__
#define __CUBE_LOGGER_H__

#include <stdarg.h>
#include <memory>

#define M_LOG(level, format, args...) \
    do { \
            ::cube::logging::GetLogger()->level("%s|%d|%s|" format "\n", __FILE__, __LINE__, __FUNCTION__ , ##args); \
    } while(false)

#define M_LOG_LOG(format, args...) M_LOG(Log, "LOG|" format, ##args)
#define M_LOG_DEBUG(format, args...) M_LOG(Debug, "DEBUG|" format, ##args)
#define M_LOG_TRACE(format, args...) M_LOG(Trace, "TRACE|" format, ##args)
#define M_LOG_WARN(format, args...)  M_LOG(Warn, "WARN|" format, ##args)
#define M_LOG_ERROR(format, args...) M_LOG(Error, "ERROR|" format, ##args)
#define M_LOG_INFO(format, args...)  M_LOG(Info, "INFO|" format, ##args)

namespace cube {

namespace logging {

enum LogLevel {
    LogLevel_Unknown,
    LogLevel_Trace,
    LogLevel_Debug,
    LogLevel_Warn,
    LogLevel_Info,
    LogLevel_Error,
};

class Logger {
    public:
        Logger() {}
        virtual ~Logger() {}

        // do not need to override these function in most cases
        virtual void Debug(const char *format, ...);
        virtual void Trace(const char *format, ...);
        virtual void Warn(const char *format, ...);
        virtual void Info(const char *format, ...);
        virtual void Error(const char *format, ...);
        virtual void Log(const char *format, ...); // ignore log level limit

    protected:
        // override to implement your own Output() function
        virtual void Output(const char *foramt, va_list args) = 0;
};

class StdoutLogger : public Logger {
    public:
        StdoutLogger() {}
        virtual ~StdoutLogger() {}

    protected:
        virtual void Output(const char *format, va_list args);
};

class EmptyLogger : public Logger {
    public:
        EmptyLogger() {}
        virtual ~EmptyLogger() {}

    protected:
        virtual void Output(const char *format, va_list args) {}
};

typedef std::shared_ptr<Logger> LoggerPtr;

LogLevel LoggerLevel();
void SetLoggerLevel(LogLevel logger_level);
LoggerPtr GetLogger();
void SetLogger(LoggerPtr logger);

}

}

#endif
