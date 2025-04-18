/// @file
/// @author 5cover (Scover)
/// @brief Quick unit testing library
/// @copyright Public Domain - The Unlicense
/// @date 12/04/25
/// @version 1.1.1
/// @details
/// See README.md for details and usage.
///
/// CHANGELOG
/// 1.1.1
/// Fixed UB with va_list misuse in i_stbtest_test_case
/// 1.1.0
/// Changed table display to come before the summary. Reads better.

#ifndef STB_TEST_H_
#define STB_TEST_H_

#ifndef STB_TEST_DEFINITION
/// @brief Defines modifier keywords for the definitions. Example value: static inline
#define STB_TEST_DEFINITION
#endif

#include <stdbool.h>
#include <stdio.h>

#ifdef __GNUC__
#define i_stbtest_ATTR_FORMAT(archetype, string_index, first_to_check) __attribute__((format(archetype, string_index, first_to_check)))
#else
#define i_stbtest_ATTR_FORMAT(archetype, string_index, first_to_check)
#endif // __GNUC__

/// @brief Represents a test suite with a name and a collection of test cases.
struct test {
    char const *name;
    struct i_stbtest_case *cases;
};

struct i_stbtest_case {
    bool ok;
    unsigned line;
    char const *expr; // can be null
    char *name;
    char *file;
};

/// @brief Start a new test suite with the given name.
///
/// This function creates a new `struct test` instance with the provided name and an empty list of test cases.
///
/// @param name The name of the test suite.
/// @return The newly created `struct test` instance.
STB_TEST_DEFINITION struct test test_start(char const *name);

/// @brief Add a test case that is always a failure.
#define test_fail(test, name, ...) i_stbtest_test_case(__LINE__, __FILE__, (test), false, "(fail)", (name)__VA_OPT__(, ) __VA_ARGS__)
/// @brief Add a test case.
#define test_case(test, expr, name, ...) i_stbtest_test_case(__LINE__, __FILE__, (test), (expr), #expr, (name)__VA_OPT__(, ) __VA_ARGS__)
/// @brief Add a test case with the expr and name columns merged.
#define test_case_wide(test, expr, name, ...) i_stbtest_test_case(__LINE__, __FILE__, (test), (expr), NULL, (name)__VA_OPT__(, ) __VA_ARGS__)

STB_TEST_DEFINITION bool i_stbtest_test_case(unsigned line, char *file, struct test *test, bool ok, char const *expr, char const *fmt_name, ...)
    i_stbtest_ATTR_FORMAT(printf, 6, 7);

/// @brief Finish a test suite and prints the results to the provided output stream.
///
/// This function takes a `struct test` instance and an output stream, and prints the results of the test suite to the output stream. It includes information about the number of successful and failed test cases, as well as the names and file locations of the failed test cases.
///
/// @remark This function destroys @p test Dereferencing @p test after calling this function is undefined behavior.
///
/// @param test The test suite to be finished.
/// @param output The output stream to print the results to.
/// @return @c true if all test cases passed
/// @return @c false otherwise.
STB_TEST_DEFINITION bool test_end(struct test *test, FILE *output);

#endif // STB_TEST_H_

#ifdef STB_TEST_IMPLEMENTATION

#include "stb_ds.h"

#include <math.h>
#include <stdarg.h>

#define i_stbtest_digit_count(n, base) ((n) == 0 ? 1 : (int)(log(n) / log(base)) + 1)

struct test test_start(char const *name) {
    return (struct test) {
        .name = name,
        .cases = NULL,
    };
}

bool i_stbtest_test_case(unsigned line, char *file, struct test *test, bool ok, char const *expr, char const *fmt_name, ...) {
    va_list ap, ap1;

    va_start(ap, fmt_name);
    
    va_copy(ap1, ap);
    size_t name_size = vsnprintf(NULL, 0, fmt_name, ap1) + 1;
    va_end(ap1);

    char *name = malloc(sizeof *name * name_size);
    if (!name) abort();

    vsnprintf(name, name_size, fmt_name, ap);

    va_end(ap);

    arrput(test->cases,
        ((struct i_stbtest_case) {
            .ok = ok,
            .line = line,
            .expr = expr,
            .name = name,
            .file = file,
        }));

    return ok;
}

bool test_end(struct test *test, FILE *output) {
    // Establish case success counts and column lengths
    int i, nb_ko = 0, nb_ok = 0;

    int col_len_file = sizeof "file";
    int col_len_expr = sizeof "expr";
    int col_len_name = sizeof "info";

    for (i = 0; i < arrlen(test->cases); ++i) {
        struct i_stbtest_case const *c = &test->cases[i];
        c->ok ? ++nb_ok : ++nb_ko;
        if (c->expr) {
            int len;
            if ((len = strlen(c->file)) > col_len_file) col_len_file = len;
            if ((len = strlen(c->expr)) > col_len_expr) col_len_expr = len;
            if ((len = strlen(c->name)) > col_len_name) col_len_name = len;
        }
    }

    // Show table if test failed
    if (nb_ko != 0) {
        int const col_len_num = i_stbtest_digit_count(arrlenu(test->cases), 10);
        int const col_len_line = i_stbtest_digit_count(arrlast(test->cases).line, 10);

        for (i = 0; i < col_len_num; ++i)
            putc('#', output);
        fprintf(output, " | OK | %-*s | %*s | %-*s | %-*s |\n",
            col_len_file, "file",
            col_len_line, "L",
            col_len_expr, "expr",
            col_len_name, "info");

        for (i = 0; i < col_len_num; ++i)
            putc('-', output);
        fputs(" | -- | ", output);
        for (i = 0; i < col_len_file; ++i)
            putc('-', output);
        fputs(" | ", output);
        for (i = 0; i < col_len_line; ++i)
            putc('-', output);
        fputs(" | ", output);
        for (i = 0; i < col_len_expr; ++i)
            putc('-', output);
        fputs(" | ", output);
        for (i = 0; i < col_len_name; ++i)
            putc('-', output);
        fputs(" |\n", output);

        // Print cases

        for (i = 0; i < arrlen(test->cases); ++i) {
            struct i_stbtest_case const *const c = &test->cases[i];
            char const *ok = c->ok ? "\033[32;49mOK\033[39;49m" : "\033[31;49mKO\033[39;49m";
            if (c->expr)
                fprintf(output, "%*d | %s | %-*s | %*u | %-*s | %-*s |\n",
                    col_len_num, i, ok,
                    col_len_file, c->file,
                    col_len_line, c->line,
                    col_len_expr, c->expr,
                    col_len_name, c->name);
            else
                fprintf(output, "%*d | %s | %-*s | %*u | %s\n",
                    col_len_num, i, ok,
                    col_len_file, c->file,
                    col_len_line, c->line,
                    c->name);
        }
    }

    // Print summary
    fprintf(output, "test %s: %d ko, %d ok, %d total: %s\n",
        nb_ko == 0 ? "\033[32;49msuccess\033[39;49m" : "\033[31;49mfailure\033[39;49m",
        nb_ko,
        nb_ok,
        nb_ko + nb_ok,
        test->name);

    // Deallocate
    for (i = 0; i < arrlen(test->cases); ++i)
        free(test->cases[i].name); // Free test
    arrfree(test->cases);

    return nb_ko == 0;
}

#endif // STB_TEST_IMPLEMENTATION
