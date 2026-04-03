#ifndef HTTP_SERVER_H
#define HTTP_SERVER_H

#define HTTP_BUFFER_SIZE    8192
#define HTTP_BACKLOG        10

// Start the HTTP server on the given port (runs in its own thread)
void start_http_server(int port);

#endif