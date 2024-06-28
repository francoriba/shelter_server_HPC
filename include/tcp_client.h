#pragma once

#include "cJSON.h"
#include <arpa/inet.h>
#include <fcntl.h>
#include <getopt.h>
#include <netdb.h>
#include <netinet/in.h>
#include <signal.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/mman.h>
#include <sys/select.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <unistd.h>

#define SERVER_IP_V4_LOOP "127.0.0.1"
#define SERVER_IP_V6_LOOP "::1"
#define SHARED_MEM_PORT "/port_shared_memory"
#define BUFFER_SIZE 1024
#define DEFAULT_PORT -1
#define OS_RELEASE_PATH "/etc/os-release"
#define OS_RELEASE_ID_FIELD 3
#define SOCK_PATH "/tmp/client_unix_sock"
#define SAVE_ZIP_PATH "./cannyResult.zip"

/**
 * @brief Structure to store the TCP and UDP port numbers.
 */
struct PortInfo
{
    int tcp_port; /**< TCP port number. */
    int udp_port; /**< UDP port number. */
};

// Function prototypes
/**
 * @brief Parses command-line arguments to extract the port number.
 *
 * @param argc The number of command-line arguments.
 * @param argv An array of strings containing the command-line arguments.
 * @param port A pointer to an integer to store the extracted port number.
 * @param ip_address A pointer to a string to store the extracted ip address.
 */
void parse_command_line_args(int argc, char* argv[], int* port, int* ipv6, char** ip_address);

/**
 * @brief Retrieves the UDP port number from shared memory.
 *
 * @return The UDP port number retrieved from shared memory.
 *         Returns -1 on failure.
 */
int get_port_whith_shared_mem();

/**
 * @brief Establishes a connection with the server.
 *
 * This function creates a socket, configures the server address, and establishes
 * a connection with the server using the specified port number.
 *
 * @param port The port number to connect to.
 * @param ipv6 A flag to indicate whether to use IPv6.
 * @return The socket file descriptor for the established connection.
 */
int establish_connection(int port, int ipv6);

/**
 * @brief Closes the connection with the server.
 *
 * This function closes the connection with the server by closing the socket.
 *
 * @param sockfd The socket file descriptor.
 */
void disconnect(int sockfd);

/**
 * @brief Sends a JSON message to the server.
 *
 * This function sends a JSON message to the server over the established connection.
 *
 * @param sockfd The socket file descriptor.
 * @param json A cJSON object representing the JSON message to send.
 */
void send_json(int sockfd, cJSON* json);

/**
 * @brief Handles the SIGINT signal.
 *
 * This function handles the SIGINT signal (Ctrl+C) by terminating the client.
 */
void handle_sigint();

/**
 * @brief Handles server messages.
 *
 * This function continuously receives JSON messages from the server
 * and processes them accordingly.
 *
 * @param sockfd The socket file descriptor.
 */
void handle_server_messages(int sockfd);

/**
 * @brief Handles user input and interacts with the server.
 *
 * This function continuously prompts the user for input, sends requests to the server,
 * and processes server responses.
 *
 * @param sockfd The socket file descriptor.
 */
void handle_user_input(int sockfd);

/**
 * @brief Receives a JSON message from the server.
 *
 * This function receives a JSON message from the server over the established connection
 * and processes it accordingly.
 *
 * @param sockfd The socket file descriptor.
 */
void receive_json(int sockfd);

/**
 * @brief Retrieves the port number from command-line arguments or shared memory.
 *
 * This function retrieves the port number from command-line arguments, if provided,
 * or from shared memory if not provided.
 *
 * @param port A pointer to an integer to store the port number.
 * @return The port number obtained either from command-line arguments or shared memory.
 */
void get_port(int* port);

/**
 * @brief Redirects output to the parent process.
 *
 * This function redirects output to the parent process.
 */
void redirect_output_to_parent();

/**
 * @brief Prints menu options for user interaction.
 *
 * This function prints menu options for user interaction.
 */
void print_menu_options();

/**
 * @brief Retrieves the OS ID from a file.
 *
 * This function retrieves the OS ID from a specified file.
 *
 * @return The OS ID string.
 **/
char* get_os_id();

/**
 * @brief Extracts a value from a line.
 *
 * This function extracts a value from a line using a specified delimiter.
 *
 * @param line The input line.
 * @return The extracted value.
 */
char* extract_value(char* line);

/**
 * @brief Encodes a string to UTF-8 format.
 *
 * This function encodes a string to UTF-8 format.
 *
 * @param input The input string.
 * @return The UTF-8 encoded string.
 */
char* encode_utf8(const char* input);

/**
 * @brief Authenticates the client with the server.
 *
 * This function authenticates the client with the server by sending
 * an authentication message containing the hostname.
 *
 * @param sockfd The socket file descriptor.
 */
void authenticate(int sockfd);

/**
 * @brief Connects to a server using the provided host and port.
 *
 * This function attempts to establish a TCP connection to the specified server using the provided
 * host name or IP address and port number. It supports both IPv4 and IPv6 addresses.
 *
 * @param server The host name or IP address of the server to connect to.
 * @param port The port number to connect to on the server.
 *
 * @return On success, the file descriptor of the connected socket is returned. On failure, -1 is returned.
 */
int do_connect(char* server, char* port);

/**
 * @brief Receives a ZIP file from the server through the provided socket and writes it to the specified file path.
 * @param sockfd The socket file descriptor connected to the server from which to receive the ZIP file.
 * @param file_path The path to the file where the received ZIP file will be written.
 */
void receive_zip(int sockfd, const char* file_path);
