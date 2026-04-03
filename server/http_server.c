#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <pthread.h>
#include <arpa/inet.h>
#include "server.h"
#include "http_server.h"

//--------------------------------------------------------------------------------------------------------------------
// Base64 decoding for HTTP Basic Auth

static const unsigned char b64_table[256] = {
    ['A']=0,['B']=1,['C']=2,['D']=3,['E']=4,['F']=5,['G']=6,['H']=7,
    ['I']=8,['J']=9,['K']=10,['L']=11,['M']=12,['N']=13,['O']=14,['P']=15,
    ['Q']=16,['R']=17,['S']=18,['T']=19,['U']=20,['V']=21,['W']=22,['X']=23,
    ['Y']=24,['Z']=25,['a']=26,['b']=27,['c']=28,['d']=29,['e']=30,['f']=31,
    ['g']=32,['h']=33,['i']=34,['j']=35,['k']=36,['l']=37,['m']=38,['n']=39,
    ['o']=40,['p']=41,['q']=42,['r']=43,['s']=44,['t']=45,['u']=46,['v']=47,
    ['w']=48,['x']=49,['y']=50,['z']=51,['0']=52,['1']=53,['2']=54,['3']=55,
    ['4']=56,['5']=57,['6']=58,['7']=59,['8']=60,['9']=61,['+']=62,['/']=63
};

static int base64_decode(const char *in, char *out, int out_size) {
    int len = strlen(in);
    int i = 0, j = 0;
    unsigned int buf = 0;
    int bits = 0;

    while (i < len && j < out_size - 1) {
        char c = in[i++];
        if (c == '=' || c == '\n' || c == '\r') break;
        buf = (buf << 6) | b64_table[(unsigned char)c];
        bits += 6;
        if (bits >= 8) {
            bits -= 8;
            out[j++] = (buf >> bits) & 0xFF;
        }
    }
    out[j] = '\0';
    return j;
}

//--------------------------------------------------------------------------------------------------------------------
// HTTP response helpers

static void http_send(int fd, const char *status, const char *content_type, const char *body, int body_len) {
    char header[512];
    int hlen = snprintf(header, sizeof(header),
        "HTTP/1.1 %s\r\n"
        "Content-Type: %s\r\n"
        "Content-Length: %d\r\n"
        "Connection: close\r\n"
        "Access-Control-Allow-Origin: *\r\n"
        "\r\n",
        status, content_type, body_len);
    send(fd, header, hlen, 0);
    if (body && body_len > 0) {
        send(fd, body, body_len, 0);
    }
}

static void http_send_401(int fd) {
    const char *body = "{\"error\":\"Authentication required\"}";
    char header[512];
    int blen = strlen(body);
    int hlen = snprintf(header, sizeof(header),
        "HTTP/1.1 401 Unauthorized\r\n"
        "Content-Type: application/json\r\n"
        "Content-Length: %d\r\n"
        "WWW-Authenticate: Basic realm=\"IoT Monitor\"\r\n"
        "Connection: close\r\n"
        "\r\n",
        blen);
    send(fd, header, hlen, 0);
    send(fd, body, blen, 0);
}

static void http_send_404(int fd) {
    const char *body = "{\"error\":\"Not found\"}";
    int blen = strlen(body);
    http_send(fd, "404 Not Found", "application/json", body, blen);
}

//--------------------------------------------------------------------------------------------------------------------
// Dashboard HTML (embedded)

