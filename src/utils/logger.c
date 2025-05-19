#include "logger.h"

LOG_LEVEL current_level = DEBUG;

void
set_log_level(LOG_LEVEL new_lvl) {
    if (new_lvl >= DEBUG && new_lvl <= FATAL)
        current_level = new_lvl;
}

char *
level_desc(LOG_LEVEL level) {
    static char *desc[] = {"DEBUG", "INFO", "WARN", "ERROR", "FATAL"};
    if (level < DEBUG || level > FATAL)
        return "UNKNOWN";
    return desc[level];
}
