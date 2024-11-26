  #include <sys/types.h>
  #include <sys/socket.h>
  #include <netinet/in.h>
  #include <arpa/inet.h>
  #include <stdio.h>
  #include <stdlib.h>
  #include <string.h>
  #include <unistd.h>

const int one = 1;

int main(int argc, char ** argv) {

    struct sockaddr_in serverAdress;
    serverAdress.sin_family = AF_INET;
    serverAdress.sin_port   = htons(1100);
    serverAdress.sin_addr.s_addr  = inet_addr("127.0.0.1");
    

    int localSocket = socket(AF_INET, SOCK_DGRAM, 0);

    char msg[] = "siema";
  

    if(sendto(localSocket,(const char*)msg,strlen(msg),0,(const struct sockaddr *)&serverAdress,sizeof(serverAdress))<0){
      perror("could not send");
      close(localSocket);
      exit(EXIT_FAILURE);
    }

    close(localSocket);
}