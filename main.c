#include <stdio.h>       //! For {'',f}printf, stderr, perror
#include <stdlib.h>      //! For exit
#include <unistd.h>      //! For fork, close
#include <errno.h>       //! For errno
#include <netdb.h>       //! For struct addrinfo
#include <arpa/inet.h>   //! For inet_ntop
#include <sys/wait.h>    //! For waitpid

#include "config.h"      //! For g_port, g_backlog
#include "utils.h"
#include "request.h"

#define PRINT_STATUS printf("server: finished request with status %d.", req.status);

void signal_handler(int s)
{
    if (s == SIGINT) {
        fprintf(stderr, "SIGINT received. Exiting.");
        exit(0);
    }

    // waitpid() might overwrite errno, so we save and restore it:
    int saved_errno = errno;

    while(waitpid(-1, NULL, WNOHANG) > 0);

    errno = saved_errno;
}


int main(void)
{

    printf("Simple file server -- Vinícius R. Miguel & Celso Vieira Ribeiro Lopes\n");

    //! The socket on which we'll be listening on
    fd_t sockfd;
    //! Is going to hold the new connections
    fd_t new_conn;
    struct addrinfo hints = {0};
    //! service_info will hold the return of getaddrinfo, meaning that it'll
    //! contain an Internet address that can be specified
    //! in a call to bind or connect.
    struct addrinfo *service_info;
    //! Connector's address information
    struct sockaddr_storage their_addr;

    struct sigaction sa;

    hints.ai_family   = AF_UNSPEC;
    hints.ai_socktype = SOCK_STREAM;
    hints.ai_flags    = AI_PASSIVE; // use my IP

    int ret_val = getaddrinfo(
                NULL,       //    The AI_PASSIVE flag was specified in hints.ai_flags, and node == NULL,
                            //    so the returned socket addresses will be suitable for
                            //    binding a socket that will accept connections.  The
                            //    returned socket address will contain INADDR_ANY for IPv4 addresses, IN6ADDR_ANY_INIT for IPv6
                            //    address (wildcard addresses).
                            //    This wildcard address is used by applications
                            //    (typically servers) that intend to accept connections on any of
                            //    the host's network addresses.

                g_port,     //
                &hints,     //
                &service_info   //
    );

    if (ret_val != 0) {
        fprintf(stderr, "getaddrinfo: %s\n", gai_strerror(ret_val));
        return 1;
    }

    struct addrinfo * p;
    int yes=1;
    for(p = service_info; p != NULL; p = p->ai_next) {
        sockfd = socket(
                    p->ai_family,
                    p->ai_socktype,
                    p->ai_protocol
        );

        if (sockfd == -1){
            perror("server: socket");
            continue;
        }

        if (setsockopt(sockfd, SOL_SOCKET, SO_REUSEADDR, &yes,
                sizeof(int)) == -1) {
            perror("setsockopt");
            exit(1);
        }

        if (bind(sockfd, p->ai_addr, p->ai_addrlen) == -1) {
            close(sockfd);
            perror("server: bind");
            continue;
        }

        break;
    }

    freeaddrinfo(service_info); // all done with this structure

    if (p == NULL)  {
        fprintf(stderr, "server: failed to bind\n");
        exit(1);
    }

    if (listen(sockfd, g_backlog) == -1) {
        perror("listen");
        exit(1);
    }

    sa.sa_handler = signal_handler;
    sigemptyset(&sa.sa_mask);

    sa.sa_flags = SA_RESTART;
    if (sigaction(SIGCHLD, &sa, NULL) == -1) {
        perror("sigaction");
        exit(1);
    }

    printf("server: waiting for connections\n");

    while(1) {  // main accept() loop
        // ---- Accept
        socklen_t addr_length = sizeof their_addr;
        // Accept will block the server until it gets a connection
        new_conn = accept(
                    sockfd,
                    (struct sockaddr *)&their_addr,
                    &addr_length
        );
        if (new_conn == -1) {
            perror("accept");
            continue;
        }

        char client_ip[INET6_ADDRSTRLEN];
        inet_ntop(
                    their_addr.ss_family,
                    get_address((struct sockaddr *) &their_addr),
                    client_ip,
                    INET6_ADDRSTRLEN
        );
        printf("server: got connection from %s\n", client_ip);

        // ---- recv
        char request[2048] = {0};
        ssize_t bytes_recvd = // The quantity of bytes received from the client
                recv(
                    new_conn, // File descriptor of the connection
                    request,  // The buffer to where the request will be written to
                    g_max_request_len, // The max. quantity of bytes that can be written to the `request` buffer
                    0
        );

        // Trim trailing whitespace
        char * trimmed_request = rtrim(request);

        if (bytes_recvd > 0) {
            printf("server: %ld bytes received\nserver: received request: '%s'\n", bytes_recvd, request);
        }

        if (!fork()) {
            //! Code in this block runs in the child process
            close(sockfd); //! The child process doesn't need the listener anymore

            request_t req = process_request(trimmed_request);

            if (req.status != 200) {
                // Request returned failure
                PRINT_STATUS
                send_err(new_conn, req.status);
                exit(0);
            }
            ssize_t bytes_sent = send_response(req, new_conn);

//            TODO: free malloc'd memory

            if (bytes_sent == -1) {
                perror("send");
            } else {
                printf("server: finished sending %ld bytes.\n", bytes_sent);
            }

            PRINT_STATUS
            close(new_conn);
            exit(0);
        }
        close(new_conn);
    }    
    close(sockfd);
    return 0;
}

