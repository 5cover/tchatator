/// @file
/// @author Raphaël
/// @brief DAL - Implementation
/// @date 23/01/2025

#include "tchatator413/cfg.h"
#include <assert.h>
#include <bcrypt/bcrypt.h>
#include <byteswap.h>
#include <netinet/in.h>
#include <postgresql/libpq-fe.h>
#include <stdlib.h>
#include <tchatator413/db.h>
#include <tchatator413/util.h>

#define TBL_USER "tchatator.user"
#define TBL__MSG "tchatator._msg"
#define TBL_INBOX "tchatator.inbox"
#define TBL_MEMBRE "pact.membre"
#define TBL_PRO "pact.professionnel"
#define CALL_FUN_SEND_MSG(arg1, arg2, arg3) "tchatator.send_msg(" arg1 "::int," arg2 "::int," arg3 "::varchar)"

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

/// @returns @ref role_flags_t
/// @returns @ref errstatus_error on error
static inline int user_kind_to_role(user_kind_t kind) {
    _Static_assert((int)errstatus_error < min_role || (int)errstatus_error > max_role,
        "role_flags_t must not have errstatus_handled in order to avoid return value ambiguity");
    switch (kind) {
    case user_kind_member: return role_membre;
    case user_kind_pro_prive: [[fallthrough]];
    case user_kind_pro_public: return role_pro;
    default:
        return errstatus_error;
    }
}

