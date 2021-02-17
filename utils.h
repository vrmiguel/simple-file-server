#ifndef UTILS_H
#define UTILS_H
#include <netdb.h>

//struct get_request_s {
//    char * contents;
//    size_t size;
//    unsigned char status;
//};
//typedef struct get_request_s get_request_t;

typedef int fd_t;

//struct

char * rtrim(char *);

void * get_address(struct sockaddr *);
//get_request_t get_file(const char *);

#endif // UTILS_H
