/// @file
/// @author Raphaël
/// @brief DAL - Implementation
/// @date 23/01/2025

#include "tchatator413/db.h"
#include "tchatator413/cfg.h"
#include "util.h"
#include <assert.h>
#include <bcrypt/bcrypt.h>
#include <byteswap.h>
#include <netinet/in.h>
#include <postgresql/libpq-fe.h>
#include <stdlib.h>

#define SCHEMA "tchatator"
#define TBL_USER SCHEMA ".user"
#define TBL_MSG SCHEMA ".msg"
#define TBL__MSG SCHEMA "._msg"
#define TBL_MSG_ORDERED SCHEMA ".msg_ordered"
#define TBL_MEMBER SCHEMA ".member"
#define TBL_PRO SCHEMA ".pro"
#define CALL_SEND_MSG(arg1, arg2, arg3) SCHEMA ".send_msg(" arg1 "::int," arg2 "::int," arg3 "::varchar)"

#if __BYTE_ORDER == __BIG_ENDIAN
#define ntohll(x) x
#elif __BYTE_ORDER == __LITTLE_ENDIAN
#define ntohll(x) bswap_64(x)
#else
#error "unsupported byte order"
#endif

#ifdef __GNUC__
#define pq_send_l(val) __extension__({_Static_assert(sizeof (val) == 4, "type has wrong size"); htonl((uint32_t)(val)); })
#define pq_recv_l(type, val) __extension__({_Static_assert(sizeof (type) == sizeof (uint32_t), "use pq_recv_ll"); (type)(ntohl(*(uint32_t *)(void*)(val))); })    // NOLINT(bugprone-casting-through-void)
#define pq_recv_ll(type, val) __extension__(({_Static_assert(sizeof (type) == sizeof (uint64_t), "use pq_recv_l"); (type)(ntohll(*(uint64_t *)(void*)(val))); })) // NOLINT(bugprone-casting-through-void)
#else
#define pq_send_l(val) htonl(val)
#define pq_recv_l(type, val) (type)(ntohl(*(type *)(void *)(val)))
#define pq_recv_ll(type, val) (type)(ntohll(*(type *)(void *)(val)))
#endif // __GNUC__

#define PG_EPOCH 946684800

#define pq_recv_timestamp(val) ((time_t)(pq_recv_ll(uint64_t, val) / 1000000 + PG_EPOCH))

#define LOG_CATEGORY "database"
#define log_fmt_pq(db) LOG_CATEGORY ": %s\n", PQerrorMessage(db)
#define log_fmt_pq_result(result) LOG_CATEGORY ": %s\n", PQresultErrorMessage(result)

union db {
    PGconn *i_conn_do_not_use_directly;
};
_Static_assert(sizeof(db_t) == sizeof(PGconn *));
#define db2conn(db) (PGconn *)(db)
#define conn2db(conn) (db_t *)(conn)

db_t *db_connect(cfg_t *cfg, char const *host, char const *port, char const *database, char const *username, char const *password) {
    PGconn *conn = PQsetdbLogin(
        host,
        port,
        NULL, NULL,
        database,
        username,
        password);
    const int v = cfg_verbosity(cfg);
    PQsetErrorVerbosity(conn,
        v <= -2
            ? PQERRORS_SQLSTATE
            : v == -1
            ? PQERRORS_TERSE
            : v == 0
            ? PQERRORS_DEFAULT
            // verbosity >= 1
            : PQERRORS_VERBOSE);

    if (PQstatus(conn) != CONNECTION_OK) {
        cfg_log(cfg, log_error, log_fmt_pq(conn));
        PQfinish(conn);
        return NULL;
    }

    cfg_log(cfg, log_info, "connected to db '%s' on %s:%s as %s\n", database, host, port, username);

    return conn2db(conn);
}

void db_destroy(db_t *db) {
    PQfinish(db2conn(db));
}

