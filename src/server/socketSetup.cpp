#include "socketSetup.hpp"
#include <arpa/inet.h>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <iostream>
#include <netinet/in.h>
#include <sys/socket.h>
#include <unistd.h>

SocketSetup::SocketSetup()
{
}

SocketSetup::~SocketSetup()
{
}

int SocketSetup::setUnixSocket(const char* socket_path, int MAX_CONNECTIONS, bool connection_oriented)
{
    // Create the Unix domain socket
    int socket_fd = socket(AF_UNIX, (connection_oriented ? SOCK_STREAM : SOCK_DGRAM), 0);
    if (socket_fd < 0)
    {
        perror("ERROR while creating Unix domain socket");
        exit(EXIT_FAILURE);
    }

    // Set the Unix domain socket address
    struct sockaddr_un address_unix;
    address_unix.sun_family = AF_UNIX;
    strncpy(address_unix.sun_path, socket_path, sizeof(address_unix.sun_path) - 1);
    address_unix.sun_path[sizeof(address_unix.sun_path) - 1] = '\0';

    // Bind the address to the socket
    if (bind(socket_fd, (struct sockaddr*)&address_unix, sizeof(address_unix)) < 0)
    {
        perror("BIND FAILED for Unix domain socket");
        exit(EXIT_FAILURE);
    }
    else if (!connection_oriented)
    {
        std::cout << "UNIX SOCKET READY (connectionless) \u2713" << std::endl;
    }
    else
    {
        std::cout << "UNIX SOCKET READY (connection-oriented) \u2713" << std::endl;
    }

    // If connection-oriented, prepare to accept connections on the socket.
    if (connection_oriented)
    {
        if (listen(socket_fd, MAX_CONNECTIONS) < 0)
        {
            perror("LISTEN FAILED");
            exit(EXIT_FAILURE);
        }
    }

    return socket_fd;
}

int SocketSetup::setTcpSocket(struct sockaddr_in6 address_ipv6, int port, int MAX_CONNECTIONS)
{
    int sd = -1;
    int on = 1;

    // Create the socket
    if ((sd = socket(AF_INET6, SOCK_STREAM, 0)) < 0)
    {
        perror("socket() failed");
        exit(EXIT_FAILURE);
    }

    // Allow the local address to be reused
    if (setsockopt(sd, SOL_SOCKET, SO_REUSEADDR, (char*)&on, sizeof(on)) < 0)
    {
        perror("setsockopt(SO_REUSEADDR) failed");
        exit(EXIT_FAILURE);
    }

    // Bind the socket to the specified port and any available address
    memset(&address_ipv6, 0, sizeof(address_ipv6));
    address_ipv6.sin6_family = AF_INET6;
    address_ipv6.sin6_port = htons((uint16_t)port);
    address_ipv6.sin6_addr = in6addr_any;

    if (bind(sd, (struct sockaddr*)&address_ipv6, sizeof(address_ipv6)) < 0)
    {
        perror("bind() failed");
        exit(EXIT_FAILURE);
    }

    // Get the address information after binding
    struct sockaddr_in6 local_address;
    socklen_t address_length = sizeof(local_address);
    if (getsockname(sd, (struct sockaddr*)&local_address, &address_length) < 0)
    {
        perror("getsockname() failed");
        exit(EXIT_FAILURE);
    }
    // Convert IPv6 address to human-readable form
    char ip_address[INET6_ADDRSTRLEN];
    if (inet_ntop(AF_INET6, &local_address.sin6_addr, ip_address, INET6_ADDRSTRLEN) == NULL)
    {
        perror("inet_ntop() failed");
        exit(EXIT_FAILURE);
    }

    // Print the address and port
    std::cout << "TCP SOCKET READY \u2713" << std::endl;
    std::cout << "Listening on [" << ip_address << "]:" << ntohs(local_address.sin6_port) << std::endl;

    // Listen for incoming connections
    if (listen(sd, MAX_CONNECTIONS) < 0)
    {
        perror("listen() failed");
        exit(EXIT_FAILURE);
    }

    return sd;
}

int SocketSetup::setUdpSocket(struct sockaddr_in6 address_ipv6, int port)
{
    // Create the UDP socket
    int socket_fd = socket(AF_INET6, SOCK_DGRAM, 0);
    if (socket_fd < 0)
    {
        perror("ERROR while creating UDP socket");
        exit(EXIT_FAILURE);
    }

    int option = 0;
    if (setsockopt(socket_fd, SOL_SOCKET, SO_REUSEPORT, &option, sizeof(option)) < 0)
    {
        perror("ERROR setting UDP socket option");
        exit(EXIT_FAILURE);
    }

    memset(&address_ipv6, '\0', sizeof(address_ipv6));
    // Configure the address structure for the server
    address_ipv6.sin6_family = AF_INET6;
    address_ipv6.sin6_addr = in6addr_any;
    address_ipv6.sin6_port = htons((uint16_t)port);

    // Associate the address with the socket
    if (bind(socket_fd, (struct sockaddr*)&address_ipv6, sizeof(address_ipv6)) < 0)
    {
        perror("BIND FAILED for IPv6 UDP");
        exit(EXIT_FAILURE);
    }
    else
    {
        std::cout << "UDP SOCKET READY \u2713" << std::endl;
    }

    return socket_fd;
}
