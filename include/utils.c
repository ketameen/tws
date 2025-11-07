#include "utils.h"
#include "logger.h"
#include "server.h"
#include <string.h>

// FILE
// =========
// returns index content if no path is given
// TODO: find a better alternative
int get_file(char *file_path, FILE **file)
{
        char full_path[256] = DEFAULT_ROOT_PATH;
        
        if(file_path == NULL)
                strcat(full_path, "/index.html");
        else if(strcmp(file_path, "/") != 0)
                strcat(full_path, file_path);
        else
                strcat(full_path, "/index.html");
        
        LOG_DEBUG("file path : %s\n", full_path);
        *file = fopen(full_path, "rb");
        LOG_DEBUG("file opened : %s\n", full_path);
        
        if(*file == NULL)
        {
                LOG_ERROR("Cannot read file.\n");
                return -1;
        }
        return 0;
}

long get_file_size(FILE *file) 
{
        fseek(file, 0, SEEK_END);
        long size = ftell(file);
        rewind(file);          
        return size;
}

void get_file_extension(char *file_path, char *dest_extension)
{
        char *prev, *curr, *cp;
        const char delimiters[] = "/.";
        
        LOG_DEBUG("GETTING EXTENSION");
        
        // avoid this cast
        cp   = strdup((const char*)file_path);
        curr = strtok(cp, delimiters);
        
        while(prev = strtok(NULL, delimiters))
        {
                LOG_DEBUG("PARSING ...");
                curr = prev;
                LOG_DEBUG("CURRENT TOKEN : %s", curr);
        }
        LOG_DEBUG("PARSED");
        
        if(curr != NULL)
        {
                strcpy(dest_extension, curr);
                LOG_DEBUG("GOT EXTENSION");
        }
        
}
// =========

// HTTP
// =========
void get_requested_path(char *request_buffer, char *dest_path)
{
        char *file_path, *cp;
        const char delimiters[] = " ,;:!-";
        
        // this assumes that the path is the second token in a HTTP request
        // This may not be valid for all cases
        cp        = strdup((const char*)request_buffer);
        file_path = strtok(cp  , delimiters);
        file_path = strtok(NULL, delimiters);
        
        LOG_DEBUG("GOT PATH : %s", file_path);
        if(file_path)
        strcpy(dest_path, file_path);
        
}

void get_http_request_info(char *request_buffer, http_header_info_t *header_info)
{
        sscanf(request_buffer, "%s %s %s",
                header_info->method,
                header_info->path,
                header_info->version);
}

// =========