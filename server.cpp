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

#define ACCELERATION 0.1f
#define MAX_SPEED 2.0f
#define TURN_SPEED 0.1f
#define SECONDS_PER_TICK (1.0f / 60.0f)
#define MAX_CLIENTS 12
#define CLIENT_TIMEOUT 5.0f

const unsigned short PORT = 1100;
const unsigned int SOCKET_BUFFER_SIZE = 1024;


// Funckje pomocnicze do fixed tick rate
timespec timespec_diff(const timespec &start, const timespec &end);
float timespec_to_seconds(const timespec &ts);

// Struktura przechowujaca informacje o kliencie
struct IP_Endpoint
{
    unsigned int address;
    unsigned short port;
};
bool operator==(const IP_Endpoint &a, const IP_Endpoint &b) { return a.address == b.address && a.port == b.port; }

struct Player_State
{
    float x, y, facing, speed;
};

struct Player_Input
{
    bool up, down, left, right; //!!! Szansa ze powinien byc bool zamiast int
};

// Enumy przechowujace rozne pakiety
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

int main(int argc, char **argv)
{

    struct sockaddr_in localAddress;

    // Konfiguracja adresu lokalnego
    localAddress.sin_family = AF_INET;
    localAddress.sin_port = htons(PORT); 
    localAddress.sin_addr.s_addr = INADDR_ANY;

    // Tworzenie gniazda
    int server_socket = socket(AF_INET, SOCK_DGRAM, 0);
    if (server_socket < 0)
    {
        perror("Could not create socket");
        exit(EXIT_FAILURE);
    }

    

    // Przypisanie adresu do gniazda
    if (bind(server_socket, (struct sockaddr *)&localAddress, sizeof(localAddress)) == -1)
    {
        perror("Could not bind");
        close(server_socket);
        exit(EXIT_FAILURE);
    }
     
    // Ustawienie socketa w tryb nieblokujacy
    int flags = fcntl(server_socket, F_GETFL, 0);
    fcntl(server_socket, F_SETFL, flags | O_NONBLOCK);
    // Bufor do odbioru danych
   
    char buffer[SOCKET_BUFFER_SIZE];


    // Ustawienie czasu startowego
    timespec clock_frequency, tick_start_time, tick_end_time;
    clock_gettime(CLOCK_MONOTONIC, &clock_frequency);

    IP_Endpoint client_endpoints[MAX_CLIENTS];
    float time_since_heard_from_clients[MAX_CLIENTS];
    Player_State client_objects[MAX_CLIENTS];
    Player_Input client_inputs[MAX_CLIENTS];


   

    for (unsigned short i = 0; i < MAX_CLIENTS; ++i)
    {
        client_endpoints[i] = {};
    }

    bool is_running = true;

    while (is_running)
    {
        clock_gettime(CLOCK_MONOTONIC, &tick_start_time);

        while (true)
        {

            struct sockaddr_in clientAddress;
            // Odbior danych od klienta
            socklen_t clientAddress_size = sizeof(clientAddress);
            int bytes_received = recvfrom(server_socket, buffer, SOCKET_BUFFER_SIZE, 0, (sockaddr *)&clientAddress, &clientAddress_size);
            if (bytes_received == -1)
            {
                int error = errno; // Pobierz kod błędu z errno
                if (error != EAGAIN && error != EWOULDBLOCK)
                {
                    printf("recvfrom returned -1, errno %d\n", error);
                }
                break;
            }

            IP_Endpoint clientAddress_endpoint;
            clientAddress_endpoint.address = clientAddress.sin_addr.s_addr; //!!! S_un
            clientAddress_endpoint.port = clientAddress.sin_port;

            switch (static_cast<Client_Message>(buffer[0])) // Wybor jakiego rodzaju pakiet ma byc wyslany
            {
            case Client_Message::Join:
            {
                printf("Client_Message::Join from %u:%hu\n", clientAddress_endpoint.address, clientAddress_endpoint.port);

                unsigned short slot = 65535; // przypisanie slot ostatniej mozliwej wartosci czyli 65535
                // Szukamy wolnego slota
                for (unsigned short i = 0; i < MAX_CLIENTS; ++i)
                {
                    if (client_endpoints[i].address == 0)
                    {
                        slot = i;
                        break;
                    }
                }

                buffer[0] = (char)Server_Message::Join_Result;
                // Jesli znaleziono wolny slot, uzyty jest on jako ID dla klienta
                if (slot != 65535)
                {
                    printf("client will be assigned to slot %hu\n", slot);
                    buffer[1] = 1;                // Przypisanie ID sie udalo
                    memcpy(&buffer[2], &slot, 2); // Przypisanie klientowi ID

                    flags = 0;
                    if (sendto(server_socket, buffer, 4, flags, (sockaddr *)&clientAddress, clientAddress_size) != -1)
                    {
                        client_endpoints[slot] = clientAddress_endpoint;
                        time_since_heard_from_clients[slot] = 0.0f;
                        client_objects[slot] = {};
                        client_inputs[slot] = {};
                    }
                    else
                    {
                        printf("sendto Server_Message::Join failed\n");
                    }
                }
                // Brak wolnych slotow
                else
                {
                    printf("could not find a slot for player\n");
                    buffer[1] = 0;
                    flags = 0;
                    if (sendto(server_socket, buffer, 2, flags, (sockaddr *)&clientAddress, clientAddress_size) == -1)
                    {
                        printf("sendto 2 Client_Message::Join failed\n");
                    }
                }
            }
            break;

            case Client_Message::Leave:
            {
                // Zwolnienie slota
                unsigned short slot;
                memcpy(&slot, &buffer[1], 2);
                if (client_endpoints[slot] == clientAddress_endpoint)
                {
                    client_endpoints[slot] = {};
                }
                printf("Client_Message::Leave from %hu(%u:%hu)\n", slot, clientAddress_endpoint.address, clientAddress_endpoint.port);
            }
            break;

            case Client_Message::Input:
            {
                unsigned short slot;
                memcpy(&slot, &buffer[1], 2);

                printf("%d %hu\n", bytes_received, slot);

                if (client_endpoints[slot] == clientAddress_endpoint)
                {
                    char input = buffer[3];

                    client_inputs[slot].up = input & 0x1;
                    client_inputs[slot].down = input & 0x2;
                    client_inputs[slot].left = input & 0x4;
                    client_inputs[slot].right = input & 0x8;

                    time_since_heard_from_clients[slot] = 0.0f;

                    printf("Client_Message::Input from %hu:%d\n", slot, int(input));
                }
                else
                {
                    printf("Client_Message::Input discarded, was from %u:%hu but expected %u:%hu\n", clientAddress_endpoint.address, clientAddress_endpoint.port, client_endpoints[slot].address, client_endpoints[slot].port);
                }
            }
            break;
            }
        }

        for (unsigned short i = 0; i < MAX_CLIENTS; ++i)
        {
            if (client_endpoints[i].address) // Jesli klient jest w uzyciu
            {
                // Aktualizuje player state na podstawie player input
                if (client_inputs[i].up)
                {
                    client_objects[i].speed += ACCELERATION * SECONDS_PER_TICK;
                    if (client_objects[i].speed > MAX_SPEED)
                    {
                        client_objects[i].speed = MAX_SPEED;
                    }
                }
                if (client_inputs[i].down)
                {
                    client_objects[i].speed -= ACCELERATION * SECONDS_PER_TICK;
                    if (client_objects[i].speed < 0.0f)
                    {
                        client_objects[i].speed = 0.0f;
                    }
                }
                if (client_inputs[i].left)
                {
                    client_objects[i].facing -= TURN_SPEED * SECONDS_PER_TICK;
                }
                if (client_inputs[i].right)
                {
                    client_objects[i].facing += TURN_SPEED * SECONDS_PER_TICK;
                }

                client_objects[i].x += client_objects[i].speed * SECONDS_PER_TICK * sinf(client_objects[i].facing);
                client_objects[i].y += client_objects[i].speed * SECONDS_PER_TICK * cosf(client_objects[i].facing);

                time_since_heard_from_clients[i] += SECONDS_PER_TICK;
                if (time_since_heard_from_clients[i] > CLIENT_TIMEOUT)
                {
                    client_endpoints[i] = {};
                }
            }
        }

        // tworzenie state packet
        buffer[0] = (unsigned char)Server_Message::State;
        int bytes_written = 1;
        for (unsigned short i = 0; i < MAX_CLIENTS; ++i)
        {
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
            }
        }

        // Wyslanie pakietu z powrotem do klientow
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
                to.sin_port = client_endpoints[i].port;

                if (sendto(server_socket, buffer, bytes_written, flags, (sockaddr*)&to, to_length) == -1)
                {
                    printf("sendto back to client failed\n");
                }
            }
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

    close(server_socket);
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