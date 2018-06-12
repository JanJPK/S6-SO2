// S6-SO2-3.cpp : Defines the entry point for the console application.
//
#include "stdafx.h"
#include "curses.h"
#include <io.h>
#include <fcntl.h>
#include <random>
#include <cstdio>
#include <vector>
#include <iostream>
#include <stdlib.h>
#include <windows.h>
#include <mutex>
#include <thread>
#include <ctime>

#define BACKGROUND ' '
#define WALL L'\u2588'

bool run = true;
//bool run = false;
#pragma region Utility

struct Coord
{
	int y;
	int x;
};

#pragma endregion

#pragma region ConditionVariables

std::mutex mParkingSpot;
std::condition_variable cvParkingSpot;

std::mutex mCurses;

#pragma endregion

#pragma region Drawable

class Drawable
{
public:
	char symbol;
	Coord position;
	int colorIndex;
	int waitTime;
};

#pragma endregion

#pragma region Drawing

void drawBlock(Coord& coord, char symbol, int colorIndex)
{
	mCurses.lock();
	attron(COLOR_PAIR(colorIndex));
	move(coord.y, coord.x);
	addch(symbol);
	attroff(COLOR_PAIR(colorIndex));
	refresh();
	mCurses.unlock();
}

void drawBlock(Drawable& drawable)
{
	mCurses.lock();
	attron(COLOR_PAIR(drawable.colorIndex));
	move(drawable.position.y, drawable.position.x);
	addch(drawable.symbol);
	attroff(COLOR_PAIR(drawable.colorIndex));
	refresh();
	mCurses.unlock();
}

void eraseBlock(Coord& coord)
{
	mCurses.lock();
	mvaddch(coord.y, coord.x, BACKGROUND);
	refresh();
	mCurses.unlock();
}

#pragma endregion

#pragma region ProductPoint

class ProductPoint : public Drawable
{
public:
	bool full;
	std::mutex m;
	std::condition_variable cv;
};

#pragma endregion

#pragma region Carrier

enum CarrierState
{
	waiting,
	carrying,
	returning
};

class Carrier : public Drawable
{
public:
	CarrierState state;

	Carrier(char symbol, int x, int y, int colorIndex, int waitTime)
	{
		this->symbol = symbol;
		this->position.x = x;
		this->position.y = y;
		this->colorIndex = colorIndex;
		this->waitTime = waitTime;
		this->state = waiting;
	}
};

void CarrierThread(Carrier& carrier, ProductPoint& spawner, ProductPoint& receiver)
{	
	drawBlock(carrier);	
	while (run)
	{
		switch (carrier.state)
		{
			case waiting:
			{
				std::unique_lock<std::mutex> uniqueLock(spawner.m);
				(spawner.cv).wait(uniqueLock, [&spawner] {return spawner.full; });
				spawner.symbol = 'O';
				spawner.full = false;
				drawBlock(spawner);
				carrier.state = carrying;
				carrier.symbol = '1';
				drawBlock(carrier.position, carrier.symbol, carrier.colorIndex);
				break;
			}

			case carrying:
			{
				if (carrier.position.x == receiver.position.x - 1)
				{
					std::lock_guard<std::mutex> lockGuard(receiver.m);
					receiver.symbol = carrier.symbol;
					receiver.full = true;
					drawBlock(receiver);					
					carrier.state = returning;
					carrier.symbol = 'C';
					drawBlock(carrier);
					(receiver.cv).notify_all();
				}
				else
				{
					eraseBlock(carrier.position);
					carrier.position.x++;
					drawBlock(carrier);
				}
				break;
			}

			case returning:
			{
				if (carrier.position.x == spawner.position.x + 1)
				{
					carrier.state = waiting;
				}
				else
				{
					eraseBlock(carrier.position);
					carrier.position.x--;
					drawBlock(carrier);
				}
				break;
			}
		}
		Sleep(carrier.waitTime);
	}
}

#pragma endregion

#pragma region Assembler

enum AssemblerState
{
	hasNone,
	hasA,
	hasB
};

