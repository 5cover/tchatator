/// @file
/// @author Raphaël
/// @brief json-c library
/// @date 03/02/2025

#ifndef JSON_C
#define JSON_C

#include <json-c/json.h> // IWYU pragma: export

// Some error-catching mechanisms

#ifdef __GNUC__
#define json_object_new_int(i) __extension__({_Static_assert(sizeof (i) == sizeof (int32_t), "use json_object_new_int64"); json_object_new_int(i); })
#define json_object_new_int64(i) __extension__({_Static_assert(sizeof (i) == sizeof (int64_t), "use json_object_new_int"); json_object_new_int64(i); })
#endif

#endif // JSON_C
