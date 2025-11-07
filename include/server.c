#include <stdio.h>
#include <stdlib.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <errno.h>
#include <arpa/inet.h>
#include <sys/time.h>
#include <string.h>
#include "server.h"
#include "utils.h"
#include "logger.h"
#include "thread_pool.h"

// Minimal HTTP response header format
// TODO: add keep-alive handeling
static const char *HTTP_HEADER_FORMAT = "HTTP/1.1 %s\r\n"
                                        "Cache-Control: no-cache\r\n"
                                        "Content-Type: %s\r\n"
                                        "Content-Length: %zu\r\n"
                                        "Connection: close\r\n"
                                        "\r\n";

// TESTING VARS
static const char *HTTP_NOT_FOUND_MESSAGE       = "<html>\n <body> <h1> 404 Not Found </h1> </body></html>";
static const char *HTTP_DEFAULT_POST_RESPONSE   = "<html>\n <body> <h1> POST REQUEST RECEIVED </h1> </body></html>";
static const char *HTTP_DEFAULT_DELETE_RESPONSE = "<html>\n <body> <h1> DELETE REQUEST RECEIVED </h1> </body></html>";
//


void init_server(sock *server, uint32_t address, uint16_t port)
{
	server->info.sin_family      = AF_INET;
        server->info.sin_addr.s_addr = address;
	server->info.sin_port        = htons(port);

        server->fd = socket(AF_INET, SOCK_STREAM, 0);
        bind(server->fd, (struct sockaddr_in*)&server->info, sizeof(server->info));
        listen(server->fd, SOMAXCONN);
}

int run_server(sock *server)
{
        thread_pool_t *thread_pool = create_thread_pool(64, 128);
        while(1)
        {
                LOG_INFO("awaiting incoming connections ...\n");

                sock_data *client_data = calloc(1, sizeof(sock_data));
                if(client_data == NULL)
                {
                        LOG_ERROR("calloc client_data failed");
                        return 1;
                }
                client_data->socket = calloc(1, sizeof(sock));
                if(client_data->socket == NULL)
                {
                        LOG_ERROR("calloc client_data->socket failed");
                        return 1;
                }
                sock *client_socket = client_data->socket;
                socklen_t client_addr_size = sizeof(struct sockaddr_in);
                client_socket->fd = accept(server->fd , (struct sockaddr_in*)&(client_data->socket->info), &client_addr_size);
                if(client_socket->fd == -1)
                {
                        printf("client connection failed: %s\n", strerror(errno));
                        continue;
                }
                else
                {
                        // set idle timeout
                        // recv() will returns -1 after RECV_IDLE_TIMEOUT_DELAY
                        /* struct timeval tv = { .tv_sec = RECV_IDLE_TIMEOUT_DELAY, .tv_usec = 0 };
                        setsockopt(client_socket->fd, SOL_SOCKET, SO_RCVTIMEO, &tv, sizeof(tv)); */
                }
                LOG_DEBUG("connection accepted\n");
                
                // TODO:
                // SERVE CLIENT WITH A WORKER THREAD
                // =================================
                //serve(client_data);
                push_task(thread_pool, serve, client_data);
                // =================================

        }

        // graceful
        release_thread_pool(thread_pool, true);
        shutdown(server->fd, SHUT_WR);
        close(server->fd);
}

// TODO: FIX
void serve(sock_data *client_data)
{
        // FIX: buffer receives once 
        char request_buffer[4096];
        memset(request_buffer, 0, sizeof(request_buffer)); 
        
        int n_received_bytes;
        sock *client_socket = client_data->socket;
        while(1)
        {
                n_received_bytes = recv(client_socket->fd, request_buffer, sizeof(request_buffer) - 1, 0);
                if(n_received_bytes < 0)
                {
                        if (errno == EAGAIN || errno == EWOULDBLOCK) 
                        {
                                LOG_INFO("Client idle timeout reached. Closing connection.");
                                break;
                        } 
                        else 
                        {
                                LOG_ERROR("Fatal recv failed with error: %s\n", strerror(errno));
                                break;
                        }
                }
                /* if(handle_request(client_data, request_buffer, n_received_bytes) < 0)
                        break; */
                handle_request(client_data, request_buffer, n_received_bytes);
                break;
        }
        
        LOG_INFO("Connection closing...\n");
        close(client_socket->fd);
        free(client_data->socket);
        free(client_data);
        return 0;
}