static inline bool check_password(char const *password, char const hash[static const BCRYPT_HASHSIZE]) {
    if (!password && !hash) return true;
    if (!password || !hash) return false;
    switch (bcrypt_checkpw(password, hash)) {
    case -1: errno_exit("bcrypt_checkpw");
    case 0: return true;
    default: return false;
    }
}

static inline void cfg_log_incorrect_role(cfg_t *cfg, role_t role) {
    cfg_log(cfg, log_error, "database: incorrect user role recieved: %d\n", role);
}

static inline bool validate_db_role(cfg_t *cfg, role_t role) {
    if (role != role_admin && role != role_member && role != role_pro) {
        cfg_log_incorrect_role(cfg, role);
        return false;
    }
    return true;
}

errstatus_t db_verify_user_constr(db_t *db, cfg_t *cfg, user_identity_t *out_user, constr_t constr) {
    if (cfg_verify_root_constr(cfg, constr)) {
        out_user->role = role_admin;
        out_user->id = 0;
        return errstatus_ok;
    }

    char api_key_repr[UUID4_REPR_LENGTH + 1];
    uuid4_repr(constr.api_key, api_key_repr)[UUID4_REPR_LENGTH] = '\0';
    const char *args[] = { api_key_repr };

    PGresult *result = PQexecParams(db2conn(db), "select role,password_hash,user_id from " TBL_USER " where api_key=$1",
        1, NULL, args, NULL, NULL, 1);

    errstatus_t res;
    if (PQresultStatus(result) != PGRES_TUPLES_OK) {
        cfg_log(cfg, log_error, log_fmt_pq_result(result));
        res = errstatus_handled;
    } else if (PQntuples(result) == 0) {
        res = errstatus_error;
    } else {
        out_user->role = pq_recv_l(role_t, PQgetvalue(result, 0, 0));
        if (validate_db_role(cfg, out_user->role)) {
            res = (errstatus_t)check_password(constr.password, PQgetvalue(result, 0, 1));
            if (res) out_user->id = pq_recv_l(serial_t, PQgetvalue(result, 0, 2));
        } else {
            res = errstatus_handled;
        }
    }

    PQclear(result);
    return res;
}

int db_get_user_role(db_t *db, cfg_t *cfg, serial_t user_id) {
    uint32_t const arg1 = pq_send_l(user_id);
    char const *const args[] = { (char const *)&arg1 };
    int const args_len[array_len(args)] = { sizeof arg1 };
    int const args_fmt[array_len(args)] = { 1 };
    PGresult *result = PQexecParams(db2conn(db), "select role from " TBL_USER " where user_id=$1",
        array_len(args), NULL, args, args_len, args_fmt, 1);

    int res;

    if (PQresultStatus(result) != PGRES_TUPLES_OK) {
        cfg_log(cfg, log_error, log_fmt_pq_result(result));
        res = errstatus_handled;
    } else if (PQntuples(result) == 0) {
        res = errstatus_error;
    } else {
        res = pq_recv_l(role_t, PQgetvalue(result, 0, 0));
        if (!validate_db_role(cfg, res)) res = errstatus_handled;
    }

    PQclear(result);
    return res;
}

serial_t db_get_user_id_by_email(db_t *db, cfg_t *cfg, const char *email) {
    PGresult *result = PQexecParams(db2conn(db), "select user_id from " TBL_USER " where email = $1",
        1, NULL, &email, NULL, NULL, 1);

    serial_t res;

    if (PQresultStatus(result) != PGRES_TUPLES_OK) {
        cfg_log(cfg, log_error, log_fmt_pq_result(result));
        res = errstatus_handled;
    } else if (PQntuples(result) == 0) {
        res = errstatus_error;
    } else {
        res = pq_recv_l(serial_t, PQgetvalue(result, 0, 0));
    }

    PQclear(result);
    return res;
}

