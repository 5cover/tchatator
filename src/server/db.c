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
#define TBL__MSG SCHEMA "._msg"
#define TBL_INBOX SCHEMA ".inbox"
#define TBL_MEMBER SCHEMA ".member"
#define TBL_PRO SCHEMA ".pro"
#define CALL_FUN_SEND_MSG(arg1, arg2, arg3) SCHEMA ".send_msg(" arg1 "::int," arg2 "::int," arg3 "::varchar)"

#if __BYTE_ORDER == __BIG_ENDIAN
#define ntohll(x) x
#elif __BYTE_ORDER == __LITTLE_ENDIAN
#define ntohll(x) bswap_64(x)
#else
#error "unsupported byte order"
#endif

#ifdef __GNUC__
#define pq_send_l(val) __extension__({_Static_assert(sizeof val == 4, "type has wrong size"); htonl((uint32_t)val); })
#define pq_recv_l(type, val) __extension__({_Static_assert(sizeof (type) == sizeof (uint32_t), "use pq_recv_ll"); (type)(ntohl(*(uint32_t *)(val))); })
#define pq_recv_ll(type, val) __extension__(({_Static_assert(sizeof (type) == sizeof (uint64_t), "use pq_recv_l"); (type)(ntohll(*(uint64_t *)val)); }))
#else
#define pq_send_l(val) htonl(val)
#define pq_recv_l(type, val) (type)(ntohl(*(type *)(val)))
#define pq_recv_ll(type, val) (type)(ntohll(*(type *)val))
#endif // __GNUC__

#define PG_EPOCH 946684800

#define pq_recv_timestamp(val) ((time_t)(pq_recv_ll(uint64_t, val) / 1000000 + PG_EPOCH))

#define log_fmt_pq(db) "database: %s\n", PQerrorMessage(db)
#define log_fmt_pq_result(result) "database: %s\n", PQresultErrorMessage(result)

db_t *db_connect(cfg_t *cfg, char const *host, char const *port, char const *database, char const *username, char const *password) {
    PGconn *db = PQsetdbLogin(
        host,
        port,
        NULL, NULL,
        database,
        username,
        password);
    const int v = cfg_verbosity(cfg);
    PQsetErrorVerbosity(db,
        v <= -2
            ? PQERRORS_SQLSTATE
            : v == -1
            ? PQERRORS_TERSE
            : v == 0
            ? PQERRORS_DEFAULT
            // verbosity >= 1
            : PQERRORS_VERBOSE);

    if (PQstatus(db) != CONNECTION_OK) {
        cfg_log(cfg, log_error, log_fmt_pq(db));
        PQfinish(db);
        return NULL;
    }

    cfg_log(cfg, log_info, "connected to db '%s' on %s:%s as %s\n", database, host, port, username);

    return db;
}

void db_destroy(db_t *db) {
    PQfinish(db);
}

