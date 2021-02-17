#ifndef UTILS_H
#define UTILS_H
#include <netdb.h>

// Signal that the passed variable is unused.
void unused_var(int unused);
void * get_address(struct sockaddr *);


#endif // UTILS_H
