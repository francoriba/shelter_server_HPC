#ifndef SERVER_HPP
#define SERVER_HPP

#include "cannyEdgeFilter.hpp"
#include "httplib.h"
//#include "rocksDbWrapper.hpp"
#include "myRocksDbWrapper.hpp"
#include "socketSetup.hpp"
#include "utils.hpp"
#include <nlohmann/json.hpp>

#include <algorithm>
#include <array>
#include <cerrno>
#include <chrono>
#include <csignal>
#include <cstdio>
#include <cstring>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <memory>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>
#include <vector>
//#include <zip.h>

#define CONN_ORIENTED 1
#define MAX_UNIX_CONNECTIONS 1
#define BUFFER_SIZE 1024
#define BUFFER_521 512
#define BUFFER_256 256
#define BUFFER_64 64
#define SECONDS_IN_MINUTE 60
#define ADMIN_USER "ubuntu"

using json = nlohmann::json;
namespace fs = std::filesystem;

extern "C"
{
#include "../lib/alertInfection/include/alertInfection.h"
#include "../lib/emergNotif/include/emergNotif.h"
#include "../lib/suppliesData/include/supplies_module.h"
}

constexpr int MAX_TCP_CONNECTIONS = 10;
constexpr int MAX_UDP_CONNECTIONS = 10;

/**
 * @brief Enumeration representing the address family (IPv4 or IPv6).
 */
enum class AddressFamily
{
    IPv4, /**< IPv4 address family. */
    IPv6  /**< IPv6 address family. */
};

/**
 * @brief Struct to hold data related to a UDP client.
 */
struct UDPClientData
{
    int sockfd;                   /**< Socket file descriptor. */
    sockaddr_storage client_addr; /**< Client address structure. */
    socklen_t addr_len;           /**< Size of the client address structure. */
    AddressFamily family;         /**< Address family (IPv4 or IPv6). */
};

/**
 * @brief Structure to hold the count of alerts for each entry.
 */
struct EntryAlertsCount
{
    int north; /**< Number of alerts in the north direction. */
    int south; /**< Number of alerts in the south direction. */
    int east;  /**< Number of alerts in the east direction. */
    int west;  /**< Number of alerts in the west direction. */

    /**
     * @brief Constructor to initialize alerts count to zero.
     */
    EntryAlertsCount() : north(0), south(0), east(0), west(0)
    {
    }
};

/**
 * @brief Class implemented as a refactor of the first lab server code in C.
 */
class Server
{
  public:
    /**
     * @brief Constructor for the Server class.
     *
     * Initializes a Server object with the specified TCP and UDP ports.
     * Additionally, sets up signal handlers and initializes ID generators for supplies, alerts, and emergency
     * notifications.
     *
     * @param tcp_port The TCP port number to listen on.
     * @param udp_port The UDP port number to listen on.
     */
    Server(int tcp_port, int udp_port);

    /**
     * @brief Destructor for the Server class.
     *
     * This destructor deallocates any dynamically allocated resources owned by the Server class.
     * It ensures proper cleanup when instances of Server are destroyed.
     */
    ~Server();

    /**
     * @brief Starts the server and enters the main event loop to handle incoming connections and events.
     *
     * This method initializes necessary server components, such as logging, directory creation, and socket setup,
     * before entering a continuous loop to handle incoming connections and events from various sockets.
     * It utilizes TCP, UDP, and Unix domain sockets for communication.
     */
    void start();

    /**
     * @brief Stops the server.
     */
    void stop();

    // private:
    /**
     * @brief Prefix for keys related to alerts.
     */
    const std::string ALERTS_KEY_PREFIX = "alert_";

    /**
     * @brief Prefix for keys related to supplies.
     */
    const std::string SUPPLIES_KEY_PREFIX = "supplies_";

    /**
     * @brief Prefix for keys related to emergency notifications.
     */
    const std::string EMERGENCY_NOTIF_KEY_PREFIX = "emergencyNotification_";

