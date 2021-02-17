#include "utils.h"
#include "stdio.h"
#include "stdlib.h"

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

file_request_t get_file(const char * filename) {
    file_request_t file_request;
    FILE *file = fopen(filename, "rb");
    if (!file) {
        fprintf(stderr, "server: error: could not open file '%s'.\n", filename);
        file_request.status = 1; // We're adopting 1 as an error, 0 as OK.
        return file_request;
    }

    // Get size of file
    fseek(file, 0L, SEEK_END);
    size_t sz = ftell(file);
    rewind(file);

    file_request.contents = malloc(sz);
    fread(file_request.contents, sz, 1, file);
    file_request.size = sz;
    file_request.status = 0;

    return file_request;
}
