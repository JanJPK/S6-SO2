// S6-SO2-3B.cpp : Defines the entry point for the console application.
//

#include "stdafx.h"
#include "curses.h"
#include <io.h>
#include <fcntl.h>
#include <cstdio>
#include <windows.h>
#include <mutex>
#include <thread>
#include <atomic>
#include <string>
#include <random>

#pragma region Coord

struct Coord
{
	int y;
	int x;
};

#pragma endregion

#pragma region Global

#pragma region LoadingZone

class LoadingZone
{
public:
	std::mutex m;
	std::condition_variable cv;
	bool occupied;
	Coord position;
};

#pragma endregion

std::atomic<int> atom;

bool run = true;
int timeScale = 1;

std::random_device rd;
std::mt19937 gen(rd());
std::uniform_int_distribution<> dis(1, 10);
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

std::mutex mCurses;

#define BACKGROUND ' '

void DrawBlock(Coord& coord, char symbol, int colorIndex)
{
	mCurses.lock();
	attron(COLOR_PAIR(colorIndex));
	move(coord.y, coord.x);
	addch(symbol);
	attroff(COLOR_PAIR(colorIndex));
	refresh();
	mCurses.unlock();
}

void DrawBlock(Drawable& drawable)
{
	mCurses.lock();
	attron(COLOR_PAIR(drawable.colorIndex));
	move(drawable.position.y, drawable.position.x);
	addch(drawable.symbol);
	attroff(COLOR_PAIR(drawable.colorIndex));
	refresh();
	mCurses.unlock();
}

void DrawBlockNoLock(Coord& coord, char symbol, int colorIndex)
{
	attron(COLOR_PAIR(colorIndex));
	move(coord.y, coord.x);
	addch(symbol);
	attroff(COLOR_PAIR(colorIndex));
	refresh();
}

void DrawBlockNoLock(Drawable& drawable)
{
	attron(COLOR_PAIR(drawable.colorIndex));
	move(drawable.position.y, drawable.position.x);
	addch(drawable.symbol);
	attroff(COLOR_PAIR(drawable.colorIndex));
	refresh();
}

void DrawString(int x, int y, std::string text, int colorIndex)
{
	mCurses.lock();
	attron(COLOR_PAIR(colorIndex));
	move(y, x);
	clrtoeol();
	addstr(text.c_str());
	attroff(COLOR_PAIR(colorIndex));
	refresh();
	mCurses.unlock();
}

void Draw(int x, int y, char symbol, int colorIndex)
{
	mCurses.lock();
	attron(COLOR_PAIR(colorIndex));
	move(y, x);
	addch(symbol);
	attroff(COLOR_PAIR(colorIndex));
	refresh();
	mCurses.unlock();
}

void UpdateAtom()
{
	mCurses.lock();
	attron(COLOR_PAIR(6));
	move(1, 6);
	std::string s = std::to_string(atom);
	addstr(s.c_str());
	attroff(COLOR_PAIR(6));
	refresh();
	mCurses.unlock();
}

void EraseBlock(Coord& coord)
{
	mCurses.lock();
	mvaddch(coord.y, coord.x, BACKGROUND);
	refresh();
	mCurses.unlock();
}

void EraseBlockNoLock(Coord& coord)
{
	mvaddch(coord.y, coord.x, BACKGROUND);
	refresh();
}

#pragma endregion

#pragma region ProductPoint

class ProductPoint : public Drawable
{
public:
	bool full;
	std::mutex m;
	std::condition_variable cv;

