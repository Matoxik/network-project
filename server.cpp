// Server
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <cmath>
#include <ctime>
#include <fcntl.h>
#include <errno.h>
#include <pthread.h>
#include <chrono>

#define ACCELERATION 0.66f
#define X_VELOCITY 3.3f
#define JUMP_VELOCITY 16.0f
#define SECONDS_PER_TICK (1.0f / 120.0f)
#define MAX_CLIENTS 3
#define CLIENT_TIMEOUT 150.0f
#define MAX_GAMES 10

const unsigned short PORT = 5050;
const unsigned int SOCKET_BUFFER_SIZE = 1024 * 64;

// Structure representing a game instance
struct Game
{
    int server_socket;
    struct sockaddr_in clientAddress;
};

struct sockaddr_in localAddress;

// Structure representing a client's IP endpoint
struct IP_Endpoint
{
    unsigned int address;
    unsigned short port;

    // Equality operator to compare two IP endpoints
    bool operator==(const IP_Endpoint &other) const
    {
        return address == other.address && port == other.port;
    }
};
// Structure representing the state of a player in the game
struct Player_State
{
    float x, y, facing, y_velocity; // Player's position
    bool is_button_push;            // Indicates if a button is being pressed
};

// Structure representing the player's input (keyboard controls)
struct Player_Input
{
    bool up, down, left, right;
};

// Enum defining message types sent from the client to the server
enum class Client_Message : unsigned char
{
    Join,  // Tell server we're new here
    Leave, // Tell server we're leaving
    Input  // Tell server our user input
};

// Enum defining message types sent from the server to the client
enum class Server_Message : unsigned char
{
    Join_Result, // Tell client they're accepted/rejected
    State        // Tell client game state
};

pthread_t game_threads[MAX_GAMES]; // Array to hold thread identifiers for each active game
pthread_mutex_t game_lock = PTHREAD_MUTEX_INITIALIZER;
unsigned int game_count = 0;
bool is_empty_thread = false;

