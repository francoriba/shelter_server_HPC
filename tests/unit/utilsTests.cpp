#include "cJSON.h"
#include "myRocksDbWrapper.hpp"
#include "nlohmann/json.hpp"
#include "server.hpp" // Suponiendo que server.hpp contiene la declaración del método logEvent
#include "gmock/gmock.h"
#include "gtest/gtest.h"
#include <cstdlib>
#include <fstream>
#include <iostream>
#include <regex>
#include <sstream>
#include <vector>

TEST(UtilsTest, CreateDirectoriesIfNotExists)
{
    // Test with a non-existing directory
    std::string nonExistingDir = "test_directory";
    ASSERT_FALSE(fs::exists(nonExistingDir)); // Ensure directory doesn't exist initially
    Utils::createDirectoriesIfNotExists(nonExistingDir);
    EXPECT_TRUE(fs::exists(nonExistingDir)); // Expect directory to be created

    // Test with an existing directory
    std::string existingDir = "test_directory";
    ASSERT_TRUE(fs::exists(existingDir));
    Utils::createDirectoriesIfNotExists(existingDir);
    EXPECT_TRUE(fs::exists(existingDir));
    Utils::deleteDirectory(existingDir);
    Utils::deleteDirectory(nonExistingDir);
}

TEST(UtilsTest, DeleteDirectory)
{
    // Test with an existing directory
    std::string existingDir = "test_directory";
    // Create the directory first
    Utils::createDirectoriesIfNotExists(existingDir);
    ASSERT_TRUE(fs::exists(existingDir));
    Utils::deleteDirectory(existingDir);
    EXPECT_FALSE(fs::exists(existingDir));

    // Test with a non-existing directory
    std::string nonExistingDir = "non_existing_directory";
    ASSERT_FALSE(fs::exists(nonExistingDir));
    Utils::deleteDirectory(nonExistingDir);
    EXPECT_FALSE(fs::exists(nonExistingDir));
}

TEST(UtilsTest, GetFileSize)
{
    // Test with an existing file
    std::string existingFile = "test_file.txt";
    std::ofstream outfile(existingFile);
    outfile << "Hello, World!";
    outfile.close();
    int fileSize = Utils::getFileSize(existingFile);
    EXPECT_GT(fileSize, 0);       // Expect file size to be greater than 0
    remove(existingFile.c_str()); // Remove the file after testing

    // Test with a non-existing file
    std::string nonExistingFile = "non_existing_file.txt";
    int nonExistingFileSize = Utils::getFileSize(nonExistingFile);
    EXPECT_EQ(nonExistingFileSize, -1); // Expect -1 since file doesn't exist

    // Test with an invalid file path
    std::string invalidFilePath = "/invalid/file/path";
    int invalidFileSize = Utils::getFileSize(invalidFilePath);
    EXPECT_EQ(invalidFileSize, -1); // Expect -1 since file path is invalid
}

TEST(UtilsTest, FileExists)
{
    // Test with an existing file
    std::string existingDirectory = "./";
    std::string existingFile = "test_file.txt";
    std::ofstream outfile(existingDirectory + existingFile);
    outfile << "Hello, World!";
    outfile.close();
    bool existingFileExists = Utils::fileExists(existingDirectory, existingFile);
    EXPECT_TRUE(existingFileExists); // Expect file to exist

    // Test with a non-existing file
    std::string nonExistingFile = "non_existing_file.txt";
    bool nonExistingFileExists = Utils::fileExists(existingDirectory, nonExistingFile);
    EXPECT_FALSE(nonExistingFileExists); // Expect file not to exist
}

TEST(UtilsTest, CleanupFifo)
{
    // Create a temporary FIFO file
    const char* fifoPath = "test_fifo";
    mkfifo(fifoPath, 0666);

    // Test cleanup_fifo with an existing FIFO file
    Utils::cleanup_fifo(fifoPath);
    // Check if the FIFO file exists after cleanup
    bool fifoExistsAfterCleanup = access(fifoPath, F_OK) != -1;
    EXPECT_FALSE(fifoExistsAfterCleanup);

    // Test cleanup_fifo with a non-existing FIFO file
    Utils::cleanup_fifo(fifoPath); // Cleanup again to verify handling of non-existing file
}

TEST(UtilsTest, GetCurrentTimestamp)
{
    // Call the getCurrentTimestamp function
    std::string timestamp = Utils::getCurrentTimestamp();

    // Define a regular expression to match the expected timestamp format
    std::regex timestampRegex("\\[\\d{4}-\\d{2}-\\d{2} \\d{2}:\\d{2}:\\d{2}\\] ");

    // Check if the timestamp matches the expected format
    bool formatMatched = std::regex_match(timestamp, timestampRegex);
    EXPECT_TRUE(formatMatched);
}

