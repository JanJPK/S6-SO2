#pragma once
#ifndef DRIVER_HPP
#define DRIVER_HPP
#include "Global.h"
#include "Drawing.h"
#include "LoadingZone.h"

class Truck : public Drawable
{
public:
	std::mutex m;
	std::condition_variable cv;
	bool loaded;
};


class Driver : public Drawable
{
public:
	Truck truck;
	Coord waitingPosition;

	Driver(char symbol, int x, int y, int colorIndex, int waitTime)
	{
		this->waitingPosition.x = x;
		this->waitingPosition.y = y;
		this->position.x = x;
		this->position.y = y;
		this->symbol = symbol;
		this->colorIndex = colorIndex;
		this->waitTime = waitTime;

		this->truck.position.x = x - 1;
		this->truck.position.y = y;
		this->truck.symbol = '0';
		this->truck.colorIndex = colorIndex;
	}
};

enum DriverState
{
	queued,
	loading,
	leaving
};

void DriverThread(Driver& driver, LoadingZone& lz)
{
	DrawBlock(driver);
	DrawBlock(driver.truck);

	while (false)
	{
		std::unique_lock<std::mutex> uniqueLock1(lz.m);
		(lz.cv).wait(uniqueLock1, [&lz] { return !lz.occupied; });
		lz.driver = &driver;

		std::unique_lock<std::mutex> uniqueLock2(driver.truck.m);
		(driver.truck.cv).wait(uniqueLock2, [&driver] { return driver.truck.loaded; });
		EraseBlock(driver.position);
		EraseBlock(driver.truck.position);		
		uniqueLock2.unlock();
		uniqueLock1.unlock();
		(lz.cv).notify_one();
		std::this_thread::sleep_for(std::chrono::milliseconds(10000));
		driver.position.x = driver.waitingPosition.x;
		driver.position.y = driver.waitingPosition.y;
		driver.truck.position.x = driver.waitingPosition.x - 1;
		driver.truck.position.y = driver.waitingPosition.y;
	}
}
#endif
