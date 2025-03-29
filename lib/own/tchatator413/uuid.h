/// @file
/// @author Raphaël
/// @brief UUID library - Interface
/// @date 23/01/2025

#ifndef UUID_H
#define UUID_H

#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <tchatator413/slice.h>

/// @brief Version 4 UUID.
typedef struct {
    uint8_t data[16];
} uuid4_t;

/// @brief Length of the canonical representation of a version 4 UUID, excluding the null terminator.
#define UUID4_REPR_LENGTH 36

/// @brief Generate the representation of a version 4 UUID.
/// @param uuid The UUID.
/// @param repr The representation buffer.
/// @return @p {repr}.
/// @remark This function doesn't add a null terminator at the end of @p repr.
char *uuid4_repr(uuid4_t uuid, char repr[static const UUID4_REPR_LENGTH]);

bool uuid4_parse_slice(uuid4_t *out_uuid, slice_t repr_slice);

/// @brief Parse a version 4 UUID from its canonical representation.
/// @param out_uuid Mutated to the parsed UUID.
/// @param repr The string containing the representation.
/// @return @c true parsing successful @p out_uuid is set.
/// @return @c false parsing unsucessful.
/// @remark The syntax ABNF can be found at https://www.rfc-editor.org/rfc/rfc9562.html#section-4-5. Lowercase hex digits are allowed.
bool uuid4_parse(uuid4_t *out_uuid, char const repr[static const UUID4_REPR_LENGTH]);

/// @brief Put the canonical representation of version 4 UUID.
/// @param uuid The UUID Version 4 to write.
/// @param stream The stream to write to.
void uuid4_put(uuid4_t uuid, FILE *stream);

/// @brief Are two version 4 UUIDs equal?
/// @param a The first UUID.
/// @param b The second UUID.
/// @return A boolean indicating whether @p a and @p b are equal.
bool uuid4_eq(uuid4_t a, uuid4_t b);

/// @brief Create a new version 4 UUID from the specified values.
/// @return A new UUID Version 4.
#define uuid4_of(x1, x2, x3, x4, x5, x6, x7, x8, x9, x10, x11, x12, x13, x14, x15, x16) \
    (uuid4_t) uuid4_init(x1, x2, x3, x4, x5, x6, x7, x8, x9, x10, x11, x12, x13, x14, x15, x16)

#define uuid4_init(x1, x2, x3, x4, x5, x6, x7, x8, x9, x10, x11, x12, x13, x14, x15, x16) \
    {                                                                                     \
        .data = {                                                                         \
            x1,                                                                           \
            x2,                                                                           \
            x3,                                                                           \
            x4,                                                                           \
            x5,                                                                           \
            x6,                                                                           \
            x7,                                                                           \
            x8,                                                                           \
            x9,                                                                           \
            x10,                                                                          \
            x11,                                                                          \
            x12,                                                                          \
            x13,                                                                          \
            x14,                                                                          \
            x15,                                                                          \
            x16,                                                                          \
        }                                                                                 \
    }

#endif // UUID_H
