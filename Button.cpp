#include "Button.h"

// Method definitions of button class
Button::Button()
{
    // Load the texture for the button
    if (!texture.loadFromFile("textures/buttons.png"))
    {
        Exception *exception = new Exception(3, "Error loading button texture");
        throw exception;
    }
    // Set the texture for the button sprite and define its position and scale
    button.setTexture(texture);
    button.setPosition(180, 685);
    button.setTextureRect({200, 110, 180, 120});
    button.setOrigin(90, 60);
    button.setScale(0.5, 0.5);
}

void Button::draw(RenderWindow &window)
{
    window.draw(button);
}

void Button::setPosition(int _x, int _y)
{
    button.setPosition(_x, _y);
}


void Button::setNormalTexture(unsigned short playerId)
{
    switch (playerId % 3)
    {
    case 0:
        // Blue
        button.setTextureRect({200, 280, 180, 120});
        break;
    case 1:
        // Red
        button.setTextureRect({200, 110, 180, 120});
        break;
    case 2:
        // Yellow
        button.setTextureRect({200, 455, 180, 120});
        break;
    default:

        break;
    }
}

void Button::setPushedTexture(unsigned short _playerId)
{
    switch (_playerId % 3)
    {
    case 0:
        // Blue
        button.setTextureRect({495, 280, 180, 120});
        break;
    case 1:
        // Red
        button.setTextureRect({495, 110, 180, 120});
        break;
    case 2:
        // Yellow
        button.setTextureRect({495, 455, 180, 120});
        break;
    default:

        break;
    }
}