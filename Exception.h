#pragma once
#include <iostream>
#include <stdlib.h>

#include <SFML/Graphics.hpp>
#include <SFML/Window.hpp>

using namespace std;
using namespace sf;


class Exception
{
public:
    Exception(int nr, string op);

    int getNumber();
    string getDescription();

private:
    int number;
    string description;
};