    /**
     * @brief Key for the last alert.
     */
    const std::string LAST_ALERT_ID_KEY = "last_alert";
    /**
     * @brief Key for the last supplies.
     */
    const std::string LAST_SUPPLIES_ID_KEY = "last_supplies";
    /**
     * @brief Key for the last notification.
     */
    const std::string LAST_NOTIF_ID_KEY = "last_notif";

    /**
     * @brief Key for the last event.
     */
    const std::string LAST_EVENT_KEY = "lastEvent";

    /**
     * @brief Key for the latest alert.
     */
    const std::string LATEST_ALERT_KEY = "latestAlert";

    /**
     * @brief Key for the latest supplies.
     */
    const std::string LATEST_SUPPLIES_KEY = "latestSupplies";

    /**
     * @brief Path to the image directory.
     */
    const std::string IMAGE_PATH = "../img/inputImg/";

    /**
     * @brief Path to the zip files directory.
     */
    const std::string ZIP_PATH = "../img/zipFiles/";

    /**
     * @brief Path to the output image directory.
     */
    const std::string CONVERTION_OUT_PATH = "../img/outputImg/";

    /**
     * @brief Name of the Canny result image.
     */
    const std::string CANNY_RESULT = "canny.png";

    /**
     * @brief Path to the alerts FIFO.
     */
    const char* fifoPath = "/tmp/alerts_fifo2";

    /**
     * @brief Port for the REST API.
     */
    const int REST_API_PORT = 8011;

    /**
     * @brief TCP port.
     */
    int tcp_port_;

    /**
     * @brief UDP port.
     */
    int udp_port_;

    /**
     * @brief Flag indicating if the server is running.
     */
    static bool SERVER_RUNNING;

    /**
     * @brief Pointer to the server instance.
     */
    static Server* serverInstance;

    /**
     * @brief Process ID for alerts.
     */
    pid_t alertsPid;

    /**
     * @brief Process ID for power outage.
     */
    pid_t powerOutagePid;

    /**
     * @brief Process ID for REST listener.
     */
    pid_t restListener;

    /**
     * @brief Count of entry alerts.
     */
    EntryAlertsCount entryAlertsCount;

    /**
     * @brief ID generator for supplies.
     */
    Utils::IdGen* suppliesIdGen;

    /**
     * @brief ID generator for alerts.
     */
    Utils::IdGen* alertsIdGen;

    /**
     * @brief ID generator for emergency notifications.
     */
    Utils::IdGen* emergNotifIdGen;

    /**
     * @brief List of TCP clients.
     */
    std::vector<int> tcpClientsList;

    /**
     * @brief List of UDP clients.
     */
    std::vector<UDPClientData> udpClientsList;

    /**
     * @brief Getter function to retrieve the TCP clients list.
     *
     * This function returns a reference to the constant vector of integers representing
     * TCP clients.
     *
     * @return const std::vector<int>& A reference to the constant vector of integers representing TCP clients.
     */
    const std::vector<int>& getTcpClients() const
    {
        return tcpClientsList;
    };

    /**
     * @brief Getter function to retrieve the UDP clients list.
     *
     * This function returns a reference to the constant vector of UDPClientData objects
     * representing UDP clients.
     *
     * @return const std::vector<UDPClientData>& A reference to the constant vector of UDPClientData objects
     * representing UDP clients.
     */
    const std::vector<UDPClientData>& getUdpClients() const
    {
        return udpClientsList;
    };

    /**
     * @brief Handles a TCP connection.
     *
     * This method is responsible for handling a TCP connection. It retrieves the client's IP address,
     * checks for any errors or disconnections, and logs relevant events. If an error or disconnection
     * occurs, it closes the socket, removes it from the master file descriptor set, and cleans up
     * associated data structures.
     *
     * @param sockfd The file descriptor for the TCP socket to handle.
     * @param master_fds Pointer to the master file descriptor set.
     */
    void handleTcpConn(int sockfd, fd_set* master_fds);

