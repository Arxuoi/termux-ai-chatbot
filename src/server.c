#include "../include/server.h"
#include "../include/config.h"
#include "../include/utils.h"
#include "../include/openrouter.h"

extern AppConfig g_config;
extern volatile sig_atomic_t server_running;

int server_init(int port) {
    int server_fd;
    struct sockaddr_in address;
    int opt = 1;
    
    if ((server_fd = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
        perror("[!] Socket creation failed");
        return -1;
    }
    
    if (setsockopt(server_fd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt)) < 0) {
        perror("[!] setsockopt failed");
        close(server_fd);
        return -1;
    }
    
    address.sin_family = AF_INET;
    address.sin_addr.s_addr = INADDR_ANY;
    address.sin_port = htons(port);
    
    if (bind(server_fd, (struct sockaddr *)&address, sizeof(address)) < 0) {
        perror("[!] Bind failed");
        close(server_fd);
        return -1;
    }
    
    if (listen(server_fd, 10) < 0) {
        perror("[!] Listen failed");
        close(server_fd);
        return -1;
    }
    
    return server_fd;
}

void server_run(int server_fd) {
    struct sockaddr_in client_addr;
    socklen_t addr_len = sizeof(client_addr);
    
    while (server_running) {
        int client_fd = accept(server_fd, (struct sockaddr *)&client_addr, &addr_len);
        
        if (client_fd < 0) {
            if (server_running) perror("[!] Accept failed");
            continue;
        }
        
        // Set socket timeout
        struct timeval timeout;
        timeout.tv_sec = 30;
        timeout.tv_usec = 0;
        setsockopt(client_fd, SOL_SOCKET, SO_RCVTIMEO, &timeout, sizeof(timeout));
        setsockopt(client_fd, SOL_SOCKET, SO_SNDTIMEO, &timeout, sizeof(timeout));
        
        client_info_t *client_info = malloc(sizeof(client_info_t));
        client_info->client_fd = client_fd;
        client_info->client_addr = client_addr;
        
        pthread_t thread;
        pthread_create(&thread, NULL, handle_client, client_info);
        pthread_detach(thread);
    }
}

void *handle_client(void *arg) {
    client_info_t *info = (client_info_t *)arg;
    int client_fd = info->client_fd;
    char client_ip[INET_ADDRSTRLEN];
    
    inet_ntop(AF_INET, &info->client_addr.sin_addr, client_ip, sizeof(client_ip));
    free(info);
    
    char buffer[MAX_BUFFER] = {0};
    ssize_t bytes_read = recv(client_fd, buffer, sizeof(buffer) - 1, 0);
    
    if (bytes_read <= 0) {
        close(client_fd);
        return NULL;
    }
    buffer[bytes_read] = '\0';
    
    http_request_t req;
    memset(&req, 0, sizeof(req));
    parse_http_request(buffer, &req);
    
    printf("[%s] %s %s\n", client_ip, req.method, req.path);
    
    // Routing
    if (strcmp(req.method, "GET") == 0) {
        if (strcmp(req.path, "/") == 0 || strcmp(req.path, "/index.html") == 0) {
            strcpy(req.path, "/index.html");
            handle_static_file(client_fd, &req);
        } else if (strstr(req.path, "/api/") == req.path) {
            if (strcmp(req.path, "/api/models") == 0) {
                handle_api_models(client_fd, &req);
            } else {
                handle_not_found(client_fd, &req);
            }
        } else {
            handle_static_file(client_fd, &req);
        }
    } else if (strcmp(req.method, "POST") == 0) {
        if (strcmp(req.path, "/api/chat") == 0) {
            handle_chat_completion(client_fd, &req);
        } else {
            handle_not_found(client_fd, &req);
        }
    } else {
        handle_not_found(client_fd, &req);
    }
    
    close(client_fd);
    return NULL;
}

void parse_http_request(const char *raw, http_request_t *req) {
    // Parse first line
    sscanf(raw, "%15s %255s %15s", req->method, req->path, req->version);
    
    // Find headers
    const char *headers_start = strstr(raw, "\r\n");
    if (headers_start) {
        headers_start += 2;
        const char *headers_end = strstr(headers_start, "\r\n\r\n");
        if (headers_end) {
            size_t headers_len = headers_end - headers_start;
            if (headers_len < MAX_HEADERS - 1) {
                strncpy(req->headers, headers_start, headers_len);
                req->headers[headers_len] = '\0';
            }
            
            // Parse body
            const char *body_start = headers_end + 4;
            size_t body_len = strlen(body_start);
            if (body_len < MAX_BODY - 1) {
                strcpy(req->body, body_start);
            }
        }
    }
    
    // Get content length
    char *cl = strstr(req->headers, "Content-Length:");
    if (cl) {
        req->content_length = atoi(cl + 15);
    }
}

