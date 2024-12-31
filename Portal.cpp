#include "Portal.h"


Portal::Portal()
{
    if (!textureFirst.loadFromFile("textures/portal1v1.png"))
    {
        Exception* exception = new Exception(3, "Error loading texture in Portal class ");
        throw exception;
    }
    // Set up initial properties
    portal.setTexture(textureFirst);
    portal.setPosition(1700, 120);
    portal.setScale(0.5, 0.5);
    portal.setOrigin(193, 243); // Setting the center of the character
}



void Portal::draw(RenderWindow& window)
{
    window.draw(portal);
}

void Portal::changeTexture()
{
    if (!textureSecond.loadFromFile("textures/portal1v2.png"))
    {
        Exception* exception = new Exception(3, "Error loading texture after change in Portal class ");
        throw exception;
    }
    
    portal.setTexture(textureSecond);
    portal.setScale(0.5, 0.5);
    portal.setTextureRect({ 0,0,383,486 });
}