    /**
     * @brief Handles a UDP connection.
     *
     * This method is responsible for handling a UDP connection. It checks for any incoming messages
     * from UDP clients and processes them accordingly.
     *
     * @param sockfd The file descriptor for the UDP socket to handle.
     */
    void handleUdpConn(int sockfd);

    /**
     * @brief Accepts a TCP connection.
     *
     * Accepts an incoming TCP connection on the specified socket and retrieves the client's address.
     * If successful, logs the connection and adds the client to the list of connected clients.
     *
     * @param sockfd The file descriptor of the TCP socket.
     * @param addr Pointer to a sockaddr structure to store the client's address.
     * @param addrlen The length of the sockaddr structure.
     * @return The file descriptor of the accepted client socket, or -1 if an error occurred.
     */
    int acceptTcpConn(int sockfd, sockaddr* addr, socklen_t addrlen);

    /**
     * @brief Adds a TCP client to the list.
     *
     * Adds a new TCP client socket to the list of connected clients.
     *
     * @param client_fd The client socket's file descriptor.
     */
    void addTcpClient(int client_fd);

    /**
     * @brief Checks for messages from TCP clients.
     *
     * Checks if any of the connected TCP clients have sent a message.
     *
     * @param client_fd The client socket's file descriptor.
     * @return True if a message was received, false otherwise.
     */
    bool checkTcpClientsMsgs(int client_fd);

    /**
     * @brief Receives a JSON message from a TCP client.
     *
     * Receives a JSON message from the specified TCP client socket.
     *
     * @param sockfd The client socket's file descriptor.
     * @return The received JSON message.
     */
    std::string recvTcpJson(int sockfd);

    /**
     * @brief Retrieves the IP address of a TCP client.
     *
     * Retrieves the IP address of the specified TCP client socket.
     *
     * @param client_fd The client socket's file descriptor.
     * @param client_ip Reference to a string to store the client's IP address.
     */
    void getTcpClientIp(int client_fd, std::string& client_ip);

    /**
     * @brief Removes a TCP client from the list.
     *
     * Removes a TCP client socket from the list of connected clients.
     *
     * @param client_fd The client socket's file descriptor.
     */
    void remvTcpClient(int client_fd);

    /**
     * @brief Sends a JSON message to a TCP client.
     *
     * Sends a JSON message to the specified TCP client socket.
     *
     * @param sockfd The client socket's file descriptor.
     * @param json_data The JSON message to send.
     */
    void sendJsonToTcpClient(int sockfd, const json& json_data);

    /**
     * @brief Converts food and medicine supply data into a JSON object.
     *
     * This function takes pointers to FoodSupply and MedicineSupply objects and converts
     * their data into a JSON object.
     *
     * @param food_supply Pointer to the FoodSupply object containing food supply data.
     * @param medicine_supply Pointer to the MedicineSupply object containing medicine supply data.
     * @return json A JSON object containing the converted food and medicine supply data.
     */
    json suppliesToJson(FoodSupply* food_supply, MedicineSupply* medicine_supply);

    /**
     * @brief Checks for messages from UDP clients.
     *
     * Checks if any of the connected UDP clients have sent a message.
     *
     * @param sockfd The UDP socket's file descriptor.
     * @return True if a message was received, false otherwise.
     */
    bool checkUdpClientsMsgs(int sockfd);

    /**
     * @brief Receives a JSON message from a UDP client.
     *
     * Receives a JSON message from the specified UDP client socket.
     *
     * @param sockfd The client socket's file descriptor.
     * @param client_addr Pointer to a sockaddr_storage structure to store the client's address.
     * @param client_addrlen Pointer to the size of the sockaddr_storage structure.
     * @return The received JSON message.
     */
    json recvUdpJson(int sockfd, struct sockaddr_storage* client_addr, socklen_t* client_addrlen);

