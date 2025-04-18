/// @file
/// @author Raphaël
/// @brief JSON-C extra functions - Implementation
/// @date 29/01/2025

#include "tchatator413/json-helpers.h"
#include "json_object.h"

static inline uint16_t clamp_uint16(int32_t x) {
    int32_t const t = x < 0 ? 0 : x;
    return t > UINT16_MAX ? UINT16_MAX : (uint16_t)t;
}

bool json_object_get_uint16_strict(json_object const *jo, uint16_t *out) {
    if (!json_object_is_type(jo, json_type_int)) return false;
    if (out) *out = clamp_uint16(json_object_get_int(jo));
    return true;
}

bool json_object_get_int_strict(json_object const *jo, int32_t *out) {
    if (!json_object_is_type(jo, json_type_int)) return false;
    if (out) *out = json_object_get_int(jo);
    return true;
}

bool json_object_get_int64_strict(json_object const *jo, int64_t *out) {
    if (!json_object_is_type(jo, json_type_int)) return false;
    if (out) *out = json_object_get_int64(jo);
    return true;
}

bool json_object_get_string_strict(json_object *jo, slice_t *out) {
    if (!json_object_is_type(jo, json_type_string)) return false;
    if (out) {
        out->len = (size_t)json_object_get_string_len(jo);
        out->val = json_object_get_string(jo);
    }
    return true;
}