	void ChangeState(bool full, char symbol)
	{
		this->full = full;
		this->symbol = symbol;
		DrawBlock(*this);
	}
};

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
	DrawBlock(position, 'A', 3);
	DrawBlock(receiverA);
	DrawBlock(receiverB);

	int waitTime = 1500;
	DrawString(1, 18, "Assembler waiting for first part.", 3);
	while (run)
	{
		switch (state)
		{
			case hasNone:
			{								
				std::unique_lock<std::mutex> uniqueLock1(spawner.m);
				(spawner.cv).wait(uniqueLock1, [&spawner] { return !spawner.full; });
				std::unique_lock<std::mutex> uniqueLock2(receiverA.m);
				(receiverA.cv).wait(uniqueLock2, [&receiverA] { return receiverA.full; });
				spawner.ChangeState(false, '1');
				receiverA.ChangeState(false, 'O');
				(receiverA.cv).notify_all();
				state = hasA;
				DrawString(1, 18, "Assembler waiting for second part.", 3);
				break;
			}

			case hasA:
			{				
				std::unique_lock<std::mutex> uniqueLock(receiverB.m);
				(receiverB.cv).wait(uniqueLock, [&receiverB] { return receiverB.full; });
				spawner.ChangeState(false, '2');
				receiverB.ChangeState(false, 'O');
				(receiverB.cv).notify_all();
				state = hasB;		
				DrawString(1, 18, "Assembler combining parts.", 3);
				break;
			}

			case hasB:
			{				
				std::unique_lock<std::mutex> uniqueLock(spawner.m);
				spawner.ChangeState(true, '3');
				(spawner.cv).notify_all();
				state = hasNone;	
				DrawString(1, 18, "Assembler waiting for first part.", 3);
				break;
			}
		}
		std::this_thread::sleep_for(std::chrono::milliseconds(waitTime));
	}
}

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
	DrawBlock(carrier);	
	DrawString(1, 16, "Carrier waiting.", 3);

	int xText = spawner.symbol - '0';
	while (run)
	{
		switch (carrier.state)
		{
			case waiting:
			{
				
				std::unique_lock<std::mutex> uniqueLock(spawner.m);
				(spawner.cv).wait(uniqueLock, [&spawner] { return spawner.full; });
				spawner.full = false;
				DrawBlock(spawner.position, 'O', spawner.colorIndex);
				carrier.state = carrying;
				carrier.symbol = spawner.symbol;
				DrawBlock(carrier.position, carrier.symbol, carrier.colorIndex);
				DrawString(1, 15 + xText, "Carrier walking.", carrier.colorIndex);
				break;
			}

			case carrying:
			{
				
				if (carrier.position.x == receiver.position.x - 1)
				{
					std::unique_lock<std::mutex> uniqueLock(receiver.m);
					(receiver.cv).wait(uniqueLock, [&receiver] { return !receiver.full; });
					receiver.symbol = carrier.symbol;
					receiver.full = true;
					DrawBlock(receiver);
					carrier.state = returning;
					carrier.symbol = 'C';
					DrawBlock(carrier);
					(receiver.cv).notify_all();					
					atom++;
					UpdateAtom();
					DrawString(1, 15 + xText, "Carrier returning.", carrier.colorIndex);
				}
				else
				{
					EraseBlock(carrier.position);
					carrier.position.x++;
					DrawBlock(carrier);
				}				
				break;
			}

			case returning:
			{								
				if (carrier.position.x == spawner.position.x + 1)
				{
					carrier.state = waiting;
					DrawString(1, 15 + xText, "Carrier waiting.", carrier.colorIndex);
				}
				else
				{
					EraseBlock(carrier.position);
					carrier.position.x--;
					DrawBlock(carrier);
				}
				break;
			}
		}
		std::this_thread::sleep_for(std::chrono::milliseconds(carrier.waitTime));
	}
}

#pragma endregion

#pragma region Driver

class Truck : public Drawable
{
public:
	std::mutex m;
	std::condition_variable cv;
	bool loaded;
	bool ready;
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
		this->truck.loaded = false;
		this->truck.ready = false;
	}
};

enum DriverState
{
	queued,
	loading,
	leaving
};

