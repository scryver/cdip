#ifndef KEYVAL_LOG_H
#define KEYVAL_LOG_H

#define keyval_print(...)      do { fprintf(stdout, __VA_ARGS__); fputc('\n', stdout); } while (0)
#define keyval_log(...)        do { fprintf(stdout, "%s:%d:%s: ", __FILE__, __LINE__, __func__); \
fprintf(stdout, __VA_ARGS__); fputc('\n', stdout); } while (0)
#define keyval_error(...)      do { fprintf(stderr, "%s:%d:%s: ", __FILE__, __LINE__, __func__); \
fprintf(stderr, __VA_ARGS__); fputc('\n', stderr); } while (0)
#define keyval_fatal(...)      do { fprintf(stderr, "%s:%d:%s: ", __FILE__, __LINE__, __func__); \
fprintf(stderr, __VA_ARGS__); fputc('\n', stderr); exit(1); } while (0)

#endif //KEYVAL_LOG_H
