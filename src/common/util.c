/// @file
/// @author Raphaël
/// @brief Tchatator413 miscaelannous utilities - Implementation
/// @date 1/02/2025

#include "util.h"
#include <limits.h>
#include <stdarg.h>
#include <sysexits.h>

char *strfmt(const char *fmt, ...) {
    va_list ap;
    va_start(ap, fmt);
    char *p = vstrfmt(fmt, ap);
    va_end(ap);
    return p;
}

char *vstrfmt(const char *fmt, va_list ap) {
    int n = 0;
    size_t size = 0;
    char *p = NULL;
    va_list ap_copy;
    va_copy(ap_copy, ap);

    /* Determine required size */

    n = vsnprintf(p, size, fmt, ap);

    if (n < 0) {
        va_end(ap_copy);
        return NULL;
    }

    /* One extra byte for '\0' */

    size = (size_t)n + 1;
    p = malloc(size);
    if (p == NULL) {
        va_end(ap_copy);
        return NULL;
    }

    n = vsnprintf(p, size, fmt, ap_copy);
    va_end(ap_copy);

    if (n < 0) {
        free(p);
        return NULL;
    }
    return p;
}

char *fslurp(FILE *fp) {
    char *answer;
    char *temp;
    size_t bufsize = 1024;
    size_t i = 0;
    int c;

    answer = malloc(1024);
    if (!answer) return 0;

    while ((c = fgetc(fp)) != EOF) {
        if (i == bufsize - 2) {
            if (bufsize > INT_MAX - 100 - bufsize / 10) {
                free(answer);
                return 0;
            }
            bufsize = bufsize + 100 * bufsize / 10;
            temp = realloc(answer, bufsize);
            if (temp == 0) {
                free(answer);
                return 0;
            }
            answer = temp;
        }
        answer[i++] = (char)c;
    }
    answer[i++] = 0;

    temp = realloc(answer, i);
    return COALESCE(temp, answer);
}
