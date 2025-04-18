/// @file
/// @author Raphaël
/// @brief Testing - implementation of testing utilities
/// @date 1/02/2025

#include "tests.h"
#include "tchatator413/json-helpers.h"
#include <stdarg.h>
#include <sys/types.h>
#include <sysexits.h>

char _gd_test_case_eq_uuid_repr[UUID4_REPR_LENGTH * 2];

test_t *base_on_action(void *test) {
    test_t *t = (test_t *)test;
    ++t->n_actions;
    return t;
}

test_t *base_on_response(void *test) {
    test_t *t = (test_t *)test;
    ++t->n_responses;
    return t;
}

void test_case_n_actions(test_t *test, int expected) {
    test_case_count(&test->t, test->n_actions, expected, "action");
    test_case_count(&test->t, test->n_responses, expected, "response");
}

static inline char const *json_object_get_fmt(json_object *jo) {
    json_object *jo_fmt;
    return json_object_is_type(jo, json_type_object) && json_object_object_length(jo) == 1
        ? json_object_object_get_ex(jo, "$fmt_quoted", &jo_fmt)
            ? min_json(jo_fmt)
            : json_object_object_get_ex(jo, "$fmt", &jo_fmt)
            ? json_object_get_string(jo_fmt)
            : NULL
        : NULL;
}

json_object *load_json(char const *input_filename) {
    json_object *jo = json_object_from_file(input_filename);
    if (!jo) {
        fprintf(stderr, LOG_FMT_JSON_C("failed to load %s", input_filename));
        exit(EX_DATAERR);
    }
    return jo;
}

json_object *load_jsonf(const char *input_filename, ...) {
    FILE *finput = fopen(input_filename, "r");
    if (!finput) errno_exit("fopen");
    char *input_fmt = fslurp(finput);
    fclose(finput);
    if (!input_fmt) errno_exit("fslurp");

    // possible optimization : reuse input_fmt memory: call sprintf with null to get size, then realloc it
    va_list ap;
    va_start(ap, input_filename);
    char *input = vstrfmt(input_fmt, ap);
    va_end(ap);
    free(input_fmt);

    json_object *jo = json_tokener_parse(input);
    if (!jo) {
        fprintf(stderr, LOG_FMT_JSON_C("failed to load %s", input_filename));
        exit(EX_DATAERR);
    }
    free(input);

    return jo;
}

bool json_object_eq_fmt(json_object *jo_actual, json_object *jo_expected) {
    // Special handling of our JSON pattern matching mechanism
    const char *fmt = json_object_get_fmt(jo_expected);
    if (fmt) {
        // json object -> arguments
        char const *str = json_object_get_string(jo_actual);
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wformat-security" // The format string that we get is a the contents of a local file, under our control.
        int n = sscanf(str, fmt);
#pragma GCC diagnostic pop
        assert(n == 0 || n == EOF);
        return n != EOF;
    }

    json_type const actual_type = json_object_get_type(jo_actual), expected_type = json_object_get_type(jo_expected);
    if (actual_type != expected_type) return false;
    switch (actual_type) {
    case json_type_null: return true;
    case json_type_boolean: return json_object_get_boolean(jo_actual) == json_object_get_boolean(jo_expected);
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wfloat-equal" // We don't compare the values, we compare the representations. No need for an epsilon.
    case json_type_double: return json_object_get_double(jo_actual) == json_object_get_double(jo_expected);
#pragma GCC diagnostic pop
    case json_type_int: return json_object_get_int(jo_actual) == json_object_get_int(jo_expected);
    case json_type_object:
        // Two JSON objects are equal if they contain the same properties, regardless of order
        if (json_object_object_length(jo_actual) != json_object_object_length(jo_expected)) return false;
        json_object_object_foreach(jo_actual, key, jo_actual_value) {
            json_object *jo_expected_value;
            if (!json_object_object_get_ex(jo_expected, key, &jo_expected_value)
                || !json_object_eq_fmt(jo_actual_value, jo_expected_value)) return false;
        }
        return true;
    case json_type_array: {
        size_t const actual_len = json_object_array_length(jo_actual), expected_len = json_object_array_length(jo_expected);
        bool equal = actual_len == expected_len;
        for (size_t i = 0; equal && i < actual_len; ++i) {
            equal = json_object_eq_fmt(
                json_object_array_get_idx(jo_actual, i),
                json_object_array_get_idx(jo_expected, i));
        }
        return equal;
    }
    case json_type_string:
        return streq(json_object_get_string(jo_actual), json_object_get_string(jo_expected));
    default: unreachable();
    }
}

bool test_output_json_file(test_t *test, json_object *jo_output, char const *expected_output_filename) {
    json_object *jo_output_expected = json_object_from_file(expected_output_filename);
    if (!jo_output_expected) {
        fprintf(stderr, LOG_FMT_JSON_C("failed to parse test output JSON file at '%s'", expected_output_filename));
        exit(EX_DATAERR);
    }

    bool ok = json_object_eq_fmt(jo_output, jo_output_expected);
    json_object_put(jo_output_expected);
    return test_case_wide(&test->t, ok, "%s == cat %s", min_json(jo_output), expected_output_filename);
}
