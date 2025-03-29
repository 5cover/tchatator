/// @file
/// @author Raphaël
/// @brief Tchatator413 dynamic server state - Interface
/// @date 29/01/2025

#ifndef SERVER_STATE_H
#define SERVER_STATE_H

#include "types.h"
#include <stdbool.h>
#include <time.h>

typedef struct {
    /// @brief Timestamp of the last request.
    time_t last_request_at;
    /// @brief Number of requests performed since an hour.
    int n_requests_h;
    /// @brief Number of requests performed since a minute.
    int n_requests_m;
} user_stats_t;

/// @brief Opaque type handle representing a server instance.
typedef struct server server_t;

/// @brief Creates a new server instance.
/// @param admin_api_key The admin API key.
/// @param admin_password The admin password.
/// @return A new server instance.
server_t *server_create(api_key_t admin_api_key, char const *admin_password);

/// @brief Destroys the specified server instance.
/// @param server The server instance to destroy.
void server_destroy(server_t *server);

/// @brief Creates a new session, logging in an user.
/// @param server The server.
/// @param user_id The ID of the user to login.
/// @return The new session token.
/// @return @c 0 if the the session could not be created. This happens if the same user tries logs in twice in the same second.
token_t server_login(server_t *server, serial_t user_id);

/// @brief Checks if an API key is the admin API key.
/// @param server The server.
/// @param api_key The API key to check.
/// @return @c true if @p api_key is the admin API key.
/// @return @c false otherwise.
bool server_is_admin_api_key(server_t *server, api_key_t api_key);

/// @brief Check a password against the admin password.
/// @param server The server.
/// @param password The password to check. 
/// @return @c true if the provided password matches the admin password.
/// @return @c false otherwise.
bool server_check_admin_password(server_t *server, char const *password);

/// @brief Deletes a session, logging out an user
/// @param server The server.
/// @param token The session token to invalidate.
/// @return @c true on successful log out.
/// @return @c false if the token is invalid.
bool server_logout(server_t *server, token_t token);

/// @brief Verifies a token, returning its owning user ID.
/// @param server The server.
/// @param token The session token to verify.
/// @return @c -1 if the session token is invalid.
/// @return The ID of the user that owns @p token if it is valid.
serial_t server_verify_token(server_t *server, token_t token);

#endif // SERVER_STATE_H
