#ifndef OPENROUTER_H
#define OPENROUTER_H

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/socket.h>
#include <netdb.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <openssl/ssl.h>
#include <openssl/err.h>

typedef struct {
    char *response;
    size_t length;
} openrouter_response_t;

// OpenRouter API functions
int openrouter_chat_completion(const char *api_key, 
                               const char *model,
                               const char *user_message, 
                               char *response_buffer, 
                               size_t buffer_size);

// SSL/TLS functions
SSL_CTX *init_ssl_context(void);
int https_post_json(const char *hostname, 
                    const char *path, 
                    const char *api_key,
                    const char *json_body, 
                    char *response, 
                    size_t max_len);

// JSON utilities
void json_escape(char *dest, const char *src, size_t max_len);
int json_extract_content(const char *json, char *content, size_t max_len);

#endif
