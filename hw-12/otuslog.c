#include "otuslog.h"

#define _POSIX_C_SOURCE 200809L

#include <stdlib.h>
#include <pthread.h>
#include <errno.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/syscall.h>
#include <time.h>

#ifdef LIBUNWIND_Y
#include <libunwind.h>
static int do_backtrace_lunwind (void);
#else
#include <execinfo.h>
static int do_backtrace_gcc (void);
#endif

int log_fsize;
int wr_ch_count;
FILE *fp;
pthread_mutex_t lock;

int init_log(const char *filename, size_t fsize)
{
  int ret = pthread_mutex_init(&lock, NULL);
  if (ret) {
	  return ret;
  }
	
  fp = fopen(filename, "w");
  if (!fp) {
    perror("Error in open log file");
    
    return -1;
  }

  log_fsize = fsize;
  wr_ch_count = 0;
  
  return 0;
}

int final_log()
{	
  fclose(fp);

  int ret = pthread_mutex_destroy(&lock);
  if (ret) {
	  return ret;
  }
  
  return 0;
}

int log_to_file(enum log_level lvl, int lineno, const char *filename, const char *fmt, ...)
{
  char *log_level_name[] = {"D", "I", "W", "E"};

  time_t timep = time(NULL);
  struct tm *ltm = localtime(&timep);
  
  pthread_mutex_lock(&lock);
  
  va_list args;
  va_start(args, fmt);
  
  wr_ch_count += fprintf(fp, "%s tid=%d %02d:%02d:%02d %s:%d : ",
			 log_level_name[lvl], (int) syscall(SYS_gettid),
			 ltm->tm_hour, ltm->tm_min, ltm->tm_sec,
			 filename, lineno);

  wr_ch_count += vfprintf(fp, fmt, args);

  wr_ch_count += fprintf(fp, "\n");
  
  if (lvl == ERROR) {
#ifdef LIBUNWIND_Y
    wr_ch_count += do_backtrace_lunwind();
#else
    wr_ch_count += do_backtrace_gcc();
#endif
  }

  if (wr_ch_count >= log_fsize) {
    rewind(fp);
  }

  va_end(args);
  
  pthread_mutex_unlock(&lock);
  
  return 0;
}

#ifdef LIBUNWIND_Y
// print call stack with libunwind
int do_backtrace_lunwind (void)
{
  int wr_ch_count = 0;
  
  unw_cursor_t cursor;
  unw_context_t context;

  unw_getcontext(&context);
  unw_init_local(&cursor, &context);
  
  while (unw_step(&cursor) > 0) {
    unw_word_t offset, pc;
    char fname[128];
    
    unw_get_reg(&cursor, UNW_REG_IP, &pc);
    
    fname[0] = '\0';
    (void) unw_get_proc_name(&cursor, fname, sizeof(fname), &offset);
    
    wr_ch_count += fprintf (fp, "\t%p : (%s+0x%x)\n",
			    (void *)pc,
			    fname,
			    (unsigned int)offset);
  }

  return wr_ch_count;
}
#else
// print call stack with gcc functionality
int do_backtrace_gcc (void)
{
  int wr_ch_count = 0;
  
  void *array[10];
  char **strings;
  int size, i;

  size = backtrace (array, 10);
  strings = backtrace_symbols (array, size);
  if (strings != NULL) {
    wr_ch_count += fprintf (fp, "Obtained %d stack frames:\n", size);
    for (i = 0; i < size; i++) {
      wr_ch_count += fprintf (fp, "%s\n", strings[i]);
    }
  }

  free (strings); strings = NULL;

  return wr_ch_count;
}
#endif

void print_help(void)
{
  printf("The log library for C.\n\n");
  printf("Output format:\n"
	 "LOG_LEVEL_CHAR tid=x hh:mm:ss FNAME:LINENO : MSG\n"
	 "\t[CALLSTACK] (for Error log level only)\n"
	 ", where LOG_LEVEL_CHAR = D | I | W | E\n"
	 "for Debug, Info, Warning and Error log level, respectively;\n"
	 "x - thread ID;\n"
	 "hh:mm:ss - time of the logger function call;\n"
	 "FNAME - filename of the source code which call logger function;\n"
	 "LINENO - line number;\n"
	 "MSG - arbitrary message;\n"
	 "callstack is printed line by line 'PC FUNCTION-NAME+ADDR-INSTRUCTION' (when libunwind is installed).\n\n");  
}
