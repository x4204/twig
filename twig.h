#ifndef TWIG_H
#define TWIG_H

#include <stdarg.h>     /* va_list, va_start(), va_end() */
#include <stddef.h>     /* NULL */

typedef enum {
  TWIG_LFDEBUG = 1 << 0,
  TWIG_LFINFO  = 1 << 1,
  TWIG_LFWARN  = 1 << 2,
  TWIG_LFERROR = 1 << 3,
} twig_lflag_t;

typedef enum {
  TWIG_LNONE  = 0,
  TWIG_LERROR = TWIG_LNONE  | TWIG_LFERROR,
  TWIG_LWARN  = TWIG_LERROR | TWIG_LFWARN,
  TWIG_LINFO  = TWIG_LWARN  | TWIG_LFINFO,
  TWIG_LDEBUG = TWIG_LINFO  | TWIG_LFDEBUG,
  TWIG_LALL   = TWIG_LDEBUG,
} twig_level_t;

typedef enum {
  TWIG_ERRNO_OK = 0,
  TWIG_ERRNO_ERROR,
} twig_errno_t;

typedef struct twig_record_s twig_record_t;
typedef struct twig_sink_s twig_sink_t;
typedef struct twig_s twig_t;

typedef twig_errno_t (*twig_sink_write_cb)(twig_sink_t *sink, twig_record_t *record, char *buffer);

typedef twig_errno_t (*twig_fmt_cb)(void *data, char **buffer, char *fmt, va_list varargs);

struct twig_record_s {
  twig_lflag_t lflag;
  char *label;
  char *sfile;
  int sline;
};

struct twig_sink_s {
  void *dest;
  twig_sink_write_cb write_cb;
  twig_level_t level;
};

struct twig_s {
  twig_sink_t **sinks;
  char *label;
  void *fmt_data;
  twig_fmt_cb fmt_cb;
};

twig_errno_t
twig(twig_t *logger, char *sfile, int sline, twig_lflag_t lflag, char *fmt, ...);

#define twig_debug(logger, fmt, ...) \
  twig(logger, __FILE__, __LINE__, TWIG_LFDEBUG, fmt, __VA_ARGS__)

#define twig_info(logger, fmt, ...) \
  twig(logger, __FILE__, __LINE__, TWIG_LFINFO, fmt, __VA_ARGS__)

#define twig_warn(logger, fmt, ...) \
  twig(logger, __FILE__, __LINE__, TWIG_LFWARN, fmt, __VA_ARGS__)

#define twig_error(logger, fmt, ...) \
  twig(logger, __FILE__, __LINE__, TWIG_LFERROR, fmt, __VA_ARGS__)

twig_errno_t
twig(twig_t *logger, char *sfile, int sline, twig_lflag_t lflag, char *fmt, ...)
{
  va_list varargs;
  char *fmt_buffer = NULL;

  va_start(varargs, fmt);

  if (logger->fmt_cb(logger->fmt_data, &fmt_buffer, fmt, varargs) != TWIG_ERRNO_OK) {
    va_end(varargs);
    return TWIG_ERRNO_ERROR;
  }

  for (twig_sink_t **iter = logger->sinks; *iter != NULL; iter++) {
    twig_sink_t *sink = *iter;

    if ((sink->level & lflag) == 0) {
      continue;
    }
    twig_record_t record = {
      .lflag = lflag,
      .label = logger->label,
      .sfile = sfile,
      .sline = sline,
    };

    if (sink->write_cb(sink, &record, fmt_buffer) != TWIG_ERRNO_OK) {
      va_end(varargs);
      return TWIG_ERRNO_ERROR;
    }
  }

  va_end(varargs);

  return TWIG_ERRNO_OK;
}

#endif /* TWIG_H */
