  #include <sys/types.h>
  #include <sys/socket.h>
  #include <netinet/in.h>
  #include <arpa/inet.h>
  #include <stdio.h>
  #include <stdlib.h>
  #include <string.h>
  #include <unistd.h>



const unsigned int SOCKET_BUFFER_SIZE = 1024;


int main(int argc, char ** argv) {

    struct sockaddr_in serverAdress;
    serverAdress.sin_family = AF_INET;
    serverAdress.sin_port   = htons(1100);
    serverAdress.sin_addr.s_addr  = inet_addr("127.0.0.1");
    

    int localSocket = socket(AF_INET, SOCK_DGRAM, 0);

    char buffer[SOCKET_BUFFER_SIZE];
    int player_x;
    int player_y;

    printf( "type w, a, s, or d to move, q to quit\n" );

    bool is_running = 1;
    while (is_running)
    {
      // Obsluga wejscia
      scanf( "\n%c", &buffer[0], 1 );
      
      // Wyslanie wejscia do serwera
      int buffer_length = 1;
      if (sendto(localSocket, buffer, buffer_length, 0, (const struct sockaddr *)&serverAdress, sizeof(serverAdress)) <0) {
       perror("Client could not send");
      close(localSocket);
      exit(EXIT_FAILURE);
      }

      //Czekanie na informacje zwrotna (pakiet od serwera)
      
      sockaddr_in from;
      socklen_t from_size = sizeof(from);
      int bytes_received = recvfrom(localSocket, buffer, SOCKET_BUFFER_SIZE, 0, (sockaddr *)&from, &from_size);

      if (bytes_received < 0)
      {
        perror("Could not receive");
        close(localSocket);
        exit(EXIT_FAILURE);
      }

      // grab data from packet
      int read_index = 0;

      memcpy(&player_x, &buffer[read_index], sizeof(player_x));
      read_index += sizeof(player_x);

      memcpy(&player_y, &buffer[read_index], sizeof(player_y));
      read_index += sizeof(player_y);

      memcpy(&is_running, &buffer[read_index], sizeof(is_running));

      printf("x:%d, y:%d, is_running:%d\n", player_x, player_y, is_running);
    }
    

    close(localSocket);
}
