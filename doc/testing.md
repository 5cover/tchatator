# Testing

## Test database

- Pro API keys
  1. bb1b5a1f-a482-4858-8c6b-f4746481cffa
  2. 52d43379-8f75-4fbd-8b06-d80a87b2c2b4
- Member API keys
  1. 123e4567-e89b-12d3-a456-426614174000
  2. 9ea59c5b-bb75-4cc9-8f80-77b4ce851a0b
- Three (3) users without API keys
  - 1 public pro
  - 1 private pro
  - 1 member

## The jsonf format

The JSONf format is like JSON expect printf format specifers can appear anywhere.

The contents of a JSONf file is a format string.

## JSON format objects

When the exact output isn't known, one can use *JSON format objects*. Basically, any value can be replaced by a special object with a single key `$fmt_quoted` or `$fmt`, as such:

```json
{
  "$fmt_quoted": "%*ld"
}
```

The value of the property is treated as a *scanf* format string; Each conversion specifier **must** have the assignment-suppression (`*`) modifier, as the values of the argument are not used. `sscanf` is only called to validate the string.

The values retrieved cannot be used, as since JSON object order is undefined, the order of appearance of format objects in the source file, the iteration order in the C function, and the order of the variadic arguments passed to it may not be identical.

This is also the reason why we can't use JSON format objects for input - we have to use the custom `jsonf` format, which treats the input as a string, formats it, and then parses it a JSON. JSON format objects parse the object first, then apply validate formatting, thus we loose the initial order. While testing has shown that it seems to works on practice, and the file order is preserved, JSON object iteration order is not something you want to rely on.

The difference between `$fmt_quoted` and `$fmt` is that `$fmt_quoted` includes the quotes around the string in the format string passed to `sscanf`.