// Handle http request
// --> This function is simply for testing
// TODO: create a mechanism to register callbacks for each request method
int handle_request(sock_data *client_data, char *request_buffer, size_t request_buffer_size)
{
        http_header_info_t header_info;
        char file_extension  [16];
        memset(file_extension, 0, sizeof file_extension);
        char response_buffer [1024];
        memset(response_buffer, 0, sizeof response_buffer);

        sock *client_socket = client_data->socket;

        LOG_INFO("Sending Response to client ...");
        LOG_INFO("client ip : %s, client_port : %d",
                 inet_ntoa(client_socket->info.sin_addr),
                 client_socket->info.sin_port);
        
        request_buffer[request_buffer_size] = '\0';
        LOG_INFO("Request received from client:=========================\n %s\n", request_buffer);
        LOG_INFO("======================================================");
        
        get_http_request_info(request_buffer, &header_info);
        get_file_extension(header_info.path, file_extension);

        if(strcmp(header_info.method, "POST") == 0) // process POST request
        {
                LOG_DEBUG("received POST request");
                snprintf
                (
                        response_buffer,
                        sizeof response_buffer,
                        HTTP_HEADER_FORMAT,
                        "200 OK",
                        "text/html;",
                        strlen(HTTP_DEFAULT_POST_RESPONSE)
                );
                strcat(response_buffer, HTTP_DEFAULT_POST_RESPONSE);

                send(client_socket->fd, response_buffer, strlen(response_buffer), 0);

                return 0;
                /* close(client_socket->fd);
                free(client_data);
                return -1; */
        }
        else if(strcmp(header_info.method, "DELETE") == 0) // process DELETE request
        {
                LOG_DEBUG("received DELETE request");
                snprintf
                (
                        response_buffer,
                        sizeof response_buffer,
                        HTTP_HEADER_FORMAT,
                        "200 OK",
                        "text/html;",
                        strlen(HTTP_DEFAULT_DELETE_RESPONSE)
                );
                strcat(response_buffer, HTTP_DEFAULT_DELETE_RESPONSE);

                send(client_socket->fd, response_buffer, strlen(response_buffer), 0);

                return 0;  
        }
        // ELSE
        // PROCESS GET REQUEST
        // ===================
        LOG_DEBUG("PATH REQUESTED BY CLIENT: %s\n", header_info.path);
        LOG_DEBUG("PATH EXTENSTION : %s\n"        , file_extension);

        FILE *file = NULL;
        if(get_file(header_info.path, &file) < 0)
        {
                LOG_ERROR("error reading file");
                snprintf
                (
                        response_buffer,
                        sizeof response_buffer,
                        HTTP_HEADER_FORMAT,
                        "404 Not Found",
                        "text/html;",
                        strlen(HTTP_NOT_FOUND_MESSAGE)
                );
                strcat(response_buffer, HTTP_NOT_FOUND_MESSAGE);

                send(client_socket->fd, response_buffer, strlen(response_buffer), 0);

                return -1;
        }
        else
                LOG_INFO("GOT FILE");
        //
        init_http_header(file_extension, response_buffer, sizeof response_buffer, get_file_size(file));
        send(client_socket->fd, response_buffer, strlen(response_buffer), client_data->flags);
        LOG_DEBUG("HEADER SENT :\n %s", response_buffer);

        LOG_DEBUG("SENDING FILE CONTENT ...");
        if(send_file(client_socket->fd, file) < 0)
        {
                LOG_ERROR("ERROR SENDING FILE");
                //return -1;
        }
        else
                LOG_DEBUG("FILE CONTENT SENT ...");
        
        LOG_INFO("Response was sent ...\n");
        return 0;
}

// Sends file content in chunks
int send_file(int socket_fd, FILE *file)
{
        char buffer[4096];
        
        LOG_INFO("SENDING FILE ...");
        LOG_INFO("FILE SIZE : %ld", get_file_size(file));
        if(file)
        {
                LOG_DEBUG("FILE NOT NULL");
                // Read file in chunks and send
                size_t bytes_read;
                while ((bytes_read = fread(buffer, 1, sizeof buffer, file)) > 0) 
                {
                        LOG_DEBUG("Read %zu bytes from file", bytes_read);
                        LOG_DEBUG("Previewing 64 Bytes from buffer:\n");
                        #ifdef ENABLE_LOGGING
                        for (size_t i = 0; i < bytes_read && i < 64; i++) 
                        {
                                printf("%02X ", (unsigned char)buffer[i]);
                                // square shaped buffer
                                if(!((i+1) % 8)) printf("\n");
                        }
                        printf("\n");
                        #endif
                        LOG_DEBUG("==================================");
                        LOG_DEBUG("ASCII DATA: %s", buffer);
                        LOG_DEBUG("==================================");

                        LOG_DEBUG("SENDING FILE CHUNK");
                        // MSG_NOSIGNAL: omits SIGPIPE error
                        if(send(socket_fd, buffer, bytes_read, MSG_NOSIGNAL) != bytes_read) 
                        {
                                perror("send failed");
                                fclose(file);
                                //close(socket_fd);
                                return -1;
                        }
                }
        }
        else
        {
                LOG_ERROR("ERROR READING FILE");
                return -1;
        }

        LOG_INFO("FILE SENT SUCCESSFULLY.");
        fclose(file);
        return 0;
}

// Infer HTTP response header from file extension
// TODO: add support for more formats
// FIX: find a way to remove redundancy
void init_http_header(const char *file_extension, char *response_buffer, size_t response_buffer_size, size_t content_size)
{
        if (strcmp(file_extension, "js") == 0)
        {
                snprintf
                (
                        response_buffer,
                        response_buffer_size,
                        HTTP_HEADER_FORMAT,
                        "200 OK",
                        "application/javascript",
                        content_size
                );
        }
        else if (strcmp(file_extension, "ico") == 0)
        {
                snprintf
                (
                        response_buffer,
                        response_buffer_size,
                        HTTP_HEADER_FORMAT,
                        "200 OK",
                        "image/x-icon",
                        content_size
                );
        }
        else // Default
        {
                snprintf
                (
                        response_buffer,
                        response_buffer_size,
                        HTTP_HEADER_FORMAT,
                        "200 OK",
                        "text/html; charset=UTF-8",
                        content_size
                );
        }
}