void *game_instance(void *arg)
{
    printf("New game created\n");

    pthread_mutex_lock(&game_lock);

    // Retrieve the game object from the thread argument
    Game *game = (Game *)arg;

    // Extract the client address and server socket from the game object
    struct sockaddr_in *clientAddressNewThread = &game->clientAddress;
    int server_socket = game->server_socket;

    pthread_mutex_unlock(&game_lock);

    // Initialize clock variables for time tracking
    timespec clock_frequency, tick_start_time, tick_end_time;
    clock_gettime(CLOCK_MONOTONIC, &clock_frequency);

    // Initialize arrays for storing client data
    IP_Endpoint client_endpoints[MAX_CLIENTS] = {};
    float time_since_heard_from_clients[MAX_CLIENTS] = {};
    Player_State client_objects[MAX_CLIENTS] = {};
    Player_Input client_inputs[MAX_CLIENTS] = {};

    // Endpoint for the current client address
    IP_Endpoint clientAddress_endpoint;

    // Initialize client endpoint data to default (0 for address and port)
    for (unsigned short i = 0; i < MAX_CLIENTS; ++i)
    {
        client_endpoints[i] = {};
        client_endpoints[i].address = 0;
        client_endpoints[i].port = 0;
    }

    // Flags and counters for game state
    bool is_running = true;
    bool is_portal_open = false;
    bool is_game_finished = false;
    bool is_my_input = true;
    bool is_new_game_created = false;
    unsigned int counter = 0; // Close game counter

    // Buffer for receiving data
    char buffer[SOCKET_BUFFER_SIZE];
    socklen_t clientAddress_size = sizeof(struct sockaddr_in);
    int bytes_received;

    // Main game loop
    while (is_running)
    {
        // Record the start time of the tick
        clock_gettime(CLOCK_MONOTONIC, &tick_start_time);
        is_my_input = true;

        while (true)
        {
            // Receive data from the client
            bytes_received = recvfrom(server_socket, buffer, SOCKET_BUFFER_SIZE, 0, (sockaddr *)clientAddressNewThread, &clientAddress_size);

            // Handle errors during data reception
            if (bytes_received == -1)
            {
                int error = errno; // Pobierz kod błędu z errno
                if (error != EAGAIN && error != EWOULDBLOCK)
                {
                    printf("Recvfrom returned -1 in 141, errno %d\n", error);
                }
                break;
            }
            // Extract the client's IP address and port
            clientAddress_endpoint.address = clientAddressNewThread->sin_addr.s_addr;
            clientAddress_endpoint.port = ntohs(clientAddressNewThread->sin_port);

            // Process the received message based on packet type
            switch (static_cast<Client_Message>(buffer[0]))
            {
            case Client_Message::Join: // Handle client joining
            {
                printf("Client_Message::Join from %u:%hu\n", clientAddress_endpoint.address, clientAddress_endpoint.port);

                counter = 0;
                unsigned short slot = 65535; // Default slot value indicating no free slot

                // Search for a free slot for the new client
                for (unsigned short i = 0; i < MAX_CLIENTS; ++i)
                {
                    if (client_endpoints[i].address == 0)
                    {
                        slot = i;
                        break;
                    }
                }

                buffer[0] = (char)Server_Message::Join_Result;
                // If a free slot is found, assign it to the client
                if (slot != 65535)
                {
                    printf("Client will be assigned to slot %hu\n", slot);

                    buffer[1] = 1;                // Join successful
                    memcpy(&buffer[2], &slot, 2); // Assign client ID

                    // Send join result to the client
                    if (sendto(server_socket, buffer, 4, 0, (sockaddr *)clientAddressNewThread, clientAddress_size) != -1)
                    {
                        client_endpoints[slot] = clientAddress_endpoint;
                        time_since_heard_from_clients[slot] = 0.0f;
                        client_objects[slot] = {};
                        client_inputs[slot] = {};
                    }
                    else
                    {
                        perror("Sendto Server_Message::Join failed in 187\n");
                        exit(EXIT_FAILURE);
                    }
                }

                // If no free slot is found, create a new game instance
                else if (!is_new_game_created || is_empty_thread)
                {
                    printf("Could not find a slot for player, creating new game...\n");
                    buffer[0] = (char)Server_Message::Join_Result;
                    buffer[1] = 0; // Join failed

                    // Send failure message to the client
                    if (sendto(server_socket, buffer, 2, 0, (sockaddr *)clientAddressNewThread, clientAddress_size) == -1)
                    {
                        perror("Sendto 2 Client_Message::Join failed in 209\n");
                        exit(EXIT_FAILURE);
                    }

                    pthread_mutex_lock(&game_lock);

                    if (game_count == 9) // Ensure maximum number of games is not exceeded
                    {
                        break;
                    }
                    is_empty_thread = false;
                    game_count++;
                    Game *game_copy = (Game *)malloc(sizeof(Game));
                    memcpy(game_copy, game, sizeof(Game));
                    // Creating new thread for new game
                    pthread_create(&game_threads[game_count], NULL, game_instance, game_copy);
                    pthread_mutex_unlock(&game_lock);

                    is_new_game_created = true;
                    clientAddress_endpoint.address = 0;
                    clientAddress_endpoint.port = 0;

                    break;
                }
            }

            break;

            case Client_Message::Leave: // Handle client leaving
            {
                unsigned short slot_leave = MAX_CLIENTS; // Default value if client is not found

                // Find the slot assigned to this client based on its address
                for (unsigned short i = 0; i < MAX_CLIENTS; ++i)
                {
                    if (client_endpoints[i] == clientAddress_endpoint)
                    {
                        slot_leave = i;
                        break;
                    }
                }
                if (slot_leave < MAX_CLIENTS)
                {
                    // If the slot is found, free it
                    client_endpoints[slot_leave] = {};
                    printf("Client_Message::Leave from slot %hu (%u:%hu)\n", slot_leave, clientAddress_endpoint.address, clientAddress_endpoint.port);
                }
                else
                {
                    // If the slot is not found, report an error
                    printf("Client_Message::Leave received from unknown client (%u:%hu)\n", clientAddress_endpoint.address, clientAddress_endpoint.port);
                }
            }
            break;

            case Client_Message::Input: // Handle client input
            {
                unsigned short slot;
                memcpy(&slot, &buffer[1], 2); // Extract the slot number from the message

                // Validate the input based on the client's address and port
                if (client_endpoints[slot] == clientAddress_endpoint)
                {
                    char input = buffer[3];

                    client_inputs[slot].up = input & 0x1;
                    client_inputs[slot].down = input & 0x2;
                    client_inputs[slot].left = input & 0x4;
                    client_inputs[slot].right = input & 0x8;

                    time_since_heard_from_clients[slot] = 0.0f; // Reset the timeout timer
                }
                else
                {
                    is_my_input = false; // Input does not belong to this client
                }
            }
            break;
            }
        }

        if (is_my_input) // Process input only if it's valid
        {

            for (unsigned short i = 0; i < MAX_CLIENTS; ++i)
            {
                if (client_endpoints[i].address) // Check if the client slot is in use
                {
                    // Handle jumping
                    if (client_inputs[i].up && client_objects[i].y_velocity == 0 &&
                        (client_objects[i].y == 30.0f || client_objects[i].y == -277.0f || client_objects[i].y == -570.0f || client_objects[i].y == -720.0f))
                    {
                        client_objects[i].y_velocity = -JUMP_VELOCITY; // Assign upward velocity for the jump
                    }

                    // Apply gravity
                    if (client_objects[i].y_velocity < 11.0f) // Cap the maximum falling velocity
                    {
                        client_objects[i].y_velocity += ACCELERATION * 0.5f; // Add gravitational acceleration
                    }

                    // Update the vertical position of the player
                    client_objects[i].y += client_objects[i].y_velocity;

                    // Collision with the bottom floor
                    if (client_objects[i].y >= 30.0f) // Ground position
                    {
                        client_objects[i].y = 30.0f;
                        client_objects[i].y_velocity = 0.0f; // Reset velocity on collision
                    }

                    // Collision with the middle lowest floor
                    if ((client_objects[i].y >= -277.0f && client_objects[i].y < -250.0f) &&
                        (client_objects[i].x <= 858 || client_objects[i].x >= 1200))
                    {
                        client_objects[i].y = -277.0f;       // Align with the platform
                        client_objects[i].y_velocity = 0.0f; // Reset vertical velocity
                    }

                    // Hitting the middle lowest floor from below
                    if ((client_objects[i].y >= -230.0f && client_objects[i].y <= -70.0f) &&
                        (client_objects[i].x <= 858 || client_objects[i].x >= 1200))
                    {
                        client_objects[i].y_velocity = 0.0f; // Stop upward movement
                    }

                    // Collision with the middle medium floor
                    if ((client_objects[i].y >= -570.0f && client_objects[i].y < -540.0f) &&
                        (client_objects[i].x <= 400))
                    {
                        client_objects[i].y = -570.0f;       // Align with the platform
                        client_objects[i].y_velocity = 0.0f; // Reset vertical velocity
                    }

                    // Hitting the middle medium floor from below
                    if ((client_objects[i].y >= -390.0f && client_objects[i].y < -350.0f) &&
                        (client_objects[i].x <= 400))
                    {
                        client_objects[i].y_velocity = 0.0f; // Stop upward movement
                    }

                    // Collision with the highest floor
                    if ((client_objects[i].y >= -720.0f && client_objects[i].y < -690.0f) &&
                        (client_objects[i].x >= 630))
                    {
                        client_objects[i].y = -720.0f;       // Align with the platform
                        client_objects[i].y_velocity = 0.0f; // Reset vertical velocity
                    }

                    // Hitting the highest floor from below
                    else if ((client_objects[i].y >= -520.0f && client_objects[i].y < -500.0f) &&
                             (client_objects[i].x >= 630))
                    {
                        client_objects[i].y_velocity = 0.0f; // Stop upward movement
                    }

                    // Handle movement to the left
                    if ((client_inputs[i].left) && (client_objects[i].x > -50))
                    {
                        client_objects[i].x += -X_VELOCITY; // Move left
                        client_objects[i].facing = true;    // Update facing direction
                    }

                    // Handle movement to the right
                    if ((client_inputs[i].right) && (client_objects[i].x < 1680))
                    {
                        client_objects[i].x += X_VELOCITY; // Move right
                        client_objects[i].facing = false;  // Update facing direction
                    }

                    // Handle the blue button press
                    if (client_objects[0].x <= 95 && client_objects[0].x >= -80 &&
                        client_objects[0].y > -287 && client_objects[0].y < -267)
                    {
                        client_objects[0].is_button_push = true; // Blue button is pressed
                    }
                    else
                    {
                        client_objects[0].is_button_push = false; // Blue button is not pressed
                    }

                    // Handle the red button press
                    if (client_objects[1].x <= 1580 && client_objects[1].x >= 1420 &&
                        client_objects[1].y > -287 && client_objects[1].y < -267)
                    {
                        client_objects[1].is_button_push = true; // Red button is pressed
                    }
                    else
                    {
                        client_objects[1].is_button_push = false; // Red button is not pressed
                    }

                    // Handle the yellow button press
                    if (client_objects[2].x <= 95 && client_objects[2].x >= -80 &&
                        client_objects[2].y > -580 && client_objects[2].y < -560)
                    {
                        client_objects[2].is_button_push = true; // Yellow button is pressed
                    }
                    else
                    {
                        client_objects[2].is_button_push = false; // Yellow button is not pressed
                    }

                    // Open the portal if all buttons are pressed
                    if (client_objects[0].is_button_push && client_objects[1].is_button_push && client_objects[2].is_button_push)
                    {
                        is_portal_open = true; // Open the portal
                    }

                    // End the game if all clients reach the portal
                    if (is_portal_open &&
                        client_objects[0].y > -730 && client_objects[0].y < -710 && client_objects[0].x >= 1450 &&
                        client_objects[1].y > -730 && client_objects[1].y < -710 && client_objects[1].x >= 1450 &&
                        client_objects[2].y > -730 && client_objects[2].y < -710 && client_objects[2].x >= 1450)
                    {
                        is_game_finished = true; // End the game
                    }

                    // Check for client timeout
                    time_since_heard_from_clients[i] += SECONDS_PER_TICK;
                    if (time_since_heard_from_clients[i] > CLIENT_TIMEOUT)
                    {
                        printf("Client timeout\n");
                        client_endpoints[i] = {}; // Free the client slot
                    }
                }
            }

            // Build a state packet to send back to clients
            buffer[0] = (unsigned char)Server_Message::State;
            int bytes_written = 1;

            for (unsigned short i = 0; i < MAX_CLIENTS; ++i)
            {
                // If the client exists
                if (client_endpoints[i].address)
                {
                    memcpy(&buffer[bytes_written], &i, sizeof(i));
                    bytes_written += sizeof(i);
                    memcpy(&buffer[bytes_written], &client_objects[i].x, sizeof(client_objects[i].x));
                    bytes_written += sizeof(client_objects[i].x);
                    memcpy(&buffer[bytes_written], &client_objects[i].y, sizeof(client_objects[i].y));
                    bytes_written += sizeof(client_objects[i].y);
                    memcpy(&buffer[bytes_written], &client_objects[i].facing, sizeof(client_objects[i].facing));
                    bytes_written += sizeof(client_objects[i].facing);
                    memcpy(&buffer[bytes_written], &client_objects[i].is_button_push, sizeof(client_objects[i].is_button_push));
                    bytes_written += sizeof(client_objects[i].is_button_push);
                    memcpy(&buffer[bytes_written], &is_portal_open, sizeof(is_portal_open));
                    bytes_written += sizeof(is_portal_open);
                    memcpy(&buffer[bytes_written], &is_game_finished, sizeof(is_game_finished));
                    bytes_written += sizeof(is_game_finished);
                }
                else
                {
                    unsigned short empty_slot = i | 0x8000; // Mark as "to be removed"
                    memcpy(&buffer[bytes_written], &empty_slot, sizeof(empty_slot));
                    bytes_written += sizeof(empty_slot);
                }
            }

            // Send the state packet back to clients
            int flags = 0;
            struct sockaddr_in to;
            to.sin_family = AF_INET;
            to.sin_port = htons(PORT);
            int to_length = sizeof(to);

            for (unsigned short i = 0; i < MAX_CLIENTS; ++i)
            {
                if (client_endpoints[i].address)
                {
                    to.sin_addr.s_addr = client_endpoints[i].address;
                    to.sin_port = htons(client_endpoints[i].port);

                    if (sendto(server_socket, buffer, bytes_written, flags, (sockaddr *)&to, to_length) == -1)
                    {
                        perror("Sendto game state packet back to client failed in 485\n");
                        exit(EXIT_FAILURE);
                    }
                }
            }
        }

        // If there are no players in game for 100s or game is finished, leave thread
        if (client_endpoints[0].address == 0 && client_endpoints[1].address == 0 && client_endpoints[2].address == 0)
        {
            counter++;
            sleep(1);
            if ((counter >= 100) || is_game_finished)
            {

                break;
            }
        }

        // Measure elapsed time for the tick
        clock_gettime(CLOCK_MONOTONIC, &tick_end_time);

        timespec temp;
        if ((tick_end_time.tv_nsec - tick_start_time.tv_nsec) < 0)
        {
            temp.tv_sec = tick_end_time.tv_sec - tick_start_time.tv_sec - 1;
            temp.tv_nsec = 1000000000 + tick_end_time.tv_nsec - tick_start_time.tv_nsec;
        }
        else
        {
            temp.tv_sec = tick_end_time.tv_sec - tick_start_time.tv_sec;
            temp.tv_nsec = tick_end_time.tv_nsec - tick_start_time.tv_nsec;
        }
        timespec elapsed_time = temp;
        float time_taken_s = elapsed_time.tv_sec + elapsed_time.tv_nsec / 1000000000.0f;

        // Wait if the tick ended too quickly
        while (time_taken_s < SECONDS_PER_TICK)
        {
            float time_left_s = SECONDS_PER_TICK - time_taken_s;
            timespec sleep_time = {0, (long)(time_left_s * 1000000000)};
            nanosleep(&sleep_time, nullptr);

            clock_gettime(CLOCK_MONOTONIC, &tick_end_time);
            timespec temp;
            if ((tick_end_time.tv_nsec - tick_start_time.tv_nsec) < 0)
            {
                temp.tv_sec = tick_end_time.tv_sec - tick_start_time.tv_sec - 1;
                temp.tv_nsec = 1000000000 + tick_end_time.tv_nsec - tick_start_time.tv_nsec;
            }
            else
            {
                temp.tv_sec = tick_end_time.tv_sec - tick_start_time.tv_sec;
                temp.tv_nsec = tick_end_time.tv_nsec - tick_start_time.tv_nsec;
            }
            timespec elapsed_time = temp;
            time_taken_s = elapsed_time.tv_sec + elapsed_time.tv_nsec / 1000000000.0f;
        }
    }

    pthread_mutex_lock(&game_lock);
    is_empty_thread = true;
    pthread_mutex_unlock(&game_lock);
    printf("Leaving thread\n");
    return NULL;
}

