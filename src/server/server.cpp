#include "server.hpp"

// Initialize static members
bool Server::SERVER_RUNNING = true;
Server* Server::serverInstance = nullptr;

Server::Server(int tcp_port, int udp_port) : tcp_port_(tcp_port), udp_port_(udp_port)
{
    serverInstance = this;
    signal(SIGINT, sigintHandler);
    suppliesIdGen = new Utils::IdGen();
    alertsIdGen = new Utils::IdGen();
    emergNotifIdGen = new Utils::IdGen();
}

Server::~Server()
{
    delete suppliesIdGen;
    delete alertsIdGen;
    delete emergNotifIdGen;
}

void Server::start()
{

    Utils::logEvent("Server started");
    std::cout << "Server started" << std::endl;

    std::cout << "Validating necessary directories...\n";
    Utils::createDirectoriesIfNotExists(IMAGE_PATH);
    Utils::createDirectoriesIfNotExists(ZIP_PATH);
    Utils::createDirectoriesIfNotExists(CONVERTION_OUT_PATH);

    int lastSuppliesId = getLastId(LAST_SUPPLIES_ID_KEY);
    int lastAlertsId = getLastId(LAST_ALERT_ID_KEY);

    suppliesIdGen->setId(lastSuppliesId);
    alertsIdGen->setId(lastAlertsId);

    std::cout << "Initiating modules...\n";
    createInfectionAlertsProcess();
    createPowerOutageAlertsProcess();
    createRestListenerProcess();

    // Write initial event to RocksDB entry
    try
    {
        RocksDbWrapper dbWrapper(DB_NAME);
        dbWrapper.put(LAST_EVENT_KEY, "Server just started");
    }
    catch (const std::exception& e)
    {
        std::cerr << "Error writing last event to RocksDB: " << e.what() << std::endl;
    }

    std::cout << "Cleaning residual files...\n";
    Utils::cleanUpUnixSocket(SOCK_PATH);

    // TODO: Pseudo - persistence of supplies data
    // init_shared_memory_supplies();

    // Use when supplies module uses RocksDB
    init_rocksdb_supplies();

    // Set up the TCP, UDP, and Unix domain server sockets using SocketSetup
    SocketSetup socketSetup;
    int tcp_socket_fd = socketSetup.setTcpSocket(sockaddr_in6(), tcp_port_, MAX_TCP_CONNECTIONS);
    int udp_socket_fd = socketSetup.setUdpSocket(sockaddr_in6(), udp_port_);
    int unix_socket_fd = socketSetup.setUnixSocket(SOCK_PATH, MAX_UNIX_CONNECTIONS, CONN_ORIENTED);

    sleep(1); // wait to make sure that child process created the fifo

    int fifo_fd = open(FIFO_PATH, O_RDWR); // O_RDWR O_RDONLY
    if (fifo_fd < 0)
    {
        perror("open");
        exit(EXIT_FAILURE);
    }

    // File descriptor set for select
    fd_set master_fds;
    int max_fd = std::max({tcp_socket_fd, udp_socket_fd});
    max_fd = std::max(max_fd, fifo_fd);
    max_fd = std::max(max_fd, unix_socket_fd);

    FD_ZERO(&master_fds);
    FD_SET(tcp_socket_fd, &master_fds);
    FD_SET(udp_socket_fd, &master_fds);
    FD_SET(fifo_fd, &master_fds);
    FD_SET(unix_socket_fd, &master_fds);

    while (SERVER_RUNNING)
    {
        fd_set read_fds = master_fds;

        // Wait for events on sockets using select
        if (select(max_fd + 1, &read_fds, NULL, NULL, NULL) == -1)
        {
            perror("select");
            exit(EXIT_FAILURE);
        }

        // Handle activity on the TCP, UDP sockets
        for (int fd = 0; fd <= max_fd; fd++)
        {
            if (FD_ISSET(fd, &read_fds))
            {
                if (fd == tcp_socket_fd)
                {
                    sockaddr_storage client_addr;
                    socklen_t addrlen = sizeof(client_addr);
                    int new_tcp_client_fd =
                        acceptTcpConn(tcp_socket_fd, reinterpret_cast<sockaddr*>(&client_addr), addrlen);
                    if (new_tcp_client_fd != -1)
                    {
                        FD_SET(new_tcp_client_fd, &master_fds);
                        max_fd = std::max(max_fd, new_tcp_client_fd);
                    }
                }
                else if (fd == udp_socket_fd)
                {
                    handleUdpConn(fd);
                }
                else if (fd == unix_socket_fd)
                {
                    handleUnixConn(unix_socket_fd, "Unix", CONN_ORIENTED);
                }
                else if (fd == fifo_fd)
                {
                    checkAlerts();
                }
                else
                {
                    handleTcpConn(fd, &master_fds);
                }
            }
        }
    }

    Utils::logEvent("Server turned off");
}

void Server::handleTcpConn(int sockfd, fd_set* master_fds)
{
    struct sockaddr_storage client_addr;
    socklen_t addrlen = sizeof(client_addr);
    if (getpeername(sockfd, reinterpret_cast<struct sockaddr*>(&client_addr), &addrlen) == 0)
    {
        std::string client_ip;
        if (client_addr.ss_family == AF_INET)
        {
            struct sockaddr_in* addr_ipv4 = reinterpret_cast<struct sockaddr_in*>(&client_addr);
            char ip[INET_ADDRSTRLEN];
            inet_ntop(AF_INET, &(addr_ipv4->sin_addr), ip, INET_ADDRSTRLEN);
            client_ip = ip;
        }
        else if (client_addr.ss_family == AF_INET6)
        {
            struct sockaddr_in6* addr_ipv6 = reinterpret_cast<struct sockaddr_in6*>(&client_addr);
            char ip[INET6_ADDRSTRLEN];
            inet_ntop(AF_INET6, &(addr_ipv6->sin6_addr), ip, INET6_ADDRSTRLEN);
            client_ip = ip;
        }
        else
        {
            client_ip = "Unknown";
        }
        if (!checkTcpClientsMsgs(sockfd))
        {
            // Print white circle
            printf("\033[37m\u25CF ");
            printf("\033[0m");
            printf("Error or disconnection occurred with TCP client at IP: %s\n", client_ip.c_str());
            std::string log_message = "TCP client disconnected from IP: " + client_ip;
            Utils::logEvent(log_message);
            close(sockfd);
            FD_CLR(sockfd, master_fds);
            remvTcpClient(sockfd);
        }
    }
    else
    {
        perror("getpeername");
        close(sockfd);
        FD_CLR(sockfd, master_fds);
        remvTcpClient(sockfd);
    }
}

