#ifndef UTILS_H
#define UTILS_H

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <time.h>

// File utilities
char *read_file(const char *filename, size_t *size);
int file_exists(const char *filename);
const char *get_mime_type(const char *filename);

// String utilities
void trim(char *str);
void url_decode(char *str);
char *str_replace(char *str, const char *old, const char *new_str);
void log_request(const char *method, const char *path, int status, const char *client_ip);

// Environment/config
int load_env_file(const char *filename);
char *get_env_value(const char *key);

// Time utilities
void get_http_date(char *buffer, size_t size);

#endif
