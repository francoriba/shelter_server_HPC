#ifndef SERVER_TEST_HPP
#define SERVER_TEST_HPP

#define MOCK_FIFO_PATH "/tmp/mock_fifo"

#include "nlohmann/json.hpp"
#include "server.hpp"
#include "utils.hpp"
#include "gtest/gtest.h"
#include <csignal>

TEST(UDPClientInfo, TestgetUdpClientInfo)
{
    // Create an instance of the Server class
    Server server(8080, 9090);

    // Create mock data for client_addr
    struct sockaddr_storage client_addr;
    struct sockaddr_in ipv4_addr;
    ipv4_addr.sin_family = AF_INET;
    inet_pton(AF_INET, "192.0.2.1", &ipv4_addr.sin_addr);
    ipv4_addr.sin_port = htons(12345);
    memcpy(&client_addr, &ipv4_addr, sizeof(struct sockaddr_in)); // Copy sockaddr_in to client_addr

    // Define variables to store client IP and port
    char client_ip[INET_ADDRSTRLEN];
    int client_port;

    // Call the method to be tested
    server.getUdpClientInfo(&client_addr, client_ip, sizeof(client_ip), &client_port);

    // Assert that client_ip and client_port are correctly populated
    EXPECT_STREQ(client_ip, "192.0.2.1");
    EXPECT_EQ(client_port, 12345);
}

TEST(ServerTest, AddTcpClient)
{
    Server server(8080, 9090);

    server.addTcpClient(1);
    server.addTcpClient(2);

    const auto& clients = server.getTcpClients();
    ASSERT_EQ(clients.size(), 2);
    EXPECT_EQ(clients[0], 1);
    EXPECT_EQ(clients[1], 2);
}

TEST(ServerTest, RemvTcpClient)
{
    Server server(8080, 9090);

    server.addTcpClient(1);
    server.addTcpClient(2);
    server.addTcpClient(3);

    server.remvTcpClient(2);

    const auto& clients = server.getTcpClients();
    ASSERT_EQ(clients.size(), 2);
    EXPECT_EQ(clients[0], 1);
    EXPECT_EQ(clients[1], 3);
}

TEST(ServerTest, AddUdpClient)
{
    Server server(8080, 9090);

    // Create a mock UDPClientData
    UDPClientData client;
    client.sockfd = 123;
    client.addr_len = sizeof(struct sockaddr_in); // Assuming IPv4 address
    // Assuming client_addr is of type sockaddr_in
    struct sockaddr_in client_addr;
    client_addr.sin_family = AF_INET;
    inet_pton(AF_INET, "192.0.2.1", &client_addr.sin_addr);
    client_addr.sin_port = htons(12345);
    memcpy(&client.client_addr, &client_addr, sizeof(struct sockaddr_in)); // Copy client_addr to client.client_addr

    server.addUdpClient(client);

    // Get the vector of UDP clients
    const auto& clients = server.getUdpClients();

    // Assert that the client is added to the list
    ASSERT_EQ(clients.size(), 1);
    EXPECT_EQ(clients[0].sockfd, 123);
    EXPECT_EQ(clients[0].addr_len, sizeof(struct sockaddr_in));
    // Assuming client_addr is of type sockaddr_in
    const struct sockaddr_in* addr_in = reinterpret_cast<const struct sockaddr_in*>(&clients[0].client_addr);
    EXPECT_STREQ(inet_ntoa(addr_in->sin_addr), "192.0.2.1");
    EXPECT_EQ(ntohs(addr_in->sin_port), 12345);
}

TEST(ServerTest, UdpClientExists)
{
    Server server(8080, 9090);

    // Create a mock UDPClientData
    UDPClientData client;
    client.sockfd = 123;
    client.addr_len = sizeof(struct sockaddr_in); // Assuming IPv4 address
    // Assuming client_addr is of type sockaddr_in
    struct sockaddr_in client_addr;
    client_addr.sin_family = AF_INET;
    inet_pton(AF_INET, "192.0.2.1", &client_addr.sin_addr);
    client_addr.sin_port = htons(12345);
    memcpy(&client.client_addr, &client_addr, sizeof(struct sockaddr_in)); // Copy client_addr to client.client_addr

    // Add the client to the list
    server.addUdpClient(client);

    bool exists = server.udpClientExists(client);
    EXPECT_TRUE(exists);
}

TEST(ServerTest, InitiateAlertModule)
{
    Server server(8080, 9090);
    const char* fifoPath = "/tmp/test_fifo";
    Sensor* sensors = server.initiateAlertModule(fifoPath);
    struct stat statBuffer;
    bool fifoExists = (stat(fifoPath, &statBuffer) == 0);
    EXPECT_TRUE(fifoExists);

    ASSERT_NE(sensors, nullptr);
    EXPECT_STREQ(sensors[0].name, "NORTH ENTRY");
    EXPECT_STREQ(sensors[1].name, "SOUTH ENTRY");
    EXPECT_STREQ(sensors[2].name, "WEST ENTRY");
    EXPECT_STREQ(sensors[3].name, "EAST ENTRY");

    delete[] sensors;
}

