#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <errno.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <pthread.h>
#include <arpa/inet.h>
#include "server.h"


static sensor_t *sensors[MAX_SENSORS] = {0};
static int sensor_count = 0;
static int next_sensor_id = 1;
static client_t *operators[MAX_OPERATORS] = {0};
static int operator_count = 0;
static int next_operator_id = 1;

static alert_t alerts[MAX_ALERTS] = {0};
static int alert_count = 0;

static pthread_mutex_t sensor_mutex = PTHREAD_MUTEX_INITIALIZER;
static pthread_mutex_t operator_mutex = PTHREAD_MUTEX_INITIALIZER;
static pthread_mutex_t alert_mutex = PTHREAD_MUTEX_INITIALIZER;
static pthread_mutex_t log_mutex = PTHREAD_MUTEX_INITIALIZER;

FILE *log_file;

//--------------------------------------------------------------------------------------------------------------------
// Alert management
void store_alert(int sensor_id, const char *alert_type, const char *message) {
    pthread_mutex_lock(&alert_mutex);
    if (alert_count < MAX_ALERTS) {
        alert_t *a = &alerts[alert_count++];
        a->sensor_id = sensor_id;
        strncpy(a->alert_type, alert_type, sizeof(a->alert_type) - 1);
        strncpy(a->message, message, sizeof(a->message) - 1);

        time_t now = time(NULL);
        struct tm *tm_info = gmtime(&now);
        strftime(a->timestamp, sizeof(a->timestamp), "%Y-%m-%dT%H:%M:%S", tm_info);
    }
    pthread_mutex_unlock(&alert_mutex);
}

void broadcast_alert_to_operators(const char *alert_msg) {
    pthread_mutex_lock(&operator_mutex);
    for (int i = 0; i < operator_count; i++) {
        if (operators[i] != NULL) {
            send(operators[i]->fd, alert_msg, strlen(alert_msg), MSG_NOSIGNAL);
        }
    }
    pthread_mutex_unlock(&operator_mutex);
}

//--------------------------------------------------------------------------------------------------------------------
// Helper functions
void log_event(client_t *client, const char *received, const char *sent) {
    pthread_mutex_lock(&log_mutex);
    if (client->type == CONN_SENSOR) {
        sensor_t *sensor = (sensor_t *)client->data;
        fprintf(log_file, "[%s:%d]Sensor %d (%s)=> RECV: %s | SENT: %s\n", client->ip, client->port, sensor->id, sensor->type, received, sent);
    } else if (client->type == CONN_OPERATOR) {
        operator_t *op = (operator_t *)client->data;
        fprintf(log_file, "[%s:%d]Operator %d (%s)=> RECV: %s | SENT: %s\n", client->ip, client->port, op->id, op->username, received, sent);
    } else {
        fprintf(log_file, "[%s:%d]Unidentified client=> RECV: %s | SENT: %s\n", client->ip, client->port, received, sent);
    }
    fflush(log_file);
    pthread_mutex_unlock(&log_mutex);
}

int tokenize(char *message, char **tokens, int max_tokens) {
    if (message == NULL) return -1;
    
    int count = 0;
    char *token = strtok(message, " ");
    
    while (token != NULL && count < max_tokens) {
        tokens[count++] = token;
        token = strtok(NULL, " ");
    }
    
    return count;
}

//--------------------------------------------------------------------------------------------------------------------
// Socket management
int create_server_socket(int port) {
    int fd = socket(AF_INET, SOCK_STREAM, 0);

    if (fd < 0) {
        perror("socket");
        exit(EXIT_FAILURE);
    }

    int opt = 1;
    setsockopt(fd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));
    
    struct sockaddr_in server_addr = {
        .sin_family = AF_INET,
        .sin_addr.s_addr = INADDR_ANY,
        .sin_port = htons(port)
    };
    
    if (bind(fd, (struct sockaddr *)&server_addr, sizeof(server_addr)) < 0) {
        perror("bind");
        close(fd);
        exit(EXIT_FAILURE);
    }

    if (listen(fd, BACKLOG) < 0) {
        perror("listen");
        close(fd);
        exit(EXIT_FAILURE);
    }

    return fd;
}

