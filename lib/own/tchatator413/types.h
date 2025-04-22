/// @file
/// @author Raphaël
/// @brief General types - Standalone header
/// @date 23/01/2025

#ifndef TYPES_H
#define TYPES_H

#include "util.h"
#include "uuid.h"
#include <limits.h>
#include <stdint.h>
#include <time.h>

/// @brief An user API key.
typedef uuid4_t api_key_t;
/// @brief A page number (1..2^31-1)
typedef int32_t page_number_t;
/// @brief A PosrgreSQL SERIAL primary key value starting at 1. (1..2^31-1)
typedef int32_t serial_t;

/// @brief Maximum length of an user e-mail.
#define EMAIL_LENGTH 319
/// @brief Maximum length of a client (member) user pseudo.
#define PSEUDO_LENGTH 255

typedef struct {
    api_key_t api_key; ///< @brief User API key
    char const *password; ///< @brief Provided password (@c NULL if no password provided)
} constr_t;

/// @brief A bit flags enumeration representing the roles of an user.
typedef enum ATTR_FLAG_ENUM {
    role_admin = 1 << 0,  ///< @brief Administrator role.
    min_role = role_admin, ///< @brief Smallest value of the enumeration.
    role_member = 1 << 1, ///< @brief Member role.
    role_pro = 1 << 2, ///< @brief Professional role.
    role_all = role_admin | role_member | role_pro, ///< @brief All roles.
    max_role = role_all, ///< @brief Largest value of the enumeration.
} role_t;

/// @brief Information about the identity of an user.
typedef struct {
    /// @brief The roles of the user.
    role_t role;
    /// @brief The ID of the user or @c 0 for the root user.
    serial_t id;
} user_identity_t;

/// @brief Represents a message in the chat application.
///
/// This struct contains the necessary information to represent a message,
/// including the message content, the time it was sent, and various ages
/// related to the message's read, edit, and deletion status. It also
/// includes the IDs of the sender and recipient of the message.
typedef struct {
    char *content;
    time_t sent_at;
    int32_t read_age; ///< @brief @c 0 if not relevant.
    int32_t edited_age; ///< @brief @c 0 if not relevant.
    int32_t deleted_age; ///< @brief @c 0 if not relevant.
    serial_t id, user_id_sender, user_id_recipient;
} msg_t;

#endif // TYPES_H
