#pragma once
#include <iostream>

#include <SFML/Graphics.hpp>
#include <SFML/Window.hpp>

#include "Exception.h"

using namespace std;
using namespace sf;

class StartBackground
{

public:
	StartBackground();
	void draw(RenderWindow& window);

private:
	RectangleShape backgroundShape; // Shape representing the background
	Texture backgroundTexture;

};


