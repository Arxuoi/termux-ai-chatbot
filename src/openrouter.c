#include "../include/openrouter.h"
#include "../include/config.h"
#include <sys/socket.h>
#include <netdb.h>
#include <netinet/in.h>
#include <arpa/inet.h>

int http_post_json(const char *hostname, const char *path, const char *api_key,
                    const char *json_body, char *response, size_t max_len) {
    int sock;
    struct hostent *server;
    struct sockaddr_in addr;
    
    server = gethostbyname(hostname);
    if (!server) return -1;
    
    sock = socket(AF_INET, SOCK_STREAM, 0);
    if (sock < 0) return -1;
    
    addr.sin_family = AF_INET;
    memcpy(&addr.sin_addr.s_addr, server->h_addr, server->h_length);
    addr.sin_port = htons(80);
    
    if (connect(sock, (struct sockaddr *)&addr, sizeof(addr)) < 0) {
        close(sock);
        return -1;
    }
    
    char request[MAX_BUFFER];
    char auth_header[512];
    snprintf(auth_header, sizeof(auth_header), "Authorization: Bearer %s", api_key);
    
    int req_len = snprintf(request, sizeof(request),
        "POST %s HTTP/1.1\r\n"
        "Host: %s\r\n"
        "Content-Type: application/json\r\n"
        "%s\r\n"
        "HTTP-Referer: %s\r\n"
        "X-Title: %s\r\n"
        "Content-Length: %zu\r\n"
        "Connection: close\r\n"
        "\r\n"
        "%s",
        path, hostname, auth_header, HTTP_REFERER, APP_TITLE, strlen(json_body), json_body
    );
    
    send(sock, request, req_len, 0);
    
    memset(response, 0, max_len);
    int total = 0, bytes;
    while ((bytes = recv(sock, response + total, max_len - total - 1, 0)) > 0) {
        total += bytes;
        if (total >= (int)max_len - 1) break;
    }
    response[total] = '\0';
    
    close(sock);
    return 0;
}

void json_escape(char *dest, const char *src, size_t max_len) {
    size_t i = 0, j = 0;
    while (src[i] && j < max_len - 1) {
        switch (src[i]) {
            case '"': dest[j++] = '\\'; dest[j++] = '"'; break;
            case '\\': dest[j++] = '\\'; dest[j++] = '\\'; break;
            case '\n': dest[j++] = '\\'; dest[j++] = 'n'; break;
            case '\r': dest[j++] = '\\'; dest[j++] = 'r'; break;
            case '\t': dest[j++] = '\\'; dest[j++] = 't'; break;
            default: dest[j++] = src[i]; break;
        }
        i++;
    }
    dest[j] = '\0';
}

int json_extract_content(const char *json, char *content, size_t max_len) {
    const char *content_start = strstr(json, "\"content\"");
    if (!content_start) return -1;
    
    content_start = strchr(content_start, ':');
    if (!content_start) return -1;
    content_start++;
    
    while (*content_start && (*content_start == ' ' || *content_start == '"')) content_start++;
    
    size_t i = 0;
    while (*content_start && *content_start != '"' && i < max_len - 1) {
        if (*content_start == '\\') {
            content_start++;
            if (*content_start == 'n') content[i++] = '\n';
            else if (*content_start == 't') content[i++] = '\t';
            else if (*content_start == 'r') content[i++] = '\r';
            else if (*content_start == '"') content[i++] = '"';
            else if (*content_start == '\\') content[i++] = '\\';
        } else {
            content[i++] = *content_start;
        }
        content_start++;
    }
    content[i] = '\0';
    return i;
}

int openrouter_chat_completion(const char *api_key, const char *model,
                               const char *user_message, char *response_buffer,
                               size_t buffer_size) {
    static char json_body[MAX_BUFFER];
    char escaped_message[MAX_BUFFER * 2];
    
    json_escape(escaped_message, user_message, sizeof(escaped_message));
    
    snprintf(json_body, sizeof(json_body),
        "{"
        "\"model\":\"%s\","
        "\"messages\":["
        "{\"role\":\"system\",\"content\":\"You are a helpful AI assistant running on Termux.\"},"
        "{\"role\":\"user\",\"content\":\"%s\"}"
        "]"
        "}",
        model, escaped_message
    );
    
    char raw_response[MAX_BUFFER * 2];
    int result = http_post_json(
        "openrouter.ai",
        "/api/v1/chat/completions",
        api_key,
        json_body,
        raw_response,
        sizeof(raw_response)
    );
    
    if (result != 0) {
        snprintf(response_buffer, buffer_size, "Error: Failed to connect to OpenRouter API");
        return -1;
    }
    
    char *body = strstr(raw_response, "\r\n\r\n");
    if (!body) {
        snprintf(response_buffer, buffer_size, "Error: Invalid response from API");
        return -1;
    }
    body += 4;
    
    if (strstr(body, "\"error\"")) {
        char *msg = strstr(body, "\"message\"");
        if (msg) {
            msg = strchr(msg, ':') + 1;
            while (*msg == ' ' || *msg == '"') msg++;
            size_t i = 0;
            while (*msg && *msg != '"' && i < buffer_size - 1) {
                response_buffer[i++] = *msg++;
            }
            response_buffer[i] = '\0';
        } else {
            snprintf(response_buffer, buffer_size, "Error: API error");
        }
        return -1;
    }
    
    if (json_extract_content(body, response_buffer, buffer_size) < 0) {
        snprintf(response_buffer, buffer_size, "Error: Could not parse response");
        return -1;
    }
    
    return 0;
}
