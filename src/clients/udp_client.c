#include "udp_client.h"

static struct sockaddr_in server_addr;
static struct sockaddr_in6 server_addrv6;

int CONNECTED = 1;
pid_t pid_child;
int ipv6;
char* ip_address = NULL;

int main(int argc, char* argv[])
{
    signal(SIGINT, handle_sigint);

    pid_t pid = getpid();
    printf("PID of process: %d\n", pid);

    int port;
    int ipv6 = get_port(argc, argv, &port);
    int sockfd = establish_connection(port, ipv6);

    printf("Connecting to server at ip: %s\n", ip_address);

    pid_child = fork();
    if (pid_child == -1)
    {
        perror("fork");
        exit(EXIT_FAILURE);
    }
    if (pid_child == 0)
    {
        redirect_output_to_parent();
        handle_server_messages(sockfd);
    }
    else
    {
        printf("PID of child process: %d\n", pid_child);
        handle_user_input(sockfd, ipv6);
    }
    return 0;
}

void parse_command_line_args(int argc, char* argv[], int* port, int* ipv6, char** ip_address)
{
    int option;
    int address_flag = 0; // Variable to control if the -a flag has been specified

    *port = -1; // Set default port value to -1
    *ipv6 = 0;  // Default to IPv4

    while ((option = getopt(argc, argv, "p:a:t:")) != -1)
    {
        switch (option)
        {
        case 'p':
            *port = atoi(optarg); // Convert string argument to integer
            break;
        case 'a':
            if (strstr(optarg, "::") != NULL)
            { // Search "::" in the IP address
                *ipv6 = 1;
            }
            else
            {
                *ipv6 = 0;
            }
            *ip_address = optarg; // Save the provided IP address
            address_flag = 1;     // -a flag was specified
            break;
        case 't':
            if (address_flag)
            { // If -a flag was already specified, -t is not allowed
                fprintf(
                    stderr,
                    "Cannot specify both -t and -a flags. IPv4 or IPv6 type is inferred from the IP address format\n");
                exit(EXIT_FAILURE);
            }
            if (strcmp(optarg, "v4") == 0)
            {
                *ipv6 = 0;
            }
            else if (strcmp(optarg, "v6") == 0)
            {
                *ipv6 = 1;
            }
            else
            {
                fprintf(stderr, "Invalid value for -t option. Use 0 for IPv4 or 'ipv6' for IPv6.\n");
                exit(EXIT_FAILURE);
            }
            break;
        default:
            fprintf(stderr, "Usage: %s [-p port] [-a ip_address] [-t 0|ipv6]\n", argv[0]);
            exit(EXIT_FAILURE);
        }
    }
}

int get_port_whith_shared_mem()
{
    int shm_fd;
    struct PortInfo* shared_port_info;

    // Open the shared memory object
    shm_fd = shm_open(SHARED_MEM_PORT, O_RDONLY, 0666);
    if (shm_fd == -1)
    {
        perror("shm_open");
        return -1;
    }

    // Map the shared memory object
    shared_port_info = mmap(NULL, sizeof(struct PortInfo), PROT_READ, MAP_SHARED, shm_fd, 0);
    if (shared_port_info == MAP_FAILED)
    {
        perror("mmap");
        close(shm_fd);
        return -1;
    }

    // Read the UDP port number from shared memory
    int udp_port = shared_port_info->udp_port;

    // Unmap and close shared memory
    if (munmap(shared_port_info, sizeof(struct PortInfo)) == -1)
    {
        perror("munmap");
    }
    close(shm_fd);

    return udp_port;
}

void set_server_addr(int port, int ipv6)
{
    if (ipv6)
    {
        server_addrv6.sin6_family = AF_INET6;
        server_addrv6.sin6_port = htons((uint16_t)port);
        inet_pton(AF_INET6, ip_address, &server_addrv6.sin6_addr);
    }
    else
    {
        server_addr.sin_family = AF_INET;
        server_addr.sin_port = htons((uint16_t)port);
        inet_pton(AF_INET, ip_address, &server_addr.sin_addr);
    }
}

