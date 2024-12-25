# network-project

Uruchomienie serwera:
gcc -Wall server.cpp -o ser.out -lm && ./ser.out

Uruchomienie klienta:
g++ -Wall client.cpp -o cli.out && ./cli.out 

Uruchomienie klienta z SFML:
g++ -Wall client.cpp Player.cpp Exception.cpp Background.cpp -o cli.out -lsfml-graphics -lsfml-window -lsfml-system && ./cli.out 


Instalacja SFML:
sudo apt-get remove libsfml-dev
sudo apt install libsfml-dev
ls /usr/include/SFML

