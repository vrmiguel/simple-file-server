#include "request.h"
#include "utils.h"
#include <string.h>
#include <stdbool.h>
#include <stdio.h>
#include <ctype.h>
#include <stdlib.h>
#include <assert.h>
#include <errno.h>
#include <unistd.h>

static inline bool is_get_req(const char * request) {
    return (!strncmp(request,"GET", 3)) ||
           (!strncmp(request,"get", 3));
}

static inline bool is_append_req(const char * request) {
    return (!strncmp(request,"APPEND", 6)) ||
           (!strncmp(request,"append", 6));
}

static inline bool is_create_req(const char * request) {
    return (!strncmp(request,"CREATE", 5)) ||
           (!strncmp(request,"create", 5));
}

static inline bool is_remove_req(const char * request) {
    return (!strncmp(request,"REMOVE", 6)) ||
           (!strncmp(request,"remove", 6));
}


static request_t process_get_request(const char * request) {
    printf("server: processing GET request.\n");
    request_t req_struct;
    req_struct.type = Get;

    if (strlen(request) <= 4)
    {
        printf("server: error: missing body in GET request.\n");
        req_struct.status = 422;
        return req_struct;
    }

    char * filename = strdup(request + 4);

    FILE *file = fopen(filename, "rb");
    if (!file) {
        printf("server: error: could not open file '%s'.\n", filename);
        free(filename);
        req_struct.status = 404; // 404: not found
        return req_struct;
    }

    fseek(file, 0L, SEEK_END);
    size_t sz = ftell(file);
    rewind(file);

    printf("server: file '%s' has %ld bytes.\n", filename, sz);


    req_struct.status = 200; // 200: OK
    req_struct.data.get_req.contents = malloc(sz);
    req_struct.data.get_req.size     = sz;
    fread(req_struct.data.get_req.contents, sz, 1, file);

    free(filename);
    fclose(file);
    return req_struct;
}

request_t process_remove_request(const char * request) {
    printf("server: processing REMOVE request.\n");
    request_t req_struct;
    req_struct.type = Remove;

    if (strlen(request) <= 7) {
        printf("server: error: missing body in REMOVE request.\n");
        req_struct.status = 422;
        return req_struct;
    }

    char * to_be_removed = strdup(request + 7);
    int ret = remove(to_be_removed);
    if (ret == 0) {
        printf("server: file '%s' succesfully removed.\n", to_be_removed);
        req_struct.status = 200;
    } else {
        printf("server: error: could not remove '%s'.\n", to_be_removed);
        req_struct.status = 500;
    }

    req_struct.data.remove_req.filename = strdup(to_be_removed);

    free(to_be_removed);
    return req_struct;
}

request_t process_append_request(char * request) {
    printf("server: processing APPEND request.\n");
    request_t req_struct;
    req_struct.type = Append;

    if (strlen(request) <= 7) {
        printf("server: error: missing body in APPEND request.\n");
        req_struct.status = 422;
        return req_struct;
    }

    // The position of the last ' ' character in the request string
    ssize_t last_space_pos = -1;
    int i, j = 0;
    size_t len = strlen(request);

    for(int i = len - 1; i >= 0; i--)
    {
        if (request[i] == ' ') {
            last_space_pos = i;
            break;
        }
    }

    char * filename = malloc(len - last_space_pos);
    for (i = last_space_pos+1; i < (signed) len; i++, j++) {
//        printf("i-last_space_pos-1 = %ld", i-last_space_pos+1);
//        printf("%c", request[i]);
        filename[j] = request[i];
    }
    filename[j] = '\0';
    request[last_space_pos] = '\0';

    if ( access(filename, F_OK ) != 0) {
        printf("server: error: cannot append to '%s' since it does not exist. Use CREATE to create new files.\n", filename);
        req_struct.status = 404;
        free(filename);
        return req_struct;
    }

    FILE * file = fopen(filename, "a");
    if (!file) {
        printf("server: error: could not open '%s' for appending.\n", filename);
        free(filename);
        req_struct.status = 500;
        return req_struct;
    }

    char * new_text = strdup(request+7);
    printf("server: appending %ld bytes to '%s'.\n", strlen(new_text), filename);
    req_struct.status = 200;
    fprintf(file, "%s", new_text);

    // Undo the modification on the request string
    request[last_space_pos] = ' ';
    free(new_text);
    free(filename);
    return req_struct;
}