static inline bool check_password(char const *password, char const hash[BCRYPT_HASHSIZE]) {
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

    // todo: remove debug loggin

    // fprintf(stderr, "[debug] about to call PQexecParams with api_key_repr = \"%s\"\n", api_key_repr);
    // fprintf(stderr, "[debug] args = %p args[0] = %p (%s)\n", (void const *)args, (void const *)args[0], args[0]);

    PGresult *result = PQexecParams(db, "select role,password_hash,user_id from " TBL_USER " where api_key=$1",
        1, NULL, args, NULL, NULL, 1);

    fprintf(stderr, "[debug] PQstatus: %d\n", PQstatus(db));
    fprintf(stderr, "[debug] PQerrorMessage: %s\n", PQerrorMessage(db));

    errstatus_t res;
    if (PQresultStatus(result) != PGRES_TUPLES_OK) {
        cfg_log(cfg, log_error, log_fmt_pq_result(result));
        res = errstatus_handled;
    } else if (PQntuples(result) == 0) {
        res = errstatus_error;
    } else {
        out_user->role = pq_recv_l(role_t, PQgetvalue(result, 0, 0));
        if (validate_db_role(cfg, out_user->role)) {
            res = check_password(constr.password, PQgetvalue(result, 0, 1));
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
    PGresult *result = PQexecParams(db, "select role from " TBL_USER " where user_id=$1",
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
    PGresult *result = PQexecParams(db, "select user_id from " TBL_USER " where email = $1",
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
    PGresult *result = PQexecParams(db, "select user_id from " TBL_MEMBER " where user_name=$1",
        1, NULL, &name, NULL, NULL, 1);

    serial_t res;

    if (PQresultStatus(result) != PGRES_TUPLES_OK) {
        cfg_log(cfg, log_error, log_fmt_pq_result(result));
        res = errstatus_handled;
    } else if (PQntuples(result) == 0) {
        PQclear(result);
        // Fallback to pro business name (there must be only 1)
        result = PQexecParams(db, "select user_id from " TBL_PRO " where business_name=$1",
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

errstatus_t db_get_user(db_t *db, memlst_t **pmem, cfg_t *cfg, user_t *out_user) {
    uint32_t const arg1 = pq_send_l(out_user->id);
    char const *const args[] = { (char const *)&arg1 };
    int const args_len[array_len(args)] = { sizeof arg1 };
    int const args_fmt[array_len(args)] = { 1 };
    PGresult *result = memlst_add(pmem, (fn_dtor_t)PQclear,
        PQexecParams(db, "select role,user_id,member_user_name,pro_business_name from " TBL_USER " where user_id=$1",
            array_len(args), NULL, args, args_len, args_fmt, 1));

    if (PQresultStatus(result) != PGRES_TUPLES_OK) {
        cfg_log(cfg, log_error, log_fmt_pq_result(result));
        return errstatus_handled;
    }
    if (PQntuples(result) == 0) {
        return errstatus_error;
    }

    out_user->role = pq_recv_l(role_t, PQgetvalue(result, 0, 0));
    out_user->id = pq_recv_l(serial_t, PQgetvalue(result, 0, 1));
    switch (out_user->role) {
    case role_admin:
        return errstatus_ok;
    case role_member:
        out_user->member.user_name = PQgetvalue(result, 0, 2);
        return errstatus_ok;
    case role_pro:
        out_user->pro.business_name = PQgetvalue(result, 0, 3);
        return errstatus_ok;
    default:
        cfg_log_incorrect_role(cfg, out_user->role);
        return errstatus_handled;
    }
}

int db_count_msg(db_t *db, cfg_t *cfg, serial_t sender_id, serial_t recipient_id) {
    uint32_t const arg1 = pq_send_l(sender_id), arg2 = pq_send_l(recipient_id);
    char const *const args[] = { (char const *)&arg1, (char const *)&arg2 };
    int const args_len[array_len(args)] = { sizeof arg1, sizeof arg2 };
    int const args_fmt[array_len(args)] = { 1, 1 };
    PGresult *result = PQexecParams(db, "select count(*) from " TBL__MSG " where coalesce(user_id_sender,0)=$1 and user_id_recipient=$2",
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
    PGresult *result = PQexecParams(db, "select " CALL_FUN_SEND_MSG("$1", "$2", "$3"),
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

errstatus_t db_get_inbox(db_t *db, memlst_t **pmem, cfg_t *cfg,
    int32_t limit,
    int32_t offset,
    serial_t recipient_id,
    msg_list_t *out_msgs) {

    uint32_t const arg1 = pq_send_l(recipient_id), arg2 = pq_send_l(limit), arg3 = pq_send_l(offset);
    char const *const args[] = { (char const *)&arg1, (char const *)&arg2, (char const *)&arg3 };
    int const args_len[array_len(args)] = { sizeof arg1, sizeof arg2, sizeof arg3 };
    int const args_fmt[array_len(args)] = { 1, 1, 1 };
    PGresult *result = memlst_add(pmem, (fn_dtor_t)PQclear,
        PQexecParams(db, "select msg_id, content, sent_at, read_age, edited_age, user_id_sender from " TBL_INBOX " where user_id_recipient=$1 limit $2::int offset $3::int",
            array_len(args), NULL, args, args_len, args_fmt, 1));

    if (PQresultStatus(result) != PGRES_TUPLES_OK) {
        cfg_log(cfg, log_error, log_fmt_pq_result(result));
        return errstatus_handled;
    }

    int32_t ntuples = MIN(PQntuples(result), limit);
    out_msgs->msgs = memlst_add(pmem, free, malloc(sizeof *out_msgs->msgs * ntuples));
    if (!out_msgs->msgs) errno_exit("malloc");
    out_msgs->n_msgs = (size_t)ntuples;
    for (int32_t i = 0; i < ntuples; ++i) {
        msg_t *m = &out_msgs->msgs[i];
        m->id = pq_recv_l(serial_t, PQgetvalue(result, i, 0));
        m->content = PQgetvalue(result, i, 1);
        m->sent_at = pq_recv_timestamp(PQgetvalue(result, i, 2));
        m->read_age = PQgetisnull(result, i, 3) ? 0 : pq_recv_l(int32_t, PQgetvalue(result, i, 3));
        m->edited_age = PQgetisnull(result, i, 4) ? 0 : pq_recv_l(int32_t, PQgetvalue(result, i, 4));
        m->deleted_age = 0;
        m->user_id_sender = PQgetisnull(result, i, 5) ? 0 : pq_recv_l(serial_t, PQgetvalue(result, i, 5));
        m->user_id_recipient = recipient_id;
    }

    return errstatus_ok;
}

errstatus_t db_get_msg(db_t *db, memlst_t **pmem, cfg_t *cfg, msg_t *msg) {
    uint32_t const arg1 = pq_send_l(msg->id);
    char const *const args[] = { (char const *)&arg1 };
    int const args_len[array_len(args)] = { sizeof arg1 };
    int const args_fmt[array_len(args)] = { 1 };
    PGresult *result = memlst_add(pmem, (fn_dtor_t)PQclear,
        PQexecParams(db, "select content, sent_at, read_age, edited_age, deleted_age, user_id_sender, user_id_recipient from " TBL__MSG " where msg_id=$1",
            array_len(args), NULL, args, args_len, args_fmt, 1));

    if (PQresultStatus(result) != PGRES_TUPLES_OK) {
        cfg_log(cfg, log_error, log_fmt_pq_result(result));
        return errstatus_handled;
    }
    if (PQntuples(result) == 0) {
        return errstatus_error;
    }

    msg->content = PQgetvalue(result, 0, 0);
    msg->sent_at = pq_recv_timestamp(PQgetvalue(result, 0, 1));
    msg->read_age = PQgetisnull(result, 0, 2) ? 0 : pq_recv_l(int32_t, PQgetvalue(result, 0, 2));
    msg->edited_age = PQgetisnull(result, 0, 3) ? 0 : pq_recv_l(int32_t, PQgetvalue(result, 0, 3));
    msg->deleted_age = PQgetisnull(result, 0, 4) ? 0 : pq_recv_l(int32_t, PQgetvalue(result, 0, 4));
    msg->user_id_sender = PQgetisnull(result, 0, 5) ? 0 : pq_recv_l(serial_t, PQgetvalue(result, 0, 5));
    msg->user_id_recipient = pq_recv_l(serial_t, PQgetvalue(result, 0, 6));
    return errstatus_ok;
}

errstatus_t db_rm_msg(db_t *db, cfg_t *cfg, serial_t msg_id) {
    uint32_t const arg1 = pq_send_l(msg_id);
    char const *const args[] = { (char const *)&arg1 };
    int const args_len[array_len(args)] = { sizeof arg1 };
    int const args_fmt[array_len(args)] = { 1 };
    PGresult *result = PQexecParams(db, "delete from " TBL__MSG " where msg_id=$1",
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

errstatus_t db_transaction(db_t *db, cfg_t *cfg, fn_transaction_t body, void *ctx) {
    PGresult *result = PQexec(db, "begin");

    errstatus_t res;

    if (PQresultStatus(result) != PGRES_COMMAND_OK) {
        cfg_log(cfg, log_error, log_fmt_pq_result(result));
        res = errstatus_handled;
    } else {
        PQclear(result);

        // We begun the transaction.

        res = body(db, cfg, ctx);

        // End the transaction now.
        result = PQexec(db, res == errstatus_ok ? "commit" : "rollback");

        if (PQresultStatus(result) != PGRES_COMMAND_OK) {
            cfg_log(cfg, log_error, log_fmt_pq_result(result));
            res = errstatus_handled;
        }
    }

    PQclear(result);

    return res;
}
