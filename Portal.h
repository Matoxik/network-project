
#pragma once
#include <iostream>

#include <SFML/Graphics.hpp>
#include <SFML/Window.hpp>

#include "Exception.h"

using namespace std;
using namespace sf;



class Portal
{
public:
	Portal();

	void changeTexture();

	void draw(RenderWindow& window);

private:
	Texture textureFirst;
	Texture textureSecond;
	Sprite portal;
};