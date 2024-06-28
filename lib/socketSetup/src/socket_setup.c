#include "socket_setup.h"

int set_tcp_ipv4_socket(struct sockaddr_in address_ipv4, int port, int MAX_CONNECTIONS){
	/* creating socket A */
    int socketA_fd = socket(AF_INET, SOCK_STREAM, 0);
    if (socketA_fd < 0) {
        perror("ERROR while creating socket B");
        exit(EXIT_FAILURE);
    }
    // Allow the socket to be reused immediately after it is closed
    int option = 1;
    setsockopt(socketA_fd, SOL_SOCKET, SO_REUSEPORT, &option, sizeof(option));

	/* setting socket address for ipv4 */
	address_ipv4.sin_family = AF_INET;
	address_ipv4.sin_addr.s_addr = INADDR_ANY;
	address_ipv4.sin_port = htons((uint16_t)port);

	/* binding address_ipv4 and socket */
	if (bind(socketA_fd, (struct sockaddr*)&address_ipv4, sizeof(address_ipv4)) < 0) {
		perror("BIND FAILED for IPv4 TCP");
		exit(EXIT_FAILURE);
	} else {
        printf("TCP IPv4 SERVER ON :)\n");
    }

	/* Prepare to accept connections on socket FD. MAX_CONNECTIONS connection requests will be queued before further requests are refused.*/
	if (listen(socketA_fd, MAX_CONNECTIONS) < 0) {
		perror("LISTEN FAILED");
		exit(EXIT_FAILURE);
	}
	return socketA_fd;
}

int set_tcp_ipv6_socket(struct sockaddr_in6 address_ipv6, int port, int MAX_CONNECTIONS){
	/* creating socket B */
    int socketB_fd = socket(AF_INET6, SOCK_STREAM, 0);
    if (socketB_fd < 0) {
        perror("ERROR while creating socket B");
        exit(EXIT_FAILURE);
    }
    int option = 1;
    setsockopt(socketB_fd, SOL_SOCKET, SO_REUSEPORT, &option, sizeof(option));

	/* setting socket address for ipv6 */
	address_ipv6.sin6_family = AF_INET6;
	address_ipv6.sin6_addr = in6addr_any;
	address_ipv6.sin6_port = htons((uint16_t)port);

	/* binding address_ipv6 and socket */
	if (bind(socketB_fd, (struct sockaddr*)&address_ipv6, sizeof(address_ipv6)) < 0) {
		perror("BIND FAILED for IPv6 TCP");
		exit(EXIT_FAILURE);
	} else {
		printf("TCP IPv6  SERVER ON :)\n");
	}

	/* Prepare to accept connections on socket FD. MAX_CONNECTIONS connection requests will be queued before further requests are refused.*/
	if (listen(socketB_fd, MAX_CONNECTIONS) < 0) {
		perror("LISTEN FAILED");
		exit(EXIT_FAILURE);
	}
	return socketB_fd;
}

int set_udp_ipv4_socket(struct sockaddr_in address_ipv4, int port) {
    // Crear el socket UDP
    int socket_fd = socket(AF_INET, SOCK_DGRAM, 0);
    if (socket_fd < 0) {
        perror("ERROR while creating UDP socket");
        exit(EXIT_FAILURE);
    }

    // Configurar la opción SO_REUSEADDR
    int option = 1;
    if (setsockopt(socket_fd, SOL_SOCKET, SO_REUSEPORT, &option, sizeof(option)) < 0) {
        perror("ERROR setting UDP socket option");
        exit(EXIT_FAILURE);
    }

    // Configurar la dirección del servidor IPv4
    address_ipv4.sin_family = AF_INET;
    address_ipv4.sin_addr.s_addr = INADDR_ANY;
    address_ipv4.sin_port = htons((uint16_t)port);

    // Asociar la dirección con el socket
    if (bind(socket_fd, (struct sockaddr*)&address_ipv4, sizeof(address_ipv4)) < 0) {
        perror("BIND FAILED for IPv4 UDP");
        exit(EXIT_FAILURE);
    } else {
        printf("UDP IPv4 SERVER ON :)\n");
    }

    return socket_fd;
}

int set_udp_ipv6_socket(struct sockaddr_in6 address_ipv6, int port) {
    // Crear el socket UDP
    int socket_fd = socket(AF_INET6, SOCK_DGRAM, 0);
    if (socket_fd < 0) {
        perror("ERROR while creating UDP socket");
        exit(EXIT_FAILURE);
    }

    // Configurar la opción SO_REUSEADDR
    int option = 1;
    if (setsockopt(socket_fd, SOL_SOCKET, SO_REUSEPORT, &option, sizeof(option)) < 0) {
        perror("ERROR setting UDP socket option");
        exit(EXIT_FAILURE);
    }

    // Configurar la dirección del servidor IPv6
    address_ipv6.sin6_family = AF_INET6;
    address_ipv6.sin6_addr = in6addr_any;
    address_ipv6.sin6_port = htons((uint16_t)port);

    // Asociar la dirección con el socket
    if (bind(socket_fd, (struct sockaddr*)&address_ipv6, sizeof(address_ipv6)) < 0) {
        perror("BIND FAILED for IPv6 UDP");
        exit(EXIT_FAILURE);
    } else {
        printf("UDP IPv6 SERVER ON :)\n");
    }

    return socket_fd;
}

