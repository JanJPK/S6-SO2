#pragma once
#ifndef DRAWABLE_HPP
#define DRAWABLE_HPP
#include "Coord.h"

class Drawable
{
public:
	char symbol;
	Coord position;
	int colorIndex;
	int waitTime;
};
#endif