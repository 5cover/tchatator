/// @file
/// @author Raphaël
/// @brief Error status enumeration - Interface
/// @date 29/01/2025

#ifndef ERRSTATUS_H
#define ERRSTATUS_H

/// @brief An error status.
/// @remark errstatus constants are used in other integral expressions, by considering -1 and 0 as errors and any other value as successful. When this is the case, it will be documented.
#include <stdbool.h>
typedef enum {
    /// @brief An error occured but it has already been handled, no action needed besides propagation.
    errstatus_handled = -1,
    /// @brief Smallest value of the enuemration.
    min_errstatus = errstatus_handled,
    /// @brief An error occurred.
    errstatus_error,
    /// @brief No error. Can be shadowed by the payload value in supertypes.
    errstatus_ok,
    /// @brief Largest value of the enumeration.
    max_errstatus = errstatus_ok
} errstatus_t;

_Static_assert(errstatus_error == false, "errstatus_error must equal to false for boolean logic");
_Static_assert(errstatus_ok == true, "errstatus_ok must be true for boolean logic");
_Static_assert(errstatus_handled == -1, "errstatus_ok must be -1 (Unix error convention)");

#endif // ERRSTATUS_H
