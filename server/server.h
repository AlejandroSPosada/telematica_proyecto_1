#ifndef SERVER_H
#define SERVER_H

#include <stdio.h>
#include <time.h>
#include <arpa/inet.h>

#define MAX_SENSORS     100
#define MAX_OPERATORS   50
#define MAX_CLIENTS     (MAX_SENSORS + MAX_OPERATORS)
#define MAX_TOKENS      16
#define BUFFER_SIZE     1024
#define BACKLOG         10
#define MAX_HISTORY     100
#define MAX_ALERTS      100

#define TIMEOUT_IDLE_SEC    60
#define TIMEOUT_OFFLINE_SEC 60

typedef enum { CONN_UNIDENTIFIED, CONN_SENSOR, CONN_OPERATOR } client_type_t;
typedef enum { SENSOR_CONNECTED, SENSOR_IDLE, SENSOR_OFFLINE } sensor_status_t;
typedef enum { OPERATOR_CONNECTED } operator_status_t;
typedef enum { AUTH_PENDING, AUTH_SUCCESS, AUTH_FAILED } auth_status_t;

typedef struct {
    int fd;
    client_type_t type;
    char ip[INET_ADDRSTRLEN];
    int port;
    void *data;  // points to sensor_t, or operator_t
} client_t;

typedef struct {
    float value;
    char timestamp[32];
} reading_t;

typedef struct {
    int sensor_id;
    char alert_type[32];
    char message[128];
    char timestamp[32];
} alert_t;

typedef struct {
    int id;
    char type[32];
    sensor_status_t state;
    int client_fd;              // current connection fd, -1 if disconnected
    reading_t history[MAX_HISTORY];
    int history_count;
    int history_index;          // ring buffer write position
    time_t last_active;
} sensor_t;

typedef struct {
    int id;
    char username[50];
    operator_status_t op_status;
    auth_status_t auth_status;
} operator_t;

extern FILE *log_file;

// Alerts
void store_alert(int sensor_id, const char *alert_type, const char *message);
void broadcast_alert_to_operators(const char *alert_msg);

// Logging
void log_event(client_t *client, const char *received, const char *sent);

// Parsing
int tokenize(char *message, char **tokens, int max_tokens);

// Socket management
int create_server_socket(int port);
void server_close_socket(int fd);
void send_response(client_t *client, const char *response);

// Client handling
void *handle_client(void *arg);

// Operation handlers
int handle_register(client_t *client, char *sensor_type, char *response, int response_size);
int handle_connect(client_t *client, char *sensor_id_str, char *sensor_type, char *response, int response_size);
int handle_auth(client_t *client, char *username, char *password, char *response, int response_size);
int handle_read_batch(client_t *client, char **tokens, int token_count, char *response, int response_size);
int handle_list(client_t *client, char **tokens, int token_count, char *response, int response_size);
int handle_history(client_t *client, char **tokens, int token_count, char *response, int response_size);
int handle_alerts(client_t *client, char *response, int response_size);
int handle_disconnect(client_t *client, char *response, int response_size);
int process_message(client_t *client, char *message, char *response, int response_size);

#endif