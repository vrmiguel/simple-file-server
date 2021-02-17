#include "utils.h"

// On socket and datagram sockets:
//  https://stackoverflow.com/questions/5815675/what-is-sock-dgram-and-sock-stream

//struct addrinfo make_tcp_addr_info() {

//};

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
