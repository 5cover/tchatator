# Conventionsù

- Out pointer parameters: `out_` prefix
- Macros : use only where necessary, use `static inline` functions elsewhere (complex macros are a a nightmare to debug)
- Macro naming : lowercase if acts completely transparently (like a function), meaning
  - no non-expression arguments
  - ... otherwise UPPER_CASE
- function pointer typedef naming `fn_` prefix
- typedef naming `_t` suffix
- signed/unsigned types : sizes and port numbers are the only acceptable use case for unsigned types.