TEST(UtilsTest, LogEvent)
{
    // Message to be logged
    std::string message = "Test log message";

    // Call the logEvent function
    Utils::logEvent(message);

    // Get the user's home directory
    const char* homeDir = getenv("HOME");
    ASSERT_NE(homeDir, nullptr); // Ensure HOME environment variable is set

    // Construct the log file path
    std::string logDirPath = std::string(homeDir) + REFUGE_DIR;
    std::string logFilePath = logDirPath + LOG_FILENAME;

    // Check if the log file was created and the message was logged
    std::ifstream logFile(logFilePath);
    ASSERT_TRUE(logFile.is_open()); // Ensure log file was created successfully

    std::string line;
    bool messageLogged = false;
    while (std::getline(logFile, line))
    {
        // Check if the message was logged
        if (line.find(message) != std::string::npos)
        {
            messageLogged = true;
            break;
        }
    }
    logFile.close();

    EXPECT_TRUE(messageLogged); // Expect the message to be logged in the file
}

TEST(UtilsTest, NextIdTest)
{
    Utils::IdGen idGen;
    EXPECT_EQ(idGen.getNextId(), "0");
    EXPECT_EQ(idGen.getNextId(), "1");
    EXPECT_EQ(idGen.getNextId(), "2");
    EXPECT_EQ(idGen.getNextId(), "3");
}

TEST(UtilsTest, SetIdTest)
{
    Utils::IdGen idGen;
    idGen.setId(10);
    EXPECT_EQ(idGen.getNextId(), "11");
    EXPECT_EQ(idGen.getNextId(), "12");
    EXPECT_EQ(idGen.getNextId(), "13");
    EXPECT_EQ(idGen.getNextId(), "14");
}

TEST(UtilsTest, RedirectOutputToParent)
{
    // Create a pipe for communication between parent and child
    int pipe_fd[2];
    ASSERT_NE(pipe(pipe_fd), -1) << "Pipe creation failed";

    // Create a child process
    pid_t pid = fork();
    ASSERT_NE(pid, -1) << "Fork failed";

    if (pid == 0)
    { // Child process
        // Close the read end of the pipe
        close(pipe_fd[0]);

        // Redirect child's stdout to the pipe
        Utils::redirectOutputToParent(pipe_fd[1]);

        // Write to stdout in child process
        std::cout << "Message from child" << std::endl;

        // Close the write end of the pipe
        close(pipe_fd[1]);

        // Terminate the child process
        _exit(EXIT_SUCCESS);
    }
    else
    { // Parent process
        // Close the write end of the pipe
        close(pipe_fd[1]);

        // Read from the read end of the pipe
        char buffer[100];
        ssize_t bytes_read = read(pipe_fd[0], buffer, sizeof(buffer));
        ASSERT_GT(bytes_read, 0) << "Reading from pipe failed";

        // Null-terminate the buffer
        buffer[bytes_read] = '\0';

        // Verify the message received from the child
        EXPECT_STREQ("Message from child\n", buffer);

        // Close the read end of the pipe
        close(pipe_fd[0]);
    }
}

TEST(UtilsTest, ConvertValidJson)
{
    // Create a sample JSON object
    json json_obj = {{"key1", "value1"}, {"key2", 123}, {"key3", {{"nested_key1", "nested_value1"}}}};

    // Convert JSON object to cJSON
    cJSON* cJSON_obj = Utils::convertJsonToCJson(json_obj);

    // Check if cJSON object is not null
    ASSERT_NE(cJSON_obj, nullptr) << "Conversion failed";

    // Check if the cJSON object matches the original JSON
    bool jsonMatch = true;
    if (cJSON_obj->type == cJSON_Object)
    {
        for (auto it = json_obj.begin(); it != json_obj.end(); ++it)
        {
            cJSON* cJSON_child = cJSON_GetObjectItemCaseSensitive(cJSON_obj, it.key().c_str());
            if (cJSON_child == nullptr)
            {
                jsonMatch = false;
                break;
            }
            if (it.value().is_string())
            {
                if (strcmp(cJSON_child->valuestring, it.value().get<std::string>().c_str()) != 0)
                {
                    jsonMatch = false;
                    break;
                }
            }
            else if (it.value().is_number())
            {
                if (cJSON_child->valuedouble != it.value().get<double>())
                {
                    jsonMatch = false;
                    break;
                }
            }
            else if (it.value().is_object())
            {
                // Handle nested objects recursively
                // (you might need to implement this based on your cJSON structure)
            }
        }
    }
    else
    {
        jsonMatch = false;
    }

    EXPECT_TRUE(jsonMatch) << "cJSON object doesn't match the original JSON";

    // Free the cJSON object
    cJSON_Delete(cJSON_obj);
}

TEST(UtilsTest, ConvertInvalidJson)
{
    // Create a valid JSON string
    std::string valid_json_str = "{\"key1\": \"value1\", \"key2\": \"value2\"}";

    // Convert valid JSON string to cJSON
    cJSON* cJSON_obj = Utils::convertJsonToCJson(json::parse(valid_json_str));

    // Check if cJSON object is not null
    ASSERT_NE(cJSON_obj, nullptr) << "Conversion should succeed for valid JSON";

    // Free the cJSON object (if not nullptr)
    if (cJSON_obj != nullptr)
    {
        cJSON_Delete(cJSON_obj);
    }
}

