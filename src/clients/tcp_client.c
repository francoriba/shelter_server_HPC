#include "tcp_client.h"

int CONNECTED = 1;
pid_t pid_child;
pid_t pid_parent;
int authenticated = 0;
char* ip_address = NULL;
int zip_size_bytes = 0;

int main(int argc, char* argv[])
{

    // handle SIGINT signal
    signal(SIGINT, handle_sigint);

    // print the PID of the process
    pid_parent = getpid();
    printf("PID of process: %d\n", pid_parent);

    int port;
    int is_ipv6;

    parse_command_line_args(argc, argv, &port, &is_ipv6, &ip_address);
    get_port(&port);

    // If the IP address is not provided, use the default IP address for loopback
    if (ip_address == NULL)
    {
        ip_address = (is_ipv6 == 1) ? SERVER_IP_V6_LOOP : SERVER_IP_V4_LOOP;
    }

    int sockfd = establish_connection(port, is_ipv6);

    // Authenticate the client with the server after establishing the connection
    if (!authenticated)
    {
        authenticate(sockfd);
        authenticated = 1;
    }

    pid_child = fork();
    if (pid_child < 0)
    {
        perror("fork");
        exit(EXIT_FAILURE);
    }
    else if (pid_child == 0)
    {
        redirect_output_to_parent();
        printf("\nPID of child process: %d\n", getppid());
        handle_server_messages(sockfd);
    }
    else
    {
        handle_user_input(sockfd);
    }

    return 0;
}

void parse_command_line_args(int argc, char* argv[], int* port, int* ipv6, char** ip_address)
{
    int option;
    int address_flag = 0; // Variable to control if the -a flag has been specified

    // Set default port value to -1
    *port = -1;
    *ipv6 = 0; // Default to IPv4

    // Parse command-line arguments
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
            // printf("IP Address: %s\n", *ip_address);
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

    // Open the shared memory region
    shm_fd = shm_open(SHARED_MEM_PORT, O_RDONLY, 0666);
    if (shm_fd == -1)
    {
        perror("shm_open");
        return -1;
    }

    // Map the shared memory region into the process address space
    shared_port_info = mmap(NULL, sizeof(struct PortInfo), PROT_READ, MAP_SHARED, shm_fd, 0);
    if (shared_port_info == MAP_FAILED)
    {
        perror("mmap");
        close(shm_fd);
        return -1;
    }

    // Read the TCP port from the shared memory region
    int tcp_port = shared_port_info->tcp_port;

    // Unmap and close the shared memory region
    if (munmap(shared_port_info, sizeof(struct PortInfo)) == -1)
    {
        perror("munmap");
    }
    close(shm_fd);

    return tcp_port;
}

int establish_connection(int port, int is_ipv6)
{
    int sockfd;

    // Call doConnect() function to handle connection
    char port_str[6];                                 // Buffer to store port as string
    snprintf(port_str, sizeof(port_str), "%d", port); // Convert port to string
    sockfd = do_connect(ip_address, port_str);        // Call doConnect() with IP address and port

    // Print the type of IP address used
    printf("Connected using %s\n", (is_ipv6 ? "IPv6" : "IPv4"));

    return sockfd;
}

void disconnect(int sockfd)
{
    close(sockfd);
    exit(EXIT_SUCCESS);
}

void send_json(int sockfd, cJSON* json)
{
    char* json_string = cJSON_Print(json);
    send(sockfd, json_string, strlen(json_string), 0);
    // TODO: comment this when not debugging
    // printf("JSON sent to server: %s\n", json_string);
    free(json_string);
}