    /**
     * @brief Retrieves the IP address of a UDP client.
     *
     * Retrieves the IP address of the specified UDP client socket.
     *
     * @param client_addr Pointer to a sockaddr_storage structure containing the client's address.
     * @param client_ip Reference to a string to store the client's IP address.
     * @param ip_buffer_size The size of the buffer to store the IP address.
     * @param client_port Pointer to an integer to store the client's port number.
     */
    void getUdpClientInfo(struct sockaddr_storage* client_addr, char* client_ip, size_t ip_buffer_size,
                          int* client_port);

    /**
     * @brief Checks if a UDP client exists in the server's list of UDP clients.
     *
     * This function takes a UDPClientData object as input and checks if there is a
     * corresponding client in the server's list of UDP clients.
     *
     * @param client The UDPClientData object representing the client to check for existence.
     * @return bool True if the client exists in the server's list of UDP clients, false otherwise.
     */
    bool udpClientExists(const UDPClientData& client);

    /**
     * @brief Adds a UDP client to the list.
     *
     * Adds a new UDP client socket to the list of connected clients.
     *
     * @param client The UDP client data to add.
     */
    void addUdpClient(UDPClientData client);

    /**
     * @brief Sends a JSON response to a UDP client.
     *
     * This function sends a JSON response to the specified UDP client using the provided socket file descriptor,
     * client address, and client address length.
     *
     * @param sockfd The socket file descriptor.
     * @param client_addr Pointer to the sockaddr structure containing the client's address.
     * @param client_addrlen The length of the client's address structure.
     * @param json_response The JSON response to send to the client.
     */
    void sendJsonToUdpClient(int sockfd, const sockaddr* client_addr, socklen_t client_addrlen,
                             const json& json_response);

    /**
     * @brief Sends a json message to all connected TCP clients.
     *
     * Sends the specified message to all connected TCP clients.
     *
     * @param json_data The message to be sent.
     */
    void sendJsonToAllTcpClients(const json& json_data);

    /**
     * @brief Sends a message to all connected UDP clients.
     *
     * Sends the specified message to all connected UDP clients.
     *
     * @param message The message to be sent.
     * @param message_len The length of the message.
     */
    void sendToAllUdpClients(const char* message, size_t message_len);

    /**
     * @brief Creates a process to handle infection alerts.
     *
     * Creates a child process to monitor sensor data for infection alerts.
     * The child process periodically checks sensor values and writes to a FIFO file if certain thresholds are exceeded.
     * The parent process continues its execution without waiting for the child process to finish.
     */
    void createInfectionAlertsProcess();

    /**
     * @brief Signal handler for SIGINT.
     *
     * Handles the SIGINT signal (Ctrl+C) by shutting down the server.
     * If the server instance exists and the associated child processes are running,
     * sends SIGTERM signals to terminate them.
     *
     * @param signal The signal received (expected to be SIGINT).
     */
    static void sigintHandler(int signal);

    /**
     * @brief Initializes the alert module.
     *
     * Creates a FIFO file for communication with the alert module.
     * Allocates memory for an array of sensors and initializes their names.
     *
     * @return A pointer to the array of initialized sensors.
     */
    Sensor* initiateAlertModule(const char* fifoPath);

    /**
     * @brief Checks for alerts from the FIFO and processes them.
     *
     * Reads any available alert message from the FIFO file and processes it.
     * If an alert is detected, logs it, sends it to all connected TCP and UDP clients,
     * writes it to RocksDB, and updates the entry alerts count.
     */
    void checkAlerts();

