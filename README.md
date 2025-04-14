# Tchatator

## Build

Dependencies:

- gcc
- libjson-c-dev
- libpq-dev
- libbcrypt (vendored)

Rename the env file and fill placeholder values

```bash
cp env .env
# edit .env...
```

Build and run:

```bash
make # build the client and the server
./bin/tchatator413
```

## Tests

1. Provide test.env in the same directory as the Makefile
2. run `make test` to run the tests. Or run `make bin/test` to just build the test binary.

## Conventions

Naming: use `snake_case` unless specified otherwise.

### Naming variables

Naming: use hugarian notation with prefixes.

prefix|use for|means|example
-|-|-|-
`out_`|out pointer parameters|this is an out parameter; the dereferenced value is undefined at the start of the function. The value may be assigned according to the function's contract. Implies `p_` after being assigned to.|`out_user`
`g_`|globals|this is a **g**lobal variable|`g_global_state`
`s_`|statics|this is a **s**tatic variable|`static_state`
`p_`|dereferenceable pointers (not `void*` or opaque handles, or types not meant to be dereferenced by calling code, like `FILE*`)|this is a **p**ointer that can be dereferenced|`p_test`
`d_`|indexable pointers or arrays: derefenceable with an offset; implies `p_`|this can be in**d**exed|`d_array`
`z_`|null-terminated buffers: indexable, terminated by a null byte; implies `d_`|this is a null-terminated buffer|`z_string`
`jo_`|JSON objects (`struct json_object*`, from `json-c`)|this is a **J**SON **o**bject handle|`jo_user`

Prefixes can be combined in table order (such as `gsp_` for a global static pointer).

Prefixes can be repeated when applicable such as `pp_` for double pointers. Example:

```c
// modifiers are ordered based on the type syntax. here z is first because the char pointer star comes first.
char **zd_argv; // z for the inner strings, d for the indesable array of pointers
```

Note: `p_` does not apply to double-pointers to opaque or void pointers. Example:

```c
opaque_t **opaque; // not p_opaque
```

### Typedefs

Naming: use suffixes

suffix|use for|means|example
-|-|-|-
`_t`|regular typedefs|this is a type|`db_t`
`_fn`|function pointer typedefs|this is a function pointer type. values of this type can be called as functions|`dtor_fn`

### Function-like macros

Use only where necessary, use `static inline` functions elsewhere (complex macros are a a nightmare to debug)

Naming: use `snake_case` if a function-like macro acts completely transparently (like a function), meaning:

- [ ] no non-expression arguments
- [ ] no control flow affecting the caller
- [ ] no risk of double evaluation of side-effectful arguments
- [ ] can be used as a function (except when taking its address, but that's acceptable because doing so results in a compiler error anyway)

Otherwise, use `CONST_CASE`.

```c
#define streq(x, y) (strcmp((x), (y)) == 0)
#define MAX(a, b) ((a) > (b) ? (a) : (b))
```

### Object-like macros and macros that don't expand to anything

Naming: `CONST_CASE`.

```c
#define MEMLST_IMPL
#define MSG_CONTENT "I. Am. A. God. (i am a god)"
```

### X-Macros

Naming: `X_CONST_CASE` (X_ prefix).

X-Macros shall accept arguments for each iteration macro, instead of expecting macros of specific names to exist.

```c
#define X_42226 O O O O H O O H O O H O O H O O O O O O // wrong
#define X_42226(O, H) O O O O H O O H O O H O O H O O O O O O // ok
```

### Other (magic) macros

`CONST_CASE`. Make sure to provide meaningful and semantic names to such macros, even if this makes the code more verbose in appearance. Use them sparsely.

```c
#define FILE_LINE_PARAMS , char const *file, int line
#define FILE_LINE_ARGS , file, line
```

### Signed/unsigned types

Use unsigned types only for:

- Byte sizes (size_t...)
- port numbers (uint16_t)

### Enum values

Enums have two casing conventions:

#### Regular enum values

Use `snake_case` with a disctinct enum name prefix. This can be the enum name, or a clear, distinct prefix.

```c
// Enum name prefix
typedef enum {
    state_todo,
    state_doing,
    state_done,
} state_t;

// Clear, distinct prefix that doesn't hinder readability (more concise than a full log_lvl prefix)
typedef enum {
    log_error,
    log_info,
    log_warning,
} log_lvl_t;
```

#### Enum "min" and "max" values

Enum min and max values are named by `min_<prefix>` and `max_<prefix>`. They are not required. But if one is present, the other must be, too.

```c
typedef enum {
    errstatus_handled = -1,
    min_errstatus = errstatus_handled,
    errstatus_error,
    errstatus_ok,
    max_errstatus = errstatus_ok // notice how we omit the trailing comma here -- since no value should be defined below the max.
} errstatus_t;
```

Why `min_errstatus` and not `errstatus_min`? Because `min_errstatus` is not a regular value, it serves a special purpose and the name intends to reflect that by separating them from other values. Also it prevents conflicts if there's a value named `min` or `max`.

#### FancierMacros(TM) (anonymous and free-standing) enum values

Use `CONST_CASE` for anonymous enum values that are not used as a type in a declaration (so they're just integer constants screamed into the gaping void) No particular prefix is required.

This is preferred over successive integer object-like macros, because:

- enums allow grouping values logically
- enums provide automatic incrementation

```c
enum {
    EX_NODB = EX__MAX + 1,
};
```

Note: in this enum, `state_unconnected` and `state_connected` are in `snake_case` and not `CONST_CASE`. This is because the enum is declared as a type of `tag` field, so it's not "freefstating" This means there is still a form of semantics, rather than just a set of named integer constants.

```c
static struct {
    enum {
        state_unconnected,
        state_connected,
    } tag;
    union {
        int unconnected;
        int connected;
    } data;
} gs_state;
```
