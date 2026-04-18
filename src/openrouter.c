#include "../include/openrouter.h"
#include "../include/config.h"
#include <curl/curl.h>

struct response_data {
    char *buffer;
    size_t size;
};

static size_t write_callback(void *contents, size_t size, size_t nmemb, void *userp) {
    size_t realsize = size * nmemb;
    struct response_data *mem = (struct response_data *)userp;
    
    char *ptr = realloc(mem->buffer, mem->size + realsize + 1);
    if (!ptr) return 0;
    
    mem->buffer = ptr;
    memcpy(&(mem->buffer[mem->size]), contents, realsize);
    mem->size += realsize;
    mem->buffer[mem->size] = 0;
    
    return realsize;
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
    const char *p = strstr(json, "\"content\"");
    if (!p) return -1;
    
    p = strchr(p, ':');
    if (!p) return -1;
    p++;
    
    while (*p && (*p == ' ' || *p == '"')) p++;
    
    size_t i = 0;
    while (*p && *p != '"' && i < max_len - 1) {
        if (*p == '\\') {
            p++;
            if (*p == 'n') content[i++] = '\n';
            else if (*p == 't') content[i++] = '\t';
            else if (*p == 'r') content[i++] = '\r';
            else if (*p == '"') content[i++] = '"';
            else if (*p == '\\') content[i++] = '\\';
            else { content[i++] = '\\'; content[i++] = *p; }
        } else {
            content[i++] = *p;
        }
        p++;
    }
    content[i] = '\0';
    return i;
}

int openrouter_chat_completion(const char *api_key, const char *model,
                               const char *user_message, char *response_buffer,
                               size_t buffer_size) {
    CURL *curl;
    CURLcode res;
    struct response_data response_data = {0};
    
    // Build JSON request
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
    
    // Initialize curl
    curl_global_init(CURL_GLOBAL_ALL);
    curl = curl_easy_init();
    
    if (!curl) {
        snprintf(response_buffer, buffer_size, "Error: Failed to initialize CURL");
        return -1;
    }
    
    // Set headers
    struct curl_slist *headers = NULL;
    char auth_header[512];
    snprintf(auth_header, sizeof(auth_header), "Authorization: Bearer %s", api_key);
    
    headers = curl_slist_append(headers, "Content-Type: application/json");
    headers = curl_slist_append(headers, auth_header);
    headers = curl_slist_append(headers, "HTTP-Referer: https://termux-ai-chatbot.local");
    headers = curl_slist_append(headers, "X-Title: Termux AI Chatbot");
    
    // Set curl options
    curl_easy_setopt(curl, CURLOPT_URL, "https://openrouter.ai/api/v1/chat/completions");
    curl_easy_setopt(curl, CURLOPT_HTTPHEADER, headers);
    curl_easy_setopt(curl, CURLOPT_POSTFIELDS, json_body);
    curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, write_callback);
    curl_easy_setopt(curl, CURLOPT_WRITEDATA, &response_data);
    curl_easy_setopt(curl, CURLOPT_TIMEOUT, 30L);
    curl_easy_setopt(curl, CURLOPT_SSL_VERIFYPEER, 0L);
    curl_easy_setopt(curl, CURLOPT_SSL_VERIFYHOST, 0L);
    
    // Perform request
    res = curl_easy_perform(curl);
    
    if (res != CURLE_OK) {
        snprintf(response_buffer, buffer_size, "Error: %s", curl_easy_strerror(res));
        curl_easy_cleanup(curl);
        curl_slist_free_all(headers);
        free(response_data.buffer);
        return -1;
    }
    
    // Check for API error
    if (strstr(response_data.buffer, "\"error\"")) {
        char *msg = strstr(response_data.buffer, "\"message\"");
        if (msg) {
            msg = strchr(msg, ':') + 1;
            while (*msg == ' ' || *msg == '"') msg++;
            size_t i = 0;
            while (*msg && *msg != '"' && i < buffer_size - 1) {
                response_buffer[i++] = *msg++;
            }
            response_buffer[i] = '\0';
        } else {
            snprintf(response_buffer, buffer_size, "Error: API returned an error");
        }
        curl_easy_cleanup(curl);
        curl_slist_free_all(headers);
        free(response_data.buffer);
        return -1;
    }
    
    // Extract content
    if (json_extract_content(response_data.buffer, response_buffer, buffer_size) < 0) {
        // Try alternative parsing: find first "content" after "choices"
        char *choices = strstr(response_data.buffer, "\"choices\"");
        if (choices) {
            char *content = strstr(choices, "\"content\"");
            if (content) {
                content = strchr(content, ':') + 1;
                while (*content == ' ' || *content == '"') content++;
                size_t i = 0;
                while (*content && *content != '"' && i < buffer_size - 1) {
                    if (*content == '\\') {
                        content++;
                        if (*content == 'n') response_buffer[i++] = '\n';
                        else if (*content == 't') response_buffer[i++] = '\t';
                        else if (*content == 'r') response_buffer[i++] = '\r';
                        else if (*content == '"') response_buffer[i++] = '"';
                        else { response_buffer[i++] = '\\'; response_buffer[i++] = *content; }
                    } else {
                        response_buffer[i++] = *content;
                    }
                    content++;
                }
                response_buffer[i] = '\0';
            }
        }
    }
    
    curl_easy_cleanup(curl);
    curl_slist_free_all(headers);
    free(response_data.buffer);
    
    return 0;
}