void Server::handleUdpConn(int sockfd)
{
    checkUdpClientsMsgs(sockfd);
}

int Server::acceptTcpConn(int sockfd, sockaddr* addr, socklen_t addrlen)
{
    int client_fd = accept(sockfd, addr, &addrlen);
    if (client_fd == -1)
    {
        std::cerr << "Error accepting connection" << std::endl;
        perror("accept");
    }
    else
    {
        char client_ip[INET6_ADDRSTRLEN]; // Use INET6_ADDRSTRLEN to accommodate IPv6 addresses
        char log_message[BUFFER_256];     // Allocate space for the log message
        if (addr->sa_family == AF_INET6)
        {
            sockaddr_in6* client_addr_ipv6 = reinterpret_cast<sockaddr_in6*>(addr);
            in6_addr ipv6_addr = client_addr_ipv6->sin6_addr;
            if (IN6_IS_ADDR_V4MAPPED(&ipv6_addr))
            {
                // IPv4-mapped IPv6 address detected
                in_addr ipv4_addr;
                memcpy(&ipv4_addr, &ipv6_addr.s6_addr[12], sizeof(in_addr));
                inet_ntop(AF_INET, &ipv4_addr, client_ip, INET_ADDRSTRLEN);
                std::cout << "\033[32m\u25CF "; // Change color to green and then print filled circle
                std::cout << "\033[0m";         // Restore color to default value
                std::cout << "New TCP IPv4 client connected from IP: " << client_ip << std::endl;
                snprintf(log_message, sizeof(log_message), "New TCP IPv4 client connected from IP: %s", client_ip);
            }
            else
            {
                // Regular IPv6 address
                inet_ntop(AF_INET6, &client_addr_ipv6->sin6_addr, client_ip, INET6_ADDRSTRLEN);
                std::cout << "\033[32m\u25CF ";
                std::cout << "\033[0m";
                std::cout << "New TCP IPv6 client connected from IP: " << client_ip << std::endl;
                snprintf(log_message, sizeof(log_message), "New TCP IPv6 client connected from IP: %s", client_ip);
            }
        }
        else if (addr->sa_family == AF_INET)
        {
            sockaddr_in* client_addr_ipv4 = reinterpret_cast<sockaddr_in*>(addr);
            inet_ntop(AF_INET, &client_addr_ipv4->sin_addr, client_ip, INET_ADDRSTRLEN);
            snprintf(log_message, sizeof(log_message), "New IPv4 client connected from IP: %s", client_ip);
        }
        Utils::logEvent(log_message);
        addTcpClient(client_fd); // add the client to the list of connected clients
    }
    return client_fd;
}

