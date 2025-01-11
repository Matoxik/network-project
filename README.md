# network-project
Kinga Prośniewska 155998, L2
Maciej Andrzejewski 155093, L5


Temat: Wieloosobowa gra czasu rzeczywistego

Uruchomienie serwera:
g++ -Wall server.cpp -pthread -lm -o ser.out && ./ser.out

Uruchomienie klienta z SFML:
g++ -Wall client.cpp Player.cpp Exception.cpp Background.cpp Ground.cpp Button.cpp Portal.cpp -o cli.out -lsfml-graphics -lsfml-window -lsfml-system && ./cli.out


Instalacja SFML:
sudo apt-get remove libsfml-dev
sudo apt install libsfml-dev
ls /usr/include/SFML