int establish_connection(int port, int ipv6)
{
    int sockfd;

    if (ipv6)
    {
        sockfd = socket(AF_INET6, SOCK_DGRAM, 0);
        if (sockfd < 0)
        {
            perror("Socket creation failed");
            exit(EXIT_FAILURE);
        }
    }
    else
    {
        sockfd = socket(AF_INET, SOCK_DGRAM, 0);
        if (sockfd < 0)
        {
            perror("Socket creation failed");
            exit(EXIT_FAILURE);
        }
    }
    set_server_addr(port, ipv6);
    return sockfd;
}

void send_to_server_v4(int sockfd, const char* message, const struct sockaddr_in* server_addr)
{
    char buffer[BUFFER_SIZE];
    ssize_t bytes_sent;

    // Ensure message length does not exceed buffer size
    size_t message_length = strlen(message);
    if (message_length >= BUFFER_SIZE)
    {
        fprintf(stderr, "Error: Message exceeds buffer size\n");
        return;
    }
    strcpy(buffer, message);
    bytes_sent = sendto(sockfd, buffer, message_length, 0, (struct sockaddr*)server_addr, sizeof(*server_addr));
    if (bytes_sent == -1)
    {
        perror("sendto");
        return;
    }
    printf("[+]Data sent: %s\n", buffer);
    printf("  Destination: %s:%d\n", inet_ntoa(server_addr->sin_addr), ntohs(server_addr->sin_port));
}

void send_to_server_v6(int sockfd, const char* message, const struct sockaddr_in6* server_addr)
{
    char buffer[BUFFER_SIZE];
    ssize_t bytes_sent;

    // Ensure message length does not exceed buffer size
    size_t message_length = strlen(message);
    if (message_length >= BUFFER_SIZE)
    {
        fprintf(stderr, "Error: Message exceeds buffer size\n");
        return;
    }

    // Copy message to buffer
    strcpy(buffer, message);

    bytes_sent = sendto(sockfd, buffer, message_length, 0, (struct sockaddr*)server_addr, sizeof(*server_addr));
    if (bytes_sent == -1)
    {
        perror("sendto");
        return;
    }
    printf("[+]Data sent: %s\n", buffer);
    printf("  Destination: %s\n", inet_ntop(AF_INET6, &server_addr->sin6_addr, buffer, sizeof(buffer)));
}

void disconnect(int sockfd)
{
    close(sockfd);
}

void sendJson_v4(int sockfd, cJSON* json, const struct sockaddr_in* server_addr)
{
    char* json_string = cJSON_Print(json);
    if (json_string == NULL)
    {
        fprintf(stderr, "Error: Unable to convert cJSON object to JSON string\n");
        return;
    }
    send_to_server_v4(sockfd, json_string, server_addr);
    free(json_string);
}

void sendJson_v6(int sockfd, cJSON* json, const struct sockaddr_in6* server_addr)
{
    char* json_string = cJSON_Print(json);
    if (json_string == NULL)
    {
        fprintf(stderr, "Error: Unable to convert cJSON object to JSON string\n");
        return;
    }
    send_to_server_v6(sockfd, json_string, server_addr);
    free(json_string);
}

int get_port(int argc, char* argv[], int* port)
{
    int ipv6;
    parse_command_line_args(argc, argv, port, &ipv6, &ip_address);
    if (*port == -1)
    {
        *port = get_port_whith_shared_mem(); // Get UDP port from shared memory if not provided via command line
        if (*port == -1)
        {
            fprintf(stderr, "Error retrieving UDP port from shared memory\n");
            exit(EXIT_FAILURE);
        }
        printf("UDP Port obtained from shared memory: %d\n", *port);
    }
    else
    {
        printf("UDP Port passed as parameter: %d\n", *port);
    }
    return ipv6;
}

void handle_sigint()
{
    CONNECTED = 0;
    exit(EXIT_SUCCESS);
}

void redirect_output_to_parent()
{
    if (dup2(STDOUT_FILENO, pid_child) == -1)
    {
        perror("dup2");
        exit(EXIT_FAILURE);
    }
}

