#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <unistd.h>

#define SOCK_PATH "/tmp/refugie_unix_socket"
#define FAILURE_MESSAGE "Electricity failure. Disconnecting all clients."

/**
 * @brief Initialize the emergency notification module.
 *
 * This function establishes a connection with the server via UNIX socket
 * and initializes any necessary parameters for the electricity failure simulation.
 *
 * @param socket_path The path to the UNIX socket.
 * @return 0 on success, -1 on failure.
 */
int init_emergency_notification();

/**
 * @brief Simulate an electricity failure.
 *
 * This function generates an electricity failure and notifies the server
 * via the UNIX socket. The server then sends a notification to all connected clients.
 *
 * @return 0 on success, -1 on failure.
 */
void simulate_electricity_failure(int sockfd);

/**
 * @brief Generates a random number of minutes between 5 and 10.
 *
 * This function generates a random number of minutes between 5 and 10 (inclusive).
 *
 * @return A random number of minutes between 5 and 10.
 */
int get_random_failure_minutes();
