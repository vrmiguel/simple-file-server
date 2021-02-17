#ifndef REQUEST_H
#define REQUEST_H

#include "utils.h"
#include <stddef.h> // For size_t
#include <unistd.h> // For ssize_t

enum request_type {
    Append,
    Create,
    Get,
    Remove
};

struct get_request_s {
    char * contents;
    size_t size;
};

struct append_request_s {
    //! The text to be inserted on the file
    char * to_be_added;
    //! New bytes to be added (size of to_be_added)
    size_t new_bytes;
    //! The name of the file on where the text will be inserted
    char * filename;
};

struct remove_request_s {
    //! The name of the file to be removed
    char * filename;
};

struct create_request_s {
    //! The name of the file to be created
    char * filename;
};

typedef enum request_type       request_type_t;
typedef struct get_request_s    get_request_t;
typedef struct create_request_s create_request_t;
typedef struct remove_request_s remove_request_t;
typedef struct append_request_s append_request_t;

struct request_s {
    request_type_t type;
    unsigned short status;
    union Data {
        get_request_t       get_req;
        create_request_t create_req;
        append_request_t append_req;
        remove_request_t remove_req;
    } data;
};

typedef struct request_s request_t;

request_t process_request(const char * request);
ssize_t send_response(request_t, fd_t);

#endif // REQUEST_H
