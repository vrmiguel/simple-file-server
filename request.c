#include "request.h"
#include "utils.h"
#include <string.h>
#include <stdbool.h>
#include <stdio.h>
#include <ctype.h>
#include <stdlib.h>
#include <assert.h>

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
        fprintf(stderr, "server: error: missing body in GET request.\n");
        req_struct.status = 1;
    }

    char * filename = strdup(request + 4);
    if (filename[strlen(filename)-1] == '\n') {
        printf("newline found");
    }

    FILE *file = fopen(filename, "rb");
    if (!file) {
        fprintf(stderr, "server: error: could not open file '%s'.\n", filename);
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
//        return ;
    }


    else if (is_append_req(request)) {

    }


    else if (is_remove_req(request)) {
        //
    }

}

static ssize_t send_file(request_t req, fd_t dest_sock) {
    ssize_t bytes_sent = send(
        dest_sock,
        req.data.get_req.contents,
        req.data.get_req.size,
        0
    );
    return bytes_sent;
}


ssize_t send_response(request_t req, fd_t dest_sock) {
    switch (req.type) {
        case Get:
            return send_file(req, dest_sock);
        case Append: break;
        case Create: break;
        case Remove: break;
    }
}
