/// @file
/// @author Raphaël
/// @brief Tchatator413 manual testing - put_role
/// @date 1/02/2025

#include <tchatator413/action.h>

#include "tests.h"

void observe_put_role(void) {
    for (role_flags_t r = 0; r <= role_all; ++r) {
        printf("%x -> ", r);
        put_role(r, stdout);
        putchar('\n');
    }
}
