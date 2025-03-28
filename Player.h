#pragma once
#include <iostream>

#include "time.h"

#include <SFML/Graphics.hpp>
#include <SFML/Window.hpp>

#include "Exception.h"

using namespace sf;
using namespace std;

class Player
{
public:
	Player(const std::string &textureFile, std::map<std::string, sf::Texture> &textureMap);
	
	
	float x, y, facing;
	bool is_active;

	void draw(RenderWindow& window);

    
    // Update player movement
    void update();
	
    
	//Set player texture based on player ID
	void assignTexture(unsigned short playerId);
	void setStartingPosition();

	
	
private:
	Texture texture;
	Sprite player;
	
	
};



