#pragma once
#ifndef LOADINGZONE_HPP
#define LOADINGZONE_HPP
#include "Driver.h"

class LoadingZone
{
public:
	std::mutex m;
	std::condition_variable cv;
	bool occupied;
	Driver* driver;
	Coord position;

};
#endif