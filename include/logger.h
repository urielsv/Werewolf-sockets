#ifndef __logger_h__
#define __logger_h__

#include <stdio.h>
#include <stdarg.h>
#include <time.h>
#include <stdlib.h>

typedef enum {
    DEBUG,
    INFO,
    WARN,
    ERROR,
    FATAL
} LOG_LEVEL;

extern LOG_LEVEL current_level;

#define log(level, fmt, ...) do { \
    if (level >= current_level) { \
        FILE *out = level >= ERROR ? stderr : stdout; \
        time_t t = time(NULL); \
        struct tm *tm = localtime(&t); \
        fprintf(out, "%d-%02d-%02dT%02d:%02d:%02dZ\t[%s] ", \
                tm->tm_year + 1900, tm->tm_mon + 1, tm->tm_mday, \
                tm->tm_hour, tm->tm_min, tm->tm_sec, \
                level_desc(level)); \
        fprintf(out, fmt "\n", ##__VA_ARGS__); \
        if (level == FATAL) exit(1); \
    } \
} while(0)

void set_log_level(LOG_LEVEL new_level);
char * level_desc(LOG_LEVEL level);

#endif // __logger_h__
