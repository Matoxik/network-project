#include "Player.h"

Player::Player(const std::string &textureFile, std::map<std::string, sf::Texture> &textureMap)
{

    // If texture does not exit in map
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

// Update player position 
void Player::update()
{
    player.setPosition(this->x + 150, this->y + 900); 
}


void Player::assignTexture(unsigned short playerId)
{
    switch (playerId % 3)
    {
    case 0:
        // set blue texture
        if (facing)
        {
            player.setTextureRect({950, 55, 400, 330});
        }
        else
        {
            player.setTextureRect({1440, 55, 400, 330});
        }
        break;
    case 1:
        // set red texture
        if (facing)
        {
            player.setTextureRect({50, 530, 400, 330});
        }
        else
        {
            player.setTextureRect({545, 530, 400, 330});
        }
        break;
    case 2:
        // set yellow texture 
        if (facing)
        {
            player.setTextureRect({960, 530, 400, 330});
        }
        else
        {
            player.setTextureRect({1450, 530, 400, 330});
        }
        break;
    default:
        player.setTextureRect({50, 55, 400, 330});
        break;
    }
}