void send_http_response(int client_fd, http_response_t *resp) {
    char header[4096];
    const char *status_text = "OK";
    
    switch (resp->status_code) {
        case 200: status_text = "OK"; break;
        case 400: status_text = "Bad Request"; break;
        case 404: status_text = "Not Found"; break;
        case 500: status_text = "Internal Server Error"; break;
        default: status_text = "Unknown"; break;
    }
    
    int header_len = snprintf(header, sizeof(header),
        "HTTP/1.1 %d %s\r\n"
        "Content-Type: %s\r\n"
        "Content-Length: %zu\r\n"
        "Access-Control-Allow-Origin: *\r\n"
        "Access-Control-Allow-Methods: GET, POST, OPTIONS\r\n"
        "Access-Control-Allow-Headers: Content-Type\r\n"
        "Connection: close\r\n"
        "\r\n",
        resp->status_code, status_text, resp->content_type, resp->body_length
    );
    
    send(client_fd, header, header_len, 0);
    if (resp->body && resp->body_length > 0) {
        send(client_fd, resp->body, resp->body_length, 0);
    }
}

void handle_chat_completion(int client_fd, http_request_t *req) {
    // Extract message from JSON body
    char *message = NULL;
    char *msg_start = strstr(req->body, "\"message\"");
    
    if (msg_start) {
        msg_start = strchr(msg_start, ':');
        if (msg_start) {
            msg_start++;
            while (*msg_start == ' ' || *msg_start == '"') msg_start++;
            message = msg_start;
            char *msg_end = strchr(message, '"');
            if (msg_end) *msg_end = '\0';
        }
    }
    
    if (!message || strlen(message) == 0) {
        http_response_t resp = {
            .status_code = 400,
            .content_type = "application/json",
            .body = "{\"error\":\"Message is required\"}",
            .body_length = 29
        };
        send_http_response(client_fd, &resp);
        return;
    }
    
    // URL decode message
    url_decode(message);
    
    printf("[*] Chat request: \"%.50s...\"\n", message);
    
    // Call OpenRouter API
    char ai_response[MAX_BUFFER] = {0};
    int result = openrouter_chat_completion(
        g_config.api_key,
        g_config.model,
        message,
        ai_response,
        sizeof(ai_response)
    );
    
    if (result != 0) {
        http_response_t resp = {
            .status_code = 500,
            .content_type = "application/json",
            .body = "{\"error\":\"Failed to get AI response\"}",
            .body_length = 36
        };
        send_http_response(client_fd, &resp);
        return;
    }
    
    // Build JSON response
    static char json_response[MAX_BUFFER];
    char escaped[MAX_BUFFER * 2];
    json_escape(escaped, ai_response, sizeof(escaped));
    
    int json_len = snprintf(json_response, sizeof(json_response),
        "{\"response\":\"%s\"}", escaped);
    
    http_response_t resp = {
        .status_code = 200,
        .content_type = "application/json",
        .body = json_response,
        .body_length = json_len
    };
    
    send_http_response(client_fd, &resp);
}

void handle_static_file(int client_fd, http_request_t *req) {
    char filepath[512];
    
    // Security: prevent directory traversal
    if (strstr(req->path, "..")) {
        handle_not_found(client_fd, req);
        return;
    }
    
    snprintf(filepath, sizeof(filepath), "web%s", req->path);
    
    size_t file_size;
    char *content = read_file(filepath, &file_size);
    
    if (!content) {
        handle_not_found(client_fd, req);
        return;
    }
    
    http_response_t resp = {
        .status_code = 200,
        .content_type = "",
        .body = content,
        .body_length = file_size
    };
    
    strcpy(resp.content_type, get_mime_type(filepath));
    send_http_response(client_fd, &resp);
    free(content);
}

void handle_api_models(int client_fd, http_request_t *req) {
    const char *json = 
        "{"
        "\"models\":["
        "{\"id\":\"openai/gpt-3.5-turbo\",\"name\":\"GPT-3.5 Turbo\"},"
        "{\"id\":\"openai/gpt-4\",\"name\":\"GPT-4\"},"
        "{\"id\":\"anthropic/claude-3-haiku\",\"name\":\"Claude 3 Haiku\"},"
        "{\"id\":\"google/gemini-pro\",\"name\":\"Gemini Pro\"}"
        "],"
        "\"current\":\"%s\""
        "}";
    
    static char response[1024];
    int len = snprintf(response, sizeof(response), json, g_config.model);
    
    http_response_t resp = {
        .status_code = 200,
        .content_type = "application/json",
        .body = response,
        .body_length = len
    };
    
    send_http_response(client_fd, &resp);
}

void handle_not_found(int client_fd, http_request_t *req) {
    const char *body = "<html><body><h1>404 Not Found</h1></body></html>";
    
    http_response_t resp = {
        .status_code = 404,
        .content_type = "text/html",
        .body = (char *)body,
        .body_length = strlen(body)
    };
    
    send_http_response(client_fd, &resp);
}
