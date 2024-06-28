#ifndef UTILS_HPP
#define UTILS_HPP

#include "cJSON.h"
#include <cerrno>
#include <cstdlib>
#include <cstring>
#include <ctime>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <nlohmann/json.hpp>
#include <openssl/sha.h>
#include <string>
#include <sys/stat.h>
#include <unistd.h>
//#include <zip.h>
#include <zlib.h>

#define LOG_FILENAME "refuge_lab2.log"
#define REFUGE_DIR "/.refuge/"
#define DB_NAME "../build/database"

using json = nlohmann::json;

namespace Utils
{
/**
 * @brief Creates directories if they do not exist.
 *
 * This method checks if the specified directories exist and creates them if they do not exist.
 * It handles the creation of directories for storing images, compressed files, and conversion outputs.
 * If a directory fails to be created, an error message is printed to standard error.
 */
void createDirectoriesIfNotExists(const std::string& path);

/**
 * @brief Logs an event message to a file.
 *
 * Logs the specified event message along with a timestamp to a log file located in the user's home directory.
 * If the log file or directory does not exist, it creates them.
 *
 * @param message The message to be logged.
 */
void logEvent(const std::string& message);

/**
 * @brief Gets the current timestamp in string format.
 *
 * Retrieves the current system time and formats it into a string representing the timestamp
 * in the format "[YYYY-MM-DD HH:MM:SS]".
 *
 * @return A string containing the current timestamp.
 */
std::string getCurrentTimestamp();

/**
 * @brief Cleans up a FIFO file.
 *
 * Removes the specified FIFO file. If the file does not exist, no action is taken.
 *
 * @param fifo_path The path to the FIFO file to be removed.
 */
void cleanup_fifo(const char* fifo_path);

/**
 * @brief Checks if a file exists in the specified directory.
 *
 * This method checks if a file with the specified filename exists in the specified directory.
 *
 * @param directory The directory where the file is expected to be located.
 * @param filename The name of the file to check for existence.
 * @return True if the file exists and is a regular file, otherwise false.
 */
bool fileExists(const std::string& directory, const std::string& filename);

/**
 * @brief Gets the size of a file.
 *
 * This method retrieves the size of the file specified by the given file path.
 * It uses the `stat` function to obtain information about the file, including its size.
 * If the file does not exist or if there's an error retrieving its size, it returns -1.
 *
 * @param filePath The path to the file.
 * @return The size of the file in bytes, or -1 if the file does not exist or if there's an error.
 */
int getFileSize(const std::string& zipPath);

/**
 * @brief Deletes a directory and its contents.
 *
 * This method deletes the specified directory and all its contents, including subdirectories and files.
 * If the directory does not exist, it prints a message indicating that the directory does not exist.
 * If an error occurs while deleting the directory, it prints an error message.
 *
 * @param path The path to the directory to be deleted.
 */
void deleteDirectory(const std::string& path);

/**
 * @brief Redirects the output of a child process to its parent.
 *
 * Redirects the output of the child process to its parent process.
 *
 * @param child_stdout The file descriptor representing the output stream of the child process.
 */
void redirectOutputToParent(int child_stdout);

/**
 * @brief Converts a JSON object to a cJSON object.
 *
 * Converts a JSON object to a cJSON object.
 *
 * @param json_obj The JSON object to convert.
 * @return The converted cJSON object.
 */
cJSON* convertJsonToCJson(const json& json_obj);

/**
 * @brief Detects the entry associated with an alert message.
 *
 * Determines the entry (north, south, east, or west) associated with the given alert message.
 *
 * @param alertMessage The alert message to analyze.
 * @return A pointer to a C-style string representing the detected entry, or nullptr if no entry is detected.
 */
const char* detectEntry(const char* alertMessage);

/**
 * @brief Cleans up a Unix domain socket.
 *
 * Removes the specified Unix domain socket file. If the file does not exist, no action is taken.
 *
 * @param socket_path The path to the Unix domain socket file to be removed.
 */
void cleanUpUnixSocket(const char* socket_path);

/**
 * @brief Finds available images in the specified directory.
 *
 * This method searches for available image files in the specified directory path.
 *
 * @param directoryPath The path to the directory containing the images.
 * @return A vector of strings representing the filenames of the available images.
 */
std::vector<std::string> findAvailableImages(const std::string& directoryPath);

/**
 * @brief Compresses an image file into a ZIP archive.
 *
 * This method compresses the specified image file into a ZIP archive.
 *
 * @param imagePath The path to the image file to be compressed.
 * @param zipPath The path to the ZIP archive to be created.
 * @return True if the compression is successful, false otherwise.
 */
bool compressImg(const std::string& imagePath, const std::string& zipPath);

/**
 * @brief Class for generating unique IDs.
 */
class IdGen
{
  public:
    /**
     * @brief Default constructor of the `IdGen` class.
     *
     * Initializes the counter to 0.
     */
    IdGen() : counter(0)
    {
    }

    /**
     * @brief Sets the value of the counter to the provided value.
     *
     * @param value The value to set the counter to.
     */
    void setId(int value)
    {
        counter = value + 1;
    }

    /**
     * @brief Generates the next unique ID.
     *
     * @return A string representing the next unique ID.
     *
     * This method generates the next unique ID by incrementing the internal counter and converting it to a string.
     */
    std::string getNextId()
    {
        std::ostringstream oss;
        oss << counter++;
        return oss.str();
    }

  private:
    int counter; /**< Counter to track the next ID. */
};

} // namespace Utils

#endif // UTILS_HPP