static const char *DASHBOARD_HTML =
"<!DOCTYPE html>\n"
"<html lang=\"en\">\n"
"<head>\n"
"<meta charset=\"UTF-8\">\n"
"<meta name=\"viewport\" content=\"width=device-width, initial-scale=1.0\">\n"
"<title>IoT Monitoring Dashboard</title>\n"
"<style>\n"
"  * { margin: 0; padding: 0; box-sizing: border-box; }\n"
"  body { font-family: 'Segoe UI', system-ui, sans-serif; background: #0f1923; color: #e0e0e0; }\n"
"  .header { background: #1a2733; padding: 16px 24px; border-bottom: 2px solid #2d7d9a; }\n"
"  .header h1 { font-size: 20px; color: #4fc3f7; }\n"
"  .header span { font-size: 13px; color: #78909c; }\n"
"  .container { max-width: 1200px; margin: 0 auto; padding: 20px; }\n"
"  .stats { display: grid; grid-template-columns: repeat(auto-fit, minmax(180px, 1fr)); gap: 12px; margin-bottom: 24px; }\n"
"  .stat-card { background: #1a2733; border-radius: 8px; padding: 16px; border-left: 3px solid #2d7d9a; }\n"
"  .stat-card .label { font-size: 11px; text-transform: uppercase; color: #78909c; letter-spacing: 1px; }\n"
"  .stat-card .value { font-size: 28px; font-weight: 700; color: #4fc3f7; margin-top: 4px; }\n"
"  .section { background: #1a2733; border-radius: 8px; padding: 20px; margin-bottom: 20px; }\n"
"  .section h2 { font-size: 16px; color: #4fc3f7; margin-bottom: 14px; border-bottom: 1px solid #263238; padding-bottom: 8px; }\n"
"  table { width: 100%; border-collapse: collapse; font-size: 13px; }\n"
"  th { text-align: left; padding: 8px 10px; color: #78909c; font-weight: 600; text-transform: uppercase; font-size: 11px; letter-spacing: 0.5px; border-bottom: 1px solid #263238; }\n"
"  td { padding: 8px 10px; border-bottom: 1px solid #1e2d3a; }\n"
"  tr:hover { background: #1e2d3a; }\n"
"  .badge { padding: 2px 8px; border-radius: 10px; font-size: 11px; font-weight: 600; }\n"
"  .badge-connected { background: #1b5e20; color: #a5d6a7; }\n"
"  .badge-idle { background: #e65100; color: #ffcc80; }\n"
"  .badge-offline { background: #b71c1c; color: #ef9a9a; }\n"
"  .alert-row { border-left: 3px solid #ff5252; }\n"
"  .btn { background: #2d7d9a; color: white; border: none; padding: 6px 14px; border-radius: 4px; cursor: pointer; font-size: 12px; margin-right: 6px; }\n"
"  .btn:hover { background: #3a9fbf; }\n"
"  .btn-active { background: #4fc3f7; color: #0f1923; }\n"
"  .toolbar { margin-bottom: 14px; display: flex; gap: 6px; align-items: center; }\n"
"  .history-panel { display: none; background: #162029; border-radius: 6px; padding: 14px; margin-top: 10px; }\n"
"  .history-panel.active { display: block; }\n"
"  .refresh-info { font-size: 11px; color: #546e7a; margin-left: auto; }\n"
"  .empty { color: #546e7a; font-style: italic; padding: 20px; text-align: center; }\n"
"</style>\n"
"</head>\n"
"<body>\n"
"<div class=\"header\">\n"
"  <h1>IoT Monitoring Dashboard</h1>\n"
"  <span id=\"user-info\"></span>\n"
"</div>\n"
"<div class=\"container\">\n"
"  <div class=\"stats\" id=\"stats\"></div>\n"
"\n"
"  <div class=\"section\">\n"
"    <h2>Sensors</h2>\n"
"    <div class=\"toolbar\">\n"
"      <button class=\"btn btn-active\" onclick=\"filterSensors('')\">All</button>\n"
"      <button class=\"btn\" onclick=\"filterSensors('temperature')\">Temperature</button>\n"
"      <button class=\"btn\" onclick=\"filterSensors('pressure')\">Pressure</button>\n"
"      <button class=\"btn\" onclick=\"filterSensors('vibration')\">Vibration</button>\n"
"      <button class=\"btn\" onclick=\"filterSensors('energy')\">Energy</button>\n"
"      <span class=\"refresh-info\">Auto-refresh: 5s</span>\n"
"    </div>\n"
"    <table>\n"
"      <thead><tr><th>ID</th><th>Type</th><th>State</th><th>Last Value</th><th>Last Reading</th><th>Actions</th></tr></thead>\n"
"      <tbody id=\"sensor-table\"></tbody>\n"
"    </table>\n"
"  </div>\n"
"\n"
"  <div id=\"history-section\" class=\"section history-panel\">\n"
"    <h2>Sensor History — <span id=\"history-title\"></span></h2>\n"
"    <table>\n"
"      <thead><tr><th>Timestamp</th><th>Value</th></tr></thead>\n"
"      <tbody id=\"history-table\"></tbody>\n"
"    </table>\n"
"  </div>\n"
"\n"
"  <div class=\"section\">\n"
"    <h2>Alerts</h2>\n"
"    <table>\n"
"      <thead><tr><th>Sensor ID</th><th>Type</th><th>Message</th><th>Timestamp</th></tr></thead>\n"
"      <tbody id=\"alert-table\"></tbody>\n"
"    </table>\n"
"  </div>\n"
"</div>\n"
"\n"
"<script>\n"
"let currentFilter = '';\n"
"let sensorsData = [];\n"
"\n"
"function apiFetch(url) {\n"
"  return fetch(url).then(r => { if (r.status === 401) { location.reload(); } return r.json(); });\n"
"}\n"
"\n"
"function badgeClass(state) {\n"
"  if (state === 'connected') return 'badge-connected';\n"
"  if (state === 'idle') return 'badge-idle';\n"
"  return 'badge-offline';\n"
"}\n"
"\n"
"function filterSensors(type) {\n"
"  currentFilter = type;\n"
"  document.querySelectorAll('.toolbar .btn').forEach(b => b.classList.remove('btn-active'));\n"
"  event.target.classList.add('btn-active');\n"
"  renderSensors();\n"
"}\n"
"\n"
"function renderSensors() {\n"
"  let filtered = sensorsData;\n"
"  if (currentFilter) filtered = sensorsData.filter(s => s.type === currentFilter);\n"
"  const tb = document.getElementById('sensor-table');\n"
"  if (filtered.length === 0) { tb.innerHTML = '<tr><td colspan=\"6\" class=\"empty\">No sensors found</td></tr>'; return; }\n"
"  tb.innerHTML = filtered.map(s =>\n"
"    '<tr>' +\n"
"    '<td>' + s.id + '</td>' +\n"
"    '<td>' + s.type + '</td>' +\n"
"    '<td><span class=\"badge ' + badgeClass(s.state) + '\">' + s.state + '</span></td>' +\n"
"    '<td>' + s.last_value.toFixed(2) + '</td>' +\n"
"    '<td>' + s.last_timestamp + '</td>' +\n"
"    '<td><button class=\"btn\" onclick=\"showHistory(' + s.id + ', \\'' + s.type + ' #' + s.id + '\\')\">History</button></td>' +\n"
"    '</tr>'\n"
"  ).join('');\n"
"}\n"
"\n"
"function showHistory(sensorId, title) {\n"
"  document.getElementById('history-title').textContent = title;\n"
"  apiFetch('/api/history?sensor_id=' + sensorId).then(data => {\n"
"    const sec = document.getElementById('history-section');\n"
"    sec.classList.add('active');\n"
"    const tb = document.getElementById('history-table');\n"
"    if (!data.readings || data.readings.length === 0) { tb.innerHTML = '<tr><td colspan=\"2\" class=\"empty\">No readings yet</td></tr>'; return; }\n"
"    tb.innerHTML = data.readings.map(r =>\n"
"      '<tr><td>' + r.timestamp + '</td><td>' + r.value.toFixed(2) + '</td></tr>'\n"
"    ).join('');\n"
"  });\n"
"}\n"
"\n"
"function refresh() {\n"
"  apiFetch('/api/list').then(data => {\n"
"    sensorsData = data;\n"
"    const connected = data.filter(s => s.state === 'connected').length;\n"
"    const idle = data.filter(s => s.state === 'idle').length;\n"
"    const offline = data.filter(s => s.state === 'offline').length;\n"
"    document.getElementById('stats').innerHTML =\n"
"      '<div class=\"stat-card\"><div class=\"label\">Total Sensors</div><div class=\"value\">' + data.length + '</div></div>' +\n"
"      '<div class=\"stat-card\"><div class=\"label\">Connected</div><div class=\"value\" style=\"color:#a5d6a7\">' + connected + '</div></div>' +\n"
"      '<div class=\"stat-card\"><div class=\"label\">Idle</div><div class=\"value\" style=\"color:#ffcc80\">' + idle + '</div></div>' +\n"
"      '<div class=\"stat-card\"><div class=\"label\">Offline</div><div class=\"value\" style=\"color:#ef9a9a\">' + offline + '</div></div>';\n"
"    renderSensors();\n"
"  });\n"
"  apiFetch('/api/alerts').then(data => {\n"
"    const tb = document.getElementById('alert-table');\n"
"    if (data.length === 0) { tb.innerHTML = '<tr><td colspan=\"4\" class=\"empty\">No alerts</td></tr>'; return; }\n"
"    tb.innerHTML = data.slice().reverse().map(a =>\n"
"      '<tr class=\"alert-row\">' +\n"
"      '<td>' + a.sensor_id + '</td>' +\n"
"      '<td>' + a.type + '</td>' +\n"
"      '<td>' + a.message + '</td>' +\n"
"      '<td>' + a.timestamp + '</td>' +\n"
"      '</tr>'\n"
"    ).join('');\n"
"  });\n"
"}\n"
"\n"
"refresh();\n"
"setInterval(refresh, 5000);\n"
"</script>\n"
"</body>\n"
"</html>\n";

