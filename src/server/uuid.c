/// @file
/// @author Raphaël
/// @brief UUID library - Implementation
/// @date 23/01/2025

#include <tchatator413/uuid.h>

#include "assert.h"
#include <stdint.h>
#include <string.h>

/*
UUID     = 4hexOctet "-"
           2hexOctet "-"
           2hexOctet "-"
           2hexOctet "-"
           6hexOctet
hexOctet = HEXDIG HEXDIG
DIGIT    = %x30-39
HEXDIG   = DIGIT / "A" / "B" / "C" / "D" / "E" / "F"
*/

static inline char hex_half_to_repr(uint8_t value);
static inline uint8_t hex_repr_to_half(char c);

#define INVALID_HALF 255

#define X_42226 O O O O H O O H O O H O O H O O O O O O

char *uuid4_repr(uuid4_t uuid, char repr[static const UUID4_REPR_LENGTH]) {
    size_t idata = 0, i = 0;
#define O                                                    \
    do {                                                     \
        repr[i++] = hex_half_to_repr(uuid.data[idata] >> 4); \
        repr[i++] = hex_half_to_repr(uuid.data[idata] & 15); \
        ++idata;                                             \
    } while (0);
#define H repr[i++] = '-';
    X_42226;
#undef O
#undef H
    return repr;
}

bool uuid4_parse(uuid4_t *out_uuid, char const repr[static const UUID4_REPR_LENGTH]) {
    size_t idata = 0, i = 0;
    uint8_t v1, v2;
#define O                                                            \
    do {                                                             \
        if ((v1 = hex_repr_to_half(repr[i++])) == INVALID_HALF       \
            || (v2 = hex_repr_to_half(repr[i++])) == INVALID_HALF) { \
            return false;                                            \
        }                                                            \
        out_uuid->data[idata++] = (uint8_t)(v1 << 4) + v2;                    \
    } while (0);
#define H \
    if (repr[i++] != '-') return false;
    X_42226
#undef O
#undef H
    return true;
}

void uuid4_put(uuid4_t uuid, FILE *stream) {
    size_t idata = 0;
#define O                                                      \
    do {                                                       \
        putc(hex_half_to_repr(uuid.data[idata] >> 4), stream); \
        putc(hex_half_to_repr(uuid.data[idata] & 15), stream); \
        ++idata;                                               \
    } while (0);
#define H putc('-', stream);
    X_42226;
#undef O
#undef H
}

char hex_half_to_repr(uint8_t value) {
    assert(value < 16);
    return value < 10 ? '0' + value : 'a' - 10 + value;
}

uint8_t hex_repr_to_half(char c) {
    return '0' <= c && c <= '9'
        ? (uint8_t)c - '0'
        : 'A' <= c && c <= 'F'
        ? (uint8_t)c - 'A' + 10
        : 'a' <= c && c <= 'f'
        ? (uint8_t)c - 'a' + 10
        : INVALID_HALF;
}

bool uuid4_eq(uuid4_t a, uuid4_t b) {
    return memcmp(&a, &b, sizeof a) == 0;
}

bool uuid4_parse_slice(uuid4_t *out_uuid, slice_t repr_slice) {
    return repr_slice.len >= UUID4_REPR_LENGTH && uuid4_parse(out_uuid, repr_slice.val);
}
