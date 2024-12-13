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



#define SOCKET_BUFFER_SIZE 1024
#define ACCELERATION 0.1f
#define MAX_SPEED 2.0f
#define TURN_SPEED 0.1f
#define SECONDS_PER_TICK (1.0f / 60.0f)


timespec timespec_diff(const timespec& start, const timespec& end) {
    timespec temp;
    if ((end.tv_nsec - start.tv_nsec) < 0) {
        temp.tv_sec = end.tv_sec - start.tv_sec - 1;
        temp.tv_nsec = 1000000000 + end.tv_nsec - start.tv_nsec;
    } else {
        temp.tv_sec = end.tv_sec - start.tv_sec;
        temp.tv_nsec = end.tv_nsec - start.tv_nsec;
    }
    return temp;
}

float timespec_to_seconds(const timespec& ts) {
    return ts.tv_sec + ts.tv_nsec / 1000000000.0f;
}



int main(int argc, char **argv) {
    struct sockaddr_in localAddress, clientAddress;

    // Konfiguracja adresu lokalnego
    localAddress.sin_family = AF_INET;
    localAddress.sin_port = htons(1100); // Port 1100
    localAddress.sin_addr.s_addr = INADDR_ANY;

    // Tworzenie gniazda
    int server_socket = socket(AF_INET, SOCK_DGRAM, 0);
    if (server_socket < 0) {
        perror("Could not create socket");
        exit(EXIT_FAILURE);
    }

    // Przypisanie adresu do gniazda
    if (bind(server_socket, (struct sockaddr *)&localAddress, sizeof(localAddress)) == -1) {
        perror("Could not bind");
        close(server_socket);
        exit(EXIT_FAILURE);
    }

    // Bufor do odbioru danych
    char buffer[SOCKET_BUFFER_SIZE];
    
    //Zmienne game state (zmienne ktore sa wymieniane miedzy klientem a serwerem)
    float player_x = 0.0f;
    float player_y = 0.0f;
    float player_facing = 0.0f;
    float player_speed = 0.0f;

    
    bool is_running = true;

    // Ustawienie czasu startowego
    timespec clock_frequency, tick_start_time, tick_end_time;
    clock_gettime(CLOCK_MONOTONIC, &clock_frequency);
    
    while (is_running) {
        clock_gettime(CLOCK_MONOTONIC, &tick_start_time);
       // Odbior danych od klienta
        socklen_t clientAddress_size = sizeof(clientAddress);
        int bytes_received = recvfrom( server_socket, buffer, SOCKET_BUFFER_SIZE, 0, (sockaddr*)&clientAddress, &clientAddress_size);
        if (bytes_received < 0) {
            perror("Server could not receive");
            close(server_socket);
            exit(EXIT_FAILURE);
        }
         
        // Obsluga danych wejsciowych od klienta
        char client_input = buffer[0];

        if (client_input & 0x1) // forward
        {
            player_speed += ACCELERATION;
            if (player_speed > MAX_SPEED)
            {
                player_speed = MAX_SPEED;
            }
        }
        if (client_input & 0x2) // back
        {
            player_speed -= ACCELERATION;
            if (player_speed < 0.0f)
            {
                player_speed = 0.0f;
            }
        }
        if (client_input & 0x4) // left
        {
            player_facing -= TURN_SPEED;
        }
        if (client_input & 0x8) // right
        {
            player_facing += TURN_SPEED;
        }

        player_x += player_speed * sinf(player_facing);
        player_y += player_speed * cosf(player_facing);

        // create state packet

        int bytes_written = 0;
        memcpy(&buffer[bytes_written], &player_x, sizeof(player_x));
        bytes_written += sizeof(player_x);

        memcpy(&buffer[bytes_written], &player_y, sizeof(player_y));
        bytes_written += sizeof(player_y);

        memcpy(&buffer[bytes_written], &player_facing, sizeof(player_facing));
        bytes_written += sizeof(player_facing);

       

        // send back to client
        int buffer_length = sizeof(player_x) + sizeof(player_y) + sizeof(player_facing);

        if (sendto(server_socket, buffer, buffer_length, 0, (const struct sockaddr *)&clientAddress, sizeof(clientAddress)) < 0) {
            perror("Server could not send back");
            close(server_socket);
            exit(EXIT_FAILURE);
        }
           
        // Obliczanie czasu wykonania ticku
        clock_gettime(CLOCK_MONOTONIC, &tick_end_time);
        timespec elapsed_time = timespec_diff(tick_start_time, tick_end_time);
        float time_taken_s = timespec_to_seconds(elapsed_time);

        while (time_taken_s < SECONDS_PER_TICK) {
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
