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
