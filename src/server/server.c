/// @file
/// @author Raphaël
/// @brief Tchatator413 dynamic server state - Implementation
/// @date 29/01/2025

#include <tchatator413/server.h>
#include <tchatator413/types.h>

#include <stb_ds.h>

struct server {
    struct {
        token_t key;
        serial_t value;
    } *sessions;
    api_key_t admin_api_key;
    char const *admin_password;
};

server_t *server_create(api_key_t admin_api_key, char const *admin_password) {
    server_t *server = calloc(1, sizeof *server);
    hmdefault(server->sessions, -1);
    server->admin_api_key = admin_api_key;
    server->admin_password = admin_password;
    return server;
}

void server_destroy(server_t *server) {
    if (!server) return;
    hmfree(server->sessions);
    free(server);
}

token_t server_login(server_t *server, serial_t user_id) {
    // our key: (user_id, time)
    // we won't allow the same user to login twice in the same second. That will be a collision.
    // merge user_id (high bits) and time (low 32 bits)
    time_t const t = time(NULL);
    token_t token = ((long)user_id << 32) + ~(int32_t)t;

    if (hmgeti(server->sessions, user_id) != -1) return 0;
    hmput(server->sessions, token, user_id);

    return token;
}

bool server_logout(server_t *server, token_t token) {
    return hmdel(server->sessions, token);
}

serial_t server_verify_token(server_t *server, token_t token) {
    return hmget(server->sessions, token); // the default value is -1
}


bool server_is_admin_api_key(server_t *server, api_key_t api_key)
{
    return uuid4_eq(server->admin_api_key, api_key);
}

bool server_check_admin_password(server_t *server, char const *password)
{
    return streq(server->admin_password, password);
}