//--------------------------------------------------------------------------------------------------------------------
// HTTP request parsing and routing

// Extract a query parameter value from a URL. Returns 0 on success.
static int get_query_param(const char *url, const char *key, char *value, int value_size) {
    char search[64];
    snprintf(search, sizeof(search), "%s=", key);
    const char *pos = strstr(url, search);
    if (!pos) return -1;
    pos += strlen(search);
    int i = 0;
    while (pos[i] && pos[i] != '&' && pos[i] != ' ' && pos[i] != '#' && i < value_size - 1) {
        value[i] = pos[i];
        i++;
    }
    value[i] = '\0';
    return 0;
}

// Extract Authorization header value. Returns 0 on success.
static int get_auth_header(const char *request, char *username, int usize, char *password, int psize) {
    const char *auth = strstr(request, "Authorization: Basic ");
    if (!auth) return -1;
    auth += strlen("Authorization: Basic ");

    // Extract base64 token until \r\n
    char b64[256] = {0};
    int i = 0;
    while (auth[i] && auth[i] != '\r' && auth[i] != '\n' && i < (int)sizeof(b64) - 1) {
        b64[i] = auth[i];
        i++;
    }
    b64[i] = '\0';

    char decoded[256] = {0};
    base64_decode(b64, decoded, sizeof(decoded));

    // Split at ':'
    char *colon = strchr(decoded, ':');
    if (!colon) return -1;
    *colon = '\0';
    strncpy(username, decoded, usize - 1);
    username[usize - 1] = '\0';
    strncpy(password, colon + 1, psize - 1);
    password[psize - 1] = '\0';
    return 0;
}

