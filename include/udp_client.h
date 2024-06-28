#pragma once

#include "cJSON.h"
#include <arpa/inet.h>
#include <fcntl.h>
#include <getopt.h>
#include <netinet/in.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/mman.h>
#include <sys/socket.h>
#include <unistd.h>

#define SHARED_MEM_PORT "/port_shared_memory"
#define BUFFER_SIZE 1024
#define DEFAULT_PORT -1
#define OS_RELEASE_PATH "/etc/os-release"
#define OS_RELEASE_ID_FIELD 3

// Structure to store the TCP and UDP port numbers
struct PortInfo
{
    int tcp_port;
    int udp_port;
};

// Function prototypes
/**
 * @brief Parses command-line arguments to extract the port number.
 *
 * @param argc The number of command-line arguments.
 * @param argv An array of strings containing the command-line arguments.
 * @param port A pointer to an integer to store the extracted port number.
 */
// void parse_command_line_args(int argc, char *argv[], int *port, int *ipv6);
void parse_command_line_args(int argc, char* argv[], int* port, int* ipv6, char** ip_address);
/**
 * @brief Retrieves the UDP port number from shared memory.
 *
 * @return The UDP port number retrieved from shared memory.
 *         Returns -1 on failure.
 */
int get_port_whith_shared_mem();

/**
 * @brief Sets the server address.
 *
 * @param port The port number to set in the server address.
 */
void set_server_addr(int port, int ipv6);
/**
 * @brief Establishes a connection with the server.
 *
 * @param port The port number to connect to.
 * @return The socket file descriptor if successful, otherwise exits the program.
 */
int establish_connection(int port, int ipv6);

/**
 * @brief Sends a message to the server.
 *
 * @param sockfd The socket file descriptor.
 * @param message The message to send.
 * @param server_addr The server address structure.
 */
void send_to_server_v6(int sockfd, const char* message, const struct sockaddr_in6* server_addr);

void send_to_server_v4(int sockfd, const char* message, const struct sockaddr_in* server_addr);

/**
 * @brief Closes the socket connection.
 *
 * @param sockfd The socket file descriptor to close.
 */
void disconnect(int sockfd);

/**
 * @brief Sends a JSON object to the server.
 *
 * @param sockfd The socket file descriptor.
 * @param json The cJSON object to send.
 * @param server_addr The server address structure.
 */
void sendJson_v6(int sockfd, cJSON* json, const struct sockaddr_in6* server_addr);

void sendJson_v4(int sockfd, cJSON* json, const struct sockaddr_in* server_addr);

/**
 * @brief Handles SIGINT signal (Ctrl+C) by terminating the client.
 */
void handle_sigint();

/**
 * @brief Retrieves the UDP port number based on command-line arguments or shared memory.
 *
 * @param argc Number of command-line arguments.
 * @param argv Array of command-line argument strings.
 * @return The UDP port number.
 */
int get_port(int argc, char* argv[], int* port);
/**
 * @brief Redirects output to the parent process.
 */
void redirect_output_to_parent();

/**
 * @brief Handles incoming messages from the server.
 *
 * @param sockfd The socket file descriptor.
 */
void handle_server_messages(int sockfd);

/**
 * @brief Receives and handles JSON messages from the server.
 *
 * @param sockfd The socket file descriptor.
 */
void receive_and_handle_json(int sockfd);

/**
 * @brief Handles user input and sends requests to the server accordingly.
 *
 * @param sockfd The socket file descriptor.
 */
void handle_user_input(int sockfd, int ipv6);

/**
 * @brief Handles the received JSON message.
 *
 * @param json_string The received JSON message as a string.
 */
void handle_received_json(const char* json_string);

/**
 * @brief Prints menu options for user interaction.
 */
void print_menu_options();

/**
 * @brief Retrieves the operating system ID.
 *
 * @return The operating system ID.
 */
char* get_os_id();

/**
 * @brief Extracts the value from a line of text based on a delimiter.
 *
 * @param line The line of text to extract the value from.
 * @return The extracted value.
 */
char* extract_value(char* line);

/**
 * @brief Encodes a string to UTF-8 format.
 *
 * @param input The input string to encode.
 * @return The UTF-8 encoded string.
 */
char* encode_utf8(const char* input);