void receive_json(int sockfd)
{
    char buffer[BUFFER_SIZE];
    memset(buffer, 0, BUFFER_SIZE); // Clear buffer

    // Receive JSON data from server
    ssize_t bytes_received = recv(sockfd, buffer, BUFFER_SIZE, 0);
    if (bytes_received < 0)
    {
        perror("Error receiving JSON from server");
        return;
    }
    else if (bytes_received == 0)
    {
        printf("\n Server closed the connection.. disconnecting\n");
        disconnect(sockfd);
        return;
    }

    // Parse received JSON
    cJSON* json = cJSON_Parse(buffer);

    // if (json == NULL)
    // {
    //     printf("\nReceived message from server: %s\n", buffer);
    //     return;
    // }

    // printf("\nReceived JSON from server:\n%s\n", cJSON_Print(json));

    // Handle the received JSON
    cJSON* message = cJSON_GetObjectItem(json, "message");
    if (message)
    {
        const char* message_value = message->valuestring;

        if (strcmp(message_value, "auth_success") == 0)
        {
            printf("\n \u2713 Authentication successful. You can make supplies updates\n");
            authenticated = 1;
        }
        else if (strcmp(message_value, "auth_failure") == 0)
        {
            printf("\nAuthentication failed: Invalid hostname. You can only check the status but can't update\n");
        }
        else if (strcmp(message_value, "disconnect") == 0)
        {
            printf("\nServer closed the connection.. disconnecting\n");
            kill(pid_parent, SIGINT);
            exit(EXIT_SUCCESS);
        }
        else if (strcmp(message_value, "image_list") == 0 && getpid() == pid_parent)
        { // this branch should only be executed by the parent process

            cJSON* images = cJSON_GetObjectItem(json, "images");
            if (images && cJSON_IsArray(images))
            {
                int images_count = cJSON_GetArraySize(images);
                printf("\nAvailable images:\n");
                for (int i = 0; i < images_count; ++i)
                {
                    cJSON* image = cJSON_GetArrayItem(images, i);
                    if (image && cJSON_IsString(image))
                    {
                        printf("%d. %s\n", i + 1, image->valuestring);
                    }
                }
                int selected_image_index = 0;
                printf("\nSelect an image by entering its number: ");
                if (scanf("%d", &selected_image_index) != 1)
                {
                    printf("Error converting input to integer\n");
                    return;
                }
                getchar(); // consume the newline character
                if (selected_image_index >= 1 && selected_image_index <= images_count)
                {
                    cJSON* selected_image = cJSON_GetArrayItem(images, selected_image_index - 1);
                    if (selected_image && cJSON_IsString(selected_image))
                    {
                        printf("You selected: %s\n", selected_image->valuestring);
                        cJSON* json_selection = cJSON_CreateObject();
                        cJSON_AddStringToObject(json_selection, "message", "image_selection");
                        cJSON_AddStringToObject(json_selection, "image", selected_image->valuestring);
                        send_json(sockfd, json_selection);
                        cJSON_Delete(json_selection);
                    }
                    else
                    {
                        printf("Invalid selection.\n");
                    }
                }
                else
                {
                    printf("Invalid selection.\n");
                }
            }
            else
            {
                printf("Error: Unable to retrieve image list from JSON.\n");
            }
        }
        else if (strcmp(message_value, "file_size") == 0)
        {
            cJSON* file_size = cJSON_GetObjectItem(json, "size");
            if (file_size && cJSON_IsNumber(file_size))
            {
                // Store the size of the zip file in global variable
                zip_size_bytes = file_size->valueint;
            }
            else
            {
                printf("Error: Unable to retrieve file size from JSON.\n");
            }
        }
        else if (strcmp(message_value, "zip_ready") == 0)
        {
            printf("Server ready to send ZIP \n");
            receive_zip(sockfd, "./cannyResult.zip");
        }
        else if (strcmp(message_value, "alert") == 0)
        {
            cJSON* alert = cJSON_GetObjectItem(json, "alert_description");
            if (alert && cJSON_IsString(alert))
            {
                printf("\n[+] Alert from server: %s\n", alert->valuestring);
            }
        }
        else if (strcmp(message_value, "summary_response") == 0)
        {
            cJSON_DeleteItemFromObject(json, "message");
            char* summary_response_json = cJSON_Print(json);
            printf("\n [+] Summary: \n%s\n", summary_response_json);
            free(summary_response_json);
            fflush(stdout);
        }
        else if (strcmp(message_value, "supplies_response") == 0)
        {
            cJSON_DeleteItemFromObject(json, "message");
            char* supplies_response_json = cJSON_Print(json);
            printf("\n [+] Supplies status: \n%s\n", supplies_response_json);
            fflush(stdout);
            free(supplies_response_json);
        }
        else
        {
            printf("\n [+] Unknown message from server:\n%s\n", cJSON_Print(json));
        }
    }
    // Free memory
    cJSON_Delete(json);
}