int set_unix_socket(const char *socket_path, int MAX_CONNECTIONS, int connection_oriented) {
    // Create the Unix domain socket
    int socket_fd = socket(AF_UNIX, (connection_oriented ? SOCK_STREAM : SOCK_DGRAM), 0);
    if (socket_fd < 0) {
        perror("ERROR while creating Unix domain socket");
        exit(EXIT_FAILURE);
    }

    // Set the Unix domain socket address
    struct sockaddr_un address_unix;
    address_unix.sun_family = AF_UNIX;
    strncpy(address_unix.sun_path, socket_path, sizeof(address_unix.sun_path) - 1);
    address_unix.sun_path[sizeof(address_unix.sun_path) - 1] = '\0';

    // Bind the address to the socket
    if (bind(socket_fd, (struct sockaddr*)&address_unix, sizeof(address_unix)) < 0) {
        perror("BIND FAILED for Unix domain socket");
        exit(EXIT_FAILURE);
    } else if (connection_oriented == 0) {
        printf("UNIX SOCKET READY (connectionless) \u2713\n");
    } else{
        printf("UNIX SOCKET READY (connection-oriented) \u2713\n");
    }

    // If connection-oriented, prepare to accept connections on the socket.
    if (connection_oriented) {
        if (listen(socket_fd, MAX_CONNECTIONS) < 0) {
            perror("LISTEN FAILED");
            exit(EXIT_FAILURE);
        }
    }

    return socket_fd;
}

int set_tcp_socket(struct sockaddr_in6 address_ipv6, int port, int MAX_CONNECTIONS) {
    int sd = -1;
    int on = 1;

    // Create the socket
    if ((sd = socket(AF_INET6, SOCK_STREAM, 0)) < 0) {
        perror("socket() failed");
        exit(EXIT_FAILURE);
    }

    // Allow the local address to be reused
    if (setsockopt(sd, SOL_SOCKET, SO_REUSEADDR, (char *)&on, sizeof(on)) < 0) {
        perror("setsockopt(SO_REUSEADDR) failed");
        exit(EXIT_FAILURE);
    }

    // Bind the socket to the specified port and any available address
    memset(&address_ipv6, 0, sizeof(address_ipv6));
    address_ipv6.sin6_family = AF_INET6;
    address_ipv6.sin6_port = htons((uint16_t)port);
    address_ipv6.sin6_addr = in6addr_any;

    if (bind(sd, (struct sockaddr *)&address_ipv6, sizeof(address_ipv6)) < 0) {
        perror("bind() failed");
        exit(EXIT_FAILURE);
    }

    // Get the address information after binding
    struct sockaddr_in6 local_address;
    socklen_t address_length = sizeof(local_address);
    if (getsockname(sd, (struct sockaddr *)&local_address, &address_length) < 0) {
        perror("getsockname() failed");
        exit(EXIT_FAILURE);
    }
    // Convert IPv6 address to human-readable form
    char ip_address[INET6_ADDRSTRLEN];
    if (inet_ntop(AF_INET6, &local_address.sin6_addr, ip_address, INET6_ADDRSTRLEN) == NULL) {
        perror("inet_ntop() failed");
        exit(EXIT_FAILURE);
    }

    // Print the address and port
    printf("TCP SOCKET READY \u2713\n");
    printf("Listening on [%s]:%d\n", ip_address, ntohs(local_address.sin6_port));

    // Listen for incoming connections
    if (listen(sd, MAX_CONNECTIONS) < 0) {
        perror("listen() failed");
        exit(EXIT_FAILURE);
    }

    return sd;
}

int set_udp_socket(struct sockaddr_in6 address_ipv6, int port) {
    // Create the UDP socket
    int socket_fd = socket(AF_INET6, SOCK_DGRAM, 0);
    if (socket_fd < 0) {
        perror("ERROR while creating UDP socket");
        exit(EXIT_FAILURE);
    }

    int option = 0;
    if (setsockopt(socket_fd, SOL_SOCKET, SO_REUSEPORT, &option, sizeof(option)) < 0) {
        perror("ERROR setting UDP socket option");
        exit(EXIT_FAILURE);
    }

    memset(&address_ipv6, '\0', sizeof(address_ipv6));
    // Configure the address structure for the server
    address_ipv6.sin6_family = AF_INET6;
    address_ipv6.sin6_addr = in6addr_any;
    address_ipv6.sin6_port = htons((uint16_t)port);

    // Associate the address with the socket
    if (bind(socket_fd, (struct sockaddr*)&address_ipv6, sizeof(address_ipv6)) < 0) {
        perror("BIND FAILED for IPv6 UDP");
        exit(EXIT_FAILURE);
    } else {
        printf("UDP SOCKET READY \u2713 \n");
    }

    return socket_fd;
}
