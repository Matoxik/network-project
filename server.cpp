#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>




const unsigned int SOCKET_BUFFER_SIZE = 1024;


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
    int player_x = 0;
    int player_y = 0;
    
    bool is_running = true;
    while (is_running) {
       
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

        switch (client_input)
        {
        case 'w':
            ++player_y;
            break;

        case 'a':
            --player_x;
            break;

        case 's':
            --player_y;
            break;

        case 'd':
            ++player_x;
            break;

        case 'q':
            is_running = false;
            break;

        default:
            perror("Unhandled input");
            close(server_socket);
            exit(EXIT_FAILURE);
            break;
        }

        // create state packet
        int write_index = 0;
        memcpy(&buffer[write_index], &player_x, sizeof(player_x));
        write_index += sizeof(player_x);

        memcpy(&buffer[write_index], &player_y, sizeof(player_y));
        write_index += sizeof(player_y);

        memcpy(&buffer[write_index], &is_running, sizeof(is_running));

        // send back to client
        int buffer_length = sizeof(player_x) + sizeof(player_y) + sizeof(is_running);

        if (sendto(server_socket, buffer, buffer_length, 0, (const struct sockaddr *)&clientAddress, sizeof(clientAddress)) < 0) {
            perror("Server could not send back");
            close(server_socket);
            exit(EXIT_FAILURE);
        }
   
    }

    close(server_socket);
    return 0;
}
