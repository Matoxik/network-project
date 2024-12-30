#include "Ground.h"


// Method definitions of ground class
Ground::Ground()
{
    // Load the texture for the ground
    if (!texture.loadFromFile("textures/ground.png"))
    {
        Exception* exception = new Exception(3, "Error loading ground texture");
        throw exception;
    }
    // Set the texture for the ground sprite and define its position and scale
    ground.setTexture(texture);
    ground.setPosition(960, 1034);
     ground.setOrigin(1459, 114);
    ground.setScale(0.7, 0.4);

}

void Ground::draw(RenderWindow& window)
{
    window.draw(ground);
}

// Methods for getting current position
float Ground::getUpEdge()
{
    return this->ground.getPosition().y - 30;  
}

float Ground::getDownEdge()
{
    return this->ground.getPosition().y + 120;
}

float Ground::getLeftEdge()
{
    return this->ground.getPosition().x - 900; 
}

float Ground::getRightEdge()
{
    return this->ground.getPosition().x + 900;
}