void server_close_socket(int fd) {
    close(fd);
}

void send_response(client_t *client, const char *response) {
    send(client->fd, response, strlen(response), 0);
}

//--------------------------------------------------------------------------------------------------------------------
// Set recv timeout on a socket
static void set_recv_timeout(int fd, int seconds) {
    struct timeval tv;
    tv.tv_sec = seconds;
    tv.tv_usec = 0;
    setsockopt(fd, SOL_SOCKET, SO_RCVTIMEO, &tv, sizeof(tv));
}

//--------------------------------------------------------------------------------------------------------------------
// Client handler
void *handle_client(void *arg) {
    client_t *client = (client_t *)arg;
    char message[BUFFER_SIZE];
    char temp[BUFFER_SIZE];
    int buf_len = 0;

    printf("DEBUG: client handler started for %s:%d\n", client->ip, client->port);
    fflush(stdout);

    int continue_receiving = 1;
    while (continue_receiving) {
        ssize_t bytes_received = recv(client->fd, temp, BUFFER_SIZE - 1, 0);

        // Handle timeout (recv returns -1 with EAGAIN/EWOULDBLOCK)
        if (bytes_received < 0 && (errno == EAGAIN || errno == EWOULDBLOCK)) {
            if (client->type == CONN_SENSOR && client->data) {
                sensor_t *s = (sensor_t *)client->data;

                pthread_mutex_lock(&sensor_mutex);
                if (s->state == SENSOR_CONNECTED) {
                    // First timeout: CONNECTED -> IDLE
                    s->state = SENSOR_IDLE;
                    pthread_mutex_unlock(&sensor_mutex);

                    printf("DEBUG: sensor %d timed out -> IDLE\n", s->id);
                    fflush(stdout);

                    // Store alert and broadcast to operators
                    char alert_msg[BUFFER_SIZE];
                    snprintf(alert_msg, sizeof(alert_msg),
                             "ALERT %d BATCH_MISS Sensor missed a batch, on watchlist\n", s->id);
                    store_alert(s->id, "BATCH_MISS", "Sensor missed a batch, on watchlist");
                    broadcast_alert_to_operators(alert_msg);

                    // Set longer timeout for the IDLE -> OFFLINE transition
                    set_recv_timeout(client->fd, TIMEOUT_OFFLINE_SEC);
                    continue;

                } else if (s->state == SENSOR_IDLE) {
                    // Second timeout: IDLE -> OFFLINE
                    s->state = SENSOR_OFFLINE;
                    s->client_fd = -1;
                    pthread_mutex_unlock(&sensor_mutex);

                    printf("DEBUG: sensor %d timed out -> OFFLINE\n", s->id);
                    fflush(stdout);

                    char alert_msg[BUFFER_SIZE];
                    snprintf(alert_msg, sizeof(alert_msg),
                             "ALERT %d SENSOR_OFFLINE Sensor disconnected unexpectedly\n", s->id);
                    store_alert(s->id, "SENSOR_OFFLINE", "Sensor disconnected unexpectedly");
                    broadcast_alert_to_operators(alert_msg);

                    // Force disconnect
                    break;
                } else {
                    pthread_mutex_unlock(&sensor_mutex);
                    break;
                }
            } else {
                // Non-sensor client timed out, just ignore or break
                break;
            }
            continue;
        }

        // Connection closed or error
        if (bytes_received <= 0) break;

        printf("DEBUG: received %zd bytes from %s:%d\n", bytes_received, client->ip, client->port);
        fflush(stdout);

        // Append received bytes to message buffer
        if (buf_len + bytes_received >= BUFFER_SIZE) break;
        memcpy(message + buf_len, temp, bytes_received);
        buf_len += bytes_received;

        // Process all complete messages in the buffer
        char *newline_pos;
        while ((newline_pos = memchr(message, '\n', buf_len)) != NULL) {
            *newline_pos = '\0';

            printf("DEBUG: processing message: [%s]\n", message);
            fflush(stdout);

            char response[BUFFER_SIZE];
            int result = process_message(client, message, response, BUFFER_SIZE);
            log_event(client, message, response);
            send_response(client, response);
            printf("DEBUG: sent response: [%s]\n", response);

            if (result != 0) {
                continue_receiving = 0;
                break;
            }

            // Shift remaining bytes to front of buffer
            size_t msg_len = newline_pos - message + 1;
            size_t remaining = buf_len - msg_len;
            memmove(message, newline_pos + 1, remaining);
            buf_len = remaining;
        }
    }

    // Cleanup on disconnect
    if (client->type == CONN_SENSOR && client->data) {
        sensor_t *s = (sensor_t *)client->data;
        pthread_mutex_lock(&sensor_mutex);
        if (s->state == SENSOR_CONNECTED || s->state == SENSOR_IDLE) {
            s->state = SENSOR_OFFLINE;
        }
        s->client_fd = -1;
        pthread_mutex_unlock(&sensor_mutex);
    } else if (client->type == CONN_OPERATOR && client->data) {
        // Remove operator from active list
        pthread_mutex_lock(&operator_mutex);
        for (int i = 0; i < operator_count; i++) {
            if (operators[i] == client) {
                operators[i] = operators[--operator_count];
                operators[operator_count] = NULL;
                break;
            }
        }
        pthread_mutex_unlock(&operator_mutex);
        free(client->data);  // free operator_t
    }

    close(client->fd);
    free(client);
    return NULL;
}