serial_t db_get_user_id_by_name(db_t *db, cfg_t *cfg, const char *name) {
    // First search by member user_name since they are unique
    PGresult *result = PQexecParams(db2conn(db), "select user_id from " TBL_MEMBER " where user_name=$1",
        1, NULL, &name, NULL, NULL, 1);

    serial_t res;

    if (PQresultStatus(result) != PGRES_TUPLES_OK) {
        cfg_log(cfg, log_error, log_fmt_pq_result(result));
        res = errstatus_handled;
    } else if (PQntuples(result) == 0) {
        PQclear(result);
        // Fallback to pro business name (there must be only 1)
        result = PQexecParams(db2conn(db), "select user_id from " TBL_PRO " where business_name=$1",
            1, NULL, &name, NULL, NULL, 1);

        if (PQresultStatus(result) != PGRES_TUPLES_OK) {
            cfg_log(cfg, log_error, log_fmt_pq_result(result));
            res = errstatus_handled;
        } else if (PQntuples(result) != 1) {
            res = errstatus_error;
        } else {
            res = pq_recv_l(serial_t, PQgetvalue(result, 0, 0));
        }
    } else {
        res = pq_recv_l(serial_t, PQgetvalue(result, 0, 0));
    }

    PQclear(result);

    return res;
}

errstatus_t db_get_user(db_t *db, memlst_t **p_mem, cfg_t *cfg, user_t *p_user) {
    uint32_t const arg1 = pq_send_l(p_user->id);
    char const *const args[] = { (char const *)&arg1 };
    int const args_len[array_len(args)] = { sizeof arg1 };
    int const args_fmt[array_len(args)] = { 1 };
    PGresult *result = memlst_add(p_mem, (dtor_fn)PQclear,
        PQexecParams(db2conn(db), "select role,user_id,member_user_name,pro_business_name from " TBL_USER " where user_id=$1",
            array_len(args), NULL, args, args_len, args_fmt, 1));

    if (PQresultStatus(result) != PGRES_TUPLES_OK) {
        cfg_log(cfg, log_error, log_fmt_pq_result(result));
        return errstatus_handled;
    }
    if (PQntuples(result) == 0) {
        return errstatus_error;
    }

    p_user->role = pq_recv_l(role_t, PQgetvalue(result, 0, 0));
    p_user->id = pq_recv_l(serial_t, PQgetvalue(result, 0, 1));
    switch (p_user->role) {
    case role_admin:
        return errstatus_ok;
    case role_member:
        p_user->member.user_name = PQgetvalue(result, 0, 2);
        return errstatus_ok;
    case role_pro:
        p_user->pro.business_name = PQgetvalue(result, 0, 3);
        return errstatus_ok;
    default:
        cfg_log_incorrect_role(cfg, p_user->role);
        return errstatus_handled;
    }
}

int db_count_msg(db_t *db, cfg_t *cfg, serial_t sender_id, serial_t recipient_id) {
    uint32_t const arg1 = pq_send_l(sender_id), arg2 = pq_send_l(recipient_id);
    char const *const args[] = { (char const *)&arg1, (char const *)&arg2 };
    int const args_len[array_len(args)] = { sizeof arg1, sizeof arg2 };
    int const args_fmt[array_len(args)] = { 1, 1 };
    PGresult *result = PQexecParams(db2conn(db), "select count(*) from " TBL_MSG " where coalesce(user_id_sender,0)=$1 and user_id_recipient=$2",
        array_len(args), NULL, args, args_len, args_fmt, 1);

    int res;

    if (PQresultStatus(result) != PGRES_TUPLES_OK) {
        cfg_log(cfg, log_error, log_fmt_pq_result(result));
        res = errstatus_handled;
    } else {
        res = PQntuples(result);
    }

    PQclear(result);
    return res;
}

