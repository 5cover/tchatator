/// @file
/// @author Raphaël
/// @brief Tchatator413 Facade - Server implementation
///
/// Uses Unix sockets.
///
/// @date 1/02/2025

#include "json-c.h"
#include "stb_ds.h"
#include "tchatator413/tchatator413.h"
#include <arpa/inet.h>
#include <errno.h>
#include <signal.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <unistd.h>

#define SERVER_ADDR "127.0.0.1"

typedef struct {
    /// @brief Timestamp of the last request.
    time_t last_request_at;
    /// @brief Number of requests performed since an hour.
    int n_requests_h;
    /// @brief Number of requests performed since a minute.
    int n_requests_m;
} user_stats_t;

static inline void json_object_write(json_object *jo, cfg_t *cfg, int fd) {
    size_t len;
    ssize_t bytes_written;
    char const *output = json_object_to_json_string_length(jo, JSON_C_TO_STRING_PLAIN, &len);
    ++len; // include null terminator
    cfg_log(cfg, log_info, "preparing to write %zu bytes of response\n", len);

    do {
        bytes_written = write(fd, output, len);
        if (-1 == bytes_written) errno_exit("write");
        len -= (size_t)bytes_written;
        output += bytes_written;
        cfg_log(cfg, log_info, "wrote %zd bytes, %zu remaining\n", bytes_written, len);
    } while (len > 0);
}

static inline void interpret_request(cfg_t *cfg, db_t *db, int fd) {
    cfg_log(cfg, log_info, "interpreting request from fd %d\n", fd);

    char buf[BUFSIZ] = { 0 };
    ssize_t bytes_read = read(fd, buf, sizeof buf - 1);
    if (bytes_read > 0) buf[bytes_read] = '\0';

    json_object *jo_input = json_tokener_parse(buf);
    // if !jo_input : invalid JSON recieved

    cfg_log(cfg, log_info, "received json input, interpreting request\n");

    json_object *jo_output = tchatator413_interpret(jo_input, cfg, db, NULL, NULL, NULL);
    json_object_put(jo_input);

    json_object_write(jo_output, cfg, fd);
    json_object_put(jo_output);

    cfg_log(cfg, log_info, "request interpretation completed for fd %d\n", fd);
}

typedef struct {
    in_addr_t key;
    user_stats_t value;
} turnstile_entry;

/// @brief Checks and increments the rate limit for the specified user.
/// @param cfg The configuration.
/// @param turnstile The turnstile hash map.
/// @param in_addr The source IP of the request.
/// @return @c 0 The turnstile passes (the rate limit hasn't been reached)
/// @return > @c 0 The turnstile blocks (the rate limit has been reached). An error has been put. The return value is time of the next allowed request.
static inline time_t turnstile_rate_limit(cfg_t *cfg, turnstile_entry *turnstile, in_addr_t in_addr) {

    time_t const t = time(NULL);

    int i = hmgeti(turnstile, in_addr);
    if (i == -1) {
        hmput(turnstile, in_addr,
            ((user_stats_t) {
                .last_request_at = t,
                .n_requests_h = 1,
                .n_requests_m = 1,
            }));
        return 0;
    }

    user_stats_t *p_stats = &turnstile[i].value;

    time_t time_since_last_request = t - p_stats->last_request_at;
    p_stats->last_request_at = t;

    if (time_since_last_request >= 60) p_stats->n_requests_m = 0;
    if (time_since_last_request >= 3600) p_stats->n_requests_h = 0;

    ++p_stats->n_requests_m;
    ++p_stats->n_requests_h;

    if (p_stats->n_requests_m >= cfg_rate_limit_m(cfg)) return t + 60 - time_since_last_request;
    if (p_stats->n_requests_h >= cfg_rate_limit_h(cfg)) return t + 3600 - time_since_last_request;
    return 0;
}

static int gs_sock = -1;

static inline void close_sock(int sig) {
    (void)sig;
    if (gs_sock == -1) return;
    close(gs_sock); // error left unreported. intentionally.
    gs_sock = -1;
}

int tchatator413_run_socket(cfg_t *cfg, db_t *db) {
    // create hashmap<ip,user_stats_t> turnstile
    turnstile_entry *turnstile = NULL;

    cfg_log(cfg, log_info, "initializing server...\n");
    // Acquérir le socket
    gs_sock = socket(AF_INET, SOCK_STREAM, 0);
    if (-1 == gs_sock) errno_exit("socket");
    int sock_opt = 1;
    setsockopt(gs_sock, SOL_SOCKET, SO_REUSEADDR, &sock_opt, sizeof sock_opt);
    cfg_log(cfg, log_info, "socket created with fd %d\n", gs_sock);

    // Programmer sa libération sur Ctrl+C
    if (SIG_ERR == signal(SIGINT, close_sock)) errno_exit("signal");
    if (SIG_ERR == signal(SIGTERM, close_sock)) errno_exit("signal");

    struct sockaddr_in server_addr = {
        .sin_addr.s_addr = inet_addr(SERVER_ADDR),
        .sin_family = AF_INET,
        .sin_port = htons(cfg_port(cfg)),
    };

    if (-1 == bind(gs_sock, (struct sockaddr *)&server_addr, sizeof(server_addr))) {
        perror("bind");
        return EXIT_FAILURE;
    }
    cfg_log(cfg, log_info, "socket bound to address " SERVER_ADDR ":%hu\n", cfg_port(cfg));

    if (-1 == listen(gs_sock, cfg_backlog(cfg))) {
        perror("listen");
        return EXIT_FAILURE;
    }
    cfg_log(cfg, log_info, "listening with backlog of %d\n", cfg_backlog(cfg));

    cfg_log(cfg, log_info, "server started on " SERVER_ADDR " port %hu\n", cfg_port(cfg));

    struct sockaddr_in addr_connection;
    int size = sizeof addr_connection;

    while (true) {
        cfg_log(cfg, log_info, "waiting for new connection...\n");
        int fd = accept(gs_sock, (struct sockaddr *)&addr_connection, (socklen_t *)&size);
        if (-1 == fd) {
            // If a signal interrupted accept().
            if (EINTR == errno) {
                // If the signal handler decided to exit
                if (gs_sock == -1) break;
                cfg_log(cfg, log_info, "accept interrupted by signal, continuing...\n");
                continue;
            }
            errno_exit("accept");
        }

        time_t next_request_at = turnstile_rate_limit(cfg, turnstile, addr_connection.sin_addr.s_addr);
        if (next_request_at == 0) {
            cfg_log(cfg, log_info, "accepted new connection from %s:%d with fd %d\n",
                inet_ntoa(addr_connection.sin_addr),
                ntohs(addr_connection.sin_port),
                fd);
            interpret_request(cfg, db, fd);
        } else {
            cfg_log(cfg, log_info, "refusing connection from %s:%d with fd %d : rate limit reached\n",
                inet_ntoa(addr_connection.sin_addr),
                ntohs(addr_connection.sin_port),
                fd);
            response_t r = response_for_rate_limit(next_request_at);
            json_object *resp = response_to_json(&r);
            json_object_write(resp, cfg, fd);
            json_object_put(resp);
        }

        cfg_log(cfg, log_info, "closing connection fd %d\n", fd);
        close(fd);
    }

    cfg_log(cfg, log_info, "server exiting...\n");

    hmfree(turnstile);

    return EX_OK;
}
