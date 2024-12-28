#include "Background.h"

// Method definitions of StartBackground class
StartBackground::StartBackground()
{
    // Load the texture for the start background
    if (!backgroundTexture.loadFromFile("textures/tloStart.png"))
    {
        Exception* exception = new Exception(3, "Error loading start background texture in StartBackground class ");
        throw exception;
    }
    // Set the texture for the background shape and define its size
    backgroundShape.setTexture(&backgroundTexture);
    backgroundShape.setSize(Vector2f(1920, 1080));
}

void StartBackground::draw(RenderWindow& window)
{
    window.draw(backgroundShape);
}