serial_t db_send_msg(db_t *db, cfg_t *cfg, serial_t sender_id, serial_t recipient_id, char const *content) {
    uint32_t const arg1 = pq_send_l(sender_id), arg2 = pq_send_l(recipient_id);
    char const *const args[] = { (char const *)&arg1, (char const *)&arg2, content };
    int const args_len[array_len(args)] = { sizeof arg1, sizeof arg2 };
    int const args_fmt[array_len(args)] = { 1, 1, 0 };
    PGresult *result = PQexecParams(db2conn(db), "select " CALL_SEND_MSG("$1", "$2", "$3"),
        array_len(args), NULL, args, args_len, args_fmt, 1);

    serial_t res;

    if (PQresultStatus(result) != PGRES_TUPLES_OK) {
        cfg_log(cfg, log_error, log_fmt_pq_result(result));
        res = errstatus_handled;
    } else {
        // the sql function returns errstatus_error on error
        res = pq_recv_l(serial_t, PQgetvalue(result, 0, 0));
        _Static_assert(errstatus_error == 0, "DB compatiblity");
    }

    return res;
}

errstatus_t db_get_inbox(db_t *db, memlst_t **p_mem, cfg_t *cfg,
    int32_t limit,
    int32_t offset,
    serial_t recipient_id,
    msg_list_t *out_msgs) {

    uint32_t const arg1 = pq_send_l(recipient_id), arg2 = pq_send_l(limit), arg3 = pq_send_l(offset);
    char const *const args[] = { (char const *)&arg1, (char const *)&arg2, (char const *)&arg3 };
    int const args_len[array_len(args)] = { sizeof arg1, sizeof arg2, sizeof arg3 };
    int const args_fmt[array_len(args)] = { 1, 1, 1 };
    PGresult *result = memlst_add(p_mem, (dtor_fn)PQclear,
        PQexecParams(db2conn(db), "select msg_id, content, sent_at, read_age, edited_age, user_id_sender from " TBL_MSG_ORDERED " where user_id_recipient=$1 limit $2::int offset $3::int",
            array_len(args), NULL, args, args_len, args_fmt, 1));

    if (PQresultStatus(result) != PGRES_TUPLES_OK) {
        cfg_log(cfg, log_error, log_fmt_pq_result(result));
        return errstatus_handled;
    }

    int32_t ntuples = MIN(PQntuples(result), limit);
    out_msgs->msgs = memlst_add(p_mem, free, malloc(sizeof *out_msgs->msgs * ntuples));
    if (!out_msgs->msgs) errno_exit("malloc");
    out_msgs->n_msgs = (size_t)ntuples;
    for (int32_t i = 0; i < ntuples; ++i) {
        msg_t *p_msg = &out_msgs->msgs[i];
        p_msg->id = pq_recv_l(serial_t, PQgetvalue(result, i, 0));
        p_msg->content = PQgetvalue(result, i, 1);
        p_msg->sent_at = pq_recv_timestamp(PQgetvalue(result, i, 2));
        p_msg->read_age = PQgetisnull(result, i, 3) ? 0 : pq_recv_l(int32_t, PQgetvalue(result, i, 3));
        p_msg->edited_age = PQgetisnull(result, i, 4) ? 0 : pq_recv_l(int32_t, PQgetvalue(result, i, 4));
        p_msg->deleted_age = 0;
        p_msg->user_id_sender = PQgetisnull(result, i, 5) ? 0 : pq_recv_l(serial_t, PQgetvalue(result, i, 5));
        p_msg->user_id_recipient = recipient_id;
    }

    return errstatus_ok;
}

