#include "Player.h"

Player::Player(const std::string &textureFile, std::map<std::string, sf::Texture> &textureMap)
{

    // Jeśli tekstura nie istnieje w mapie, załaduj ją
    if (textureMap.find(textureFile) == textureMap.end())
    {
        if (!textureMap[textureFile].loadFromFile(textureFile))
        {
            Exception *exception = new Exception(3, "Error loading texture in Player class");
            throw exception;
        }
    }

    // Set up the player sprite
    player.setTexture(textureMap[textureFile]);
    player.setTextureRect({50, 55, 400, 330});
   player.setPosition(500, 900);
    player.setScale(0.5, 0.5);
    player.setOrigin(200, 165); // Setting the center of the player

    this->x = 0;
    this->y = 0;
    this->facing = 0;
}

void Player::draw(RenderWindow &window)
{
    window.draw(player);
}



// Update player movement based on key presses
void Player::update()
{
        player.setPosition(this->x + 150, 900);
}

// Methods for getting current position
float Player::getUpEdge()
{
    return this->player.getPosition().y - 82;
}

float Player::getDownEdge()
{
    return this->player.getPosition().y + 82;
}

float Player::getLeftEdge()
{
    return this->player.getPosition().x - 100;
}

float Player::getRightEdge()
{
    return this->player.getPosition().x + 100;
}

void Player::setBasicTexture()
{
    if (Keyboard::isKeyPressed(Keyboard::Left))
    {
        player.setTextureRect({50, 55, 400, 330});
    }
    if (Keyboard::isKeyPressed(Keyboard::Right))
    {
        player.setTextureRect({535, 55, 400, 330});
    }
}

void Player::setIceTexture()
{
    if (Keyboard::isKeyPressed(Keyboard::Left))
    {
        player.setTextureRect({950, 55, 400, 330});
    }
    if (Keyboard::isKeyPressed(Keyboard::Right))
    {
        player.setTextureRect({1440, 55, 400, 330});
    }
}

void Player::setFireTexture()
{
    if (Keyboard::isKeyPressed(Keyboard::Left))
    {
        player.setTextureRect({50, 530, 400, 330});
    }
    if (Keyboard::isKeyPressed(Keyboard::Right))
    {
        player.setTextureRect({545, 530, 400, 330});
    }
}

void Player::setLightTexture()
{
    if (Keyboard::isKeyPressed(Keyboard::Left))
    {
        player.setTextureRect({960, 530, 400, 330});
    }
    if (Keyboard::isKeyPressed(Keyboard::Right))
    {
        player.setTextureRect({1450, 530, 400, 330});
    }
}

void Player::assignTexture(unsigned short playerId)
{
    switch (playerId)
    {
    case 0:
        // setIceTexture
        player.setTextureRect({950, 55, 400, 330});
        break;
    case 1:
        // setFireTexture
        player.setTextureRect({50, 530, 400, 330});
        break;
    case 2:
        // setLightTexture
        player.setTextureRect({960, 530, 400, 330});
        break;
    default:
        player.setTextureRect({50, 55, 400, 330});
        break;
    }
}
