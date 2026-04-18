#ifndef SERVER_H
#define SERVER_H

#include "config.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <pthread.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <time.h>
#include <signal.h>
#include <errno.h>

typedef struct {
    int client_fd;
    struct sockaddr_in client_addr;
} client_info_t;

// HTTP request structure
typedef struct {
    char method[16];
    char path[256];
    char version[16];
    char headers[MAX_HEADERS];
    char body[MAX_BODY];
    int content_length;
} http_request_t;

// HTTP response structure
typedef struct {
    int status_code;
    char content_type[64];
    char *body;
    size_t body_length;
} http_response_t;

// Server functions
int server_init(int port);
void server_run(int server_fd);
void *handle_client(void *arg);
void parse_http_request(const char *raw, http_request_t *req);
void send_http_response(int client_fd, http_response_t *resp);

// Route handlers
void handle_chat_completion(int client_fd, http_request_t *req);
void handle_static_file(int client_fd, http_request_t *req);
void handle_api_models(int client_fd, http_request_t *req);
void handle_not_found(int client_fd, http_request_t *req);

#endif
