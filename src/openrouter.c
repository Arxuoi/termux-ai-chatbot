#include "../include/openrouter.h"
#include "../include/config.h"

SSL_CTX *init_ssl_context(void) {
    SSL_CTX *ctx = SSL_CTX_new(TLS_client_method());
    SSL_CTX_set_options(ctx, SSL_OP_NO_SSLv2 | SSL_OP_NO_SSLv3 | SSL_OP_NO_TLSv1 | SSL_OP_NO_TLSv1_1);
    return ctx;
}

int https_post_json(const char *hostname, const char *path, const char *api_key,
                    const char *json_body, char *response, size_t max_len) {
    int sock;
    struct hostent *server;
    struct sockaddr_in addr;
    SSL_CTX *ctx;
    SSL *ssl;
    int result = -1;
    
    // Resolve hostname
    server = gethostbyname(hostname);
    if (!server) return -1;
    
    // Create socket
    sock = socket(AF_INET, SOCK_STREAM, 0);
    if (sock < 0) return -1;
    
    addr.sin_family = AF_INET;
    memcpy(&addr.sin_addr.s_addr, server->h_addr, server->h_length);
    addr.sin_port = htons(443);
    
    if (connect(sock, (struct sockaddr *)&addr, sizeof(addr)) < 0) {
        close(sock);
        return -1;
    }
    
    // Initialize SSL
    ctx = init_ssl_context();
    if (!ctx) {
        close(sock);
        return -1;
    }
    
    ssl = SSL_new(ctx);
    SSL_set_fd(ssl, sock);
    
    if (SSL_connect(ssl) <= 0) {
        SSL_free(ssl);
        SSL_CTX_free(ctx);
        close(sock);
        return -1;
    }
    
    // Build HTTP request
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
    
    // Send request
    SSL_write(ssl, request, req_len);
    
    // Read response
    memset(response, 0, max_len);
    int total_read = 0;
    int bytes_read;
    
    while ((bytes_read = SSL_read(ssl, response + total_read, max_len - total_read - 1)) > 0) {
        total_read += bytes_read;
        if (total_read >= (int)max_len - 1) break;
    }
    response[total_read] = '\0';
    
    result = 0;
    
    // Cleanup
    SSL_shutdown(ssl);
    SSL_free(ssl);
    SSL_CTX_free(ctx);
    close(sock);
    
    return result;
}

int json_extract_content(const char *json, char *content, size_t max_len) {
    // Find "content" field
    const char *content_start = strstr(json, "\"content\"");
    if (!content_start) return -1;
    
    content_start = strchr(content_start, ':');
    if (!content_start) return -1;
    content_start++;
    
    // Skip whitespace and quote
    while (*content_start && (*content_start == ' ' || *content_start == '"')) content_start++;
    
    // Copy until closing quote
    size_t i = 0;
    while (*content_start && *content_start != '"' && i < max_len - 1) {
        if (*content_start == '\\') {
            content_start++;
            if (*content_start == 'n') {
                content[i++] = '\n';
            } else if (*content_start == 't') {
                content[i++] = '\t';
            } else if (*content_start == 'r') {
                content[i++] = '\r';
            } else if (*content_start == '"') {
                content[i++] = '"';
            } else if (*content_start == '\\') {
                content[i++] = '\\';
            }
        } else {
            content[i++] = *content_start;
        }
        content_start++;
    }
    content[i] = '\0';
    
    return i;
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

int openrouter_chat_completion(const char *api_key, const char *model,
                               const char *user_message, char *response_buffer,
                               size_t buffer_size) {
    // Build JSON request body
    static char json_body[MAX_BUFFER];
    char escaped_message[MAX_BUFFER * 2];
    
    json_escape(escaped_message, user_message, sizeof(escaped_message));
    
    int json_len = snprintf(json_body, sizeof(json_body),
        "{"
        "\"model\":\"%s\","
        "\"messages\":["
        "{\"role\":\"system\",\"content\":\"You are a helpful AI assistant running on Termux.\"},"
        "{\"role\":\"user\",\"content\":\"%s\"}"
        "],"
        "\"temperature\":0.7,"
        "\"max_tokens\":1024"
        "}",
        model, escaped_message
    );
    
    // Make HTTPS request
    char raw_response[MAX_BUFFER * 2];
    int result = https_post_json(
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
    
    // Extract response body (after headers)
    char *body = strstr(raw_response, "\r\n\r\n");
    if (!body) {
        snprintf(response_buffer, buffer_size, "Error: Invalid response from API");
        return -1;
    }
    body += 4;
    
    // Check for error
    if (strstr(body, "\"error\"")) {
        snprintf(response_buffer, buffer_size, "Error: API returned an error");
        return -1;
    }
    
    // Extract content from JSON
    if (json_extract_content(body, response_buffer, buffer_size) < 0) {
        snprintf(response_buffer, buffer_size, "Error: Could not parse response");
        return -1;
    }
    
    return 0;
}
