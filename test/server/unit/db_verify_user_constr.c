/// @file
/// @author Raphaël
/// @brief Tchatator413 test - db_verify_user_constr
/// @date 1/02/2025

#include "../tests.h"
#include "tchatator413/db.h"

#define NAME db_verify_user_constr

TEST_SIGNATURE(NAME) {
    test_t test = TEST_INIT(NAME);

    {
        user_identity_t user;
        errstatus_t res = db_verify_user_constr(test.db, test.cfg, &user,
            (constr_t) {
                .api_key = API_KEY_PRO1_UUID,
                .password = "pro1_mdp",
            });

        if (!TEST_CASE_EQ_INT(&test.t, res, errstatus_ok, )) return test.t;
        TEST_CASE_EQ_INT(&test.t, user.id, 1, );
        TEST_CASE_EQ_INT(&test.t, user.role, role_pro, );
    }

    {
        user_identity_t user;
        errstatus_t res = db_verify_user_constr(test.db, test.cfg, &user,
            (constr_t) {
                .api_key = API_KEY_MEMBER1_UUID,
                .password = "member1_mdp",
            });

        if (!TEST_CASE_EQ_INT(&test.t, res, errstatus_ok, )) return test.t;
        TEST_CASE_EQ_INT(&test.t, user.id, 3, );
        TEST_CASE_EQ_INT(&test.t, user.role, role_member, );
    }

    return test.t;
}