//--------------------------------------------------------------------------------------------------------------------
// Operation handlers

int handle_register(client_t *client, char *sensor_type, char *response, int response_size) {
    if (response == NULL || response_size <= 0) return 1;

    printf("DEBUG: handle_register from %s:%d with type=%s\n", client->ip, client->port, sensor_type ? sensor_type : "(null)");
    fflush(stdout);

    if (sensor_type == NULL) {
        snprintf(response, response_size, "RESPONSE 400 - Bad request\n");
        return 1;
    }

    sensor_t *sensor = malloc(sizeof(sensor_t));
    if (!sensor) {
        snprintf(response, response_size, "RESPONSE 500 - Internal server error\n");
        return 1;
    }

    if (strlen(sensor_type) >= sizeof(sensor->type)) {
        free(sensor);
        snprintf(response, response_size, "RESPONSE 400 - Sensor type too long\n");
        return 1;
    }

    pthread_mutex_lock(&sensor_mutex);
    if (sensor_count >= MAX_SENSORS) {
        pthread_mutex_unlock(&sensor_mutex);
        free(sensor);
        snprintf(response, response_size, "RESPONSE 503 - Server full\n");
        return 1;
    }

    sensor->id = next_sensor_id++;
    strncpy(sensor->type, sensor_type, sizeof(sensor->type) - 1);
    sensor->type[sizeof(sensor->type) - 1] = '\0';
    sensor->state = SENSOR_CONNECTED;
    sensor->client_fd = client->fd;
    sensor->history_count = 0;
    sensor->history_index = 0;
    sensor->last_active = time(NULL);
    memset(sensor->history, 0, sizeof(sensor->history));

    sensors[sensor_count++] = sensor;
    pthread_mutex_unlock(&sensor_mutex);

    client->data = sensor;
    client->type = CONN_SENSOR;

    // Set initial timeout for batch monitoring
    set_recv_timeout(client->fd, TIMEOUT_IDLE_SEC);

    snprintf(response, response_size, "RESPONSE 200 %d Sensor registered correctly\n", sensor->id);
    printf("DEBUG: sensor registered id=%d type=%s\n", sensor->id, sensor->type);
    fflush(stdout);
    return 0;
}

