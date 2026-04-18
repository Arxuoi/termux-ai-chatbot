#include "../include/utils.h"
#include "../include/config.h"

char *read_file(const char *filename, size_t *size) {
    FILE *fp = fopen(filename, "rb");
    if (!fp) return NULL;
    
    fseek(fp, 0, SEEK_END);
    *size = ftell(fp);
    fseek(fp, 0, SEEK_SET);
    
    char *buffer = malloc(*size + 1);
    if (!buffer) {
        fclose(fp);
        return NULL;
    }
    
    fread(buffer, 1, *size, fp);
    buffer[*size] = '\0';
    fclose(fp);
    
    return buffer;
}

int file_exists(const char *filename) {
    FILE *fp = fopen(filename, "r");
    if (fp) {
        fclose(fp);
        return 1;
    }
    return 0;
}

const char *get_mime_type(const char *filename) {
    const char *ext = strrchr(filename, '.');
    if (!ext) return "application/octet-stream";
    
    if (strcmp(ext, ".html") == 0) return "text/html";
    if (strcmp(ext, ".css") == 0) return "text/css";
    if (strcmp(ext, ".js") == 0) return "application/javascript";
    if (strcmp(ext, ".json") == 0) return "application/json";
    if (strcmp(ext, ".png") == 0) return "image/png";
    if (strcmp(ext, ".jpg") == 0) return "image/jpeg";
    if (strcmp(ext, ".ico") == 0) return "image/x-icon";
    
    return "text/plain";
}

void trim(char *str) {
    char *end;
    while (isspace((unsigned char)*str)) str++;
    if (*str == 0) return;
    end = str + strlen(str) - 1;
    while (end > str && isspace((unsigned char)*end)) end--;
    end[1] = '\0';
}

void url_decode(char *str) {
    char *src = str;
    char *dst = str;
    
    while (*src) {
        if (*src == '%') {
            int value;
            if (sscanf(src + 1, "%2x", &value) == 1) {
                *dst++ = (char)value;
                src += 3;
                continue;
            }
        } else if (*src == '+') {
            *dst++ = ' ';
            src++;
            continue;
        }
        *dst++ = *src++;
    }
    *dst = '\0';
}

int load_env_file(const char *filename) {
    FILE *fp = fopen(filename, "r");
    if (!fp) return 0;
    
    char line[MAX_ENV_LINE];
    while (fgets(line, sizeof(line), fp)) {
        trim(line);
        if (line[0] == '#' || line[0] == '\0') continue;
        putenv(strdup(line));
    }
    
    fclose(fp);
    return 1;
}

char *get_env_value(const char *key) {
    return getenv(key);
}