TEST(DetectEntryTest, TestNorthEntry)
{
    const char* alertMessage = "Alert: NORTH entrance detected!";
    const char* result = Utils::detectEntry(alertMessage);
    EXPECT_STREQ("NORTH", result);
}

TEST(DetectEntryTest, TestSouthEntry)
{
    const char* alertMessage = "Alert: SOUTH entrance detected!";
    const char* result = Utils::detectEntry(alertMessage);
    EXPECT_STREQ("SOUTH", result);
}

TEST(DetectEntryTest, TestEastEntry)
{
    const char* alertMessage = "Alert: EAST entrance detected!";
    const char* result = Utils::detectEntry(alertMessage);
    EXPECT_STREQ("EAST", result);
}

TEST(DetectEntryTest, TestWestEntry)
{
    const char* alertMessage = "Alert: WEST entrance detected!";
    const char* result = Utils::detectEntry(alertMessage);
    EXPECT_STREQ("WEST", result);
}

TEST(DetectEntryTest, TestNoEntry)
{
    const char* alertMessage = "No entrance detected!";
    const char* result = Utils::detectEntry(alertMessage);
    EXPECT_EQ(nullptr, result);
}

TEST(CleanUpUnixSocketTest, TestCleanUpExistingSocket)
{
    // Create a temporary Unix domain socket
    int sockfd = socket(AF_UNIX, SOCK_STREAM, 0);
    ASSERT_NE(sockfd, -1) << "Failed to create socket";

    struct sockaddr_un addr;
    memset(&addr, 0, sizeof(struct sockaddr_un));
    addr.sun_family = AF_UNIX;
    strncpy(addr.sun_path, "/tmp/test_socket", sizeof(addr.sun_path) - 1);

    // Bind the socket to the address
    ASSERT_NE(bind(sockfd, (struct sockaddr*)&addr, sizeof(struct sockaddr_un)), -1) << "Failed to bind socket";

    // Close the socket
    close(sockfd);

    // Call the cleanUpUnixSocket function
    Utils::cleanUpUnixSocket("/tmp/test_socket");

    // Check if the socket file is removed
    struct stat socket_stat;
    int result = stat("/tmp/test_socket", &socket_stat);
    EXPECT_EQ(result, -1) << "Socket file should be removed";
    EXPECT_EQ(errno, ENOENT) << "Socket file should not exist";
}

TEST(CleanUpUnixSocketTest, TestCleanUpNonExistingSocket)
{
    Utils::cleanUpUnixSocket("/tmp/non_existing_socket");
}

TEST(FindAvailableImagesTest, TestEmptyDirectory)
{
    // Create an empty directory
    std::string empty_directory = "/tmp/empty_directory";
    std::filesystem::create_directory(empty_directory);

    // Call findAvailableImages for the empty directory
    std::vector<std::string> result = Utils::findAvailableImages(empty_directory);

    // Check if the result is an empty vector
    EXPECT_TRUE(result.empty()) << "Result should be empty for an empty directory";

    // Remove the empty directory
    std::filesystem::remove(empty_directory);
}

TEST(FindAvailableImagesTest, TestValidDirectory)
{
    // Create a temporary directory
    std::string temp_dir_path = "/tmp/test_images";
    fs::create_directory(temp_dir_path);

    // Create some temporary image files
    std::vector<std::string> expected_images = {"image1.jpg", "image2.png", "image3.bmp"};
    for (const auto& image_name : expected_images)
    {
        std::ofstream image_file(temp_dir_path + "/" + image_name);
        image_file.close();
    }

    // Call the findAvailableImages function
    std::vector<std::string> result = Utils::findAvailableImages(temp_dir_path);

    // Check if the result matches the expected images
    ASSERT_EQ(result.size(), expected_images.size()) << "Number of images found does not match";
    for (const auto& image_name : expected_images)
    {
        EXPECT_TRUE(std::find(result.begin(), result.end(), image_name) != result.end())
            << "Image " << image_name << " not found";
    }

    // Remove the temporary directory and its contents
    fs::remove_all(temp_dir_path);
}

TEST(FindAvailableImagesTest, TestInvalidDirectory)
{
    // Call the findAvailableImages function with an invalid directory path
    std::vector<std::string> result = Utils::findAvailableImages("/non_existing_directory");

    // Check if the result vector is empty
    EXPECT_TRUE(result.empty()) << "Result vector should be empty for invalid directory";
}

TEST(CompressImgTest, TestCompressImage)
{
    // Create a temporary text file for testing
    std::string temp_txt_path = "/tmp/test_file.txt";
    std::ofstream temp_txt_file(temp_txt_path);
    temp_txt_file << "Content for testing file compression";
    temp_txt_file.close();

    std::string zip_path = "/tmp/test_zip.zip";
    bool result = Utils::compressImg(temp_txt_path, zip_path);

    ASSERT_TRUE(result) << "File compression failed";

    EXPECT_TRUE(fs::exists(zip_path)) << "File was not created";

    fs::remove(temp_txt_path);
    fs::remove(zip_path);
}