void handle_user_input(int sockfd, int ipv6)
{
    char input[BUFFER_SIZE];

    char* os_id = get_os_id();
    char* utf8_os_id = encode_utf8(os_id);

    // Clear the buffers
    memset(input, 0, BUFFER_SIZE);

    while (CONNECTED)
    {
        // Print menu options
        print_menu_options();

        if (fgets(input, BUFFER_SIZE, stdin) == NULL)
        {
            printf("Error reading input\n");
            return;
        }

        // Process user input
        if (strcmp(input, "1\n") == 0)
        {
            cJSON* json = cJSON_CreateObject();
            cJSON_AddStringToObject(json, "hostname", utf8_os_id);
            cJSON_AddStringToObject(json, "message", "status");
            if (ipv6)
            {
                sendJson_v6(sockfd, json, &server_addrv6);
            }
            else
            {
                sendJson_v4(sockfd, json, &server_addr);
            }
            cJSON_Delete(json);
        }
        else if (strcmp(input, "2\n") == 0)
        {
            printf("Select a resource to update:\n");
            printf("1. Food\n");
            printf("2. Medicine\n");
            printf("Enter your choice (1-2): ");

            if (fgets(input, BUFFER_SIZE, stdin) == NULL)
            {
                printf("Error reading input\n");
                return;
            }

            cJSON* json = cJSON_CreateObject();
            cJSON_AddStringToObject(json, "hostname", utf8_os_id);
            cJSON_AddStringToObject(json, "message", "update");

            if (strcmp(input, "1\n") == 0)
            {
                printf("Select a food resource to update:\n");
                printf("1. Meat\n");
                printf("2. Vegetables\n");
                printf("3. Fruits\n");
                printf("4. Water\n");
                printf("Enter your choice (1-4): ");

                if (fgets(input, BUFFER_SIZE, stdin) == NULL)
                {
                    printf("Error reading input\n");
                    return;
                }

                cJSON* foodJson = cJSON_CreateObject();

                int value;
                printf("Enter the amount to update (positive or negative): ");
                if (scanf("%d", &value) != 1)
                {
                    printf("Error converting input to integer\n");
                    return;
                }
                getchar(); // consume the newline character

                if (strcmp(input, "1\n") == 0)
                {
                    cJSON_AddNumberToObject(foodJson, "meat", value);
                }
                else if (strcmp(input, "2\n") == 0)
                {
                    cJSON_AddNumberToObject(foodJson, "vegetables", value);
                }
                else if (strcmp(input, "3\n") == 0)
                {
                    cJSON_AddNumberToObject(foodJson, "fruits", value);
                }
                else if (strcmp(input, "4\n") == 0)
                {
                    cJSON_AddNumberToObject(foodJson, "water", value);
                }
                else
                {
                    printf("Invalid option.\n");
                    cJSON_Delete(json);
                    continue;
                }

                cJSON_AddItemToObject(json, "food", foodJson);
            }
            else if (strcmp(input, "2\n") == 0)
            {
                printf("Select a medicine resource to update:\n");
                printf("1. Antibiotics\n");
                printf("2. Analgesics\n");
                printf("3. Bandages\n");
                printf("Enter your choice (1-3): ");

                if (fgets(input, BUFFER_SIZE, stdin) == NULL)
                {
                    printf("Error reading input\n");
                    return;
                }

                cJSON* medicineJson = cJSON_CreateObject();

                int value;
                printf("Enter the amount to update (positive or negative): ");
                if (scanf("%d", &value) != 1)
                {
                    printf("Error converting input to integer\n");
                    return;
                }
                getchar(); // consume the newline character

                if (strcmp(input, "1\n") == 0)
                {
                    cJSON_AddNumberToObject(medicineJson, "antibiotics", value);
                }
                else if (strcmp(input, "2\n") == 0)
                {
                    cJSON_AddNumberToObject(medicineJson, "analgesics", value);
                }
                else if (strcmp(input, "3\n") == 0)
                {
                    cJSON_AddNumberToObject(medicineJson, "bandages", value);
                }
                else
                {
                    printf("Invalid option.\n");
                    cJSON_Delete(json);
                    continue;
                }

                cJSON_AddItemToObject(json, "medicine", medicineJson);
            }
            else
            {
                printf("Invalid option.\n");
                cJSON_Delete(json);
                continue;
            }

            if (ipv6)
            {
                sendJson_v6(sockfd, json, &server_addrv6);
            }
            else
            {
                sendJson_v4(sockfd, json, &server_addr);
            }

            cJSON_Delete(json);
        }
        else if (strcmp(input, "3\n") == 0)
        {
            cJSON* json = cJSON_CreateObject();
            cJSON_AddStringToObject(json, "hostname", utf8_os_id);
            cJSON_AddStringToObject(json, "message", "summary");
            if (ipv6)
            {
                sendJson_v6(sockfd, json, &server_addrv6);
            }
            else
            {
                sendJson_v4(sockfd, json, &server_addr);
            }
            cJSON_Delete(json);
        }

        else if (strcmp(input, "4\n") == 0)
        {
            // Terminate the child process if it's running
            if (pid_child != 0)
            {
                kill(pid_child, SIGINT);
            }
            CONNECTED = 0;
            disconnect(sockfd);
        }
        else
        {
            printf("Invalid option. Please choose a valid option.\n");
        }
    }
}

