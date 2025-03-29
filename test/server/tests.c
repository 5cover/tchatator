/// @file
/// @author Raphaël
/// @brief Testing - implementation of testing utilities
/// @date 1/02/2025

#include "tests.h"
#include <tchatator413/json-helpers.h>
#include <stdarg.h>
#include <sys/types.h>
#include <sysexits.h>

char _g_test_case_eq_uuid_repr[UUID4_REPR_LENGTH];

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

static inline char const *json_object_get_fmt(json_object *obj) {
    json_object *fmt_obj;
    return json_object_is_type(obj, json_type_object) && json_object_object_length(obj) == 1
        ? json_object_object_get_ex(obj, "$fmt_quoted", &fmt_obj)
            ? min_json(fmt_obj)
            : json_object_object_get_ex(obj, "$fmt", &fmt_obj)
            ? json_object_get_string(fmt_obj)
            : NULL
        : NULL;
}

static inline json_object *reduce_fmt_v(json_object *obj, va_list *ap) {
    char const *fmt = json_object_get_fmt(obj);
    if (fmt) {
        // arguments -> json object
        char *fmted = vstrfmt(fmt, *ap);
        if (!fmted) errno_exit("vstrfmt");
        // va_advance_printf(ap, fmt);

        json_object_put(obj);
        obj = json_tokener_parse(fmted);
        free(fmted);
        return obj;
    }

    switch (json_object_get_type(obj)) {
    case json_type_null: break;
    case json_type_boolean: break;
    case json_type_double: break;
    case json_type_int: break;
    case json_type_object: {
        struct lh_entry *entry;
        entry = json_object_get_object(obj)->head;
        while (entry) {
            entry->v = reduce_fmt_v(lh_entry_v(entry), ap);
            entry = entry->next;
        }
        break;
    }
    case json_type_array: {
        size_t len = json_object_array_length(obj);
        for (size_t i = 0; i < len; ++i) {
            json_object *old_obj = json_object_array_get_idx(obj, i);
            json_object *new_obj = reduce_fmt_v(old_obj, ap);
            if (new_obj != old_obj) json_object_array_put_idx(obj, i, new_obj);
        }
    }
    case json_type_string: break;
    }

    return obj;
}

json_object *load_json(char const *input_filename) {
    json_object *obj = json_object_from_file(input_filename);
    if (!obj) {
        fprintf(stderr, LOG_FMT_JSON_C("failed to load %s", input_filename));
        exit(EX_DATAERR);
    }
    return obj;
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

    json_object *obj = json_tokener_parse(input);
    if (!obj) {
        fprintf(stderr, LOG_FMT_JSON_C("failed to load %s", input_filename));
        exit(EX_DATAERR);
    }
    free(input);

    return obj;
}

bool json_object_eq_fmt(json_object *obj_actual, json_object *obj_expected) {
    // Special handling of our JSON pattern matching mechanism
    const char *fmt = json_object_get_fmt(obj_expected);
    if (fmt) {
        // json object -> arguments
        char const *str = json_object_get_string(obj_actual);
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wformat-security" // The format string that we get is a the contents of a local file, under our control.
        int n = sscanf(str, fmt);
#pragma GCC diagnostic pop
        assert(n == 0 || n == EOF);
        return n != EOF;
    }

    json_type const actual_type = json_object_get_type(obj_actual), expected_type = json_object_get_type(obj_expected);
    if (actual_type != expected_type) return false;
    switch (actual_type) {
    case json_type_null: return true;
    case json_type_boolean: return json_object_get_boolean(obj_actual) == json_object_get_boolean(obj_expected);
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wfloat-equal" // We don't compare the values, we compare the representations. No need for an epsilon.
    case json_type_double: return json_object_get_double(obj_actual) == json_object_get_double(obj_expected);
#pragma GCC diagnostic pop
    case json_type_int: return json_object_get_int(obj_actual) == json_object_get_int(obj_expected);
    case json_type_object:
        // Two JSON objects are equal if they contain the same properties, regardless of order
        if (json_object_object_length(obj_actual) != json_object_object_length(obj_expected)) return false;
        json_object_object_foreach(obj_actual, k, a_v) {
            json_object *e_v;
            if (!json_object_object_get_ex(obj_expected, k, &e_v) || !json_object_eq_fmt(a_v, e_v)) return false;
        }
        return true;
    case json_type_array: {
        size_t const actual_len = json_object_array_length(obj_actual), expected_len = json_object_array_length(obj_expected);
        bool equal = actual_len == expected_len;
        for (size_t i = 0; equal && i < actual_len; ++i) {
            equal = json_object_eq_fmt(
                json_object_array_get_idx(obj_actual, i),
                json_object_array_get_idx(obj_expected, i));
        }
        return equal;
    }
    case json_type_string:
        return streq(json_object_get_string(obj_actual), json_object_get_string(obj_expected));
    default: unreachable();
    }
}

bool test_output_json_file(test_t *test, json_object *obj_output, char const *expected_output_filename) {
    json_object *obj_output_expected = json_object_from_file(expected_output_filename);
    if (!obj_output_expected) {
        fprintf(stderr, LOG_FMT_JSON_C("failed to parse test output JSON file at '%s'", expected_output_filename));
        exit(EX_DATAERR);
    }

    bool ok = json_object_eq_fmt(obj_output, obj_output_expected);
    json_object_put(obj_output_expected);
    return test_case_wide(&test->t, ok, "%s == cat %s", min_json(obj_output), expected_output_filename);
}
