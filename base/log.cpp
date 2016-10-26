#include "log.h"

namespace cube {

static LogLevel g_log_level = LOG_LEVEL_DEBUG;
void SetLogLevel(LogLevel log_level) {
    g_log_level = log_level;
}

}