void receive_and_handle_json(int sockfd)
{
    char buffer[BUFFER_SIZE];
    socklen_t addrlen = sizeof(server_addr);

    ssize_t bytes_received = recvfrom(sockfd, buffer, BUFFER_SIZE, 0, (struct sockaddr*)&server_addr, &addrlen);
    if (bytes_received > 0)
    {
        buffer[bytes_received] = '\0'; // Add null terminator
        handle_received_json(buffer);
    }
    else if (bytes_received == 0)
    {
        printf("No data received from server\n");
    }
    else
    {
        perror("recvfrom");
    }
}

void handle_received_json(const char* json_string)
{
    cJSON* json = cJSON_Parse(json_string);
    if (json)
    {
        char* formatted_json = cJSON_Print(json);
        printf("Received JSON from server: %s\n", formatted_json);
        free(formatted_json);
        cJSON_Delete(json);
    }
    else
    {
        printf("Received message from server: %s\n", json_string);
    }
}

void handle_server_messages(int sockfd)
{

    while (CONNECTED)
    {

        receive_and_handle_json(sockfd);
    }
    disconnect(sockfd);
}

void print_menu_options()
{
    printf("╔═════════════════════════ Menu ═════════════════════════╗\n");
    printf("║  1. Request supplies status                            ║\n");
    printf("║  2. Update supplies status                             ║\n");
    printf("║  3. Summary                                            ║\n");
    printf("║  5. Exit                                               ║\n");
    printf("╚════════════════════════════════════════════════════════╝\n");

    printf("Enter your choice (1-5): ");
}

char* get_os_id()
{
    FILE* file = fopen(OS_RELEASE_PATH, "r");
    if (file == NULL)
    {
        perror("Error opening file");
        exit(EXIT_FAILURE);
    }

    char* os_id = NULL;
    char line[BUFFER_SIZE];
    while (fgets(line, sizeof(line), file))
    {
        if (strncmp(line, "ID=", OS_RELEASE_ID_FIELD) == 0)
        {
            os_id = extract_value(line);
            break;
        }
    }

    fclose(file);
    return os_id;
}

char* extract_value(char* line)
{
    char* delimiter = "=";
    char* token = strtok(line, delimiter);
    token = strtok(NULL, delimiter);
    char* value = strtok(token, "\n"); // delete newline character if present
    return value;
}

char* encode_utf8(const char* input)
{
    size_t length = strlen(input);
    char* utf8_str = (char*)malloc(length * 2 + 1);
    if (utf8_str == NULL)
    {
        perror("Error de memoria");
        exit(EXIT_FAILURE);
    }

    int i = 0, j = 0;
    while (input[i])
    {
        if ((input[i] & 0x80) == 0)
        {
            utf8_str[j++] = input[i++];
        }
        else if ((input[i] & 0xE0) == 0xC0)
        {
            utf8_str[j++] = (char)(0xC0 | ((input[i] >> 6) & 0x1F));
            utf8_str[j++] = (char)(0x80 | (input[i] & 0x3F));

            i++;
        }
        else if ((input[i] & 0xF0) == 0xE0)
        {
            utf8_str[j++] = (char)(0xE0 | ((input[i] >> 12) & 0x0F));
            utf8_str[j++] = (char)(0x80 | (input[i] & 0x3F));
            utf8_str[j++] = (char)(0x80 | (input[i] & 0x3F));
            i++;
        }
        else
        {
            i++;
        }
    }
    utf8_str[j] = '\0';
    return utf8_str;
}
