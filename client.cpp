#include <iostream>
#include <cstring>
#include <arpa/inet.h>
#include <unistd.h>
#include <cmath>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctime>
#include <fcntl.h>
#include <errno.h>

#define SECONDS_PER_TICK (1.0f / 60.0f)
#define MAX_CLIENTS 12

const unsigned short PORT = 1100;
const unsigned int SOCKET_BUFFER_SIZE = 1024;

// Funckje pomocnicze do fixed tick rate
timespec timespec_diff(const timespec &start, const timespec &end);
float timespec_to_seconds(const timespec &ts);

struct IP_Endpoint
{
    unsigned int address;
    unsigned short port;
};

static IP_Endpoint ip_endpoint_create(unsigned char a, unsigned char b, unsigned char c, unsigned char d, unsigned short port)
{
    IP_Endpoint ip_endpoint = {};
    ip_endpoint.address = (a << 24) | (b << 16) | (c << 8) | d;
    ip_endpoint.port = port;
    return ip_endpoint;
}

struct Player_State
{
    float x, y, facing;
};

struct Input
{
    bool up, down, left, right;
};
static Input g_input;

enum class Client_Message : unsigned char
{
    Join,  // tell server we're new here
    Leave, // tell server we're leaving
    Input  // tell server our user input
};

enum class Server_Message : unsigned char
{
    Join_Result, // tell client they're accepted/rejected
    State        // tell client game state
};