errstatus_t db_get_msg(db_t *db, memlst_t **p_mem, cfg_t *cfg, msg_t *p_msg) {
    uint32_t const arg1 = pq_send_l(p_msg->id);
    char const *const args[] = { (char const *)&arg1 };
    int const args_len[array_len(args)] = { sizeof arg1 };
    int const args_fmt[array_len(args)] = { 1 };
    PGresult *result = memlst_add(p_mem, (dtor_fn)PQclear,
        PQexecParams(db2conn(db), "select content, sent_at, read_age, edited_age, deleted_age, user_id_sender, user_id_recipient from " TBL__MSG " where msg_id=$1",
            array_len(args), NULL, args, args_len, args_fmt, 1));

    if (PQresultStatus(result) != PGRES_TUPLES_OK) {
        cfg_log(cfg, log_error, log_fmt_pq_result(result));
        return errstatus_handled;
    }
    if (PQntuples(result) == 0) {
        return errstatus_error;
    }

    p_msg->content = PQgetvalue(result, 0, 0);
    p_msg->sent_at = pq_recv_timestamp(PQgetvalue(result, 0, 1));
    p_msg->read_age = PQgetisnull(result, 0, 2) ? 0 : pq_recv_l(int32_t, PQgetvalue(result, 0, 2));
    p_msg->edited_age = PQgetisnull(result, 0, 3) ? 0 : pq_recv_l(int32_t, PQgetvalue(result, 0, 3));
    p_msg->deleted_age = PQgetisnull(result, 0, 4) ? 0 : pq_recv_l(int32_t, PQgetvalue(result, 0, 4));
    p_msg->user_id_sender = PQgetisnull(result, 0, 5) ? 0 : pq_recv_l(serial_t, PQgetvalue(result, 0, 5));
    p_msg->user_id_recipient = pq_recv_l(serial_t, PQgetvalue(result, 0, 6));
    return errstatus_ok;
}

errstatus_t db_rm_msg(db_t *db, cfg_t *cfg, serial_t msg_id) {
    uint32_t const arg1 = pq_send_l(msg_id);
    char const *const args[] = { (char const *)&arg1 };
    int const args_len[array_len(args)] = { sizeof arg1 };
    int const args_fmt[array_len(args)] = { 1 };
    PGresult *result = PQexecParams(db2conn(db), "delete from " TBL_MSG " where msg_id=$1",
        array_len(args), NULL, args, args_len, args_fmt, 1);

    errstatus_t res;

    if (PQresultStatus(result) != PGRES_COMMAND_OK) {
        cfg_log(cfg, log_error, log_fmt_pq_result(result));
        res = errstatus_handled;
    } else if (!streq(PQcmdTuples(result), "1")) {
        res = errstatus_error;
    } else {
        res = errstatus_ok;
    }

    PQclear(result);
    return res;
}

errstatus_t db_transaction(db_t *db, cfg_t *cfg, transaction_fn body, void *ctx) {
    PGresult *result = PQexec(db2conn(db), "begin");
    cfg_log(cfg, log_debug, LOG_CATEGORY ": BEGIN\n");

    errstatus_t res;

    if (PQresultStatus(result) != PGRES_COMMAND_OK) {
        cfg_log(cfg, log_error, log_fmt_pq_result(result));
        res = errstatus_handled;
    } else {
        PQclear(result);

        // We've began the transaction.
        res = body(db, cfg, ctx);

        // End the transaction now.
        result = PQexec(db2conn(db), res == errstatus_ok ? "commit" : "rollback");
        cfg_log(cfg, log_debug, LOG_CATEGORY ": %s\n", res == errstatus_ok ? "COMMIT" : "ROLLBACK");

        if (PQresultStatus(result) != PGRES_COMMAND_OK) {
            cfg_log(cfg, log_error, log_fmt_pq_result(result));
            res = errstatus_handled;
        }
    }

    PQclear(result);

    return res;
}

errstatus_t db_use_test_data(db_t *db, cfg_t *cfg, test_data_t subject) {
    static char const *const test_data_queries[] = {
        // encapsulate strlit in macro if we add more
        [test_data_msgs] = "call " SCHEMA ".use_test_data_msgs()",
        [test_data_users] = "call " SCHEMA ".use_test_data_users()",
    };

    cfg_log(cfg, log_info, LOG_CATEGORY ": %s\n", test_data_queries[subject]);
    PGresult *result = PQexec(db2conn(db), test_data_queries[subject]);

    errstatus_t res;
    if (PQresultStatus(result) != PGRES_COMMAND_OK) {
        cfg_log(cfg, log_error, log_fmt_pq_result(result));
        res = errstatus_handled;
    } else {
        res = errstatus_ok;
    }

    PQclear(result);
    return res;
}
