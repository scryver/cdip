#ifndef KEYVAL_LOG_H
#define KEYVAL_LOG_H

#define keyval_log(fmt, ...)        fprintf(stdout, fmt "\n", ## __VA_ARGS__)
#define keyval_error(fmt, ...)      fprintf(stderr, fmt "\n", ## __VA_ARGS__)
#define keyval_fatal(fmt, ...)      (fprintf(stderr, fmt "\n", ## __VA_ARGS__), exit(1))

#endif //KEYVAL_LOG_H