int handle_connect(client_t *client, char *sensor_id_str, char *sensor_type, char *response, int response_size) {
    if (response == NULL || response_size <= 0) return 1;

    printf("DEBUG: handle_connect from %s:%d with id=%s type=%s\n", client->ip, client->port, sensor_id_str ? sensor_id_str : "(null)", sensor_type ? sensor_type : "(null)");
    fflush(stdout);

    if (sensor_id_str == NULL || sensor_type == NULL) {
        snprintf(response, response_size, "RESPONSE 400 - Bad request\n");
        return 1;
    }

    int sensor_id = atoi(sensor_id_str);

    int found = 0;
    pthread_mutex_lock(&sensor_mutex);
    for (int i = 0; i < sensor_count; i++) {
        sensor_t *s = sensors[i];
        if (s->id == sensor_id) {
            if (s->state == SENSOR_CONNECTED) {
                pthread_mutex_unlock(&sensor_mutex);
                snprintf(response, response_size, "RESPONSE 409 - Sensor already connected\n");
                return 1;
            }
            if (strcmp(sensor_type, s->type) != 0) {
                pthread_mutex_unlock(&sensor_mutex);
                snprintf(response, response_size, "RESPONSE 400 - Sensor type mismatch\n");
                return 1;
            }
            s->state = SENSOR_CONNECTED;
            s->client_fd = client->fd;
            s->last_active = time(NULL);
            client->type = CONN_SENSOR;
            client->data = s;
            found = 1;
            break;
        }
    }
    pthread_mutex_unlock(&sensor_mutex);

    if (!found) {
        snprintf(response, response_size, "RESPONSE 404 - Sensor not found\n");
        return 1;
    }

    // Set initial timeout for batch monitoring
    set_recv_timeout(client->fd, TIMEOUT_IDLE_SEC);

    snprintf(response, response_size, "RESPONSE 200 - Connection established\n");
    printf("DEBUG: sensor connected id=%d type=%s\n", sensor_id, sensor_type);
    fflush(stdout);
    return 0;
}

int handle_auth(client_t *client, char *username, char *password, char *response, int response_size) {
    if (response == NULL || response_size <= 0) return 1;

    printf("DEBUG: handle_auth from %s:%d with username=%s\n", client->ip, client->port, username ? username : "(null)");
    fflush(stdout);

    if (username == NULL || password == NULL) {
        snprintf(response, response_size, "RESPONSE 400 - Bad request\n");
        return 1;
    }

    // Placeholder: accept any credentials for now
    int auth_ok = 1;

    if (!auth_ok) {
        snprintf(response, response_size, "RESPONSE 401 - Could not authenticate user\n");
        return 1;
    }

    operator_t *op = malloc(sizeof(operator_t));
    if (!op) {
        snprintf(response, response_size, "RESPONSE 500 - Internal server error\n");
        return 1;
    }

    pthread_mutex_lock(&operator_mutex);
    if (operator_count >= MAX_OPERATORS) {
        pthread_mutex_unlock(&operator_mutex);
        free(op);
        snprintf(response, response_size, "RESPONSE 503 - Server full\n");
        return 1;
    }

    op->id = next_operator_id++;
    strncpy(op->username, username, sizeof(op->username) - 1);
    op->username[sizeof(op->username) - 1] = '\0';
    op->auth_status = AUTH_SUCCESS;
    op->op_status = OPERATOR_CONNECTED;

    operators[operator_count++] = client;
    pthread_mutex_unlock(&operator_mutex);

    client->data = op;
    client->type = CONN_OPERATOR;

    snprintf(response, response_size, "RESPONSE 200 %d User authenticated\n", op->id);
    printf("DEBUG: operator authenticated id=%d username=%s\n", op->id, op->username);
    fflush(stdout);
    return 0;
}