void DriveAway(Driver& driver)
{
	for(int i = 0; i < 12; i++)
	{
		mCurses.lock();
		EraseBlockNoLock(driver.truck.position);
		driver.position.x++;
		driver.truck.position.x++;		
		DrawBlockNoLock(driver);
		DrawBlockNoLock(driver.truck);
		mCurses.unlock();
		std::this_thread::sleep_for(std::chrono::milliseconds(100));
	}
	mCurses.lock();
	EraseBlockNoLock(driver.position);
	EraseBlockNoLock(driver.truck.position);
	mCurses.unlock();
}

void DriverThread(Driver& driver, Driver*& loadingDriver, LoadingZone& lz)
{
	mCurses.lock();
	DrawBlockNoLock(driver);
	DrawBlockNoLock(driver.truck);
	mCurses.unlock();

	while (run)
	{
		std::unique_lock<std::mutex> uniqueLock1(lz.m);
		(lz.cv).wait(uniqueLock1, [&lz] { return !lz.occupied; });
		lz.occupied = true;
		loadingDriver = &driver;
		uniqueLock1.unlock();
		lz.cv.notify_all();

		std::unique_lock<std::mutex> uniqueLock2(driver.truck.m);
		(driver.truck.cv).wait(uniqueLock2, [&driver] { return driver.truck.loaded; });
		
		std::this_thread::sleep_for(std::chrono::milliseconds(10000));
		driver.position.x = driver.waitingPosition.x;
		driver.position.y = driver.waitingPosition.y;
		driver.truck.position.x = driver.position.x - 1;
		driver.truck.position.y = driver.position.y;

		mCurses.lock();
		DrawBlockNoLock(driver);
		DrawBlockNoLock(driver.truck);
		mCurses.unlock();
	}
}

#pragma endregion

#pragma region Loader

void LoaderThread(Driver*& driver, LoadingZone& lz, ProductPoint& spawner)
{
	Coord position;
	position.x = spawner.position.x + 1;
	position.y = spawner.position.y;
	DrawBlock(position, 'L', 5);

	DrawString(1, 19, "Loader waiting for truck.", 5);
	while (run)
	{
		std::unique_lock<std::mutex> uniqueLock1(lz.m);
		(lz.cv).wait(uniqueLock1, [&lz] { return lz.occupied; });

		EraseBlock((*driver).position);
		EraseBlock((*driver).truck.position);
		(*driver).position.x = lz.position.x;
		(*driver).position.y = lz.position.y;
		(*driver).truck.position.x = (*driver).position.x - 1;
		(*driver).truck.position.y = (*driver).position.y;
		DrawBlock((*driver));
		DrawBlock((*driver).truck);

		std::lock_guard<std::mutex> lockGuard((*driver).truck.m);
		
		for (int i = 1; i <= 3; i++)
		{						
			DrawString(1, 19, "Loader waiting for assembler.", 5);
			std::unique_lock<std::mutex> uniqueLock2(spawner.m);
			(spawner.cv).wait(uniqueLock2, [&spawner] { return spawner.full; });
			DrawString(1, 19, "Loader loading product.", 5);
			spawner.ChangeState(false, 'O');
			(*driver).truck.symbol = i + '0';
			DrawBlock((*driver).truck);
			uniqueLock2.unlock();
			(spawner.cv).notify_all();

			std::this_thread::sleep_for(std::chrono::milliseconds(200));	
		}
		DrawString(1, 19, "Loader waiting for next truck.", 5);

		(*driver).truck.loaded = true;
		(*driver).truck.symbol = '0';
		(*driver).truck.cv.notify_all();
		
		DriveAway(*driver);

		uniqueLock1.unlock();
		lz.occupied = false;
		(lz.cv).notify_one();
	}
}

#pragma endregion

#pragma region Spawner

