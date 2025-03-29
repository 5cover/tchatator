/// @file
/// @author Raphaël
/// @brief General constants - Standalone header
/// @date 23/01/2025

#ifndef CONST_H
#define CONST_H

#include <sysexits.h>

/// @brief Additional sysexits codes.
enum {
    /// @brief Exit code for when the database connection failed.
    EX_NODB = EX__MAX + 1,
};

/// @brief X-macro that expands to the list of actions
#define X_ACTIONS(X) \
    X(login)         \
    X(logout)        \
    X(whois)         \
    X(send)          \
    X(motd)          \
    X(inbox)         \
    X(outbox)        \
    X(edit)          \
    X(rm)            \
    X(block)         \
    X(unblock)       \
    X(ban)           \
    X(unban)

/// @brief The name of the program.
#define PROG "tchatator-server"

/// @brief The program's helpstring.
#define HELP PROG " - A Tchatator413 implementation\n\
\n\
SYNOPSIS\n\
    " PROG " -[qv]... [-c FILE]\n\
    " PROG " -[qv]... [-c FILE] -i [REQUEST]\n\
    " PROG " --dump-config\n\
    " PROG " --help\n\
    " PROG " --version\n\
\n\
DESCRIPTION\n\
    A Tchatator413-compliant server based on Unix sockets. Accepts JSON, responds with JSON.\n\
\n\
    Mandatory arguments to long options are mandatory for short options too.\n\
\n\
    -q, --quiet        More quiet (can be repeated)\n\
    -v, --verbose      More verbose (can be repeated)\n\
    -i, --interactive  Run in interactive mode (read from STDIN or argument)\n\
    -c, --config=FILE  Configuration file\n\
    --dump-config      Dump current configuration\n\
    --help             Show this help\n\
    --version          Show version\n\
\n\
REQUIRED ENVIRONMENT VARIABLES\n\
    DB_HOST            DB host\n\
    PGDB_PORT          DB port\n\
    DB_NAME            DB name\n\
    DB_USER            DB username\n\
    DB_ROOT_PASSWORD   DB password\n\
    ADMIN_API_KEY      Administrator API key\n\
    ADMIN_PASSWORD     Administrator password"

/// @brief The program's versionstring.
#define VERSION PROG " 1.0.1"

#endif // CONST_H
