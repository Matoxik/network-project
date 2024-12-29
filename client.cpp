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
#include <SFML/Window.hpp>
#include <SFML/Graphics.hpp>
#include "Player.h"
#include "Background.h"

#define SECONDS_PER_TICK (1.0f / 60.0f)
#define MAX_CLIENTS 12

const unsigned short PORT = 1100;
const unsigned int SOCKET_BUFFER_SIZE = 65536;

timespec timespec_diff(const timespec &start, const timespec &end);
float timespec_to_seconds(const timespec &ts);

std::map<std::string, sf::Texture> textureMap; // Globalna mapa tekstur

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
    client_socket = socket(AF_INET, SOCK_DGRAM, 0);
    if (client_socket < 0)
    {
        perror("Cannot create socket");
        exit(EXIT_FAILURE);
    }

    int flags = fcntl(client_socket, F_GETFL, 0);
    fcntl(client_socket, F_SETFL, flags | O_NONBLOCK);

    char buffer[SOCKET_BUFFER_SIZE];
    setsockopt(client_socket, SOL_SOCKET, SO_RCVBUF, &buffer, sizeof(buffer));
    setsockopt(client_socket, SOL_SOCKET, SO_SNDBUF, &buffer, sizeof(buffer));
    int opt = 1;
    setsockopt(client_socket, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));

    IP_Endpoint server_endpoint = ip_endpoint_create(127, 0, 0, 1, PORT);

    buffer[0] = (char)Client_Message::Join;
    sockaddr_in server_address;
    server_address.sin_family = AF_INET;
    server_address.sin_addr.s_addr = htonl(server_endpoint.address);
    server_address.sin_port = htons(server_endpoint.port);
    int server_address_size = sizeof(server_address);

    if (sendto(client_socket, buffer, 1, 0, (sockaddr *)&server_address, server_address_size) == -1)
    {
        perror("Could not send join packet to server");
        close(client_socket);
        exit(EXIT_FAILURE);
    }

    unsigned short slot = 65535;

    timespec clock_frequency, tick_start_time, tick_end_time;
    clock_gettime(CLOCK_MONOTONIC, &clock_frequency);

    sf::RenderWindow window(sf::VideoMode(1920, 1080), "Network Project <3");
    StartBackground startBackground;
    window.setFramerateLimit(60);

    std::vector<Player> players; // Lista graczy
    bool is_running = true;
   
    while (is_running)
    {
        window.clear();
        clock_gettime(CLOCK_MONOTONIC, &tick_start_time);
        IP_Endpoint from;
        sockaddr_in from_address;
        socklen_t from_address_size = sizeof(from_address);
        int bytes_received = recvfrom(client_socket, buffer, SOCKET_BUFFER_SIZE, 0, (sockaddr *)&from_address, &from_address_size);
        if (bytes_received == -1)
        {
            int error = errno;
            if (error != EAGAIN && error != EWOULDBLOCK)
            {
                printf("recvfrom returned -1, errno %d\n", error);
            }
        }
        else
        {
            from = {};
            from.address = ntohl(from_address.sin_addr.s_addr);
            from.port = ntohs(from_address.sin_port);
            if (bytes_received > 0)
            {
                switch (static_cast<Server_Message>(buffer[0]))
                {
                case Server_Message::Join_Result:
                {
                    if (buffer[1]) // Jeśli serwer zaakceptował
                    {
                        memcpy(&slot, &buffer[2], sizeof(slot));
       
                    }
                    else
                    {
                        perror("Server didn't let us in\n");
                    }
                }
                break;

                case Server_Message::State:
                {
                    
                    
                        int bytes_read = 1;
                        unsigned short id;
                        while (bytes_read < bytes_received)
                        {
                            memcpy(&id, &buffer[bytes_read], sizeof(id));
                            bytes_read += sizeof(id);

                            // Sprawdzenie, czy gracz istnieje w wektorze
                            if (id >= players.size())
                            {
                                // Mapowanie tekstur na podstawie ID
                                std::string textureFile = "textures/ludzik.png";

                                // Dodanie nowego gracza, przekazując mapę tekstur
                                players.emplace_back(textureFile, textureMap);
                                std::cout << "Dodano gracza ID: " << id << " z teksturą: " << textureFile << "\n";
                            }

                            // Aktualizacja pozycji istniejącego gracza
                            memcpy(&players[id].x, &buffer[bytes_read], sizeof(players[id].x));
                            bytes_read += sizeof(players[id].x);
                            memcpy(&players[id].y, &buffer[bytes_read], sizeof(players[id].y));
                            bytes_read += sizeof(players[id].y);
                            memcpy(&players[id].facing, &buffer[bytes_read], sizeof(players[id].facing));
                            bytes_read += sizeof(players[id].facing);

                            // Aktualizacja pozycji i tekstury
                            players[id].assignTexture(id);
                            players[id].update();
                       
                           cout<<"X: "<<players[id].x<<"\n";
                      
                        }
    
                    
                }
                break;
                }
            }
        }

        sf::Event event;
        window.pollEvent(event);

        if (event.type == sf::Event::Closed)
        {
            is_running = false;
            break;
        }

        if (event.type == sf::Event::KeyPressed || event.type == sf::Event::KeyReleased)
        {
            bool is_pressed = (event.type == sf::Event::KeyPressed);
            switch (event.key.code)
            {
            case sf::Keyboard::Up:
                g_input.up = is_pressed;
                break;
            case sf::Keyboard::Down:
                g_input.down = is_pressed;
                break;
            case sf::Keyboard::Left:
                g_input.left = is_pressed;
                break;
            case sf::Keyboard::Right:
                g_input.right = is_pressed;
                break;
            default:
                break;
            }
        }

        if (slot != 65535)
        {
            buffer[0] = (unsigned char)Client_Message::Input;
            int bytes_written = 1;
            memcpy(&buffer[bytes_written], &slot, sizeof(slot));
            bytes_written += sizeof(slot);
            unsigned char input = (unsigned char)g_input.up | ((unsigned char)g_input.down << 1) |
                                  ((unsigned char)g_input.left << 2) | ((unsigned char)g_input.right << 3);
            buffer[bytes_written] = input;
            ++bytes_written;
            if (sendto(client_socket, buffer, bytes_written, 0, (sockaddr *)&server_address, server_address_size) == -1)
            {
                perror("Could not send state/input packet to server");
                close(client_socket);
                exit(EXIT_FAILURE);
            }
        }

        startBackground.draw(window);
        // Rysowanie wszystkich graczy
        for (unsigned short i = 0; i < players.size(); ++i)
        {
            players[i].draw(window);
        }

        window.display();

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

    buffer[0] = (unsigned char)Client_Message::Leave;
    int bytes_written_leave = 1;
    memcpy(&buffer[bytes_written_leave], &slot, sizeof(slot));
    printf("Wyslano leave slot: %d\n", slot);
    printf("Wyslano leave buffer: %d\n", buffer[1]);

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