void handle_user_input(int sockfd)
{
    char input[BUFFER_SIZE];
    memset(input, 0, BUFFER_SIZE);

    while (CONNECTED)
    {
        // Print menu options
        print_menu_options();

        printf("Enter a value: ");
        if (fgets(input, BUFFER_SIZE, stdin) == NULL)
        {
            printf("Error reading input\n");
            return;
        }

        // Process user input
        if (strcmp(input, "1\n") == 0)
        {
            cJSON* json = cJSON_CreateObject();
            cJSON_AddStringToObject(json, "message", "status");
            send_json(sockfd, json);
            cJSON_Delete(json);
        }
        else if (strcmp(input, "2\n") == 0)
        {
            if (authenticated)
            {
                // Prompt the user for resource update options
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
                cJSON_AddStringToObject(json, "message", "update");

                if (strcmp(input, "1\n") == 0)
                {
                    // User wants to update food resources
                    cJSON* foodJson = cJSON_CreateObject();
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
                    // User wants to update medicine resources
                    cJSON* medicineJson = cJSON_CreateObject();
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

                send_json(sockfd, json);
                cJSON_Delete(json);
            }
            else
            {
                printf("You are not authenticated. You can only check the status but can't update the data\n");
            }
        }
        else if (strcmp(input, "3\n") == 0)
        {
            cJSON* json = cJSON_CreateObject();
            cJSON_AddStringToObject(json, "message", "summary");
            send_json(sockfd, json);
            cJSON_Delete(json);
        }
        else if (strcmp(input, "4\n") == 0) // New option: Request image
        {
            kill(pid_child, SIGSTOP);
            cJSON* json = cJSON_CreateObject();
            cJSON_AddStringToObject(json, "message", "request_available_images");
            send_json(sockfd, json);
            cJSON_Delete(json);
            // receive_json () is used in a loop by the child process but in this case we need the parent to handle the
            // response
            receive_json(sockfd);
            kill(pid_child, SIGCONT);
        }
        else if (strcmp(input, "5\n") == 0)
        {
            // Terminate the child process if it's running
            if (pid_child != 0)
            {
                kill(pid_child, SIGINT);
            }
            // Close the connection and exit
            CONNECTED = 0;
            disconnect(sockfd);
        }
        else
        {
            printf("Invalid option. Please choose a valid option.\n");
        }
    }
}

void handle_server_messages(int sockfd)
{

    while (CONNECTED)
    {

        receive_json(sockfd);
    }
    disconnect(sockfd);
}

void handle_sigint()
{
    // printf("Ctrl+C detected. Terminating client...\n");
    CONNECTED = 0; // Update the global variable to indicate that the client is no longer connected
    exit(EXIT_SUCCESS);
}

void get_port(int* port)
{
    // parse_command_line_args(argc, argv, port, &ipv6, &ip_address);
    if (*port == -1)
    {
        *port = get_port_whith_shared_mem(); // Get TCP port from shared memory if not provided via command line
        if (*port == -1)
        {
            fprintf(stderr, "Error retrieving TCP port from shared memory\n");
            exit(EXIT_FAILURE);
        }
        printf("TCP Port obtained from shared memory: %d\n", *port);
    }
    else
    {
        printf("TCP Port passed as parameter: %d\n", *port);
    }
}

void redirect_output_to_parent()
{
    if (dup2(STDOUT_FILENO, pid_child) == -1)
    {
        perror("dup2");
        exit(EXIT_FAILURE);
    }
}

void print_menu_options()
{
    printf("╔═════════════════════════ Menu ═════════════════════════╗\n");
    printf("║  1. Request supplies status                            ║\n");
    printf("║  2. Update supplies status                             ║\n");
    printf("║  3. Summary                                            ║\n");
    printf("║  4. Request image                                      ║\n");
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
    char* value = strtok(token, "\n");
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

void authenticate(int sockfd)
{
    // Get data for the authentication message
    char* os_id = get_os_id();
    char* utf8_os_id = encode_utf8(os_id);

    // Create JSON authentication message
    cJSON* json = cJSON_CreateObject();
    cJSON_AddStringToObject(json, "message", "authenticateme");
    cJSON_AddStringToObject(json, "hostname", utf8_os_id);
    send_json(sockfd, json);
    cJSON_Delete(json);
}

int do_connect(char* server, char* port)
{
    struct addrinfo hints;
    struct addrinfo *result, *rp;
    int sfd, s;

    /* Obtain address(es) matching host/port */
    memset(&hints, 0, sizeof(struct addrinfo));
    hints.ai_family = AF_UNSPEC;     /* Allow IPv4 or IPv6 */
    hints.ai_socktype = SOCK_STREAM; /* STREAM socket */

    s = getaddrinfo(server, port, &hints, &result);
    if (s != 0)
    {
        fprintf(stderr, "getaddrinfo: %s\n", gai_strerror(s));
        exit(EXIT_FAILURE);
    }

    /* getaddrinfo() returns a list of address structures.
       Try each address until we successfully connect(2).
       If socket(2) (or connect(2)) fails, we (close the socket
       and) try the next address. */

    for (rp = result; rp != NULL; rp = rp->ai_next)
    {
        sfd = socket(rp->ai_family, rp->ai_socktype, rp->ai_protocol);
        if (sfd == -1)
            continue;

        if (connect(sfd, rp->ai_addr, rp->ai_addrlen) != -1)
            break; /* Success */

        close(sfd);
    }

    if (rp == NULL)
    { /* No address succeeded */
        fprintf(stderr, "Could not find the right address to connect\n");
        exit(EXIT_FAILURE);
    }

    freeaddrinfo(result); /* No longer needed */
    return sfd;
}

void receive_zip(int sockfd, const char* file_path)
{
    // Open the file in binary write mode
    FILE* file = fopen(file_path, "wb");
    if (file == NULL)
    {
        perror("Error opening file to write");
        return;
    }

    printf("Receiving ZIP of size %d bytes...\n", zip_size_bytes);

    // Define size of the file to receive
    ssize_t total_bytes_received = 0;
    ssize_t max_bytes_to_receive = zip_size_bytes;

    // Receive data in chunks and write to the file
    char buffer[BUFFER_SIZE];
    ssize_t bytes_received;
    while ((bytes_received = recv(sockfd, buffer, BUFFER_SIZE, 0)) > 0)
    {
        fwrite(buffer, sizeof(char), (size_t)bytes_received, file);
        total_bytes_received += bytes_received;
        if (total_bytes_received >= max_bytes_to_receive)
        {
            break; // Stop receiving data after reaching the expected size
        }
    }
    if (bytes_received < 0)
    {
        perror("Error receiving data from server");
    }
    else
    {
        printf("Zip file received successfully\n");
    }

    fclose(file);
}