void SpawnerThread(ProductPoint& spawner)
{
	DrawBlock(spawner);
	while (run)
	{
		if (!spawner.full)
		{
			std::lock_guard<std::mutex> lock(spawner.m);
			DrawBlock(spawner);
			spawner.full = true;
			(spawner.cv).notify_all();
		}
		std::this_thread::sleep_for(std::chrono::milliseconds(dis(gen) * 100));
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
	init_pair(5, COLOR_CYAN, COLOR_BLACK);
	init_pair(6, COLOR_WHITE, COLOR_BLACK);
}

int main()
{
	initialize();
	DrawString(1, 0, "Parts carried (atomic<int>):", 6);

	ProductPoint spawnerA;
	spawnerA.full = false;
	spawnerA.position.x = 3;
	spawnerA.position.y = 5;
	spawnerA.symbol = '1';
	spawnerA.colorIndex = 1;
	ProductPoint spawnerB;
	spawnerB.full = false;
	spawnerB.position.x = 3;
	spawnerB.position.y = 7;
	spawnerB.symbol = '2';
	spawnerB.colorIndex = 2;
	ProductPoint spawnerC;
	spawnerC.full = false;
	spawnerC.position.x = 11;
	spawnerC.position.y = 6;
	spawnerC.symbol = '3';
	spawnerC.colorIndex = 3;

	ProductPoint receiverA;
	receiverA.full = false;
	receiverA.position.x = 10;
	receiverA.position.y = 5;
	receiverA.symbol = 'O';
	receiverA.colorIndex = 1;
	ProductPoint receiverB;
	receiverB.full = false;
	receiverB.position.x = 10;
	receiverB.position.y = 7;
	receiverB.symbol = 'O';
	receiverB.colorIndex = 2;
	ProductPoint receiverC;
	receiverC.full = false;
	receiverC.position.x = 6;
	receiverC.position.y = 15;
	receiverC.symbol = 'O';
	receiverC.colorIndex = 3;

	LoadingZone loadingZone;
	loadingZone.occupied = false;
	loadingZone.position.x = 14;
	loadingZone.position.y = 6;

	Carrier carrierA('C', spawnerA.position.x + 1, spawnerA.position.y, 1, 500);
	Carrier carrierB('C', spawnerB.position.x + 1, spawnerB.position.y, 2, 300);
	Driver driverA('A', 10, 11, 4, 1000);
	Driver driverB('B', 10, 12, 4, 1000);
	Driver driverC('C', 10, 13, 4, 1000);
	Driver driverD('D', 10, 14, 4, 1000);
	Driver* loadingDriver;

	std::thread carrierThreadA(CarrierThread, std::ref(carrierA), std::ref(spawnerA), std::ref(receiverA));
	std::thread carrierThreadB(CarrierThread, std::ref(carrierB), std::ref(spawnerB), std::ref(receiverB));
	std::thread assemblerThread(AssemblerThread, std::ref(receiverA), std::ref(receiverB), std::ref(spawnerC));
	std::thread driverThreadA(DriverThread, std::ref(driverA), std::ref(loadingDriver), std::ref(loadingZone));
	std::thread driverThreadB(DriverThread, std::ref(driverB), std::ref(loadingDriver), std::ref(loadingZone));
	std::thread driverThreadC(DriverThread, std::ref(driverC), std::ref(loadingDriver), std::ref(loadingZone));
	std::thread driverThreadD(DriverThread, std::ref(driverD), std::ref(loadingDriver), std::ref(loadingZone));
	std::thread loaderThread(LoaderThread, std::ref(loadingDriver), std::ref(loadingZone), std::ref(spawnerC));
	std::thread spawnerThreadA(SpawnerThread, std::ref(spawnerA));
	std::thread spawnerThreadB(SpawnerThread, std::ref(spawnerB));

	spawnerThreadA.join();
	spawnerThreadB.join();
	carrierThreadA.join();
	carrierThreadB.join();
	assemblerThread.join();
	driverThreadA.join();
	driverThreadB.join();
	driverThreadC.join();
	driverThreadD.join();
	loaderThread.join();

	endwin();
	return 0;
}