static void *handle_http_client(void *arg) {
    int client_fd = *(int *)arg;
    free(arg);

    char request[HTTP_BUFFER_SIZE] = {0};
    int total = 0;

    // Read until we get full headers (\r\n\r\n)
    while (total < HTTP_BUFFER_SIZE - 1) {
        ssize_t n = recv(client_fd, request + total, HTTP_BUFFER_SIZE - 1 - total, 0);
        if (n <= 0) break;
        total += n;
        if (strstr(request, "\r\n\r\n")) break;
    }

    if (total <= 0) {
        close(client_fd);
        return NULL;
    }

    // Parse method and path
    char method[8] = {0}, path[256] = {0};
    sscanf(request, "%7s %255s", method, path);

    // Only handle GET
    if (strcmp(method, "GET") != 0) {
        const char *body = "{\"error\":\"Method not allowed\"}";
        http_send(client_fd, "405 Method Not Allowed", "application/json", body, strlen(body));
        close(client_fd);
        return NULL;
    }

    // Authenticate (except for favicon)
    if (strcmp(path, "/favicon.ico") != 0) {
        char username[64] = {0}, password[64] = {0};
        if (get_auth_header(request, username, sizeof(username), password, sizeof(password)) != 0) {
            http_send_401(client_fd);
            close(client_fd);
            return NULL;
        }
        // Validate against external auth service
        char role[32] = {0};
        if (!validate_credentials(username, password, role, sizeof(role))) {
            http_send_401(client_fd);
            close(client_fd);
            return NULL;
        }
        // Only operators can access the web dashboard
        if (strcmp(role, "OPERATOR") != 0) {
            http_send_401(client_fd);
            close(client_fd);
            return NULL;
        }
    }

    // Route
    char body[HTTP_BUFFER_SIZE * 4] = {0};
    int body_len = 0;

    if (strcmp(path, "/") == 0) {
        // Serve dashboard
        body_len = strlen(DASHBOARD_HTML);
        http_send(client_fd, "200 OK", "text/html; charset=utf-8", DASHBOARD_HTML, body_len);

    } else if (strcmp(path, "/api/list") == 0 || strncmp(path, "/api/list?", 10) == 0) {
        char filter[32] = {0};
        get_query_param(path, "type", filter, sizeof(filter));
        body_len = get_sensors_json(body, sizeof(body), strlen(filter) > 0 ? filter : NULL);
        http_send(client_fd, "200 OK", "application/json", body, body_len);

    } else if (strncmp(path, "/api/history", 12) == 0) {
        char sid[16] = {0};
        if (get_query_param(path, "sensor_id", sid, sizeof(sid)) != 0) {
            const char *err = "{\"error\":\"Missing sensor_id parameter\"}";
            http_send(client_fd, "400 Bad Request", "application/json", err, strlen(err));
        } else {
            int sensor_id = atoi(sid);
            body_len = get_history_json(body, sizeof(body), sensor_id);
            http_send(client_fd, "200 OK", "application/json", body, body_len);
        }

    } else if (strcmp(path, "/api/alerts") == 0) {
        body_len = get_alerts_json(body, sizeof(body));
        http_send(client_fd, "200 OK", "application/json", body, body_len);

    } else {
        http_send_404(client_fd);
    }

    close(client_fd);
    return NULL;
}