int main(int argc, char **argv)
{

    // Configure local address
    localAddress.sin_family = AF_INET;
    localAddress.sin_port = htons(PORT);
    localAddress.sin_addr.s_addr = INADDR_ANY;

    // Initialize the game structure
    Game game;

    // Create the socket
    game.server_socket = socket(AF_INET, SOCK_DGRAM, 0);
    if (game.server_socket < 0)
    {
        perror("Could not create socket in 566");
        exit(EXIT_FAILURE);
    }

    // Bind the socket to the local address
    if (bind(game.server_socket, (struct sockaddr *)&localAddress, sizeof(localAddress)) == -1)
    {
        perror("Could not bind");
        close(game.server_socket);
        exit(EXIT_FAILURE);
    }

    // Set the socket to non-blocking mode
    int flags = fcntl(game.server_socket, F_GETFL, 0);
    fcntl(game.server_socket, F_SETFL, flags | O_NONBLOCK);

    // Configure socket buffer sizes
    char buffer_socket[SOCKET_BUFFER_SIZE];
    setsockopt(game.server_socket, SOL_SOCKET, SO_RCVBUF, &buffer_socket, sizeof(buffer_socket));
    setsockopt(game.server_socket, SOL_SOCKET, SO_SNDBUF, &buffer_socket, sizeof(buffer_socket));
    // Enable socket address reuse
    int opt = 1;
    setsockopt(game.server_socket, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));

    // Create a new thread to handle the game instance
    pthread_create(&game_threads[game_count], NULL, game_instance, &game);

    // Wait for the created thread to complete
    pthread_join(game_threads[0], NULL);
    // Close the socket after the game instance ends
    close(game.server_socket);

    return 0;
}
