#include <stdio.h>       //! For {'',f}printf, stderr, perror
#include <stdlib.h>      //! For exit
#include <unistd.h>      //! For fork, close
#include <errno.h>       //! For errno
#include <netdb.h>       //! For struct addrinfo
#include <arpa/inet.h>   //! For inet_ntop
#include <sys/wait.h>    //! For waitpid

#include "config.h"      //! For g_port, g_backlog
#include "utils.h"

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


// get sockaddr, IPv4 or IPv6:
static void *get_in_addr(struct sockaddr *sa)
{
    if (sa->sa_family == AF_INET) {
        return &(((struct sockaddr_in*)sa)->sin_addr);
    }

    return &(((struct sockaddr_in6*)sa)->sin6_addr);
}

int main(void)
{

    printf("Simple file server -- Vinícius R. Miguel & Celso");

    //! The socket on which we'll be listening on
    fd_t sockfd;
    //! Is going to hold the new connections
    fd_t new_conn;
    struct addrinfo hints = {0};
    //! service_info will hold the return of getaddrinfo, meaning that it'll
    //! contain an Internet address that can be specified
    //! in a call to bind or connect.
    struct addrinfo *service_info;
    struct addrinfo * p;
    //! Connector's address information
    struct sockaddr_storage their_addr;

    struct sigaction sa;
    int yes=1;
    char s[INET6_ADDRSTRLEN];

    //  --> ai_family
//    This field specifies the desired address family for the
//    returned addresses.  Valid values for this field include
//    AF_INET and AF_INET6.  The value AF_UNSPEC indicates that
//    getaddrinfo() should return socket addresses for any
//    address family (either IPv4 or IPv6, for example) that can
//    be used with node and service.

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

    // loop through all the results and bind to the first we can
    for(p = service_info; p != NULL; p = p->ai_next) {
        if ((sockfd = socket(p->ai_family, p->ai_socktype,
                p->ai_protocol)) == -1) {
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

    sa.sa_handler = signal_handler; // reap all dead processes
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
        // ---- recv
        char request[2048] = {0};
        ssize_t bytes_recvd = // The quantity of bytes received from the client
                recv(
                    new_conn,
                    request,
                    g_max_request_len,
                    0
        );

        if (bytes_recvd > 0) {
            printf("server: %ld bytes received\nserver: request: %s", bytes_recvd, request);
        }



        inet_ntop(their_addr.ss_family,
            get_in_addr((struct sockaddr *)&their_addr),
            s, sizeof s);
        printf("server: got connection from %s\n", s);

        if (!fork()) { // this is the child process
            close(sockfd); // child doesn't need the listener
            if (send(new_conn, "<message>", 13, 0) == -1)
                perror("send");
            close(new_conn);
            exit(0);
        }
        close(new_conn);  // parent doesn't need this
    }
    close(sockfd);

    return 0;
}