bool Server::checkTcpClientsMsgs(int client_fd)
{
    // Receive JSON message from client
    json received_json;
    if (auto json_str = recvTcpJson(client_fd); !json_str.empty())
    {
        try
        {
            received_json = json::parse(json_str);
        }
        catch (const std::exception& e)
        {
            std::cerr << "Error parsing JSON: " << e.what() << std::endl;
            return false;
        }

        // Access the 'message' field in the JSON object
        if (auto message = received_json.find("message"); message != received_json.end())
        {
            // Verify type of message
            const std::string& message_value = *message;
            if (message_value == "authenticateme")
            {
                // Authentication message
                if (auto hostname = received_json.find("hostname"); hostname != received_json.end())
                {
                    // Verify if the hostname is the same as the admin user
                    if (*hostname == ADMIN_USER)
                    {
                        std::string client_ip;
                        getTcpClientIp(client_fd, client_ip);
                        std::string log_message = "Update request from authenticated TCP client " + client_ip;
                        Utils::logEvent(log_message);
                        std::cout << "Client TCP authenticated successfully." << std::endl;
                        // Send authentication confirmation to client
                        json auth_confirmation = {{"message", "auth_success"}};
                        sendJsonToTcpClient(client_fd, auth_confirmation);
                        return true; // Successful authentication
                    }
                    else
                    {
                        std::string client_ip;
                        getTcpClientIp(client_fd, client_ip);
                        std::string log_message = "Update request from not authenticated TCP client " + client_ip;
                        Utils::logEvent(log_message);
                        std::cout << "Client TCP authentication failed: Invalid hostname." << std::endl;
                        json auth_failure = {{"message", "auth_failure"}};
                        sendJsonToTcpClient(client_fd, auth_failure);
                        return false; // Failed authentication
                    }
                }
            }
            else
            {
                FoodSupply* food_supply = get_food_supply();
                MedicineSupply* medicine_supply = get_medicine_supply();
                if (message_value == "status")
                {
                    std::string client_ip;
                    getTcpClientIp(client_fd, client_ip);
                    std::string log_message = "Status request from TCP client " + client_ip;
                    Utils::logEvent(log_message);
                    std::cout << "Received request from client TCP: Status" << std::endl;
                    json supplies_json = suppliesToJson(food_supply, medicine_supply);
                    (supplies_json)["message"] = "supplies_response";
                    sendJsonToTcpClient(client_fd, supplies_json);

                    try
                    {
                        RocksDbWrapper dbWrapper(DB_NAME);
                        dbWrapper.put(LAST_EVENT_KEY, log_message);
                    }
                    catch (const std::exception& e)
                    {
                        std::cerr << "Error writing last event to RocksDB: " << e.what() << std::endl;
                    }
                }
                else if (message_value == "update")
                {
                    std::string client_ip;
                    getTcpClientIp(client_fd, client_ip);
                    std::string log_message = "Update request from TCP client " + client_ip;
                    Utils::logEvent(log_message);
                    std::cout << "Received request from client TCP: Update" << std::endl;

                    // Check if pointers to supplies data are valid
                    if (food_supply && medicine_supply)
                    {
                        update_supplies_from_json(food_supply, medicine_supply,
                                                  Utils::convertJsonToCJson(received_json));

                        try
                        {
                            std::string timestamp = Utils::getCurrentTimestamp();
                            std::string id = suppliesIdGen->getNextId();
                            std::string key = SUPPLIES_KEY_PREFIX + id + "_" + timestamp;
                            json supplies_json = suppliesToJson(food_supply, medicine_supply);
                            std::string suppliesJsonString = supplies_json.dump();
                            std::cout << "Supplies JSON: " << suppliesJsonString << std::endl;
                            RocksDbWrapper dbWrapper(DB_NAME);
                            dbWrapper.put(key, suppliesJsonString);
                            dbWrapper.put(LATEST_SUPPLIES_KEY, suppliesJsonString);
                            dbWrapper.put(LAST_SUPPLIES_ID_KEY, id);
                            dbWrapper.put(LAST_EVENT_KEY, log_message);
                            Utils::logEvent("Supplies update written to RocksDB with key: " + key);
                        }
                        catch (const std::exception& e)
                        {
                            std::cerr << "Error writing supplies update to RocksDB: " << e.what() << std::endl;
                        }
                    }
                    else
                    {
                        std::cerr << "Error obtaining pointers to supplies data." << std::endl;
                    }
                }
                else if (message_value == "summary")
                {
                    std::string client_ip;
                    getTcpClientIp(client_fd, client_ip);
                    std::string log_message = "Summary request from TCP client " + client_ip;
                    Utils::logEvent(log_message);
                    std::cout << "Received request from client TCP: Summary" << std::endl;
                    json* summary = createJsonSummary();
                    sendJsonToTcpClient(client_fd, *summary);
                }
                else if (message_value == "request_available_images")
                {
                    Utils::logEvent("Request for available images received from TCP client");
                    auto availableImages = Utils::findAvailableImages(IMAGE_PATH);
                    json imageList;
                    for (const auto& imageName : availableImages)
                    {
                        if (!imageName.empty())
                        {
                            imageList.push_back(imageName);
                        }
                    }
                    json response;
                    response["message"] = "image_list";
                    response["images"] = imageList;
                    sendJsonToTcpClient(client_fd, response);
                }
                else if (message_value == "image_selection")
                {
                    // Get the name of the selected image from the received JSON
                    std::string selected_image_name = received_json["image"];
                    std::cout << "Client selected image: " << selected_image_name << std::endl;

                    // Obtain the base name of the image without the extension
                    std::string imageWithoutExtension;
                    std::string::size_type pos = selected_image_name.find_last_of('.');
                    if (pos != std::string::npos)
                    {
                        imageWithoutExtension = selected_image_name.substr(0, pos);
                    }

                    // Check if a .zip file already exists with the name of the image in ZIP_PATH
                    std::string ZipFileName = imageWithoutExtension + ".zip";
                    bool zipExists = Utils::fileExists(ZIP_PATH, ZipFileName);
                    std::string zipCompletePath = ZIP_PATH + ZipFileName;
                    if (!zipExists)
                    {
                        // If the .zip file doesn't exist, perform edge detection and compression of the image
                        std::string image_path_with_name = IMAGE_PATH + selected_image_name;
                        EdgeDetection edgeDetection(40.0, 80.0, 1.0);

                        auto start_time = std::chrono::steady_clock::now();
                        edgeDetection.cannyEdgeDetection(image_path_with_name, CONVERTION_OUT_PATH);
                        auto end_time = std::chrono::steady_clock::now();
                        std::chrono::duration<double> duration = end_time - start_time;
                        std::cout << "[TIMER] Canny edge filter: " << duration.count() << " seconds\n";

                        std::string imageToCompress = CONVERTION_OUT_PATH + CANNY_RESULT;
                        Utils::compressImg(imageToCompress, zipCompletePath);
                    }
                    else
                    {
                        std::cout
                            << "A .zip file already exists for the selected image. We'll use this one to save some time"
                            << std::endl;
                    }

                    // Inform the client about the size of the .zip file
                    int fileSize = 0;
                    fileSize = Utils::getFileSize(zipCompletePath);
                    json fileSizeMessage;
                    fileSizeMessage["message"] = "file_size";
                    fileSizeMessage["size"] = fileSize;
                    sendJsonToTcpClient(client_fd, fileSizeMessage);

                    // Inform the client that the .zip file is ready to be sent
                    json zipMessageAnouncement;
                    zipMessageAnouncement["message"] = "zip_ready";
                    sendJsonToTcpClient(client_fd, zipMessageAnouncement);
                    sleep(1); // Wait a bit so the client can prepare to receive the file
                    sendFileToClient(client_fd, ZIP_PATH + ZipFileName);
                }
                else
                {
                    std::string client_ip;
                    getTcpClientIp(client_fd, client_ip);
                    std::string log_message = "Invalid request received from TCP client " + client_ip;
                    Utils::logEvent(log_message);
                    std::cout << "Invalid request received from client TCP" << std::endl;
                }
            }
        }
        return true;
    }
    else
    {
        return false;
    }
}