request_t process_create_request(const char * request) {
    printf("server: processing CREATE request.\n");
    request_t req_struct;
    req_struct.type = Create;

    if (strlen(request) <= 7) {
        printf("server: error: missing body in CREATE request.\n");
        req_struct.status = 422;
        return req_struct;
    }
    char * new_file_name = strdup(request + 7);

    FILE * file = fopen(new_file_name, "w");
    if (!file) {
        printf("server: error: could not create file '%s'\n", new_file_name);
    } else {
        printf("server: created file '%s'\n", new_file_name);
    }

    req_struct.status = 200;
    req_struct.data.create_req.filename = strdup(new_file_name);

    fclose(file);
    free(new_file_name);
    return req_struct;
}

//! process_request
request_t process_request(const char * request)
{
    printf("server: starting to process request.\n");
    if (is_get_req(request)) {
        return process_get_request(request);
    }

    else if (is_create_req(request)) {
        return process_create_request(request);
    }

    else if (is_append_req(request)) {
        return process_append_request(request);
    }

    else if (is_remove_req(request)) {
        return process_remove_request(request);
    }

    printf("server: error: unknown request\n");
    request_t req; req.status = 400;
    return req;
}

static ssize_t send_file(request_t req, fd_t dest_sock) {
    assert(req.type == Get);
    ssize_t bytes_sent = send(
        dest_sock,
        req.data.get_req.contents,
        req.data.get_req.size,
        0
    );
    return bytes_sent;
}

static ssize_t notify_removal(request_t req, fd_t dest_sock) {
    assert(req.type == Remove);
    fprintf(stderr, "estou em notify_removal\n");
    size_t message_size = strlen(req.data.remove_req.filename) + 31;
    char * message = malloc(message_size);
    snprintf(message, message_size, "server: removed file '%s' [200]\n", req.data.create_req.filename);

    ssize_t bytes_sent = send(
        dest_sock,
        message,
        message_size,
        0
    );

    free(message);
    return bytes_sent;
}

static ssize_t notify_creation(request_t req, fd_t dest_sock) {
    assert(req.type == Create);
    size_t message_size = strlen(req.data.create_req.filename) + 31;
    char * message = malloc(message_size);
    snprintf(message, message_size, "server: created file '%s' [200]\n", req.data.create_req.filename);

    ssize_t bytes_sent = send(
        dest_sock,
        message,
        message_size,
        0
    );

    free(message);

    return bytes_sent;
}

static ssize_t notify_append(request_t req, fd_t dest_sock) {
    assert(req.type == Append);
    return send(
        dest_sock,
        "APPEND returned 200.\n",
        22,
        0
    );
}

ssize_t send_response(request_t req, fd_t dest_sock) {
    switch (req.type) {
        case Get:
            return send_file(req, dest_sock);
        case Append:
            return notify_append(req, dest_sock);
        case Create:
            return notify_creation(req, dest_sock);
        case Remove:
            return notify_removal(req, dest_sock);
    }
    // Should be unreachable
    exit(1);
}

ssize_t send_err(fd_t dest, unsigned short status) {
    switch (status) {
        case 400:
            return send(dest, "400 - Bad Request\n", 19, 0);
        case 404:
            return send(dest, "404 - Not Found\n", 17, 0);
        case 422:
            return send(dest, "422 - Unprocessable Entity\n", 27, 0);
        case 500:
            return send(dest, "500 - Internal Server Error", 28, 0);
        default:
            exit(1); // Should be unreachable
    }
}