db_t *db_connect(cfg_t *cfg, int verbosity, char const *host, char const *port, char const *database, char const *username, char const *password) {
    PGconn *db = PQsetdbLogin(
        host,
        port,
        NULL, NULL,
        database,
        username,
        password);
    PGVerbosity v;
    if (verbosity <= -2)
        v = PQERRORS_SQLSTATE;
    else if (verbosity == -1)
        v = PQERRORS_TERSE;
    else if (verbosity == 0)
        v = PQERRORS_DEFAULT;
    else // verbosity >= 1
        v = PQERRORS_VERBOSE;
    PQsetErrorVerbosity(db, v);

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

void db_collect(void *memory_owner) {
    PQclear(memory_owner);
}

errstatus_t db_verify_user_api_key(db_t *db, cfg_t *cfg, user_identity_t *out_user, api_key_t api_key) {
    char api_key_repr[UUID4_REPR_LENGTH + 1];
    uuid4_repr(api_key, api_key_repr)[UUID4_REPR_LENGTH] = '\0';
    char const *arg1 = api_key_repr;
    PGresult *result = PQexecParams(db, "select kind, user_id from " TBL_USER " where api_key = $1",
        1, NULL, &arg1, NULL, NULL, 1);

    errstatus_t res;
    if (PQresultStatus(result) != PGRES_TUPLES_OK) {
        cfg_log(cfg, log_error, log_fmt_pq_result(result));
        res = errstatus_handled;
    } else if (PQntuples(result) == 0) {
        res = errstatus_error;
    } else {
        int role = user_kind_to_role(pq_recv_l(user_kind_t, PQgetvalue(result, 0, 0)));
        if (role == errstatus_error) {
            cfg_log(cfg, log_error, "database: incorrect user kind recieved: %d\n", role);
            res = errstatus_handled;
        } else {
            res = errstatus_ok;
            out_user->role = (role_flags_t)role;
            out_user->id = pq_recv_l(serial_t, PQgetvalue(result, 0, 1));
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
    PGresult *result = PQexecParams(db, "select kind from " TBL_USER " where user_id=$1",
        array_len(args), NULL, args, args_len, args_fmt, 1);

    int res;

    if (PQresultStatus(result) != PGRES_TUPLES_OK) {
        cfg_log(cfg, log_error, log_fmt_pq_result(result));
        res = errstatus_handled;
    } else if (PQntuples(result) == 0) {
        res = errstatus_error;
    } else {
        res = user_kind_to_role(pq_recv_l(user_kind_t, PQgetvalue(result, 0, 0)));
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
    // First search by member pseudo since they are unique
    PGresult *result = PQexecParams(db, "select id from " TBL_MEMBRE " where pseudo=$1",
        1, NULL, &name, NULL, NULL, 1);

    serial_t res;

    if (PQresultStatus(result) != PGRES_TUPLES_OK) {
        cfg_log(cfg, log_error, log_fmt_pq_result(result));
        res = errstatus_handled;
    } else if (PQntuples(result) == 0) {
        PQclear(result);
        // Fallback to pro display name (there must be only 1)
        result = PQexecParams(db, "select id from " TBL_PRO " where denomination=$1",
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

errstatus_t db_get_user(db_t *db, cfg_t *cfg, user_t *user) {
    uint32_t const arg1 = pq_send_l(user->id);
    char const *const args[] = { (char const *)&arg1 };
    int const args_len[array_len(args)] = { sizeof arg1 };
    int const args_fmt[array_len(args)] = { 1 };
    PGresult *result = PQexecParams(db, "select kind, email, nom, prenom, display_name from " TBL_USER " where user_id=$1",
        array_len(args), NULL, args, args_len, args_fmt, 1);

    errstatus_t res;

    if (PQresultStatus(result) != PGRES_TUPLES_OK) {
        cfg_log(cfg, log_error, log_fmt_pq_result(result));
        res = errstatus_handled;
    } else if (PQntuples(result) == 0) {
        res = errstatus_error;
    } else {
        user->kind = pq_recv_l(user_kind_t, PQgetvalue(result, 0, 0));
        user->email = PQgetvalue(result, 0, 1);
        user->last_name = PQgetvalue(result, 0, 2);
        user->first_name = PQgetvalue(result, 0, 3);
        user->display_name = PQgetvalue(result, 0, 4);
        user->memory_owner_db = result;
        return errstatus_ok;
    }

    PQclear(result);
    return res;
}

static inline bool check_password(char const *password, char const *hash) {
    switch (bcrypt_checkpw(password, hash)) {
    case -1: errno_exit("bcrypt_checkpw");
    case 0: return true;
    default: return false;
    }
}

errstatus_t db_check_password(db_t *db, cfg_t *cfg, serial_t user_id, char const *password) {
    uint32_t const arg1 = pq_send_l(user_id);
    char const *const args[] = { (char const *)&arg1 };
    int const args_len[array_len(args)] = { sizeof arg1 };
    int const args_fmt[array_len(args)] = { 1 };
    PGresult *result = PQexecParams(db, "select mdp_hash from " TBL_USER " where user_id=$1",
        array_len(args), NULL, args, args_len, args_fmt, 1);

    errstatus_t res;

    if (PQresultStatus(result) != PGRES_TUPLES_OK) {
        cfg_log(cfg, log_error, log_fmt_pq_result(result));
        res = errstatus_handled;
    } else if (PQntuples(result) == 0) {
        res = errstatus_error;
    } else {
        res = check_password(password, PQgetvalue(result, 0, 0));
    }

    PQclear(result);
    return res;
}

int db_count_msg(db_t *db, cfg_t *cfg, serial_t sender_id, serial_t recipient_id) {
    uint32_t const arg1 = pq_send_l(sender_id), arg2 = pq_send_l(recipient_id);
    char const *const args[] = { (char const *)&arg1, (char const *)&arg2 };
    int const args_len[array_len(args)] = { sizeof arg1, sizeof arg2 };
    int const args_fmt[array_len(args)] = { 1, 1 };
    PGresult *result = PQexecParams(db, "select count(*) from " TBL__MSG " where coalesce(id_compte_sender,0)=$1 and id_compte_recipient=$2",
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
    int const args_len[sizeof args] = { sizeof arg1, sizeof arg2 };
    int const args_fmt[sizeof args] = { 1, 1, 0 };
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

msg_list_t db_get_inbox(db_t *db, cfg_t *cfg,
    int32_t limit,
    int32_t offset,
    serial_t recipient_id) {
    uint32_t const arg1 = pq_send_l(recipient_id), arg2 = pq_send_l(limit), arg3 = pq_send_l(offset);
    char const *const args[] = { (char const *)&arg1, (char const *)&arg2, (char const *)&arg3 };
    int const args_len[array_len(args)] = { sizeof arg1, sizeof arg2, sizeof arg3 };
    int const args_fmt[array_len(args)] = { 1, 1, 1 };
    PGresult *result = PQexecParams(db, "select msg_id, content, sent_at, read_age, edited_age, id_compte_sender from " TBL_INBOX " where id_compte_recipient=$1 limit $2::int offset $3::int",
        array_len(args), NULL, args, args_len, args_fmt, 1);

    msg_list_t msg_list = {};

    if (PQresultStatus(result) != PGRES_TUPLES_OK) {
        cfg_log(cfg, log_error, log_fmt_pq_result(result));
        PQclear(result);
        return msg_list;
    }

    msg_list.memory_owner_db = result;
    int32_t ntuples = MIN(PQntuples(result), limit);
    msg_list.n_msgs = (size_t)ntuples;
    msg_list.msgs = malloc(sizeof *msg_list.msgs * msg_list.n_msgs);
    if (!msg_list.msgs) errno_exit("malloc");

    for (int32_t i = 0; i < ntuples; ++i) {
        msg_t *m = &msg_list.msgs[i];
        m->id = pq_recv_l(serial_t, PQgetvalue(result, i, 0));
        m->content = PQgetvalue(result, i, 1);
        m->sent_at = pq_recv_timestamp(PQgetvalue(result, i, 2));
        m->read_age = PQgetisnull(result, i, 3) ? 0 : pq_recv_l(int32_t, PQgetvalue(result, i, 3));
        m->edited_age = PQgetisnull(result, i, 4) ? 0 : pq_recv_l(int32_t, PQgetvalue(result, i, 4));
        m->deleted_age = 0;
        m->user_id_sender = PQgetisnull(result, i, 5) ? 0 : pq_recv_l(serial_t, PQgetvalue(result, i, 5));
        m->user_id_recipient = recipient_id;
    }

    return msg_list;
}

errstatus_t db_get_msg(db_t *db, cfg_t *cfg, msg_t *msg, void **out_memory_owner_db) {
    uint32_t const arg1 = pq_send_l(msg->id);
    char const *const args[] = { (char const *)&arg1 };
    int const args_len[array_len(args)] = { sizeof arg1 };
    int const args_fmt[array_len(args)] = { 1 };
    PGresult *result = PQexecParams(db, "select content, sent_at, read_age, edited_age, deleted_age, id_compte_sender, ID_compte_recipient from " TBL__MSG " where msg_id=$1",
        array_len(args), NULL, args, args_len, args_fmt, 1);

    errstatus_t res;

    if (PQresultStatus(result) != PGRES_TUPLES_OK) {
        cfg_log(cfg, log_error, log_fmt_pq_result(result));
        res = errstatus_handled;
    } else if (PQntuples(result) == 0) {
        res = errstatus_error;
    } else {
        msg->content = PQgetvalue(result, 0, 0);
        msg->sent_at = pq_recv_timestamp(PQgetvalue(result, 0, 1));
        msg->read_age = PQgetisnull(result, 0, 2) ? 0 : pq_recv_l(int32_t, PQgetvalue(result, 0, 2));
        msg->edited_age = PQgetisnull(result, 0, 3) ? 0 : pq_recv_l(int32_t, PQgetvalue(result, 0, 3));
        msg->deleted_age = PQgetisnull(result, 0, 4) ? 0 : pq_recv_l(int32_t, PQgetvalue(result, 0, 4));
        msg->user_id_sender = PQgetisnull(result, 0, 5) ? 0 : pq_recv_l(serial_t, PQgetvalue(result, 0, 5));
        msg->user_id_recipient = pq_recv_l(serial_t, PQgetvalue(result, 0, 6));
        *out_memory_owner_db = result;
        return errstatus_ok;
    }

    PQclear(result);
    return res;
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