bool Server::checkUdpClientsMsgs(int sockfd)
{
    // Receive JSON message from client
    struct sockaddr_storage client_addr;
    socklen_t client_addrlen = sizeof(client_addr);
    if (auto json_str = recvUdpJson(sockfd, &client_addr, &client_addrlen); !json_str.empty())
    {

        // Get client information
        char client_ip[INET6_ADDRSTRLEN];
        int client_port;
        getUdpClientInfo(&client_addr, client_ip, sizeof(client_ip), &client_port);

        // Add the new UDP client to the list of connected clients
        UDPClientData new_client;
        new_client.sockfd = sockfd;
        new_client.client_addr = client_addr;
        new_client.addr_len = client_addrlen;
        addUdpClient(new_client);

        // Check if data has the 'message' field
        if (auto message = json_str.find("message"); message != json_str.end())
        {
            if (auto hostname = json_str.find("hostname"); hostname != json_str.end())
            {
                const std::string& value = *message;
                const std::string& auth = *hostname;

                FoodSupply* food_supply = get_food_supply();
                MedicineSupply* medicine_supply = get_medicine_supply();

                if (value == "update")
                {
                    std::cout << "Received request from UDP client: Update" << std::endl;
                    if (auth == ADMIN_USER)
                    {
                        std::cout << "Client successfully authenticated" << std::endl;
                        update_supplies_from_json(food_supply, medicine_supply, Utils::convertJsonToCJson(json_str));
                        std::string log_message =
                            "Update request from authenticated UDP client " + std::string(client_ip);
                        Utils::logEvent(log_message);

                        try
                        {
                            std::string timestamp = Utils::getCurrentTimestamp();
                            std::string id = suppliesIdGen->getNextId();
                            std::string key = SUPPLIES_KEY_PREFIX + id + "_" + timestamp;
                            json supplies_json = suppliesToJson(food_supply, medicine_supply);
                            std::string suppliesJsonString = supplies_json.dump();

                            RocksDbWrapper dbWrapper(DB_NAME);
                            dbWrapper.put(LAST_SUPPLIES_ID_KEY, id);
                            dbWrapper.put(key, suppliesJsonString);
                            dbWrapper.put(LAST_EVENT_KEY, log_message);
                            Utils::logEvent("Supplies update written to RocksDB with key: " + key);
                        }
                        catch (const std::exception& e)
                        {
                            std::cerr << "Error writing supplies update to RocksDB: " << e.what() << std::endl;
                        }
                    }
                    else
                    {
                        std::cout << "Not authenticated UDP client tried to update data" << std::endl;
                        std::string log_message =
                            "Update request from not authenticated UDP client " + std::string(client_ip);
                        Utils::logEvent(log_message);
                    }
                }
                else if (value == "status")
                {
                    std::cout << "Received request from UDP client: Status" << std::endl;
                    std::string log_message = "Status request from UDP client " + std::string(client_ip);
                    Utils::logEvent(log_message);
                    sendJsonToUdpClient(sockfd, reinterpret_cast<const sockaddr*>(&client_addr), client_addrlen,
                                        suppliesToJson(food_supply, medicine_supply));
                    try
                    {
                        RocksDbWrapper dbWrapper(DB_NAME);
                        dbWrapper.put(LAST_EVENT_KEY, log_message);
                    }
                    catch (const std::exception& e)
                    {
                        std::cerr << "Error writing supplies update to RocksDB: " << e.what() << std::endl;
                    }
                }
                else if (value == "summary")
                {
                    std::cout << "Received request from UDP client: Summary" << std::endl;
                    std::string log_message = "Summary request from UDP client " + std::string(client_ip);
                    Utils::logEvent(log_message);
                    json* summary = createJsonSummary();
                    sendJsonToUdpClient(sockfd, reinterpret_cast<const sockaddr*>(&client_addr), client_addrlen,
                                        *summary);
                }
                else
                {
                    std::cout << "Invalid request received from UDP client" << std::endl;
                }
                return true; // Successful read
            }
        }
    }
    return false;
}

void Server::getTcpClientIp(int client_fd, std::string& client_ip)
{
    struct sockaddr_storage addr;
    socklen_t addr_len = sizeof(addr);
    char ip[INET6_ADDRSTRLEN];

    if (getpeername(client_fd, reinterpret_cast<struct sockaddr*>(&addr), &addr_len) == 0)
    {
        if (addr.ss_family == AF_INET)
        {
            struct sockaddr_in* s = reinterpret_cast<struct sockaddr_in*>(&addr);
            inet_ntop(AF_INET, &s->sin_addr, ip, sizeof(ip));
        }
        else
        { // AF_INET6
            struct sockaddr_in6* s = reinterpret_cast<struct sockaddr_in6*>(&addr);
            inet_ntop(AF_INET6, &s->sin6_addr, ip, sizeof(ip));
        }
        client_ip = ip;
    }
    else
    {
        // Error al obtener la dirección IP del cliente
        client_ip = "Unknown";
    }
}

std::string Server::recvTcpJson(int sockfd)
{
    char buffer[BUFFER_SIZE];
    ssize_t bytes_received = recv(sockfd, buffer, BUFFER_SIZE, 0);
    if (bytes_received > 0)
    {
        buffer[bytes_received] = '\0'; // add null terminator
        // std::cout << "JSON received from client: " << buffer << std::endl; //TODO: comment this line when not
        // debugging
        return std::string(buffer);
    }
    else if (bytes_received == 0)
    {
        // printf("No data received from client\n");
    }
    else
    {
        perror("recv");
    }
    return std::string(); // Return empty string for error or disconnection
}

