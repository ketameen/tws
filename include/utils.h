#ifndef UTILS_H
#define UTILS_H
#include "server.h"
#include <stdio.h>

void get_http_request_info(char *request_buffer, http_header_info_t *header_info);
void get_file_extension(char *file_path, char *dest_extension);
long get_file_size(FILE *file);
int  get_file(char *file_path, FILE **file);



#endif