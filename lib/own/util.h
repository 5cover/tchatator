/// @file
/// @author Raphaël
/// @brief General utilities - Standalone hedaer
/// @date 23/01/2025

#ifndef UTIL_H
#define UTIL_H

#include <memlst.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/// @brief Safely destroys a memory list and returns a status.
///
/// This macro destroys the given memory list and immediately returns the specified status.
/// It ensures proper memory cleanup before returning from a function.
///
/// @param mem The memory list to be destroyed.
/// @param value The value to return after destroying the memory list.
#define CLEAN_RETURN(mem, value) \
    do {                          \
        memlst_destroy(&(mem));     \
        return (value);          \
    } while (0)

/// @brief Safely destroys a memory list and exits the program.
///
/// This macro destroys the given memory list and immediately exits the program
/// with the specified status. It ensures proper memory cleanup before terminating
/// the program.
///
/// @param mem The memory list to be destroyed.
/// @param status The exit status value to use when terminating the program.
#define CLEAN_EXIT(mem, status) \
    do {                        \
        memlst_destroy(&(mem));   \
        exit(status);           \
    } while (0)

/// @brief Quote.
#define QUOTE(name) #name
/// @brief Stringify.
#define STR(macro) QUOTE(macro)

/// @brief Concatenate.
#define CAT(x, y) CAT_(x, y)
/// @brief Token-paste.
#define CAT_(x, y) x##y

/// @brief Gets the necessary buffer size for a sprintf operation.
#define buffer_size(format, ...) (snprintf(NULL, 0, (format), __VA_ARGS__) + 1) // safe byte for \0

/// @brief Compares two strings for equality.
///
/// This macro compares the two null-terminated strings @p x and @p y for equality. It returns 1 if the strings are equal, and 0 otherwise.
///
/// @param x The first string to compare.
/// @param y The second string to compare.
/// @return 1 if the strings are equal, 0 otherwise.
#define streq(x, y) (strcmp((x), (y)) == 0)

#define streq_nullable(x, y) ((x) == NULL && (y) == NULL || (x) != NULL && (y) != NULL && streq((x), (y)))

/// @brief Compares two strings for equality up to a given length.
///
/// This macro compares the first @p n characters of the null-terminated strings @p x and @p y for equality. It returns 1 if the strings are equal up to the first @p n characters, and 0 otherwise.
///
/// @param x The first string to compare.
/// @param y The second string to compare.
/// @param n The number of characters to compare.
/// @return 1 if the strings are equal up to the first @p n characters, 0 otherwise.
#define strneq(x, y, n) (strncmp((x), (y), (n)) == 0)

/// @brief Returns the maximum of two values.
///
/// This macro returns the larger of the two arguments @p a and @p b.
///
/// @remark This macro evaluates argument @p a twice.
/// @param a The first value to compare.
/// @param b The second value to compare.
/// @return The larger of @p a and @p b.
#define MAX(a, b) ((a) > (b) ? (a) : (b))

/// @brief Returns the minimum of two values.
///
/// This macro returns the smaller of the two arguments @p a and @p b.
///
/// @remark This macro evaluates argument @p a twice.
/// @param a The first value to compare.
/// @param b The second value to compare.
/// @return The smaller of @p a and @p b.
#define MIN(a, b) ((a) < (b) ? (a) : (b))

/// @brief Returns the first non-null argument.
///
/// This macro takes two arguments @p a and @p b, and returns @p a if it is not null, otherwise it returns @p b.
///
/// @remark This macro evaluates argument @p a twice.
/// @param a The first argument to check.
/// @param b The second argument to use if @p a is null.
/// @return @p a if it is not null, otherwise @p b.
#define COALESCE(a, b) ((a == NULL) ? (b) : (a))

/// @brief Exits the program with an error message and failure status.
///
/// This macro is used to exit the program with an error message and failure status when an error occurs. It calls `perror()` to print the error message and then calls `exit()` with the `EXIT_FAILURE` status.
#define errno_exit(of)      \
    do {                    \
        perror(of);         \
        exit(EXIT_FAILURE); \
    } while (0)

/// @brief Calculates the length of an array.
///
/// This macro takes an array as its argument and returns the number of elements in the array.
///
/// @param array The array to get the length of.
/// @return The number of elements in the array.
#define array_len(array) (sizeof(array) / sizeof((array)[0]))

#ifdef __clang__
#define ATTR_FLAG_ENUM [[clang::flag_enum]]
#else
#define ATTR_FLAG_ENUM
#endif // __clang__

#ifdef __GNUC__
#define ATTR_FORMAT(archetype, string_index, first_to_check) __attribute__((format(archetype, string_index, first_to_check)))
#else
#define ATTR_FORMAT(archetype, string_index, first_to_check)
#endif // __GNUC__

/// @brief Formats a string using a printf-style format string and arguments.
///
/// This function takes a format string and a variable number of arguments, and
/// returns a dynamically allocated string that contains the formatted result.
/// The caller is responsible for freeing the returned string.
///
/// @param fmt The printf-style format string.
/// @param ... The arguments to be formatted.
/// @return A dynamically allocated string containing the formatted result.
char *strfmt(const char *fmt, ...) ATTR_FORMAT(printf, 1, 2);

/// @brief Formats a string using a printf-style format string and a va_list of arguments.
///
/// This function takes a format string and a va_list of arguments, and
/// returns a dynamically allocated string that contains the formatted result.
/// The caller is responsible for freeing the returned string.
///
/// @param fmt The printf-style format string.
/// @param ap The va_list of arguments to be formatted.
/// @return A dynamically allocated string containing the formatted result.
char *vstrfmt(const char *fmt, va_list ap);

/// @brief Slurps a stream into a dynamically allocated string
///
/// The caller is responsible for freeing the returned string.
///
/// @param fp The stream to slurp
/// @return  A dynamically allocated string containing the stream's contents.
char *fslurp(FILE *fp);

// Uses compiler specific extensions if possible.
#ifdef __GNUC__ // GCC, Clang, ICC

#define unreachable() (__builtin_unreachable())

#elif defined(_MSC_VER) // MSVC

#define unreachable() (__assume(false))

#else
// Even if no extension is used, undefined behavior is still raised by
// the empty function body and the noreturn attribute.

// The external definition of unreachable_impl must be emitted in a separated TU
// due to the rule for inline functions in C.

[[noreturn]] inline void unreachable_impl() { }
#define unreachable() (unreachable_impl())

#endif

#endif // UTIL_H
