#pragma once
#include <iostream>

#include <SFML/Graphics.hpp>
#include <SFML/Window.hpp>

#include "Exception.h"

using namespace std;
using namespace sf;

class Ground
{
public:
    Ground();
    void draw(RenderWindow &window);

    void setPosition(int x, int y);


private:
    Texture texture;
    Sprite ground;
};