# Todo

## right now

- [ ] fix current single test
- [ ] finish setting up database
- [ ] progressively reenable tests (disabled by appending `.` to the extension)

---

- [x] change admin user id representation : there is a duplication : admin role + id = 0. Solution: say user with ID 0 is "root aka the super-user" and has admin permissions. Other admins can be defined.
- [x] remove session managment: it only complicate the protocol: api keys are enough of an an authentification
- [ ] use asprintf instead of bufffer_size and vstrfmt
- [ ] use bcrypt hash for admin password
- [ ] check rate limit before request parsing (limit processing to a minimum in case of spam)
- [ ] use uuid4_t in client?
- [ ] write client tests
- [ ] implement protocol stubs
- [ ] mdoc style manpage
- [ ] gendoc workflow github pages from 413
- [ ] remove JSONf. no need for a custom format. build a json object from placeholders instead. maybe using json pointers.

## bugs

- [x] does output formet check the values
- [x] when object fmt input or output, order of JSON objects is not defined, so for multiple arguments, we don't know which argument will go where
  - [x] solution : for output : only check : no more varargs : use global static variables set in onresponse

## stuff

- [x] let bcrypt be a working submodule.

- [x] propper loggin

- [x] use a `db_memory_owner` pointer on `action_t` pointing to read-only memory instead of `strcpy`ing ourselves into oblivion.

- [x] multiaction requests shouldn't increase rate limits that much\
rate limits should work with IPS so that the server can deny abusive requests immediately.\
so make rate limits part of the socket server system. we don't need them in interactive mode anyway.

- [x] elegant error handling
  - [x] think of an approach that suits JSON, DB and other sources of errors
- [x] doxygen documentation

- [x] prefix out parameter with out_

- [x] invoke build in bigpapoo/gcc-mariadb-pgsql:14.2-sae34
  - [x] make it work with make

- [x] write respone schema

- [x] unit tests
  - [x] uuid

- [ ] allow dynamic reconfiguration via signals. ensure that the server can adapt to changes in configuration without requiring a restart. basically, when recieving SIGUSR1, the server reloads its configuration.

pgsql build

```sh
apt install libjson-c-dev -y
gcc test-pgsql.c -o test-pgsql -I/usr/include/postgresql -L/usr/lib/x86_64-linux-gnu -lpq
```

pgsql:

```c
#include <stdio.h>
#include <stdlib.h>
#include <libpq-fe.h>

int main() {
    // Paramètres de connexion
    const char *conninfo = "host=localhost port=5432 dbname=testdb user=postgres password=password";

    // Connexion à PostgreSQL
    PGconn *conn = PQconnectdb(conninfo);

    // Vérification de la connexion
    if (PQstatus(conn) != CONNECTION_OK) {
        fprintf(stderr, "Erreur de connexion à la base de données : %s\n", PQerrorMessage(conn));
        PQfinish(conn);
        exit(EXIT_FAILURE);
    }

    printf("Connexion réussie à PostgreSQL !\n");

    // Requête SQL
    const char *query = "SELECT id, name FROM users";

    // Exécution de la requête
    PGresult *res = PQexec(conn, query);

    // Vérification des résultats
    if (PQresultStatus(res) != PGRES_TUPLES_OK) {
        fprintf(stderr, "Erreur lors de l'exécution de la requête : %s\n", PQerrorMessage(conn));
        PQclear(res);
        PQfinish(conn);
        exit(EXIT_FAILURE);
    }

    // Récupération et affichage des résultats
    int nrows = PQntuples(res);
    int nfields = PQnfields(res);

    printf("Résultats de la requête :\n");
    for (int i = 0; i < nrows; i++) {
        for (int j = 0; j < nfields; j++) {
            printf("%s ", PQgetvalue(res, i, j));
        }
        printf("\n");
    }

    // Libération des ressources et fermeture de la connexion
    PQclear(res);
    PQfinish(conn);

    printf("Connexion fermée.\n");
    return EXIT_SUCCESS;
```
