/// @file
/// @author Raphaël
/// @brief Tchatator413 test - db_verify_user_constr
/// @date 1/02/2025

#include "../tests.h"
#include "tchatator413/db.h"

#define NAME db_verify_user_constr

static errstatus_t transaction(db_t *db, cfg_t *cfg, void *ctx) {
    test_t *p_tst = ctx;

    db_use_test_data(db, cfg, test_data_users);

    {
        user_identity_t user;
        errstatus_t res = db_verify_user_constr(p_tst->db, p_tst->cfg, &user,
            (constr_t) {
                .api_key = API_KEY_PRO1_UUID,
                .password = "pro1_mdp",
            });

        if (!TEST_CASE_EQ_INT(&p_tst->t, res, errstatus_ok, )) return errstatus_handled;
        TEST_CASE_EQ_INT(&p_tst->t, user.id, USER_ID_PRO1, );
        TEST_CASE_EQ_INT(&p_tst->t, user.role, role_pro, );
    }

    {
        user_identity_t user;
        errstatus_t res = db_verify_user_constr(p_tst->db, p_tst->cfg, &user,
            (constr_t) {
                .api_key = API_KEY_MEMBER1_UUID,
                .password = "member1_mdp",
            });

        if (!TEST_CASE_EQ_INT(&p_tst->t, res, errstatus_ok, )) return errstatus_handled;
        TEST_CASE_EQ_INT(&p_tst->t, user.id, USER_ID_MEMBER1, );
        TEST_CASE_EQ_INT(&p_tst->t, user.role, role_member, );
    }

    return errstatus_handled;
}

TEST_SIGNATURE(NAME) {
    test_t tst = TEST_INIT(NAME);

    db_transaction(tst.db, tst.cfg, transaction, &tst);

    return tst.t;
}
