/// @file
/// @author Raphaël
/// @brief Tchatator413 test - db_get_user
/// @date 1/02/2025

#include "../tests.h"
#include <tchatator413/db.h>

#define NAME db_get_user

TEST_SIGNATURE(NAME) {
    test_t test = TEST_INIT(NAME);

    {
        user_t user = { .id = 1 };
        db_get_user(db, cfg, &user);

        TEST_CASE_EQ_INT(&test.t, user.id, 1, );
        TEST_CASE_EQ_INT(&test.t, user.role, role_pro, );
        TEST_CASE_EQ_STR(&test.t, user.pro.business_name, "pro1 corp", );

        db_collect(user.memory_owner_db);
    }

    {
        user_t user = { .id = 3 };
        db_get_user(db, cfg, &user);

        TEST_CASE_EQ_INT(&test.t, user.id, 3, );
        TEST_CASE_EQ_INT(&test.t, user.role, role_member, );
        TEST_CASE_EQ_STR(&test.t, user.member.user_name, "member1", );

        db_collect(user.memory_owner_db);
    }

    return test.t;
}