int main()
{
    int client_socket;

    // Tworzenie gniazda
    client_socket = socket(AF_INET, SOCK_DGRAM, 0);
    if (client_socket < 0)
    {
        perror("Cannot create socket");
        exit(EXIT_FAILURE);
    }

    // Ustawienie socketa w tryb nieblokujacy
    int flags = fcntl(client_socket, F_GETFL, 0);
    fcntl(client_socket, F_SETFL, flags | O_NONBLOCK);

    char buffer[SOCKET_BUFFER_SIZE];

    IP_Endpoint server_endpoint = ip_endpoint_create(127, 0, 0, 1, PORT);

    buffer[0] = (char)Client_Message::Join;

    sockaddr_in server_address;
    server_address.sin_family = AF_INET;
    server_address.sin_addr.s_addr = htonl(server_endpoint.address); // !!! Moze nie dzialac
    server_address.sin_port = htons(server_endpoint.port);
    int server_address_size = sizeof(server_address);

    // Wysylanie join packet, prosba o nadanie ID
    if (sendto(client_socket, buffer, 1, 0, (sockaddr *)&server_address, server_address_size) == -1)
    {
        perror("Could not send join packet to server");
        close(client_socket);
        exit(EXIT_FAILURE);
    }

    Player_State objects[MAX_CLIENTS];
    unsigned int num_objects = 0;
    unsigned short slot = 65535;

    // Ustawienie czasu startowego
    timespec clock_frequency, tick_start_time, tick_end_time;
    clock_gettime(CLOCK_MONOTONIC, &clock_frequency);

    bool is_running = true;
    while (is_running)
    {

        IP_Endpoint from;

        int flags = 0;
        sockaddr_in from_address;
        socklen_t from_address_size = sizeof(from_address);
        int bytes_received = recvfrom(client_socket, buffer, SOCKET_BUFFER_SIZE, 0, (sockaddr *)&from_address, &from_address_size);
        if (bytes_received == -1)
        {
            int error = errno; // Pobierz kod błędu z errno
            if (error != EAGAIN && error != EWOULDBLOCK)
            {
                printf("recvfrom returned -1, errno %d\n", error);
            }
            break;
        }

        from.address = ntohl(from_address.sin_addr.s_addr);
        from.port = ntohs(from_address.sin_port);

        while (bytes_received != 1)
        {
            // W zaleznosci od otrzymanego pakietu
            switch (static_cast<Server_Message>(buffer[0]))
            {

            case Server_Message::Join_Result:
            {
                // buffer[1] -> sukces/porazka
                // buffer[2] -> ID
                if (buffer[1])
                {
                    memcpy(&slot, &buffer[2], sizeof(slot));
                }
                else
                {
                    perror("server didn't let us in\n");
                }
            }
            break;

            // Przyjecie od serwera aktualnego stanu gry
            case Server_Message::State:
            {
                num_objects = 0;
                unsigned int bytes_read = 1;
                while (bytes_read < bytes_received)
                {
                    // Przekopiowanie calego pakietu do zmiennych, pakiet zawiera informacje na temat wszystkich graczy
                    unsigned short id; // unused
                    memcpy(&id, &buffer[bytes_read], sizeof(id));
                    bytes_read += sizeof(id);

                    memcpy(&objects[num_objects].x,
                           &buffer[bytes_read],
                           sizeof(objects[num_objects].x));
                    bytes_read += sizeof(objects[num_objects].x);

                    memcpy(&objects[num_objects].y,
                           &buffer[bytes_read],
                           sizeof(objects[num_objects].y));
                    bytes_read += sizeof(objects[num_objects].y);

                    memcpy(&objects[num_objects].facing,
                           &buffer[bytes_read],
                           sizeof(objects[num_objects].facing));
                    bytes_read += sizeof(objects[num_objects].facing);

                    ++num_objects;
                }
            }
            break;
            }
        }

        // Wysylanie inputu do serwera
        if (slot != 65535)
        {
            buffer[0] = (unsigned char)Client_Message::Input;
            int bytes_written = 1;

            memcpy(&buffer[bytes_written], &slot, sizeof(slot));
            bytes_written += sizeof(slot);

            unsigned char input = (unsigned char)g_input.up |
                                  ((unsigned char)g_input.down << 1) |
                                  ((unsigned char)g_input.left << 2) |
                                  ((unsigned char)g_input.right << 3);
            buffer[bytes_written] = input;
            ++bytes_written;

            if (sendto(client_socket, buffer, bytes_written, 0, (sockaddr *)&server_address, server_address_size) == -1)
            {
                perror("Could not send state/input packet to server");
                close(client_socket);
                exit(EXIT_FAILURE);
            }
        }

        // Update and draw dla wszystkich obiektow (maksymalnie 12 graczy)
        for (unsigned int i = 0; i < num_objects; ++i)
        {
          
            float x = objects[i].x * 0.01f;
            float y = objects[i].y * -0.01f;

            //Player.update(x,y);
            //TO DO
        }

        // Obliczanie czasu wykonania ticku
        clock_gettime(CLOCK_MONOTONIC, &tick_end_time);
        timespec elapsed_time = timespec_diff(tick_start_time, tick_end_time);
        float time_taken_s = timespec_to_seconds(elapsed_time);

        while (time_taken_s < SECONDS_PER_TICK)
        {
            float time_left_s = SECONDS_PER_TICK - time_taken_s;
            timespec sleep_time = {0, (long)(time_left_s * 1000000000)};
            nanosleep(&sleep_time, nullptr);

            clock_gettime(CLOCK_MONOTONIC, &tick_end_time);
            elapsed_time = timespec_diff(tick_start_time, tick_end_time);
            time_taken_s = timespec_to_seconds(elapsed_time);
        }
    }

   
   //Wyslanie do serwera informacji o rozlaczeniu klienta
   buffer[0] = (unsigned char)Client_Message::Leave;
   int bytes_written_leave = 1;
   memcpy(&buffer[bytes_written_leave], &slot, sizeof(slot));

     if (sendto(client_socket, buffer, bytes_written_leave, 0, (sockaddr *)&server_address, server_address_size) == -1)
            {
                perror("Could not send leave packet to server");
                close(client_socket);
                exit(EXIT_FAILURE);
            }
   
   
    close(client_socket);
    return 0;
}





timespec timespec_diff(const timespec &start, const timespec &end)
{
    timespec temp;
    if ((end.tv_nsec - start.tv_nsec) < 0)
    {
        temp.tv_sec = end.tv_sec - start.tv_sec - 1;
        temp.tv_nsec = 1000000000 + end.tv_nsec - start.tv_nsec;
    }
    else
    {
        temp.tv_sec = end.tv_sec - start.tv_sec;
        temp.tv_nsec = end.tv_nsec - start.tv_nsec;
    }
    return temp;
}

float timespec_to_seconds(const timespec &ts)
{
    return ts.tv_sec + ts.tv_nsec / 1000000000.0f;
}