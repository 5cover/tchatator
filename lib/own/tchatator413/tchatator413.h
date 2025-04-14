/// @file
/// @author Raphaël
/// @brief Facade interface
/// @date 1/02/2025

#ifndef TCHATATOR_413_H
#define TCHATATOR_413_H

#include "action.h"
#include "cfg.h"
#include "db.h"
#include <json-c.h>
#include <memlst.h>

/// @brief An event handler for when the server has parsed an action.
typedef void (*on_action_fn)(const action_t *action, void *ctx);
/// @brief An event handler for when the server has interpeted an action.
typedef void (*on_response_fn)(const response_t *response, void *ctx);

/// @brief Interpret a request.
/// @param obj_input The request JSON object.
/// @param cfg The configuration.
/// @param db The database.
/// @param on_action Event handler to call when the action is parsed. Cab be @c NULL.
/// @param on_response Event handler to call when the action is interpreted. Cab be @c NULL.
/// @param on_ctx The contect to pass to the previous event handlers.
/// @return The JSON response object to the request.
json_object *tchatator413_interpret(json_object *obj_input, cfg_t *cfg, db_t *db, on_action_fn on_action, on_response_fn on_response, void *on_ctx);

/// @brief Run the server in interactive mode.
/// @param cfg The configuration.
/// @param db The database.
/// @param argc Argument count.
/// @param argv Argument vector.
/// @return The exit code of the server.
int tchatator413_run_interactive(cfg_t *cfg, db_t *db, int argc, char **argv);

/// @brief Run the server in socket mode.
/// @param cfg The configuration.
/// @param db The database.
/// @return The exit code of the server.
int tchatator413_run_socket(cfg_t *cfg, db_t *db);

#endif // TCHATATOR_413_H
