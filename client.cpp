// Client
#include <iostream>
#include <cstring>
#include <arpa/inet.h>
#include <unistd.h>
#include <cmath>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <fcntl.h>
#include <errno.h>
#include <ctime>
#include <fstream>
#include <SFML/Window.hpp>
#include <SFML/Graphics.hpp>
#include "Player.h"
#include "Background.h"
#include "Ground.h"
#include "Button.h"
#include "Portal.h"
#include "Exception.h"

#define SECONDS_PER_TICK (1.0f / 120.0f)
#define MAX_CLIENTS 3
const unsigned short PORT = 5050;
const unsigned int SOCKET_BUFFER_SIZE = 1024 * 64;

// Function to calculate the difference between two timespec values
timespec timespec_diff(const timespec &start, const timespec &end);
// Function to convert timespec values to seconds as a floating-point number
float timespec_to_seconds(const timespec &ts);

// Global texture map
std::map<std::string, sf::Texture> textureMap;

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

    // Open an output file for logging errors and truncate it if it already exists
    std::ofstream errors("Errors.txt", ios::trunc);

    int client_socket;

    try
    {
        // Create a UDP socket
        client_socket = socket(AF_INET, SOCK_DGRAM, 0);
        if (client_socket < 0)
        {
            // Throw an exception if socket creation fails
            Exception *exception = new Exception(2, "Cannot create socket in 81\n");
            throw exception;
        }

        // Set the socket to non-blocking mode
        int flags = fcntl(client_socket, F_GETFL, 0);
        fcntl(client_socket, F_SETFL, flags | O_NONBLOCK);

        // Create buffers for socket communication
        char buffer[SOCKET_BUFFER_SIZE];
        setsockopt(client_socket, SOL_SOCKET, SO_RCVBUF, &buffer, sizeof(buffer)); // Set receive buffer size
        setsockopt(client_socket, SOL_SOCKET, SO_SNDBUF, &buffer, sizeof(buffer)); // Set send buffer size

        // Enable address reuse for the socket
        int opt = 1;
        setsockopt(client_socket, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));

        // Create a server endpoint
        IP_Endpoint server_endpoint = ip_endpoint_create(127, 0, 0, 1, PORT);

        // Configure the server address structure
        sockaddr_in server_address;
        server_address.sin_family = AF_INET;
        server_address.sin_addr.s_addr = htonl(server_endpoint.address);
        server_address.sin_port = htons(server_endpoint.port);
        int server_address_size = sizeof(server_address);

        unsigned short slot = 65535; // Variable to store the assigned slot
        bool is_id_set = false;      // Flag to indicate if an ID is set

        // Handle receiving messages from the server
        sockaddr_in from_address;
        socklen_t from_address_size = sizeof(from_address);

        // Loop to send a request for assigning an ID
        while (!is_id_set)
        {
            // Prepare a buffer to send a join request
            buffer[0] = (char)Client_Message::Join;
            if (sendto(client_socket, buffer, 1, 0, (sockaddr *)&server_address, server_address_size) == -1)
            {
                // Throw an exception if sending the join request fails
                Exception *exception = new Exception(1, "Client_Message::Join send error in 124\n");
                throw exception;
            }

            printf("Sent request to assign ID\n");

            int bytes_received;
            timespec start_time, current_time;

            // Record the start time for timeout handling
            clock_gettime(CLOCK_MONOTONIC, &start_time);

            bool response_received = false;

            while (!response_received)
            {
                // Attempt to receive a message from the server
                bytes_received = recvfrom(client_socket, buffer, SOCKET_BUFFER_SIZE, 0, (sockaddr *)&from_address, &from_address_size);

                if (bytes_received > 0)
                {
                    // Check if the server's response is a join result
                    if (static_cast<Server_Message>(buffer[0]) == Server_Message::Join_Result)
                    {
                        if (buffer[1]) // Server accepted the connection
                        {
                            // Extract and assign the slot from the server response
                            memcpy(&slot, &buffer[2], sizeof(slot));
                            is_id_set = true;
                            response_received = true;
                            printf("Assigned slot: %d\n", slot);
                        }
                        else
                        {
                            printf("Server rejected the connection, retrying...\n");
                        }
                    }
                }
                else if (bytes_received == -1)
                {
                    int error = errno;
                    // Handle non-blocking socket errors
                    if (error != EAGAIN && error != EWOULDBLOCK)
                    {
                        Exception *exception = new Exception(1, "Recvfrom error in 144\n");
                        throw exception;
                    }
                }

                // Check if the timeout (3 seconds) has been reached
                clock_gettime(CLOCK_MONOTONIC, &current_time);
                timespec elapsed_time = timespec_diff(start_time, current_time);
                float elapsed_seconds = timespec_to_seconds(elapsed_time);

                if (elapsed_seconds > 3.0f)
                {
                    printf("Timeout waiting for response, retrying...\n");
                    break;
                }

                // Delay before the next attempt to receive a response
                usleep(600000); // 600 ms
            }
        }

        // Declare variables for time measurement
        timespec clock_frequency, tick_start_time, tick_end_time;

        // Initialize the game clock frequency
        clock_gettime(CLOCK_MONOTONIC, &clock_frequency);

       
        sf::RenderWindow window(sf::VideoMode(1920, 1080), "Network Project");
        window.setFramerateLimit(60); 

        // Initialize game objects
        StartBackground startBackground; 
        Ground ground[5];               
        Button buttons[3];              
        Portal portal;                  
        std::vector<Player> players;   

        // Flags for game state
        bool is_running = true;                    
        bool is_button_push[MAX_CLIENTS] = {false}; 
        bool is_portal_open = false;                
        bool is_game_finished = false;             

        // Main game loop
        while (is_running)
        {
            window.clear(); // Clear the game window for the next frame

            // Record the start time of the current tick
            clock_gettime(CLOCK_MONOTONIC, &tick_start_time);

            // Receive data from the server
            int bytes_received = recvfrom(client_socket, buffer, SOCKET_BUFFER_SIZE, 0, (sockaddr *)&from_address, &from_address_size);

            if (bytes_received == -1)
            {
                int error = errno;
                // Handle socket errors that are not related to non-blocking mode
                if (error != EAGAIN && error != EWOULDBLOCK)
                {
                    Exception *exception = new Exception(1, "Recvfrom error in 194\n");
                    throw exception;
                }
            }
            else
            {
                // Handle incoming messages if data is received
                if (bytes_received > 0)
                {
                    switch (static_cast<Server_Message>(buffer[0]))
                    {
                    case Server_Message::Join_Result:
                        // Handle ID assignment (not used in this loop)
                        break;
                    case Server_Message::State:
                    {
                        // Process state updates from the server
                        int bytes_read = 1; // Start reading 
                        unsigned short id;  // Temporary variable to store player IDs

                        while (bytes_read < bytes_received)
                        {
                            // Read player ID from the buffer
                            memcpy(&id, &buffer[bytes_read], sizeof(id));
                            bytes_read += sizeof(id);

                            if (id & 0x8000) // Check if the player should be removed
                            {
                                id &= 0x7FFF; // Clear the removal flag
                                if (id < players.size())
                                {
                                    players.erase(players.begin() + id); // Remove player
                                    printf("Removed player ID: %d\n", id);
                                }
                                continue;
                            }

                            if (id >= players.size()) // Add new player if ID exceeds current size
                            {
                                players.emplace_back("textures/ludzik.png", textureMap);
                                printf("Added player ID: %d\n", id);
                            }

                            // Update player position, facing direction, and button press state
                            memcpy(&players[id].x, &buffer[bytes_read], sizeof(players[id].x));
                            bytes_read += sizeof(players[id].x);
                            memcpy(&players[id].y, &buffer[bytes_read], sizeof(players[id].y));
                            bytes_read += sizeof(players[id].y);
                            memcpy(&players[id].facing, &buffer[bytes_read], sizeof(players[id].facing));
                            bytes_read += sizeof(players[id].facing);
                            memcpy(&is_button_push[id], &buffer[bytes_read], sizeof(is_button_push[id]));
                            bytes_read += sizeof(is_button_push[id]);

                            // Update portal state and game completion state
                            memcpy(&is_portal_open, &buffer[bytes_read], sizeof(is_portal_open));
                            bytes_read += sizeof(is_portal_open);
                            memcpy(&is_game_finished, &buffer[bytes_read], sizeof(is_game_finished));
                            bytes_read += sizeof(is_game_finished);

                            // Assign textures and update player state
                            players[id].assignTexture(id);
                            players[id].update();

                            // Update button state if pressed
                            if (is_button_push[id])
                            {
                                buttons[id].setPushedTexture(id);
                            }
                            std::cout<<"X: "<<players[id].x<<"\n";
                        }
                    }
                    break;
                    }
                }
            }

            // Handle window events
            sf::Event event;
            window.pollEvent(event);
            if (event.type == sf::Event::Closed || is_game_finished)
            {
                is_running = false; // Stop the game loop if the window is closed or the game ends
                break;
            }

            // Handle keyboard input events
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

                // Send input data to the server
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
                        Exception *exception = new Exception(1, "Client_Message::Input send error in 350\n");
                        throw exception;
                    }
                }
            }

            // Update ground and button positions
            ground[1].setPosition(2400, 730);
            ground[2].setPosition(0, 730);
            ground[3].setPosition(1835, 290);
            ground[4].setPosition(-500, 440);
            buttons[1].setPosition(1655, 685);
            buttons[2].setPosition(180, 396);

            // Draw game objects
            startBackground.draw(window); 
            for (unsigned short i = 0; i < 5; ++i)
            {
                ground[i].draw(window);
            }
            for (unsigned short i = 0; i < players.size(); ++i)
            {
                players[i].draw(window); 
            }
            for (unsigned short i = 0; i < 3; ++i)
            {
                if (!is_button_push[i])
                {
                    buttons[i].setNormalTexture(i); // Reset button texture if not pressed
                }
                buttons[i].draw(window); 
            }

            if (is_portal_open)
            {
                portal.changeTexture(); // Change portal texture if it's open
            }
            portal.draw(window); 

            window.display(); // Display the rendered frame

            // Measure elapsed time for the tick
            clock_gettime(CLOCK_MONOTONIC, &tick_end_time);
            timespec elapsed_time = timespec_diff(tick_start_time, tick_end_time);
            float time_taken_s = timespec_to_seconds(elapsed_time);

            // Wait if the tick ended too quickly
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

        // Send a leave message to the server when exiting
        buffer[0] = (unsigned char)Client_Message::Leave;
        int bytes_written_leave = 1;
        memcpy(&buffer[bytes_written_leave], &slot, sizeof(slot));
        printf("Sent leave slot: %d\n", slot);
        if (sendto(client_socket, buffer, bytes_written_leave, 0, (sockaddr *)&server_address, server_address_size) == -1)
        {
            Exception *exception = new Exception(1, "Client_Message::Leave send error in 415\n");
            throw exception;
        }

        close(client_socket);
    }
    catch (Exception *w)
    {
        // Log errors to the "Errors.txt" file
        errors << "Nr: " << w->getNumber() << " Description: " << w->getDescription() << '\n';
        close(client_socket);
    }

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
    return temp; // Return the time difference
}

float timespec_to_seconds(const timespec &ts)
{
    return ts.tv_sec + ts.tv_nsec / 1000000000.0f;
}