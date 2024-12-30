#pragma once
#include <iostream>

#include "time.h"

#include <SFML/Graphics.hpp>
#include <SFML/Window.hpp>

#include "Exception.h"

using namespace sf;
using namespace std;

#define MAX_PLAYER_HEALTH 6

class Player
{
public:
	Player(const std::string &textureFile, std::map<std::string, sf::Texture> &textureMap);
	
	unsigned short id;
	
	float x, y, facing;

	void draw(RenderWindow& window);

	// Methods for getting current position of the player
	float getUpEdge();
	float getDownEdge();
	float getLeftEdge();
	float getRightEdge();
    
    // Update player movement
    void update();
	
	
	// Methods for choosing player texture based on elemental flags
	void setIceTexture();
	void setFireTexture();
	void setLightTexture();
	void setBasicTexture();
    
	//Set player texture based on player ID
	void assignTexture(unsigned short playerId);
	void setStartingPosition();

	//Gravity
	void moveGravity();
	

	
private:
	Texture texture;
	Sprite player;
	
	
};



