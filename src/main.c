#include "../include/config.h"
#include "../include/server.h"
#include "../include/utils.h"

AppConfig g_config = {0};
volatile sig_atomic_t server_running = 1;

void signal_handler(int sig) {
    if (sig == SIGINT || sig == SIGTERM) {
        printf("\n[*] Shutting down server...\n");
        server_running = 0;
    }
}

void print_banner(void) {
    printf("\n");
    printf("╔══════════════════════════════════════════════════════════════╗\n");
    printf("║                                                              ║\n");
    printf("║     ████████╗███████╗██████╗ ███╗   ███╗██╗   ██╗██╗  ██╗   ║\n");
    printf("║     ╚══██╔══╝██╔════╝██╔══██╗████╗ ████║██║   ██║╚██╗██╔╝   ║\n");
    printf("║        ██║   █████╗  ██████╔╝██╔████╔██║██║   ██║ ╚███╔╝    ║\n");
    printf("║        ██║   ██╔══╝  ██╔══██╗██║╚██╔╝██║██║   ██║ ██╔██╗    ║\n");
    printf("║        ██║   ███████╗██║  ██║██║ ╚═╝ ██║╚██████╔╝██╔╝ ██╗   ║\n");
    printf("║        ╚═╝   ╚══════╝╚═╝  ╚═╝╚═╝     ╚═╝ ╚═════╝ ╚═╝  ╚═╝   ║\n");
    printf("║                                                              ║\n");
    printf("║                    AI CHATBOT v1.0                           ║\n");
    printf("║                  Powered by OpenRouter                        ║\n");
    printf("╚══════════════════════════════════════════════════════════════╝\n");
    printf("\n");
}

int main(int argc, char **argv) {
    print_banner();
    
    // Set default config
    g_config.port = DEFAULT_PORT;
    strcpy(g_config.model, DEFAULT_MODEL);
    strcpy(g_config.api_url, OPENROUTER_API_URL);
    memset(g_config.api_key, 0, sizeof(g_config.api_key));
    
    // Load .env file
    if (!load_env_file(".env")) {
        fprintf(stderr, "[!] Warning: .env file not found. Using defaults.\n");
        fprintf(stderr, "[!] API key must be set in .env file!\n");
    }
    
    // Get API key from environment
    char *api_key = get_env_value("OPENROUTER_API_KEY");
    if (api_key) {
        strncpy(g_config.api_key, api_key, sizeof(g_config.api_key) - 1);
        // Mask API key for display
        char masked_key[32];
        snprintf(masked_key, sizeof(masked_key), "%.12s...%s", 
                 g_config.api_key, g_config.api_key + strlen(g_config.api_key) - 4);
        printf("[*] API Key loaded: %s\n", masked_key);
    } else {
        fprintf(stderr, "[!] ERROR: OPENROUTER_API_KEY not set in .env file!\n");
        fprintf(stderr, "[!] Create .env file with: OPENROUTER_API_KEY=your-key-here\n");
        return 1;
    }
    
    // Get model from environment (optional)
    char *model = get_env_value("OPENROUTER_MODEL");
    if (model) {
        strncpy(g_config.model, model, sizeof(g_config.model) - 1);
    }
    
    // Get port from environment (optional)
    char *port_str = get_env_value("PORT");
    if (port_str) {
        g_config.port = atoi(port_str);
    }
    
    // Override port from command line
    if (argc > 1) {
        g_config.port = atoi(argv[1]);
    }
    
    printf("[*] Configuration:\n");
    printf("    Port: %d\n", g_config.port);
    printf("    Model: %s\n", g_config.model);
    printf("    API URL: %s\n", g_config.api_url);
    printf("\n");
    
    // Set signal handlers
    signal(SIGINT, signal_handler);
    signal(SIGTERM, signal_handler);
    signal(SIGPIPE, SIG_IGN);
    
    // Initialize SSL
    SSL_library_init();
    SSL_load_error_strings();
    OpenSSL_add_all_algorithms();
    
    // Start server
    int server_fd = server_init(g_config.port);
    if (server_fd < 0) {
        fprintf(stderr, "[!] Failed to initialize server\n");
        return 1;
    }
    
    printf("[*] Server started on http://localhost:%d\n", g_config.port);
    printf("[*] Open browser and navigate to http://localhost:%d\n", g_config.port);
    printf("[*] Press Ctrl+C to stop\n\n");
    
    server_run(server_fd);
    
    close(server_fd);
    printf("[*] Server stopped. Goodbye!\n");
    
    return 0;
}
