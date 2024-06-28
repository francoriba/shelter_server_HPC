#include "server.hpp"
#include <cstdlib>
#include <cstring>
#include <iostream>
#include <stdio.h>
#include <unistd.h>

#define DEFAULT_PORT 5005

void parse_command_line_arguments(int argc, char* argv[], int* tcp_port, int* udp_port)
{
    int opt;
    while ((opt = getopt(argc, argv, "p:")) != -1)
    {
        switch (opt)
        {
        case 'p':
            if (strcmp(optarg, "tcp") == 0)
            {
                if (optind < argc)
                {
                    *tcp_port = atoi(argv[optind]);
                }
                else
                {
                    std::cout << "TCP port must be specified." << std::endl;
                    exit(EXIT_FAILURE);
                }
            }
            else if (strcmp(optarg, "udp") == 0)
            {
                if (optind < argc)
                {
                    *udp_port = atoi(argv[optind]);
                }
                else
                {
                    std::cout << "UDP port must be specified." << std::endl;
                    exit(EXIT_FAILURE);
                }
            }
            else
            {
                std::cout << "Invalid -p option. It should be 'tcp' or 'udp'." << std::endl;
                exit(EXIT_FAILURE);
            }
            break;
        default:
            std::cout << "Usage: " << argv[0] << " -p tcp <tcp_port> -p udp <udp_port>" << std::endl;
            exit(EXIT_FAILURE);
        }
    }
}

int main(int argc, char* argv[])
{

    std::cout << " _       __     __                             _____            ___       __          _     \n";
    std::cout << "| |     / /__  / /________  ____ ___  ___     / ___/__  _______/   | ____/ /___ ___  (_)___ \n";
    std::cout
        << "| | /| / / _ \\/ / ___/ __ \\/ __ `__ \\/ _ \\    \\__ \\/ / / / ___/ /| |/ __  / __ `__ \\/ / __ \\\n";
    std::cout << "| |/ |/ /  __/ / /__/ /_/ / / / / / /  __/   ___/ / /_/ (__  ) ___ / /_/ / / / / / / / / / /\n";
    std::cout << "|__/|__/\\___/_/\\___/\\____/_/ /_/ /_/\\___/   /____/\\__, /____/_/  |_|\\__,_/_/ /_/ /_/_/_/ /_/\n";
    std::cout << "                                                 /____/                                     \n";

    int tcp_port = DEFAULT_PORT;
    int udp_port = DEFAULT_PORT;

    parse_command_line_arguments(argc, argv, &tcp_port, &udp_port);

    std::cout << "TCP Port: " << tcp_port << std::endl;
    std::cout << "UDP Port: " << udp_port << std::endl;

    Server server(tcp_port, udp_port);
    server.start();

    return 0;
}