json Server::recvUdpJson(int sockfd, struct sockaddr_storage* client_addr, socklen_t* client_addrlen)
{
    char buffer[BUFFER_SIZE];

    // Receive message from client
    ssize_t bytes_received =
        recvfrom(sockfd, buffer, BUFFER_SIZE - 1, 0, (struct sockaddr*)client_addr, client_addrlen);
    if (bytes_received < 0)
    {
        perror("recvfrom");
        return json(); // Empty json
    }

    // Null-terminate the received data
    buffer[bytes_received] = '\0';

    // Get client information
    char client_ip[INET6_ADDRSTRLEN];
    int client_port;
    getUdpClientInfo(client_addr, client_ip, sizeof(client_ip), &client_port);
    std::cout << "Received UDP message from " << client_ip << ":" << client_port << ": " << buffer << std::endl;

    // Parse the received JSON string
    json parsed_json;
    try
    {
        parsed_json = json::parse(buffer);
    }
    catch (const std::exception& e)
    {
        std::cerr << "Error parsing JSON: " << e.what() << std::endl;
        return json(); // Empty json
    }

    // print the parsed JSON
    // std::cout << "Parsed JSON:\n" << parsed_json << std::endl;
    return parsed_json;
}

void Server::getUdpClientInfo(struct sockaddr_storage* client_addr, char* client_ip, size_t ip_buffer_size,
                              int* client_port)
{
    if (client_addr->ss_family == AF_INET)
    {
        struct sockaddr_in* ipv4_addr = (struct sockaddr_in*)client_addr;
        inet_ntop(AF_INET, &ipv4_addr->sin_addr, client_ip, static_cast<socklen_t>(ip_buffer_size));
        *client_port = ntohs(ipv4_addr->sin_port);
    }
    else if (client_addr->ss_family == AF_INET6)
    {
        struct sockaddr_in6* ipv6_addr = (struct sockaddr_in6*)client_addr;
        inet_ntop(AF_INET6, &ipv6_addr->sin6_addr, client_ip, static_cast<socklen_t>(ip_buffer_size));
        *client_port = ntohs(ipv6_addr->sin6_port);
    }
    else
    {
        std::cout << "Unknown address family" << std::endl;
        *client_port = -1;
    }
}

void Server::addTcpClient(int client_fd)
{
    if (tcpClientsList.size() < MAX_TCP_CONNECTIONS)
    {
        tcpClientsList.push_back(client_fd);
        Utils::logEvent("Added TCP client. Total connected: " + std::to_string(tcpClientsList.size()));
    }
    else
    {
        std::cout << "Can't add more clients" << std::endl;
    }
}

void Server::remvTcpClient(int client_fd)
{
    auto it = std::find(tcpClientsList.begin(), tcpClientsList.end(), client_fd);
    if (it != tcpClientsList.end())
    {
        tcpClientsList.erase(it);
        Utils::logEvent("TCP client disconnected. Total connected: " + std::to_string(tcpClientsList.size()));
    }
}

void Server::sendJsonToTcpClient(int sockfd, const json& json_data)
{

    // Convert JSON to string
    std::string json_string = json_data.dump();

    // Send JSON string to client
    ssize_t bytes_sent = send(sockfd, json_string.c_str(), json_string.length(), 0);

    if (bytes_sent < 0)
    {
        perror("Error sending JSON to client");
    }
    else if (bytes_sent == 0)
    {
        std::cout << "No data sent to client" << std::endl;
    }
    else
    {
        // TODO: comment this line when not debugging
        // std::cout << "JSON sent to client: " << json_string << std::endl;
    }
}

json Server::suppliesToJson(FoodSupply* food_supply, MedicineSupply* medicine_supply)
{
    // JSON object to hold the supplies data
    json supplies_json;

    // JSON object for food supplies
    json food_json;
    food_json["meat"] = food_supply->meat;
    food_json["vegetables"] = food_supply->vegetables;
    food_json["fruits"] = food_supply->fruits;
    food_json["water"] = food_supply->water;

    // Add food supplies data to the JSON object
    supplies_json["food"] = food_json;

    // JSON object for medicine supplies
    json medicine_json;
    medicine_json["antibiotics"] = medicine_supply->antibiotics;
    medicine_json["analgesics"] = medicine_supply->analgesics;
    medicine_json["bandages"] = medicine_supply->bandages;

    // Add medicine supplies data to the JSON object
    supplies_json["medicine"] = medicine_json;

    return supplies_json;
}

void Server::addUdpClient(UDPClientData client)
{
    if (udpClientExists(client))
    {
        std::cout << "Client already exists in the UDP client list." << std::endl;
        return;
    }

    if (udpClientsList.size() < MAX_UDP_CONNECTIONS)
    {
        udpClientsList.push_back(client);

        // Generate log event
        std::string num_clients_str = std::to_string(udpClientsList.size());
        std::string log_message = "Added UDP client. Total cached: " + num_clients_str;
        Utils::logEvent(log_message);
    }
    else
    {
        std::cout << "Maximum number of clients reached. Cannot add more clients." << std::endl;
    }
}

bool Server::udpClientExists(const UDPClientData& client)
{
    auto new_addr = reinterpret_cast<const struct sockaddr_in*>(&client.client_addr);
    for (const auto& existing_client : udpClientsList)
    {
        auto existing_addr = reinterpret_cast<const struct sockaddr_in*>(&existing_client.client_addr);
        if (existing_addr->sin_addr.s_addr == new_addr->sin_addr.s_addr &&
            existing_addr->sin_port == new_addr->sin_port)
        {
            return true;
        }
    }
    return false;
}

void Server::sendJsonToUdpClient(int sockfd, const sockaddr* client_addr, socklen_t client_addrlen,
                                 const json& json_response)
{
    // Convert cJSON object to JSON string
    std::string json_string = json_response.dump();

    // Send response back to client
    ssize_t bytes_sent = sendto(sockfd, json_string.c_str(), json_string.size(), 0, client_addr, client_addrlen);
    if (bytes_sent == -1)
    {
        perror("sendto");
        return;
    }
    // Print message sent information
    char client_ip[INET6_ADDRSTRLEN];
    int client_port;
    getUdpClientInfo(reinterpret_cast<struct sockaddr_storage*>(const_cast<sockaddr*>(client_addr)), client_ip,
                     sizeof(client_ip), &client_port);
    std::cout << "Message sent to " << client_ip << ":" << client_port << std::endl;
}