// READ_BATCH sensor_id timestamp reading_count r1 r2 r3 ...
int handle_read_batch(client_t *client, char **tokens, int token_count, char *response, int response_size) {
    if (response == NULL || response_size <= 0) return 1;

    sensor_t *sensor = (sensor_t *)client->data;
    if (!sensor) {
        snprintf(response, response_size, "RESPONSE 500 - Internal server error\n");
        return 1;
    }

    // tokens: [0]READ_BATCH [1]sensor_id [2]timestamp [3]reading_count [4..n]readings
    if (token_count < 5) {
        snprintf(response, response_size, "RESPONSE 400 - Bad request\n");
        return 1;
    }

    int sensor_id = atoi(tokens[1]);
    if (sensor_id != sensor->id) {
        snprintf(response, response_size, "RESPONSE 403 - Sensor ID mismatch\n");
        return 1;
    }

    char *timestamp = tokens[2];
    int reading_count = atoi(tokens[3]);

    if (reading_count <= 0 || reading_count > MAX_TOKENS - 4) {
        snprintf(response, response_size, "RESPONSE 400 - Invalid reading count\n");
        return 1;
    }

    if (token_count != 4 + reading_count) {
        snprintf(response, response_size, "RESPONSE 400 - Reading count does not match data\n");
        return 1;
    }

    printf("DEBUG: handle_read_batch sensor=%d timestamp=%s count=%d\n", sensor_id, timestamp, reading_count);
    fflush(stdout);

    // Store readings in ring buffer
    pthread_mutex_lock(&sensor_mutex);
    for (int i = 0; i < reading_count; i++) {
        reading_t *r = &sensor->history[sensor->history_index];
        r->value = atof(tokens[4 + i]);
        strncpy(r->timestamp, timestamp, sizeof(r->timestamp) - 1);
        r->timestamp[sizeof(r->timestamp) - 1] = '\0';

        sensor->history_index = (sensor->history_index + 1) % MAX_HISTORY;
        if (sensor->history_count < MAX_HISTORY) {
            sensor->history_count++;
        }
    }

    // Reset state to CONNECTED if we were IDLE, update activity
    if (sensor->state == SENSOR_IDLE) {
        sensor->state = SENSOR_CONNECTED;
        printf("DEBUG: sensor %d recovered from IDLE -> CONNECTED\n", sensor->id);
        fflush(stdout);
    }
    sensor->last_active = time(NULL);
    pthread_mutex_unlock(&sensor_mutex);

    // Reset timeout back to the initial idle threshold
    set_recv_timeout(client->fd, TIMEOUT_IDLE_SEC);

    snprintf(response, response_size, "RESPONSE 200 %d Batch received (%d readings)\n",
             sensor->id, reading_count);
    return 0;
}

// LIST [sensor_type]
// Response: RESPONSE 200 <count> <id> <type> <last_reading> <id> <type> <last_reading> ...
int handle_list(client_t *client, char **tokens, int token_count, char *response, int response_size) {
    (void)client;
    if (response == NULL || response_size <= 0) return 1;

    char *filter_type = NULL;
    if (token_count == 2) {
        filter_type = tokens[1];
        if (strcmp(filter_type, "-") == 0) filter_type = NULL;
    }

    // Build response into a large temporary buffer
    char body[BUFFER_SIZE * 4] = {0};
    int body_offset = 0;
    int match_count = 0;

    pthread_mutex_lock(&sensor_mutex);
    for (int i = 0; i < sensor_count; i++) {
        sensor_t *s = sensors[i];
        if (filter_type && strcmp(s->type, filter_type) != 0) continue;

        // Get last reading value if available
        float last_val = 0.0;
        if (s->history_count > 0) {
            int last_idx = (s->history_index - 1 + MAX_HISTORY) % MAX_HISTORY;
            last_val = s->history[last_idx].value;
        }

        int written = snprintf(body + body_offset, sizeof(body) - body_offset,
                               " %d %s %.2f", s->id, s->type, last_val);
        if (written > 0) body_offset += written;
        match_count++;
    }
    pthread_mutex_unlock(&sensor_mutex);

    snprintf(response, response_size, "RESPONSE 200 %d%s\n", match_count, body);
    printf("DEBUG: handle_list filter=%s matches=%d\n", filter_type ? filter_type : "(all)", match_count);
    fflush(stdout);
    return 0;
}

