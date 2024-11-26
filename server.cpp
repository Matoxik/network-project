#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

int main(int argc, char **argv) {
    struct sockaddr_in localAddress, clientAddress;

    // Konfiguracja adresu lokalnego
    localAddress.sin_family = AF_INET;
    localAddress.sin_port = htons(1100); // Port 1100
    localAddress.sin_addr.s_addr = inet_addr("127.0.0.1");

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
    char buff[256];
    socklen_t len = sizeof(clientAddress);

    for (;;) {
        // Wyczyszczenie bufora
        bzero(buff, sizeof(buff));

        // Odczyt danych
        int n = recvfrom(server_socket, (char *)buff, sizeof(buff), 0, (struct sockaddr *)&clientAddress, &len);
        if (n < 0) {
            perror("Could not receive");
            close(server_socket);
            exit(EXIT_FAILURE);
        }

        // Wyświetlanie odebranej wiadomości
        printf("Received msg: %s\n", buff);
    }

    close(server_socket);
    return 0;
}