void Server::sendJsonToAllTcpClients(const json& json_data)
{
    for (const int client_fd : tcpClientsList)
    {
        sendJsonToTcpClient(client_fd, json(json_data));
    }
}

void Server::sendToAllUdpClients(const char* message, size_t message_len)
{
    for (const auto& client : udpClientsList)
    {
        sendto(client.sockfd, message, message_len, 0, reinterpret_cast<const sockaddr*>(&client.client_addr),
               client.addr_len);
    }
}

void Server::sigintHandler(int signal)
{
    if (signal == SIGINT && serverInstance)
    {
        std::cout << "Shutting down server..." << std::endl;
        SERVER_RUNNING = false;
        if (serverInstance->alertsPid > 0)
        {
            kill(serverInstance->alertsPid, SIGTERM);
        }
        if (serverInstance->powerOutagePid > 0)
        {
            kill(serverInstance->powerOutagePid, SIGTERM);
        }
        if (serverInstance->restListener > 0)
        {
            kill(serverInstance->restListener, SIGTERM);
        }
    }
}

void Server::createInfectionAlertsProcess()
{
    alertsPid = fork();

    if (alertsPid < 0)
    {
        perror("fork");
        exit(EXIT_FAILURE);
    }
    else if (alertsPid == 0)
    {
        // Child process
        Utils::redirectOutputToParent(STDOUT_FILENO);
        Utils::cleanup_fifo(FIFO_PATH); // Remove the FIFO file if it already exists
        std::cout << "PID alerts: " << getpid() << std::endl;

        // Initialize sensors
        Sensor* sensors = initiateAlertModule(fifoPath);

        // Main loop for alert process
        while (SERVER_RUNNING)
        {
            sleep(30); // Sleep for 30 seconds
            update_sensor_values(sensors, NUM_SENSORS);
            check_temperature_threshold(sensors,
                                        NUM_SENSORS); // Writes fifo if temperature in some sensor higher than 38
        }
        // Cleanup resources
        free(sensors);
        exit(EXIT_SUCCESS);
    }
    else
    {
        return;
    }
}

Sensor* Server::initiateAlertModule(const char* fifoPath)
{
    if (mkfifo(fifoPath, 0666) == -1)
    {
        std::cerr << "Error creating FIFO: " << std::strerror(errno) << std::endl;
    }

    Sensor* sensors = new Sensor[NUM_SENSORS];
    if (sensors == nullptr)
    {
        std::cerr << "Error allocating memory for sensors" << std::endl;
        exit(EXIT_FAILURE);
    }

    std::strcpy(sensors[0].name, "NORTH ENTRY");
    std::strcpy(sensors[1].name, "SOUTH ENTRY");
    std::strcpy(sensors[2].name, "WEST ENTRY");
    std::strcpy(sensors[3].name, "EAST ENTRY");

    return sensors;
}

void Server::checkAlerts()
{
    int fd;
    char alert_message[BUFFER_SIZE];

    // Fill alert_message with 0
    std::memset(alert_message, 0, sizeof(alert_message));

    fd = open(FIFO_PATH, O_RDONLY);
    if (fd == -1)
    {
        std::cerr << "Error opening FIFO: " << std::strerror(errno) << std::endl;
        return;
    }

    // Read any available alert from the FIFO
    ssize_t bytes_read = read(fd, alert_message, sizeof(alert_message));
    if (bytes_read > 0)
    {
        Utils::logEvent(alert_message);

        json alertJson;
        alertJson["message"] = "alert";
        alertJson["alert_description"] = alert_message;
        sendJsonToAllTcpClients(alertJson);
        sendToAllUdpClients(alert_message, std::strlen(alert_message));
        Utils::logEvent("Sent alert notification to all connected clients");

        // Write alert message to RocksDB
        try
        {
            std::string timestamp = Utils::getCurrentTimestamp();
            std::string id = alertsIdGen->getNextId();
            std::string key = ALERTS_KEY_PREFIX + id + "_" + timestamp;
            RocksDbWrapper dbWrapper(DB_NAME);
            dbWrapper.put(key, alert_message);
            dbWrapper.put(LAST_EVENT_KEY, alert_message);
            dbWrapper.put(LAST_ALERT_ID_KEY, id);

            Utils::logEvent("Alert message written to RocksDB with key: " + key);
        }
        catch (const std::exception& e)
        {
            std::cerr << "Error writing alert message to RocksDB: " << e.what() << std::endl;
        }

        const char* entry = Utils::detectEntry(alert_message);
        if (entry != nullptr)
        {
            std::cout << "\u26A0 Detected alert at entry: " << entry << std::endl;
            std::cout << "\U0001F4E2 Sent alert notification to all connected clients" << std::endl;

            // Increase corresponding entry count
            if (std::strcmp(entry, "NORTH") == 0)
            {
                entryAlertsCount.north++;
            }
            else if (std::strcmp(entry, "SOUTH") == 0)
            {
                entryAlertsCount.south++;
            }
            else if (std::strcmp(entry, "EAST") == 0)
            {
                entryAlertsCount.east++;
            }
            else if (std::strcmp(entry, "WEST") == 0)
            {
                entryAlertsCount.west++;
            }
        }
    }
    else if (bytes_read == 0)
    {
        // No data available
        // std::cout << "No data available" << std::endl;
    }
    else
    {
        std::cerr << "Error reading from FIFO: " << std::strerror(errno) << std::endl;
    }
    close(fd);
}

