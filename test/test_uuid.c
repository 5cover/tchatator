/// @file
/// @author Raphaël
/// @brief Testing - UUID unit tests
/// @date 23/01/2025

#include "strings.h"
#include "tests.h"
#include <tchatator413/errstatus.h>
#include <tchatator413/util.h>
#include <tchatator413/uuid.h>

struct test test_uuid4(void) {
    struct test t = test_start("uuid4");

    static const uuid4_t different_uuid = uuid4_init(0xf9, 0x1d, 0x4f, 0xae, 0x7d, 0xec, 0x11, 0xd0, 0xa7, 0x65, 0x00, 0xa0, 0xc9, 0x1e, 0x6b, 0xf6);
    static char const uuids[][UUID4_REPR_LENGTH] = {
        "f81d4fae-7dec-11d0-a765-00a0c91e6bf6",
        "F81D4FAE-7DEC-11D0-A765-00A0C91E6BF6",
        "F81d4faE-7deC-11D0-A765-00A0c91e6Bf6",
        "00000000-0000-0000-0000-000000000000",
        "ffffffff-ffff-ffff-ffff-ffffffffffff",
        "FFFFFFFF-FFFF-FFFF-FFFF-FFFFFFFFFFFF",
    };

    static char const invalid_uuids[][UUID4_REPR_LENGTH] = {
        "f81g4fae-7dec-11d0-a765-00a0c91e6bf6",
        "f81d4fa-7dec-11d0-a765-00a0c91e6bf6",
        "f81d4fae-7dec11d0-a765-00a0c91e6bf6"
    };

    for (size_t i = 0; i < array_len(uuids); ++i) {
        uuid4_t uuid;
        test_case(&t,
            errstatus_ok == uuid4_parse(&uuid, uuids[i]),
            "%.*s", UUID4_REPR_LENGTH, uuids[i]);
        char repr[UUID4_REPR_LENGTH];
        test_case(&t,
            strncasecmp(uuids[i], uuid4_repr(uuid, repr), UUID4_REPR_LENGTH) == 0,
            "repr(%.*s) == repr(%.*s)", UUID4_REPR_LENGTH, uuids[i], UUID4_REPR_LENGTH, repr);
        test_case(&t,
            uuid4_eq(uuid, uuid),
            "%.*s == %.*s", UUID4_REPR_LENGTH, uuids[i], UUID4_REPR_LENGTH, repr);
        test_case(&t,
            !uuid4_eq(uuid, different_uuid) && !uuid4_eq(different_uuid, uuid),
            "%.*s != different_uuid", UUID4_REPR_LENGTH, uuids[i]);
    }

    for (size_t i = 0; i < array_len(invalid_uuids); ++i) {
        uuid4_t uuid;
        test_case(&t,
            errstatus_ok != uuid4_parse(&uuid, invalid_uuids[i]),
            "%.*s", UUID4_REPR_LENGTH, invalid_uuids[i]);
    }

    uuid4_t const uuid0 = uuid4_of(0xf8, 0x1d, 0x4f, 0xae, 0x7d, 0xec, 0x11, 0xd0, 0xa7, 0x65, 0x00, 0xa0, 0xc9, 0x1e, 0x6b, 0xf6);
    uuid4_t uuid0_rt = { 0 }; // Roundtrip
    uuid4_parse(&uuid0_rt, uuids[0]);
    test_case(&t, uuid4_eq(uuid0, uuid0_rt), "literal == from repr");

    return t;
}
