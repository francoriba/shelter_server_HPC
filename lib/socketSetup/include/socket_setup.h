#pragma once

#include <netinet/in.h>
#include <sys/un.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/socket.h>
#include <netdb.h>
#include <arpa/inet.h>
#include <ifaddrs.h>

/**
 * @brief Creates a Unix domain socket and configures it to listen on the specified path.
 * 
 * @param socket_path The path for the Unix domain socket.
 * @param MAX_CONNECTIONS The maximum number of pending connections in the socket's listen queue.
 * @param connection_oriented Indicates whether the socket should be connection-oriented (1) or connectionless (0).
 * 
 * @return The file descriptor of the created Unix domain socket.
 */
int set_unix_socket(const char* path, int MAX_CONNECTIONS, int connection_oriented);

/**
 * @brief Creates an IPv4 socket and configures it to listen on the specified port.
 * 
 * @param address_ipv4 The sockaddr_in structure to store the IPv4 address and port.
 * @param port The port number for the IPv4 socket.
 * @param MAX_CONNECTIONS The maximum number of pending connections in the socket's listen queue.
 * 
 * @return The file descriptor of the created IPv4 socket.
 */
int set_tcp_ipv4_socket(struct sockaddr_in address_ipv4, int port, int MAX_CONNECTIONS);

/**
 * @brief Creates TCP an IPv6 socket and configures it to listen on the specified port.
 * 
 * @param address_ipv6 The sockaddr_in6 structure to store the IPv6 address and port.
 * @param port The port number for the IPv6 socket.
 * @param MAX_CONNECTIONS The maximum number of pending connections in the socket's listen queue.
 * 
 * @return The file descriptor of the created IPv6 socket.
 */
int set_tcp_ipv6_socket(struct sockaddr_in6 address_ipv6, int port, int MAX_CONNECTIONS);

/**
 * @brief Creates an IPv4 UDP socket and configures it to listen on the specified port.
 * 
 * @param address_ipv4 The sockaddr_in structure to store the IPv4 address and port.
 * @param port The port number for the IPv4 UDP socket.
 * 
 * @return The file descriptor of the created IPv4 UDP socket.
 */
int set_udp_ipv4_socket(struct sockaddr_in address_ipv4, int port);

/**
 * @brief Creates an IPv6 UDP socket and configures it to listen on the specified port.
 * 
 * @param address_ipv6 The sockaddr_in6 structure to store the IPv6 address and port.
 * @param port The port number for the IPv6 UDP socket.
 * 
 * @return The file descriptor of the created IPv6 UDP socket.
 */
int set_udp_ipv6_socket(struct sockaddr_in6 address_ipv6, int port);

/**
 * @brief Creates a TCP socket capable of handling both IPv4 and IPv6 connections.
 * 
 * @param port The port number for the TCP socket.
 * @param MAX_CONNECTIONS The maximum number of pending connections in the socket's listen queue.
 * 
 * @return The file descriptor of the created TCP socket.
 */
int set_tcp_socket(struct sockaddr_in6 serveraddr, int port, int MAX_CONNECTIONS);

/**
 * @brief Set up a UDP socket with given IPv6 address and port.
 *
 * This function sets up a UDP socket with the provided IPv6 address and port,
 * configures the socket option SO_REUSEPORT, and binds the socket to the address.
 *
 * @param address_ipv6 The IPv6 address structure (struct sockaddr_in6) to bind the socket to.
 * @param port The port number to bind the socket to.
 *
 * @return The file descriptor of the created UDP socket.
 */
int set_udp_socket(struct sockaddr_in6 address_ipv6, int port);