void Server::handleUnixConn(int sockfd, const char* client_type, int connection_oriented)
{
    if (connection_oriented)
    {
        int client_fd = accept(sockfd, NULL, NULL);
        if (client_fd != -1)
        {
            char buffer[BUFFER_SIZE];
            ssize_t bytes_received = recv(client_fd, buffer, BUFFER_SIZE, 0);
            if (bytes_received > 0)
            {
                buffer[bytes_received] = '\0';
                std::cout << "Message received from " << client_type << " client: " << buffer << std::endl;

                // Save the received message to RocksDB
                try
                {
                    std::string timestamp = Utils::getCurrentTimestamp();
                    std::string id = emergNotifIdGen->getNextId();
                    std::string key = EMERGENCY_NOTIF_KEY_PREFIX + id + "_" + timestamp;
                    RocksDbWrapper dbWrapper(DB_NAME);
                    dbWrapper.put(key, buffer);
                    dbWrapper.put(LAST_EVENT_KEY, buffer);
                    Utils::logEvent("Message written to RocksDB with key: " + key);
                }
                catch (const std::exception& e)
                {
                    std::cerr << "Error writing message to RocksDB: " << e.what() << std::endl;
                }

                json disconnect_json;
                disconnect_json["message"] = "disconnect";
                sendJsonToAllTcpClients(disconnect_json);
                Utils::logEvent(buffer);
                stop(); // TODO: Comment this line for Lab 3 so server doesn't stop, or use a cron job in a Ubuntu
                        // Server VM
            }
            else if (bytes_received == 0)
            {
            }
            else
            {
                perror("recv");
            }
            close(client_fd);
        }
    }
    else
    {
        char buffer[BUFFER_SIZE];
        ssize_t bytes_received = recv(sockfd, buffer, BUFFER_SIZE, 0);
        if (bytes_received > 0)
        {
            buffer[bytes_received] = '\0';
            std::cout << "Message received from " << client_type << " client: " << buffer << std::endl;
        }
        else if (bytes_received == 0)
        {
            // std::cout << "No data received from " << client_type << " client" << std::endl;
        }
        else
        {
            perror("recv");
        }
    }
}

void Server::createPowerOutageAlertsProcess()
{
    powerOutagePid = fork();
    if (powerOutagePid < 0)
    {
        perror("fork");
        exit(EXIT_FAILURE);
    }
    else if (powerOutagePid == 0)
    {
        Utils::redirectOutputToParent(STDOUT_FILENO);
        std::cout << "PID power outage simulator: " << getpid() << std::endl;
        sleep(1); // Wait for the parent to create the socket
        srand((unsigned int)time(NULL));

        while (SERVER_RUNNING)
        {
            sleep((unsigned int)(SECONDS_IN_MINUTE * get_random_failure_minutes())); // Sleep for the failure interval
            int fd = init_emergency_notification();
            simulate_electricity_failure(fd); // Send failure message to parent process
            close(fd);
        }
        exit(EXIT_SUCCESS);
    }
    else
    {
        return;
    }
}

json* Server::createJsonSummary()
{
    json* summary = new json();

    // Get alerts data
    json* alerts = new json();
    (*alerts)["north_entry"] = countAlertsAt("NORTH");
    (*alerts)["east_entry"] = countAlertsAt("EAST");
    (*alerts)["west_entry"] = countAlertsAt("WEST");
    (*alerts)["south_entry"] = countAlertsAt("SOUTH");
    (*summary)["alerts"] = *alerts;

    // Get supplies data
    FoodSupply* food_supply = get_food_supply();
    MedicineSupply* medicine_supply = get_medicine_supply();
    json supplies = suppliesToJson(food_supply, medicine_supply);
    (*summary)["supplies"] = supplies;

    // Last keepalived event
    json* emergency = getLastEvent();
    (*summary)["last_keepalived"] = *emergency;

    // Add message field
    (*summary)["message"] = "summary_response";

    delete alerts;
    delete emergency;

    return summary;
}

int Server::countAlertsAt(const char* entry)
{
    int alertsCount = 0;
    try
    {
        RocksDbWrapper dbWrapper(DB_NAME);
        alertsCount = dbWrapper.countOccurrences(entry);
    }
    catch (const std::exception& e)
    {
        std::cerr << "Error counting occurrences RocksDB: " << e.what() << std::endl;
    }
    return alertsCount;
}

json* Server::getLastEvent()
{
    std::string lastEventValue;
    try
    {
        RocksDbWrapper dbWrapper(DB_NAME);
        lastEventValue = dbWrapper.getValueByKey(LAST_EVENT_KEY);
    }
    catch (const std::exception& e)
    {
        std::cerr << "Error getting last event: " << e.what() << std::endl;
    }
    json* lastEventJson = new json();
    lastEventJson->emplace(LAST_EVENT_KEY, lastEventValue);

    return lastEventJson;
}

void Server::createRestListenerProcess()
{
    restListener = fork();

    if (restListener < 0)
    {
        perror("fork");
        exit(EXIT_FAILURE);
    }
    else if (restListener == 0)
    {
        Utils::redirectOutputToParent(STDOUT_FILENO);
        std::cout << "PID rest listener: " << getpid() << std::endl;
        std::cout << "Listening on port " << REST_API_PORT << " for REST API requests" << std::endl;
        httplib::Server rest_api;

        auto alerts_handler = [&](const httplib::Request& req, httplib::Response& res) {
            this->handleRestAlerts(req, res);
        };

        auto supplies_handler = [&](const httplib::Request& req, httplib::Response& res) {
            this->handleRestSupplies(req, res);
        };

        rest_api.Get("/alerts", alerts_handler);
        rest_api.Get("/supplies", supplies_handler);
        rest_api.listen("0.0.0.0", REST_API_PORT);
    }
    else
    {
        return;
    }
}

