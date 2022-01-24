#include "twig.h"

#define _POSIX_C_SOURCE 200809L

#include <stdio.h>
#include <stdlib.h>
#include <syslog.h>
#include <time.h>

twig_errno_t
stream_write_cb(twig_sink_t *sink, twig_record_t *record, char *buffer);

twig_errno_t
syslog_write_cb(twig_sink_t *sink, twig_record_t *record, char *buffer);

typedef struct {
  FILE *stream;
  char *buffer;
  size_t buffer_size;
} logger_fmt_data_t;

twig_errno_t
logger_fmt_cb(void *data, char **buffer, char *fmt, va_list varargs);

/*
 * Example output:
 * -----------
 * [2021-11-16T20:23:49.844846534Z|root|main.c:38|DEBUG] Hello world!
 * [2021-11-16T20:23:49.845021168Z|root|main.c:42|INFO] Hello world!
 * [2021-11-16T20:23:49.845077763Z|root|main.c:46|WARN] Hello world!
 * [2021-11-16T20:23:49.845142378Z|root|main.c:50|ERROR] Hello world!
 * -----------
 */
int main(int argc, char **argv)
{
  (void)argc;
  (void)argv;

  // --------------------------------------------------------------------------
  logger_fmt_data_t fmt_data = { 0 };
  fmt_data.stream = open_memstream(&fmt_data.buffer, &fmt_data.buffer_size);

  openlog("twig", LOG_NDELAY | LOG_PID, LOG_USER);

  // --------------------------------------------------------------------------
  twig_sink_t sink_stdout = {
    .dest = stdout,
    .write_cb = &stream_write_cb,
    .level = TWIG_LDEBUG,
  };
  twig_sink_t sink_syslog = {
    .dest = NULL, /* syslog has global state */
    .write_cb = &syslog_write_cb,
    .level = TWIG_LWARN,
  };

  twig_sink_t *sinks[] = { &sink_stdout, &sink_syslog, NULL };
  twig_t _logger = {
    .label = "root",
    .sinks = sinks,
    .fmt_data = (void *)&fmt_data,
    .fmt_cb = &logger_fmt_cb,
  };
  twig_t *logger = &_logger;

  // --------------------------------------------------------------------------
  twig_debug(logger, "%s", "hello");
  twig_debug(logger, "hello %d", 1);
  twig_info(logger, "hello %d", 2);
  twig_warn(logger, "hello %d", 3);
  twig_error(logger, "hello %d", 4);

  // --------------------------------------------------------------------------
  closelog();
  fclose(fmt_data.stream);
  free(fmt_data.buffer);

  return 0;
}

static twig_errno_t
stream_write_prefix(twig_sink_t *sink, twig_record_t *record)
{
  static char *lflag_strings[] = {
    [TWIG_LFDEBUG] = "DEBUG",
    [TWIG_LFINFO]  = "INFO",
    [TWIG_LFWARN]  = "WARN",
    [TWIG_LFERROR] = "ERROR",
  };
  char time_buffer[32] = { 0 };
  struct timespec current_timespec;
  struct tm current_tm;

  clock_gettime(CLOCK_REALTIME, &current_timespec);
  gmtime_r(&current_timespec.tv_sec, &current_tm);
  strftime(time_buffer, sizeof(time_buffer), "%Y-%m-%dT%H:%M:%S", &current_tm);

  int char_count = fprintf(
    sink->dest,
    "[%s.%09luZ|%s|%s:%d|%s] ",
    time_buffer,
    current_timespec.tv_nsec,
    record->label,
    record->sfile,
    record->sline,
    lflag_strings[record->lflag]
  );

  if (char_count < 0) {
    return TWIG_ERRNO_ERROR;
  }
  return TWIG_ERRNO_OK;
}

twig_errno_t
stream_write_cb(twig_sink_t *sink, twig_record_t *record, char *buffer)
{
  if (stream_write_prefix(sink, record) != TWIG_ERRNO_OK) {
    return TWIG_ERRNO_ERROR;
  }
  if (fputs(buffer, sink->dest) < 0) {
    return TWIG_ERRNO_ERROR;
  }
  if (fputc('\n', sink->dest) == EOF) {
    return TWIG_ERRNO_ERROR;
  }
  return TWIG_ERRNO_OK;
}

twig_errno_t
syslog_write_cb(twig_sink_t *sink, twig_record_t *record, char *buffer)
{
  (void)sink;

  static const int lflag_to_syslog_level[] = {
    [TWIG_LFDEBUG] = LOG_DEBUG,
    [TWIG_LFINFO] = LOG_INFO,
    [TWIG_LFWARN] = LOG_WARNING,
    [TWIG_LFERROR] = LOG_ERR,
  };

  syslog(
    lflag_to_syslog_level[record->lflag],
    "[%s|%s:%d] %s",
    record->label,
    record->sfile,
    record->sline,
    buffer
  );

  return TWIG_ERRNO_OK;
}

twig_errno_t
logger_fmt_cb(void *_data, char **buffer, char *fmt, va_list varargs)
{
  logger_fmt_data_t *fmt_data = (logger_fmt_data_t *)_data;

  if (fseek(fmt_data->stream, 0, SEEK_SET) < 0) {
    return TWIG_ERRNO_ERROR;
  }
  if (vfprintf(fmt_data->stream, fmt, varargs) < 0) {
    return TWIG_ERRNO_ERROR;
  }
  if (fflush(fmt_data->stream) == EOF) {
    return TWIG_ERRNO_ERROR;
  }
  *buffer = fmt_data->buffer;

  return TWIG_ERRNO_OK;
}