void AssemblerThread(ProductPoint& receiverA, ProductPoint& receiverB, ProductPoint& spawner)
{
	AssemblerState state = hasNone;
	Coord position;
	position.x = spawner.position.x - 1;
	position.y = spawner.position.y;
	drawBlock(position, 'A', 3);
	drawBlock(receiverA);
	drawBlock(receiverB);

	int waitTime = 1500;	
	while (run)
	{
		switch (state)
		{
			case hasNone:
			{
				std::unique_lock<std::mutex> uniqueLock(receiverA.m);
				(receiverA.cv).wait(uniqueLock, [&receiverA] {return receiverA.full; });
				state = hasA;
				drawBlock(spawner.position, receiverA.symbol, receiverA.colorIndex);				
				receiverA.symbol = 'O';
				receiverA.full = false;
				drawBlock(receiverA);
				break;
			}

			case hasA:
			{
				std::unique_lock<std::mutex> uniqueLock(receiverB.m);
				(receiverB.cv).wait(uniqueLock, [&receiverB] {return receiverB.full; });
				state = hasB;
				drawBlock(spawner);				
				receiverB.symbol = 'O';
				receiverB.full = false;
				drawBlock(receiverB);
				state = hasNone;
				break;
			}

			case hasB:
			{
				// TODO: notify truck loading carrier
				break;
			}
		}
		Sleep(waitTime);
	}
}

#pragma endregion

#pragma region Loader

void LoaderThread()
{
	while(run)
	{
		// wait for truck
		// lock truck
		// start new loop
			// wait for product
			// load truck
			// check if truck is full
		// send truck away		
	}
}

#pragma endregion

#pragma region Driver

class Truck : public Drawable
{
	
};

class Driver : public Drawable
{
public:
	Truck truck;
};

enum DriverState
{
	queued, loading, leaving
};

void DriverThread()
{
	while(run)
	{
		// wait for parking spot
		// lock parking spot
			// wait for loading signal
			// drive away
		// unlock parking spot
		// sleep for long time
		// come back to truck waiting line
	}
}
#pragma endregion

#pragma region Spawner

void SpawnerThread(ProductPoint& spawner)
{		
	drawBlock(spawner);	
	while (run)
	{		
		if (!spawner.full)
		{			
			std::lock_guard<std::mutex> lock(spawner.m);
			spawner.symbol = '1';
			drawBlock(spawner);
			spawner.full = true;			
			(spawner.cv).notify_all();
		}		
		Sleep(1000);
	}
}

#pragma endregion

void initialize()
{
	_setmode(_fileno(stdout), _O_U16TEXT); // Unicode in console.
	initscr();
	start_color();
	init_pair(1, COLOR_BLUE, COLOR_BLACK);
	init_pair(2, COLOR_YELLOW, COLOR_BLACK);
	init_pair(3, COLOR_GREEN, COLOR_BLACK);
	init_pair(4, COLOR_RED, COLOR_BLACK);
}

int main()
{
	initialize();


	ProductPoint spawnerA;
	spawnerA.position.x = 3;
	spawnerA.position.y = 5;
	spawnerA.symbol = '1';
	spawnerA.colorIndex = 1;	
	ProductPoint spawnerB;
	spawnerB.position.x = 3;
	spawnerB.position.y = 7;
	spawnerB.symbol = '2';
	spawnerB.colorIndex = 2;
	ProductPoint spawnerC;
	spawnerC.position.x = 11;
	spawnerC.position.y = 6;
	spawnerC.symbol = '3';
	spawnerC.colorIndex = 3;

	ProductPoint receiverA;
	receiverA.position.x = 10;
	receiverA.position.y = 5;
	receiverA.symbol = 'O';
	receiverA.colorIndex = 1;
	ProductPoint receiverB;
	receiverB.position.x = 10;
	receiverB.position.y = 7;
	receiverB.symbol = 'O';
	receiverB.colorIndex = 2;
	ProductPoint receiverC;
	receiverC.position.x = 6;
	receiverC.position.y = 15;
	receiverC.symbol = 'O';
	receiverC.colorIndex = 3;

	Carrier carrierA('C', spawnerA.position.x + 1, spawnerA.position.y, 1, 800);
	Carrier carrierB('C', spawnerB.position.x + 1, spawnerB.position.y, 2, 500);

	std::thread spawnerThreadA(SpawnerThread, std::ref(spawnerA));
	std::thread spawnerThreadB(SpawnerThread, std::ref(spawnerB));
	std::thread carrierThreadA(CarrierThread, std::ref(carrierA), std::ref(spawnerA), std::ref(receiverA));
	std::thread carrierThreadB(CarrierThread, std::ref(carrierB), std::ref(spawnerB), std::ref(receiverB));
	//std::thread loaderThread(LoaderThread);
	std::thread assemblerThread(AssemblerThread, std::ref(receiverA), std::ref(receiverB), std::ref(spawnerC));

	spawnerThreadA.join();
	spawnerThreadB.join();
	carrierThreadA.join();
	carrierThreadB.join();
	assemblerThread.join();

	endwin();
	return 0;
}
