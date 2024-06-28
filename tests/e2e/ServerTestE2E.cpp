#ifndef SERVER_TEST_HPP
#define SERVER_TEST_HPP

#include "nlohmann/json.hpp"
#include "server.hpp"
#include "gtest/gtest.h"

// TEST(ServerTest, StartMethod) {
//     // Create an instance of the Server class
//     Server server(8080, 9090);

//     // Capture stdout to suppress terminal output during the test
//     testing::internal::CaptureStdout();

//     // Assert that the start method does not throw any exceptions
//     EXPECT_NO_THROW({
//         server.start();
//     });

//     // After starting the server, call the stop method to terminate the server process
//     server.stop();
// }

#endif // TCP_TEST_HPP
