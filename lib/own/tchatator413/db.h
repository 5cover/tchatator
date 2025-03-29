/// @file
/// @author Raphaël
/// @brief DAL - Interface
/// @date 23/01/2025

#ifndef DB_H
#define DB_H

#include "errstatus.h"
#include "tchatator413/cfg.h"
#include "types.h"

/// @brief An opaque handle to a database connection.
typedef void db_t;

/// @brief Represents a user in the system.
/// @details This struct contains the various fields that make up a user's profile, such as their ID, kind, email, names, and display name.
typedef struct {
    /// @brief Handle to the database memory owner.
    void *memory_owner_db;
    serial_t id;
    user_kind_t kind;
    char const *email, *last_name, *first_name, *display_name;
} user_t;

/// @brief Represents a list of messages.
/// @details This struct contains a pointer to an array of messages and the number of messages in the array.
typedef struct {
    void *memory_owner_db;
    msg_t *msgs;
    size_t n_msgs;
} msg_list_t;

/// @brief Initialize a database connection.
/// @param cfg The configuration.
/// @param verbosity The verbosity level.
/// @param host The database host name to use for the connection.
/// @param port The database port number to use for the connection.
/// @param database The database name to use for the connection.
/// @param username The database username to use for the connection.
/// @param password The database password to use for the connection.
/// @return A new database connection.
/// @return @c NULL if the connection failed.
db_t *db_connect(cfg_t *cfg, int verbosity, char const *host, char const *port, char const *database, char const *username, char const *password);

/// @brief Destroy a database connection.
/// @param db The database connection to destroy. No-op if @c NULL.
void db_destroy(db_t *db);

/// @brief Cleans up a memory owner.
/// @param memory_owner The memory owner to clean up.
/// @note @c NULL is no-op
void db_collect(void *memory_owner);

/// @brief Verify an API key.
/// @param db The database.
/// @param cfg The configuration.
/// @param out_user Assigned to the identity of the user.
/// @param api_key The API key to verify.
/// @return @ref errstatus_handled A database error occured. A message has been shown. @p out_user is untouched.
/// @return @ref errstatus_error The API key isn't valid. @p out_user is untouched.
/// @return @ref errstatus_ok The API key is valid.
errstatus_t db_verify_user_api_key(db_t *db, cfg_t *cfg, user_identity_t *out_user, api_key_t api_key);

/// @brief Get the ID of an user from their e-mail.
/// @param db The database.
/// @param cfg The configuration.
/// @param email The e-mail to look for.
/// @return The ID of the user with the specified e-mail.
/// @return @ref errstatus_handled A database error occured. A message has been shown. @p out_user is untouched.
/// @return @ref errstatus_error No user of e-mail @p email exists in the database.
serial_t db_get_user_id_by_email(db_t *db, cfg_t *cfg, char const *email);

/// @brief Get the ID of an user from their name.
/// @param db The database.
/// @param cfg The configuration.
/// @param name The name to look for. It can be a member's pseudo or a pro's display name
/// @return The ID of the user with the specified name.
/// @return @ref errstatus_handled A database error occured. A message has been shown. @p out_user is untouched.
/// @return @ref errstatus_error No user of name @p name exists in the database.
serial_t db_get_user_id_by_name(db_t *db, cfg_t *cfg, char const *name);

/// @brief Fills a user record from its ID. If @p user->id is undefined, the behavior is undefined.
/// @param db The database.
/// @param cfg The configuration.
/// @param user The user record to fill.
/// @return @ref errstatus_handled A database error occured. A message has been shown. @p user is untouched.
/// @return @ref errstatus_error No user of ID @p user->id exists in the database. @p user is untouched.
/// @return @ref errstatus_ok Success.
errstatus_t db_get_user(db_t *db, cfg_t *cfg, user_t *user);

/// @brief Retrieves a message from the database. If @p msg->id is undefined, the behavior is undefined.
/// @param db The database.
/// @param cfg The configuration.
/// @param msg The message to be filled with the retrieved data.
/// @param out_memory_owner_db Assigned to the owner of the memory allocated for the message data.
/// @return @ref errstatus_ok The message was successfully retrieved.
/// @return @ref errstatus_error The message could not be retrieved. No message of ID @p msg->id exists in the database.
/// @return @ref errstatus_handled A database error occured. A message has been shown. @p msg and @p out_memory_owner_db are untouched.
errstatus_t db_get_msg(db_t *db, cfg_t *cfg, msg_t *msg, void **out_memory_owner_db);

