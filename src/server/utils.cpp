#include "utils.hpp"

namespace fs = std::filesystem;

namespace Utils
{

void createDirectoriesIfNotExists(const std::string& path)
{
    if (!fs::exists(path))
    {
        if (!fs::create_directories(path))
        {
            std::cerr << "Error creating directory: " << path << std::endl;
        }
        else
        {
            std::cout << "Directory created: " << path << std::endl;
        }
    }
    else
    {
        std::cout << "Directory already exists: " << path << std::endl;
    }
}

void logEvent(const std::string& message)
{
    std::ofstream logFile;
    time_t currentTime = std::time(nullptr);
    struct tm* localTime = std::localtime(&currentTime);
    char timestamp[256];

    // Format the date and time
    std::strftime(timestamp, sizeof(timestamp), "[%Y-%m-%d %H:%M:%S]", localTime);

    // Get the user's home directory
    const char* homeDir = getenv("HOME");
    if (homeDir == nullptr)
    {
        std::cerr << "Error: HOME environment variable not set." << std::endl;
        return;
    }

    // Construct the log directory path
    std::string logDirPath = std::string(homeDir) + REFUGE_DIR;

    // Create the directory if it doesn't exist
    struct stat st;
    if (stat(logDirPath.c_str(), &st) == -1)
    {
        if (mkdir(logDirPath.c_str(), 0700) == -1)
        {
            perror("Error creating directory");
            return;
        }
        std::cout << "Directory created: " << logDirPath << std::endl;
    }

    // Construct the log file path
    std::string logFilePath = logDirPath + LOG_FILENAME;

    // Open the log file in append mode
    logFile.open(logFilePath, std::ios_base::app);
    if (!logFile.is_open())
    {
        std::cerr << "Error opening log file" << std::endl;
        return;
    }

    // Write the timestamp and message to the log file
    logFile << timestamp << " " << message << std::endl;

    // Close the log file
    logFile.close();
}

std::string getCurrentTimestamp()
{
    auto now = std::chrono::system_clock::now();
    auto now_c = std::chrono::system_clock::to_time_t(now);
    struct std::tm* timeinfo = std::localtime(&now_c);
    std::stringstream ss;
    ss << "[" << std::put_time(timeinfo, "%Y-%m-%d %H:%M:%S") << "] ";
    return ss.str();
}

void cleanup_fifo(const char* fifo_path)
{
    // Remove the FIFO file
    if (unlink(fifo_path) == -1 && errno != ENOENT)
    {
        std::cerr << "Error while unlinking FIFO: " << std::strerror(errno) << std::endl;
    }
}

bool fileExists(const std::string& directory, const std::string& filename)
{
    namespace fs = std::filesystem;
    fs::path filePath = directory + filename;
    return fs::exists(filePath) && fs::is_regular_file(filePath);
}

int getFileSize(const std::string& filePath)
{
    struct stat fileStat;
    if (stat(filePath.c_str(), &fileStat) != 0)
    {
        return -1;
    }
    return fileStat.st_size;
}

void deleteDirectory(const std::string& path)
{
    if (fs::exists(path)) // Check if directory exists
    {
        try
        {
            fs::remove_all(path); // Remove the directory and its contents
            std::cout << "Directory deleted: " << path << std::endl;
        }
        catch (const fs::filesystem_error& e)
        {
            std::cerr << "Error deleting directory: " << e.what() << std::endl;
        }
    }
    else
    {
        std::cout << "Directory doesn't exist: " << path << std::endl;
    }
}

void redirectOutputToParent(int child_stdout)
{
    if (dup2(child_stdout, STDOUT_FILENO) == -1)
    {
        perror("dup2");
        exit(EXIT_FAILURE);
    }
}

cJSON* convertJsonToCJson(const json& json_obj)
{
    // Convert JSON object to string
    std::string json_str = json_obj.dump();

    // Parse JSON string using cJSON_Parse
    cJSON* cJSON_obj = cJSON_Parse(json_str.c_str());

    if (cJSON_obj == nullptr)
    {
        // cJSON_Parse failed, print error
        const char* error_ptr = cJSON_GetErrorPtr();
        if (error_ptr != nullptr)
        {
            fprintf(stderr, "Error parsing JSON: %s\n", error_ptr);
        }
        else
        {
            fprintf(stderr, "Error parsing JSON: Unknown error\n");
        }
    }

    // Free cJSON object if it's null
    if (cJSON_obj != nullptr && cJSON_obj->type == cJSON_NULL)
    {
        cJSON_Delete(cJSON_obj);
        cJSON_obj = nullptr;
    }

    return cJSON_obj;
}

const char* detectEntry(const char* alertMessage)
{
    if (std::strstr(alertMessage, "NORTH") != nullptr)
    {
        return "NORTH";
    }
    else if (std::strstr(alertMessage, "SOUTH") != nullptr)
    {
        return "SOUTH";
    }
    else if (std::strstr(alertMessage, "EAST") != nullptr)
    {
        return "EAST";
    }
    else if (std::strstr(alertMessage, "WEST") != nullptr)
    {
        return "WEST";
    }
    else
    {
        return nullptr;
    }
}

void cleanUpUnixSocket(const char* socket_path)
{
    if (unlink(socket_path) < 0 && errno != ENOENT)
    {
        std::perror("Error while unlinking Unix domain socket");
    }
}

std::vector<std::string> findAvailableImages(const std::string& directoryPath)
{
    std::vector<std::string> availableImages;

    try
    {
        // Iterate over the files in the directory
        for (const auto& entry : fs::directory_iterator(directoryPath))
        {
            if (fs::is_regular_file(entry))
            {
                availableImages.push_back(entry.path().filename().string());
            }
        }
    }
    catch (const std::exception& e)
    {
        std::cerr << "Error reading directory: " << e.what() << std::endl;
    }

    return availableImages;
}

bool compressImg(const std::string& imagePath, const std::string& zipPath)
{
    std::cout << "Compressing image: " << imagePath << std::endl;

    // Open the image file
    std::ifstream inputFile(imagePath, std::ios::binary);
    if (!inputFile)
    {
        std::cerr << "Error opening image file" << std::endl;
        return false;
    }

    // Read the image file into a buffer
    inputFile.seekg(0, std::ios::end);
    size_t fileSize = inputFile.tellg();
    inputFile.seekg(0, std::ios::beg);
    std::vector<char> buffer(fileSize);
    inputFile.read(buffer.data(), fileSize);
    inputFile.close();

    // Open the zip file
    gzFile zipFile = gzopen(zipPath.c_str(), "wb");
    if (!zipFile)
    {
        std::cerr << "Error opening ZIP file" << std::endl;
        return false;
    }

    // Compress the image buffer and write to the zip file
    if (gzwrite(zipFile, buffer.data(), buffer.size()) != buffer.size())
    {
        std::cerr << "Error writing compressed data to ZIP file" << std::endl;
        gzclose(zipFile);
        return false;
    }

    // Close the zip file
    if (gzclose(zipFile) != Z_OK)
    {
        std::cerr << "Error closing ZIP file" << std::endl;
        return false;
    }

    return true;
}
} // namespace Utils