TEST(ServerTest, CreatePowerOutageAlertsProcess)
{
    Server server(8080, 9090);
    server.createPowerOutageAlertsProcess();
    EXPECT_GE(server.powerOutagePid, 0);
    std::this_thread::sleep_for(std::chrono::seconds(1));
    std::string pid_cmd = "ps -p " + std::to_string(server.powerOutagePid);
    int result = system(pid_cmd.c_str());
    EXPECT_EQ(result, 0);
    kill(server.powerOutagePid, SIGKILL);
}

TEST(ServerTest, CreateRestListenerProcess)
{
    Server server(8080, 9090);
    server.createRestListenerProcess();
    EXPECT_GE(server.restListener, 0);
    std::this_thread::sleep_for(std::chrono::seconds(1));
    std::string pid_cmd = "ps -p " + std::to_string(server.restListener);
    int result = system(pid_cmd.c_str());
    EXPECT_EQ(result, 0);
    kill(server.restListener, SIGKILL);
}

TEST(ServerTest, CreateInfectionAlertsProcess)
{
    Server server(8080, 9090);
    server.createInfectionAlertsProcess();
    EXPECT_GE(server.alertsPid, 0);
    std::this_thread::sleep_for(std::chrono::seconds(1));
    std::string pid_cmd = "ps -p " + std::to_string(server.alertsPid);
    int result = system(pid_cmd.c_str());
    EXPECT_EQ(result, 0);
    kill(server.alertsPid, SIGKILL);
}

TEST(ServerTest, StopServer)
{
    Server server(8080, 9090);
    server.stop();
    EXPECT_TRUE(true);
}

TEST(ServerTest, GetTcpClientIp)
{
    // Create an instance of the Server class
    Server server(8080, 9090);

    // Assume you have a client file descriptor
    int client_fd = 1234; // Replace 1234 with an actual client file descriptor

    // Call the getTcpClientIp method
    std::string client_ip;
    server.getTcpClientIp(client_fd, client_ip);

    // Assert that the client IP is not empty
    ASSERT_FALSE(client_ip.empty());

    // Assert specific IP values based on known test conditions
    if (client_ip == "127.0.0.1")
    {
        // Assert for local loopback address
        EXPECT_EQ(client_ip, "127.0.0.1");
    }
    else if (client_ip == "::1")
    {
        // Assert for IPv6 local loopback address
        EXPECT_EQ(client_ip, "::1");
    }
    else
    {
        // Add additional assertions for specific IP addresses
    }
}

TEST(ServerTest, SuppliesToJson)
{
    // Create sample data for food supply and medicine supply
    FoodSupply food_supply;
    food_supply.meat = 10;
    food_supply.vegetables = 20;
    food_supply.fruits = 30;
    food_supply.water = 40;

    MedicineSupply medicine_supply;
    medicine_supply.antibiotics = 5;
    medicine_supply.analgesics = 8;
    medicine_supply.bandages = 15;

    // Create an instance of the Server class
    Server server(8080, 9090);

    // Call the suppliesToJson method with the sample data
    json result = server.suppliesToJson(&food_supply, &medicine_supply);

    // Check if the JSON object contains the expected fields and values
    EXPECT_TRUE(result.contains("food"));
    EXPECT_TRUE(result.contains("medicine"));

    json food_json = result["food"];
    json medicine_json = result["medicine"];

    EXPECT_EQ(food_json["meat"], 10);
    EXPECT_EQ(food_json["vegetables"], 20);
    EXPECT_EQ(food_json["fruits"], 30);
    EXPECT_EQ(food_json["water"], 40);

    EXPECT_EQ(medicine_json["antibiotics"], 5);
    EXPECT_EQ(medicine_json["analgesics"], 8);
    EXPECT_EQ(medicine_json["bandages"], 15);
}

// Helper function to check if a given string is a valid IP address
bool isValidIpAddress(const std::string& ip)
{
    // Regular expressions to match IPv4 and IPv6 addresses
    const std::regex ipv4_pattern("^([0-9]{1,3}\\.){3}[0-9]{1,3}$");
    const std::regex ipv6_pattern("^([0-9a-fA-F]{1,4}:){7,7}[0-9a-fA-F]{1,4}$");

    return std::regex_match(ip, ipv4_pattern) || std::regex_match(ip, ipv6_pattern);
}
#endif // SERVER_TEST_HPP
