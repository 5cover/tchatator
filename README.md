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

Run tests:

```bash
make test
bin/test
```
