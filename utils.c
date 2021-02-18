#include "utils.h"
#include "ctype.h"
#include "string.h"

//!
//! get_address returns either an IPv4 or an IPv6 address, depending on the passed sockaddr *.
//!
void *get_address(struct sockaddr *sa)
{
    if (sa->sa_family == AF_INET) {
        //! The given address is an IPv4 address
        struct sockaddr_in * address = (struct sockaddr_in*) sa;
        return &(address->sin_addr);
    }

    //! The given address is an IPv6 address
    struct sockaddr_in6 * address = (struct sockaddr_in6*) sa;
    return &(address->sin6_addr);
}

char * rtrim(char *s)
{
    char* back = s + strlen(s);
    while(isspace(*--back));
    *(back+1) = '\0';
    return s;
}