// HISTORY <sensor_id>
// Response: RESPONSE 200 <count> <timestamp> <value> <timestamp> <value> ...
int handle_history(client_t *client, char **tokens, int token_count, char *response, int response_size) {
    (void)client;
    if (response == NULL || response_size <= 0) return 1;

    if (token_count != 2) {
        snprintf(response, response_size, "RESPONSE 400 - Bad request\n");
        return 0;  // keep connection alive, just a bad query
    }

    int sensor_id = atoi(tokens[1]);

    char body[BUFFER_SIZE * 4] = {0};
    int body_offset = 0;
    int found = 0;
    int count = 0;

    pthread_mutex_lock(&sensor_mutex);
    for (int i = 0; i < sensor_count; i++) {
        sensor_t *s = sensors[i];
        if (s->id != sensor_id) continue;
        found = 1;
        count = s->history_count;

        // Read ring buffer in chronological order
        int start;
        if (s->history_count < MAX_HISTORY) {
            start = 0;
        } else {
            start = s->history_index; // oldest entry in a full ring
        }

        for (int j = 0; j < s->history_count; j++) {
            int idx = (start + j) % MAX_HISTORY;
            reading_t *r = &s->history[idx];
            int written = snprintf(body + body_offset, sizeof(body) - body_offset,
                                   " %s %.2f", r->timestamp, r->value);
            if (written > 0) body_offset += written;
        }
        break;
    }
    pthread_mutex_unlock(&sensor_mutex);

    if (!found) {
        snprintf(response, response_size, "RESPONSE 404 - Sensor not found\n");
        return 0;
    }

    snprintf(response, response_size, "RESPONSE 200 %d%s\n", count, body);
    printf("DEBUG: handle_history sensor_id=%d readings=%d\n", sensor_id, count);
    fflush(stdout);
    return 0;
}

// ALERTS
// Response: RESPONSE 200 <count> <sensor_id> ALERT <alert_type> <timestamp> ...
int handle_alerts(client_t *client, char *response, int response_size) {
    (void)client;
    if (response == NULL || response_size <= 0) return 1;

    char body[BUFFER_SIZE * 4] = {0};
    int body_offset = 0;

    pthread_mutex_lock(&alert_mutex);
    int count = alert_count;
    for (int i = 0; i < alert_count; i++) {
        alert_t *a = &alerts[i];
        int written = snprintf(body + body_offset, sizeof(body) - body_offset,
                               " %d %s %s", a->sensor_id, a->alert_type, a->timestamp);
        if (written > 0) body_offset += written;
    }
    pthread_mutex_unlock(&alert_mutex);

    snprintf(response, response_size, "RESPONSE 200 %d%s\n", count, body);
    printf("DEBUG: handle_alerts count=%d\n", count);
    fflush(stdout);
    return 0;
}

int handle_disconnect(client_t *client, char *response, int response_size) {
    if (client->type == CONN_SENSOR && client->data) {
        sensor_t *s = (sensor_t *)client->data;
        snprintf(response, response_size, "RESPONSE 200 %d Sensor connection terminated\n", s->id);
    } else if (client->type == CONN_OPERATOR && client->data) {
        operator_t *op = (operator_t *)client->data;
        snprintf(response, response_size, "RESPONSE 200 %d User connection terminated\n", op->id);
    } else {
        snprintf(response, response_size, "RESPONSE 200 - Connection terminated\n");
    }
    return 1;  // returning 1 causes handle_client to break out of the loop
}

//--------------------------------------------------------------------------------------------------------------------
// Message dispatcher

