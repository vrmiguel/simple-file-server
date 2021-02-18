#ifndef UTILS_H
#define UTILS_H
#include <netdb.h>

typedef int fd_t;

char * rtrim(char *);
void * get_address(struct sockaddr *);

#endif // UTILS_H