void Server::handleRestAlerts(const httplib::Request& req, httplib::Response& res)
{
    std::string remote_ip = req.remote_addr;
    std::string id_param = req.get_param_value("id");

    std::cout << "Received request from " << remote_ip << " for alerts data" << std::endl;
    Utils::logEvent("Received request through API for supplies data from " + remote_ip);

    RocksDbWrapper dbWrapper(DB_NAME);

    if (req.params.size() > 1 || (req.params.size() == 1 && req.params.begin()->first != "id"))
    {
        // More than one parameter or the only parameter is not "id"
        res.status = 400;
        res.set_content("Only 'id' parameter is accepted", "text/plain");
        return;
    }

    if (id_param.empty())
    {
        // No "id" parameter, return all alerts updates
        std::vector<json> json_responses = dbWrapper.getJsonByKeySubstrings({ALERTS_KEY_PREFIX});

        std::string combined_response;
        for (const auto& json_response : json_responses)
        {
            std::string json_response_str = json_response.dump();
            combined_response += json_response_str + "\n";
        }
        res.set_header("Content-Type", "application/json");
        res.set_content(combined_response, "application/json");
    }
    else
    {
        // id param provided - search for the resource with that ID
        std::vector<json> json_responses = dbWrapper.getJsonByKeySubstrings({ALERTS_KEY_PREFIX, "_" + id_param + "_"});

        if (!json_responses.empty())
        {
            std::string combined_response;
            for (const auto& json_response : json_responses)
            {
                std::string json_response_str = json_response.dump();
                combined_response += json_response_str + "\n";
            }
            res.set_header("Content-Type", "application/json");
            res.set_content(combined_response, "application/json");
        }
        else
        {
            res.status = 404;
            res.set_content("ID not found", "text/plain");
        }
    }
}

void Server::handleRestSupplies(const httplib::Request& req, httplib::Response& res)
{
    std::string remote_ip = req.remote_addr;
    std::string id_param = req.get_param_value("id");

    std::cout << "Received request from " << remote_ip << " for supplies data" << std::endl;
    Utils::logEvent("Received request through API for supplies data from " + remote_ip);

    RocksDbWrapper dbWrapper(DB_NAME);

    if (req.params.size() > 1 || (req.params.size() == 1 && req.params.begin()->first != "id"))
    {
        // More than one parameter or the only parameter is not "id"
        res.status = 400;
        res.set_content("Only 'id' parameter is accepted", "text/plain");
        return;
    }

    if (id_param.empty())
    {
        // No "id" parameter, return all supplies updates
        std::vector<json> json_responses = dbWrapper.getJsonByKeySubstrings({SUPPLIES_KEY_PREFIX});

        if (json_responses.empty())
        {
            res.status = 404;
            res.set_content("No supplies found", "application/json");
            return;
        }

        json combined_response = json::array();
        for (const auto& json_response : json_responses)
        {
            for (auto it = json_response.begin(); it != json_response.end(); ++it)
            {
                json deserialized_response = json::parse(it.value().get<std::string>());
                combined_response.push_back(deserialized_response);
            }
        }

        res.set_header("Content-Type", "application/json");
        res.set_content(combined_response.dump(), "application/json");
    }
    else if (id_param == "latest")
    {
        // Handle request for latest supplies
        json json_response = dbWrapper.getValueByKey(LATEST_SUPPLIES_KEY);

        if (!json_response.is_null())
        {
            json deserialized_response = json::parse(json_response.begin().value().get<std::string>());
            res.set_header("Content-Type", "application/json");
            res.set_content(deserialized_response.dump(), "application/json");
        }
        else
        {
            res.status = 404;
            res.set_content("Latest supplies not found", "application/json");
        }
    }
    else
    {
        // id param provided - search for the resource with that ID
        std::vector<json> json_responses =
            dbWrapper.getJsonByKeySubstrings({SUPPLIES_KEY_PREFIX + "_" + id_param + "_"});

        if (!json_responses.empty())
        {
            json combined_response = json::array();
            for (const auto& json_response : json_responses)
            {
                for (auto it = json_response.begin(); it != json_response.end(); ++it)
                {
                    json deserialized_response = json::parse(it.value().get<std::string>());
                    combined_response.push_back(deserialized_response);
                }
            }
            res.set_header("Content-Type", "application/json");
            res.set_content(combined_response.dump(), "application/json");
        }
        else
        {
            res.status = 404;
            res.set_content("ID not found", "application/json");
        }
    }
}

void Server::sendFileToClient(int client_fd, const std::string& file_path)
{
    // Open the file in binary mode
    std::ifstream file(file_path, std::ios::binary);
    if (!file.is_open())
    {
        std::cerr << "Error: Could not open file " << file_path << std::endl;
        return;
    }
    // Read the file content into a buffer
    std::string buffer;
    file.seekg(0, std::ios::end);
    buffer.resize(file.tellg());
    file.seekg(0, std::ios::beg);
    file.read(&buffer[0], buffer.size());

    // Send the file content through the socket
    if (send(client_fd, buffer.data(), buffer.size(), 0) < 0)
    {
        std::cerr << "Error: Failed to send file " << file_path << " to client" << std::endl;
    }

    // Close the file
    file.close();
}

int Server::getLastId(const std::string& key)
{
    RocksDbWrapper dbWrapper(DB_NAME);
    try
    {
        // Intentar obtener el valor del ID desde la base de datos
        std::string valueStr = dbWrapper.getValueByKey(key);
        return std::stoi(valueStr); // Devolver el valor como entero
    }
    catch (const std::exception& e)
    {
        // Si no se encuentra la clave en la base de datos, se crea con el valor 0
        std::cerr << "Last ID key not found in the database. Creating with value 0." << std::endl;
        dbWrapper.put(key, "0");
        return 0;
    }
}

void Server::stop()
{
    kill(getpid(), SIGINT);
}
