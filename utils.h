#ifndef UTILS_H
#define UTILS_H
#include <netdb.h>

struct file_request_s {
    char * contents;
    size_t size;
    unsigned char status;
};
typedef struct file_request_s file_request_t;


void * get_address(struct sockaddr *);
file_request_t get_file(const char *);

#endif // UTILS_H
