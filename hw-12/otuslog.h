#ifndef OTUSLOG_H
#define OTUSLOG_H

#include <stdio.h>
#include <stdarg.h>

enum log_level {DEBUG, INFO, WARNING, ERROR};

int init_log(const char *filename, size_t fsize);
int final_log();
int log_to_file(enum log_level lvl, int lineno, const char *filename, const char *fmt, ...);
void print_help(void);

#define LOG_DEBUG(...) do {				\
    log_to_file(DEBUG, __LINE__, __FILE__, __VA_ARGS__);	\
  } while (0)

#define LOG_INFO(...) do {			\
    log_to_file(INFO, __LINE__, __FILE__, __VA_ARGS__);	\
  } while (0)

#define LOG_WARNING(...) do { \
    log_to_file(WARNING, __LINE__, __FILE__, __VA_ARGS__); \
  } while (0)

#define LOG_ERROR(...) do { \
    log_to_file(ERROR, __LINE__, __FILE__, __VA_ARGS__); \
  } while (0)

#endif
