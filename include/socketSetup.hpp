#ifndef SOCKET_SETUP_HPP
#define SOCKET_SETUP_HPP

#include <netinet/in.h>
#include <sys/un.h>

/**
 * @brief Class for setting up different types of sockets on Unix systems.
 */
class SocketSetup
{
  public:
    /**
     * @brief Default constructor of the `SocketSetup` class.
     */
    SocketSetup();

    /**
     * @brief Destructor of the `SocketSetup` class.
     */
    ~SocketSetup();

    /**
     * @brief Sets up a Unix domain socket.
     *
     * @param socket_path The path of the socket.
     * @param MAX_CONNECTIONS The maximum number of connections allowed for connection-oriented sockets.
     * @param connection_oriented Boolean indicating whether the socket is connection-oriented (true) or not (false).
     * @return The file descriptor of the configured Unix socket.
     *
     * This method creates and configures a Unix domain socket. If `connection_oriented` is true, the socket is set up
     * as connection-oriented.
     */
    int setUnixSocket(const char* socket_path, int MAX_CONNECTIONS, bool connection_oriented);

    /**
     * @brief Sets up an IPv6 TCP socket.
     *
     * @param address_ipv6 sockaddr_in6 structure containing IPv6 address information.
     * @param port The port to which the socket will be bound.
     * @param MAX_CONNECTIONS The maximum number of connections allowed.
     * @return The file descriptor of the configured IPv6 TCP socket.
     *
     * This method creates and configures an IPv6 TCP socket. It allows reuse of local addresses. The socket is bound to
     * the specified port and any available address.
     */
    int setTcpSocket(struct sockaddr_in6 address_ipv6, int port, int MAX_CONNECTIONS);

    /**
     * @brief Sets up an IPv6 UDP socket.
     *
     * @param address_ipv6 sockaddr_in6 structure containing IPv6 address information.
     * @param port The port to which the socket will be bound.
     * @return The file descriptor of the configured IPv6 UDP socket.
     *
     * This method creates and configures an IPv6 UDP socket. It allows reuse of ports. The socket is bound to the
     * specified port and any available address.
     */
    int setUdpSocket(struct sockaddr_in6 address_ipv6, int port);
};

#endif // SOCKET_MANAGER_HPP
