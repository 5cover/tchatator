/// @file
/// @author Raphaël
/// @brief Tchatator413 test - db_get_user
/// @date 1/02/2025

#include "../tests.h"
#include "tchatator413/db.h"

#define NAME db_get_user

static errstatus_t transaction(db_t *db, cfg_t *cfg, void *ctx) {
    test_t *p_tst = ctx;

    db_use_test_data(db, cfg, test_data_users);

    {
        user_t user = { .id = USER_ID_PRO1 };
        db_get_user(p_tst->db, p_tst->p_mem, p_tst->cfg, &user);

        TEST_CASE_EQ_INT(&p_tst->t, user.id, USER_ID_PRO1, );
        TEST_CASE_EQ_INT(&p_tst->t, user.role, role_pro, );
        TEST_CASE_EQ_STR(&p_tst->t, user.pro.business_name, "pro1 corp", );
    }

    {
        user_t user = { .id = USER_ID_MEMBER1 };
        db_get_user(p_tst->db, p_tst->p_mem, p_tst->cfg, &user);

        TEST_CASE_EQ_INT(&p_tst->t, user.id, USER_ID_MEMBER1, );
        TEST_CASE_EQ_INT(&p_tst->t, user.role, role_member, );
        TEST_CASE_EQ_STR(&p_tst->t, user.member.user_name, "member1", );
    }

    return errstatus_tested;
}

TEST_SIGNATURE(NAME) {
    test_t tst = TEST_INIT(NAME);

    db_transaction(tst.db, tst.cfg, transaction, &tst);

    return tst.t;
}
