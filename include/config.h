#ifndef CONFIG_H
#define CONFIG_H

#define MAX_BUFFER 65536
#define MAX_HEADERS 4096
#define MAX_BODY 32768
#define DEFAULT_PORT 5000
#define MAX_ENV_LINE 512

// OpenRouter defaults
#define DEFAULT_MODEL "openai/gpt-3.5-turbo"
#define OPENROUTER_API_URL "https://openrouter.ai/api/v1/chat/completions"
#define HTTP_REFERER "https://termux-ai-chatbot.local"
#define APP_TITLE "Termux AI Chatbot"

typedef struct {
    char api_key[256];
    char model[128];
    char api_url[256];
    int port;
} AppConfig;

// Global config
extern AppConfig g_config;

#endif