int process_message(client_t *client, char *message, char *response, int response_size) {
    char copy[BUFFER_SIZE];
    strncpy(copy, message, BUFFER_SIZE - 1);
    copy[BUFFER_SIZE - 1] = '\0';

    char *tokens[MAX_TOKENS];
    int token_count = tokenize(copy, tokens, MAX_TOKENS);
    char *verb = tokens[0];

    printf("DEBUG: dispatch from %s:%d verb=%s tokens=%d\n", client->ip, client->port, verb ? verb : "(null)", token_count);
    fflush(stdout);
    
    if (verb == NULL) {
        snprintf(response, response_size, "RESPONSE 400 - Bad request\n");
        return 1;
    }

    // DISCONNECT is valid from any identified state
    if (strcmp(verb, "DISCONNECT") == 0) {
        if (client->type == CONN_UNIDENTIFIED) {
            snprintf(response, response_size, "RESPONSE 400 - Bad request\n");
            return 1;
        }
        return handle_disconnect(client, response, response_size);
    }

    if (client->type == CONN_UNIDENTIFIED) {
        if (strcmp(verb, "REGISTER") == 0) {
            if (token_count != 2) {
                snprintf(response, response_size, "RESPONSE 400 - Bad request\n");
                return 1;
            }
            return handle_register(client, tokens[1], response, response_size);
        } else if (strcmp(verb, "CONNECT") == 0) {
            if (token_count != 3) {
                snprintf(response, response_size, "RESPONSE 400 - Bad request\n");
                return 1;
            }
            return handle_connect(client, tokens[1], tokens[2], response, response_size);
        } else if (strcmp(verb, "AUTH") == 0) {
            if (token_count != 3) {
                snprintf(response, response_size, "RESPONSE 400 - Bad request\n");
                return 1;
            }
            return handle_auth(client, tokens[1], tokens[2], response, response_size);
        } else {
            snprintf(response, response_size, "RESPONSE 400 - Bad request\n");
            return 1;
        }
    } else if (client->type == CONN_SENSOR) {
        if (strcmp(verb, "READ_BATCH") == 0) {
            return handle_read_batch(client, tokens, token_count, response, response_size);
        } else {
            snprintf(response, response_size, "RESPONSE 400 - Bad request\n");
            return 1;
        }
    } else if (client->type == CONN_OPERATOR) {
        if (strcmp(verb, "LIST") == 0) {
            if (token_count > 2) {
                snprintf(response, response_size, "RESPONSE 400 - Bad request\n");
                return 0;
            }
            return handle_list(client, tokens, token_count, response, response_size);
        } else if (strcmp(verb, "HISTORY") == 0) {
            return handle_history(client, tokens, token_count, response, response_size);
        } else if (strcmp(verb, "ALERTS") == 0) {
            if (token_count != 1) {
                snprintf(response, response_size, "RESPONSE 400 - Bad request\n");
                return 0;
            }
            return handle_alerts(client, response, response_size);
        } else {
            snprintf(response, response_size, "RESPONSE 400 - Bad request\n");
            return 1;
        }
    }
    return 1;
}

//--------------------------------------------------------------------------------------------------------------------
int main(int argc, char *argv[]) {
    if (argc != 3) {
        fprintf(stderr, "Usage: %s <port> <logfile>\n", argv[0]);
        return EXIT_FAILURE;
    }

    int port = atoi(argv[1]);
    log_file = fopen(argv[2], "a");

    if (!log_file) { 
        perror("fopen");
        return EXIT_FAILURE; 
    }

    int server_fd = create_server_socket(port);
    printf("Server listening on port %d\n", port);
    fflush(stdout);

    while (1) {
        struct sockaddr_in client_addr;
        socklen_t addr_len = sizeof(client_addr);

        int client_fd = accept(server_fd, (struct sockaddr *)&client_addr, &addr_len);
        if (client_fd < 0) {
            perror("accept");
            continue;   
        }

        printf("DEBUG: accepted client from %s:%d\n", inet_ntoa(client_addr.sin_addr), ntohs(client_addr.sin_port));
        fflush(stdout);

        client_t *new_client = malloc(sizeof(client_t));
        new_client->fd = client_fd;
        new_client->type = CONN_UNIDENTIFIED;
        inet_ntop(AF_INET, &client_addr.sin_addr, new_client->ip, INET_ADDRSTRLEN);
        new_client->port = ntohs(client_addr.sin_port);
        new_client->data = NULL;

        pthread_t thread_id;
        pthread_create(&thread_id, NULL, handle_client, new_client);
        pthread_detach(thread_id);
    }

    fclose(log_file);
    server_close_socket(server_fd);
}