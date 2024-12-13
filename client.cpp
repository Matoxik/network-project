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

#define SOCKET_BUFFER_SIZE 1024

int main() {
    int client_socket;
    sockaddr_in serverAddress;

    // Tworzenie gniazda
    client_socket = socket(AF_INET, SOCK_DGRAM, 0);
    if (client_socket < 0) {
        perror("Cannot create socket");
        exit(EXIT_FAILURE);
    }

    serverAddress.sin_family = AF_INET;
    serverAddress.sin_port = htons(1100);
    serverAddress.sin_addr.s_addr = inet_addr("127.0.0.1");

    char buffer[SOCKET_BUFFER_SIZE];
    float player_x = 0.0f;
    float player_y = 0.0f;
    float player_facing = 0.0f;
    bool is_running = true;

    std::cout << "Type W, A, S, or D to move. Q to quit." << std::endl;

    while (is_running) {
        // Obsługa wejścia użytkownika
        char input;
        std::cout << "Enter command: ";
        std::cin >> input;

        //Cztery pierwsze bity buffer[0] przechowuja informacje o pozycji gracza
        buffer[0] = 0;
        switch (input) {
            case 'W': case 'w':
                buffer[0] |= 0x1;
                break;
            case 'S': case 's':
                buffer[0] |= 0x2;
                break;
            case 'A': case 'a':
                buffer[0] |= 0x4;
                break;
            case 'D': case 'd':
                buffer[0] |= 0x8;
                break;
            case 'Q': case 'q':
                is_running = false;
                 std::cout << "Quit!" << std::endl;
                break;
            default:
                std::cout << "Invalid input!" << std::endl;
                continue;
        }

        // Wysłanie danych do serwera
        int buffer_length = 1;
        if (sendto(client_socket, buffer, buffer_length, 0, (sockaddr*)&serverAddress, sizeof(serverAddress)) < 0) {
            perror("Could not send data to server");
            close(client_socket);
            exit(EXIT_FAILURE);
        }

        // Oczekiwanie na odpowiedź od serwera
        sockaddr_in from;
        socklen_t from_size = sizeof(from);
        int bytes_received = recvfrom(client_socket, buffer, SOCKET_BUFFER_SIZE, 0, (sockaddr*)&from, &from_size);
        if (bytes_received < 0) {
            perror("Could not receive data from server");
            close(client_socket);
            exit(EXIT_FAILURE);
        }

        // Rozpakowywanie danych
        int read_index = 0;
        memcpy(&player_x, &buffer[read_index], sizeof(player_x));
        read_index += sizeof(player_x);
        memcpy(&player_y, &buffer[read_index], sizeof(player_y));
        read_index += sizeof(player_y);
        memcpy(&player_facing, &buffer[read_index], sizeof(player_facing));
      
        // Wyświetlenie stanu gracza
        std::cout << "Player position: (" << player_x << ", " << player_y << ") Facing: " << player_facing << std::endl;
    }

    close(client_socket);
    return 0;
}