/// @brief Check a password against the stored hash for an user.
/// @param db The database.
/// @param cfg The configuration.
/// @param user_id The ID of the user to check the password of. Can be @c 0 for the administrator.
/// @param password The clear password to check.
/// @return @ref errstatus_ok The password matched.
/// @return @ref errstatus_error The password didn't match or no user of ID @p user_id exists in the database.
/// @return @ref errstatus_handled On database error (handled).
errstatus_t db_check_password(db_t *db, cfg_t *cfg, serial_t user_id, char const *password);

/// @brief Get the role of an user.
/// @param db The database.
/// @param cfg The configuration.
/// @param user_id The ID of the user to get the role of.
/// @return @ref role_flags_t the role of the user is found.
/// @return @ref errstatus_handled A database error occured. A message has been shown. @p out_user is untouched.
/// @return @ref errstatus_error No user of ID @p user_id exists in the database.
int db_get_user_role(db_t *db, cfg_t *cfg, serial_t user_id);

/// @brief Counts the amount of messages sent from sender to a recipient
/// @param db The database.
/// @param cfg The configuration.
/// @param sender_id The ID of the sender user of the messages. (@c 0 for adminitrator)
/// @param recipient_id The ID of the recipient user of the message.
/// @return The counts of message from @p sender_id to @p recipient_id.
/// @return @ref errstatus_handled A database error occured. A message has been shown.
int db_count_msg(db_t *db, cfg_t *cfg, serial_t sender_id, serial_t recipient_id);

/// @brief Sends a message.
/// @param db The database.
/// @param cfg The configuration.
/// @param sender_id The ID of the sender user
/// @param recipient_id The ID of the recipient user
/// @param content The null-terminated string containing the content of the message.
/// @return @ref serial_t The message ID.
/// @return @ref errstatus_handled A database error occured. A message has been shown. @p out_user is untouched.
/// @return @ref errstatus_error The sender has been blocked from sending messages, either globally or by this particular recipient.
serial_t db_send_msg(db_t *db, cfg_t *cfg, serial_t sender_id, serial_t recipient_id, char const *content);

/// @brief Creates an array with the messages an user has recieved, sorted by sent/edited date, in reverse chronological order.
/// @param db The database.
/// @param cfg The configuration.
/// @param limit The maximum number of messages to fetch.
/// @param offset The offset of the query.
/// @param recipient_id The ID of the user who recieved the messages.
/// @return A message list. The memory owner is @c NULL on error.
msg_list_t db_get_inbox(db_t *db, cfg_t *cfg,
    int32_t limit,
    int32_t offset,
    serial_t recipient_id);

/// @brief Removes a message from the database.
/// @param db The database.
/// @param cfg The configuration.
/// @param msg_id The ID of the message to be removed.
/// @return @ref errstatus_ok The message was successfully removed.
/// @return @ref errstatus_error The message could not be removed.
errstatus_t db_rm_msg(db_t *db, cfg_t *cfg, serial_t msg_id);

/// @brief A transaction body function
///
/// This function is called by @ref db_transaction.
/// If this function returns something else than @ref errstatus_ok, the transaction is @b {aborted}. A @c ROLLBACK is issued. Otherwise the transaction is considered @b {valid}. A @c COMMIT is issued.
typedef errstatus_t (*fn_transaction_t)(db_t *db, cfg_t *cfg, void *ctx);

/// @brief Perform a transaction.
/// @param db The database.
/// @param cfg The configuration.
/// @param body The transaction body function.
/// @param ctx The context to pass to @p {body}. Can be @c {NULL}.
/// @return The error status of @p {body}, or of the BEGIN, COMMIT or ROLLBACK action if they weren't successful.
errstatus_t db_transaction(db_t *db, cfg_t *cfg, fn_transaction_t body, void *ctx);

#endif // DB_H
