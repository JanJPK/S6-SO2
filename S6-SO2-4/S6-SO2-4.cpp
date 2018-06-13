// S6-SO2-4.cpp : Defines the entry point for the console application.
//
#include "curses.h"
#include <io.h>
#include <fcntl.h>
#include <cstdio>
#include <windows.h>
#include <mutex>
#include <thread>
#include "Carrier.h"
#include "Spawner.h"
#include "Assembler.h"
#include "Driver.h"


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
	loadingZone.position.x = 13;
	loadingZone.position.y = 7;

	Carrier carrierA('C', spawnerA.position.x + 1, spawnerA.position.y, 1, 800);
	Carrier carrierB('C', spawnerB.position.x + 1, spawnerB.position.y, 2, 500);
	Driver driverA('A', 10, 10, 4, 1000);
	Driver driverB('B', 10, 10, 4, 1000);
	Driver driverC('C', 10, 10, 4, 1000);
	Driver driverD('D', 10, 10, 4, 1000);

	std::thread carrierThreadA(CarrierThread, std::ref(carrierA), std::ref(spawnerA), std::ref(receiverA));
	std::thread carrierThreadB(CarrierThread, std::ref(carrierB), std::ref(spawnerB), std::ref(receiverB));
	std::thread assemblerThread(AssemblerThread, std::ref(receiverA), std::ref(receiverB), std::ref(spawnerC));
	std::thread driverThreadA(DriverThread, std::ref(driverA), std::ref(loadingZone));
	std::thread driverThreadB(DriverThread, std::ref(driverB), std::ref(loadingZone));
	std::thread driverThreadC(DriverThread, std::ref(driverC), std::ref(loadingZone));
	std::thread driverThreadD(DriverThread, std::ref(driverD), std::ref(loadingZone));
	std::thread spawnerThreadA(SpawnerThread, std::ref(spawnerA));
	std::thread spawnerThreadB(SpawnerThread, std::ref(spawnerB));
	spawnerThreadA.join();
	spawnerThreadB.join();
	carrierThreadA.join();
	carrierThreadB.join();
	assemblerThread.join();

	endwin();
	return 0;
}