//--------------------------------------------------------------------------------------------------------------------
// HTTP server listener thread

static void *http_listener(void *arg) {
    int port = *(int *)arg;
    free(arg);

    int server_fd = socket(AF_INET, SOCK_STREAM, 0);
    if (server_fd < 0) {
        perror("http socket");
        return NULL;
    }

    int opt = 1;
    setsockopt(server_fd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));

    struct sockaddr_in addr = {
        .sin_family = AF_INET,
        .sin_addr.s_addr = INADDR_ANY,
        .sin_port = htons(port)
    };

    if (bind(server_fd, (struct sockaddr *)&addr, sizeof(addr)) < 0) {
        perror("http bind");
        close(server_fd);
        return NULL;
    }

    if (listen(server_fd, HTTP_BACKLOG) < 0) {
        perror("http listen");
        close(server_fd);
        return NULL;
    }

    while (1) {
        struct sockaddr_in client_addr;
        socklen_t addr_len = sizeof(client_addr);
        int client_fd = accept(server_fd, (struct sockaddr *)&client_addr, &addr_len);
        if (client_fd < 0) {
            perror("http accept");
            continue;
        }

        int *fd_ptr = malloc(sizeof(int));
        *fd_ptr = client_fd;

        pthread_t tid;
        pthread_create(&tid, NULL, handle_http_client, fd_ptr);
        pthread_detach(tid);
    }

    close(server_fd);
    return NULL;
}

void start_http_server(int port) {
    int *port_ptr = malloc(sizeof(int));
    *port_ptr = port;

    pthread_t tid;
    pthread_create(&tid, NULL, http_listener, port_ptr);
    pthread_detach(tid);
}