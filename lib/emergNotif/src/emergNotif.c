#include "emergNotif.h"

int init_emergency_notification()
{
    int sockfd;
    struct sockaddr_un server_addr;

    if ((sockfd = socket(AF_UNIX, SOCK_STREAM, 0)) == -1)
    {
        perror("socket");
        exit(EXIT_FAILURE);
    }
    server_addr.sun_family = AF_UNIX;
    strcpy(server_addr.sun_path, SOCK_PATH);

    if (connect(sockfd, (struct sockaddr*)&server_addr, sizeof(server_addr)) == -1)
    {
        perror("connect");
        exit(EXIT_FAILURE);
    }

    return sockfd;
}

void simulate_electricity_failure(int sockfd)
{
    if (sockfd < 0)
    {
        fprintf(stderr, "Error: Emergency notification module not initialized.\n");
    }

    char* message = FAILURE_MESSAGE;
    size_t message_len = strlen(message);
    if (send(sockfd, message, message_len, 0) < 0)
    {
        perror("Error sending failure message");
    }
}

int get_random_failure_minutes()
{
    return (rand() % 6) + 5;
}
