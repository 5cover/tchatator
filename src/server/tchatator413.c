/// @file
/// @author Raphaël
/// @brief Tchatator413 Facade - Implementation
/// @date 1/02/2025

#include <assert.h>
#include <getopt.h>
#include <stdio.h>
#include "tchatator413/json-helpers.h"
#include "tchatator413/tchatator413.h"
#include <unistd.h>

int tchatator413_run_interactive(cfg_t *cfg, db_t *db, int argc, char **argv) {
    memlst_t *mem = memlst_init();

    json_object *const jo_input = memlst_add(&mem, dtor_json_object,
        optind < argc
            ? json_tokener_parse(argv[optind])
            : json_object_from_fd(STDIN_FILENO));

    if (!jo_input) {
        cfg_log(cfg, log_info, LOG_FMT_JSON_C("failed to parse input"));
        CLEAN_RETURN(mem, EX_DATAERR);
    }

    json_object *jo_output = memlst_add(&mem, dtor_json_object,
        tchatator413_interpret(jo_input, cfg, db, NULL, NULL, NULL));

    // Results

    puts(min_json(jo_output));

    CLEAN_RETURN(mem, EX_OK);
}

static inline json_object *act(json_object const *jo_action, cfg_t *cfg, db_t *db, on_action_fn on_action, on_response_fn on_response, void *on_ctx) {
    memlst_t *mem = memlst_init();

    action_t action = action_parse(&mem, cfg, db, jo_action);
    if (on_action) on_action(&action, on_ctx);

    response_t response = action_evaluate(&action, &mem, cfg, db);
    if (on_response) on_response(&response, on_ctx);

    return response_to_json(&response);
}

json_object *tchatator413_interpret(json_object *jo_input, cfg_t *cfg, db_t *db, on_action_fn on_action, on_response_fn on_response, void *on_ctx) {
    json_object *jo_output;

    json_type const input_type = json_object_get_type(jo_input);
    switch (input_type) {
    case json_type_array: {
        size_t const len = json_object_array_length(jo_input);
        jo_output = json_object_new_array_ext((int)len);
        for (size_t i = 0; i < len; ++i) {
            json_object const *const action = json_object_array_get_idx(jo_input, i);
            assert(action);
            json_object_array_add(jo_output, act(action, cfg, db, on_action, on_response, on_ctx));
        }
        assert(len == json_object_array_length(jo_output)); // Same amount of input and output actions
        break;
    }
    case json_type_object:
        jo_output = json_object_new_array_ext(1);
        json_object_array_add(jo_output, act(jo_input, cfg, db, on_action, on_response, on_ctx));
        break;
    default:
        jo_output = json_object_new_array_ext(1);
        json_object_array_add(jo_output,
            response_to_json(&(response_t) {
                .type = action_type_error,
                .body.error = {
                    .type = action_error_type_type,
                    .info.type = {
                        .expected = json_type_object,
                        .jo_actual = jo_input,
                        .location = "request",
                    },
                },
            }));
    }

    return jo_output;
}
