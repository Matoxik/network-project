#pragma once
#include <iostream>

#include <SFML/Graphics.hpp>
#include <SFML/Window.hpp>

#include "Exception.h"

using namespace std;
using namespace sf;

class Button
{
public:
    Button();
    
  
    void draw(RenderWindow &window);

    void setPosition(int x, int y);
    void setNormalTexture(unsigned short playerId);
    void setPushedTexture(unsigned short playerId);
    

private:
    Texture texture;
    Sprite button;
};