    /**
     * @brief Handles communication with a Unix domain socket client.
     *
     * Handles communication with a Unix domain socket client, either in connection-oriented or connectionless mode.
     * In connection-oriented mode, accepts incoming connections and processes received messages.
     * In connectionless mode, directly processes received messages.
     *
     * @param sockfd The file descriptor of the Unix domain socket.
     * @param client_type A string indicating the type of client (e.g., "Unix").
     * @param connection_oriented Flag indicating whether the communication is connection-oriented (1) or connectionless
     * (0).
     */
    void handleUnixConn(int sockfd, const char* client_type, int connection_oriented);

    /**
     * @brief Creates a process to simulate power outage alerts.
     *
     * Creates a child process responsible for simulating power outage alerts.
     * The child process periodically sends electricity failure messages to the parent process.
     * The parent process continues its execution without waiting for the child process to finish.
     */
    void createPowerOutageAlertsProcess();

    /**
     * @brief Creates a JSON summary containing alerts, supplies, and the last keepalived event.
     *
     * Creates a JSON summary object containing information about alerts, supplies, and the last keepalived event.
     * Retrieves the number of alerts recorded for each entry, supplies data, and the last keepalived event from the
     * database.
     *
     * @return A pointer to a JSON object representing the summary.
     */
    json* createJsonSummary();

    /**
     * @brief Counts the occurrences of alerts at a specific entry.
     *
     * Retrieves the number of occurrences of alerts recorded in the RocksDB database for the specified entry.
     *
     * @param entry A C-style string representing the entry (e.g., "NORTH", "SOUTH", "EAST", "WEST").
     * @return An integer indicating the number of occurrences of alerts at the specified entry.
     */
    int countAlertsAt(const char* entry);

    /**
     * @brief Retrieves the last keepalived event from the database.
     *
     * Retrieves the value associated with the last keepalived event key from the RocksDB database.
     *
     * @return A pointer to a JSON object containing the last keepalived event.
     */
    json* getLastEvent();

    /**
     * @brief Creates a process to handle REST API requests.
     *
     * Creates a child process responsible for listening to incoming REST API requests.
     * The child process sets up HTTP GET endpoints for retrieving alerts and supplies information.
     * The parent process continues its execution without waiting for the child process to finish.
     */
    void createRestListenerProcess();

    /**
     * @brief Handles REST API requests for alerts data.
     *
     * Handles incoming HTTP GET requests for retrieving alerts data.
     * Parses the request parameters to determine whether to return all alerts updates or a specific resource
     * identified by its ID.
     *
     * @param req The HTTP request object containing the request details.
     * @param res The HTTP response object to be populated with the response data.
     */
    void handleRestAlerts(const httplib::Request& req, httplib::Response& res);

    /**
     * @brief Handles REST API requests for supplies data.
     *
     * Handles incoming HTTP GET requests for retrieving supplies data.
     * Parses the request parameters to determine whether to return all supplies updates or a specific resource
     * identified by its ID.
     *
     * @param req The HTTP request object containing the request details.
     * @param res The HTTP response object to be populated with the response data.
     */
    void handleRestSupplies(const httplib::Request& req, httplib::Response& res);

    /**
     * @brief Sends a file to a client over a socket connection.
     *
     * This method opens the specified file in binary mode and reads its content into a buffer.
     * It then sends the file content through the specified socket connection.
     * If the file cannot be opened or if there's an error sending the file content, appropriate error messages are
     * printed.
     *
     * @param client_fd The file descriptor of the socket connection to the client.
     * @param file_path The path to the file to be sent.
     */
    void sendFileToClient(int client_fd, const std::string& file_path);

    /**
     * @brief Retrieves the last ID stored in the database for the given key.
     *
     * This function attempts to get the value associated with the provided key from the RocksDB database.
     * If the key does not exist, it initializes the key with a value of "0" and returns 0.
     *
     * @param key The key for which the last ID is to be retrieved.
     * @return The last ID as an integer. If the key does not exist in the database, returns 0.
     *
     * @throws std::runtime_error If there is an error retrieving the value from the database.
     */
    int getLastId(const std::string& key);
};

#endif // SERVER_HPP
