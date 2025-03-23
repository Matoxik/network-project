# 2D multiplayer game using API bsd-sockets

Kinga Prośniewska 155998, L2
Maciej Andrzejewski 155093, L5

## Temat: Wieloosobowa gra czasu rzeczywistego

### Uruchomienie serwera:

g++ -Wall server.cpp -pthread -lm -o ser.out && ./ser.out

### Uruchomienie klienta z SFML:

g++ -Wall client.cpp Player.cpp Exception.cpp Background.cpp Ground.cpp Button.cpp Portal.cpp -o cli.out -lsfml-graphics -lsfml-window -lsfml-system && ./cli.out


## Instalacja SFML linux:
- sudo apt-get remove libsfml-dev
- sudo apt install libsfml-dev
- sudo zypper install libsfml2-2_5 sfml2-devel
- ls /usr/include/SFML

![obraz](https://github.com/user-attachments/assets/f9b10c33-1151-4c38-a98e-5d8bb716e20f)

![obraz](https://github.com/user-attachments/assets/df75c250-227d-46ec-b1a3-c589bb3c6e06)

![obraz](https://github.com/user-attachments/assets/3578763c-0e39-4e42-bac1-d4b98932bc69)





