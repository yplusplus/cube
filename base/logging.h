#ifndef __CUBE_LOGGER_H__
#define __CUBE_LOGGER_H__

#include <stdarg.h>
#include <memory>

#define M_LOG(level, format, args...) \
    do { \
            ::cube::logging::GetLogger()->level("%s|%d|%s|" format "\n", __FILE__, __LINE__, __FUNCTION__ , ##args); \
    } while(false)

#define M_LOG_DEBUG(format, args...) M_LOG(Debug, format, ##args)
#define M_LOG_TRACE(format, args...) M_LOG(Trace, format, ##args)
#define M_LOG_WARN(format, args...)  M_LOG(Warn, format, ##args)
#define M_LOG_ERROR(format, args...) M_LOG(Error, format, ##args)
#define M_LOG_INFO(format, args...)  M_LOG(Info, format, ##args)

namespace cube {

namespace logging {

class Logger {
    public:
        enum LogLevel {
            LogLevel_Debug,
            LogLevel_Trace,
            LogLevel_Warn,
            LogLevel_Info,
            LogLevel_Error,
        };

        Logger();
        Logger(LogLevel log_level);
        virtual ~Logger();

        // log level getter ans setter
        LogLevel LoggerLevel() const;
        void SetLoggerLevel(LogLevel log_level);

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

    protected:

        // log level
        LogLevel m_log_level;
};

class StdoutLogger : public Logger {
    public:
        StdoutLogger();
        StdoutLogger(LogLevel log_level);
        virtual ~StdoutLogger();

    protected:
        virtual void Output(const char *format, va_list args);
};

class EmptyLogger : public Logger {
    public:
        EmptyLogger();
        virtual ~EmptyLogger();

    protected:
        virtual void Output(const char *format, va_list args);
};

typedef std::shared_ptr<Logger> LoggerPtr;

LoggerPtr GetLogger();
void SetLogger(LoggerPtr logger);

}

}

#endif
