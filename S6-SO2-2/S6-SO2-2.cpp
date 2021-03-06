// S6-SO2-1.cpp : Defines the entry point for the console application.
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

#define BLOCK 'X'
#define BACKGROUND ' '
#define WALL L'\u2588'

#pragma region Constants

// Definition of level area
const int width = 15;
const int height = 10;
const int startZoneLeft = 1;
const int startZoneRight = width;

// Positions of walls 
const int hWall1 = 0;
const int hWall2 = height + 1;
const int hWall3 = height * 2 + 2;
const int vWall1 = 0;
const int vWall2 = width + 1;
const int vWall3 = width * 2 + 2;

#pragma endregion

#pragma region Variables

// 2D array describing level
enum Tile
{
	empty, block, wall
};
Tile tiles[height * 2 + 3][width * 2 + 3];

// RNG
static std::random_device source;
static std::mt19937 rng(source());
static std::uniform_int_distribution<int> randomDirection(0, 3);

// Coordinate of drawn items
struct Coord
{

	int y;
	int x;

};

std::mutex pdcursesGuard;
std::mutex blockQueueGuard;
std::condition_variable blockQueueConditionVariable;

#pragma endregion

class Block
{
public:
	Coord position;
	bool visibility;
};

#pragma region Drawing

void buildHorizontalWall(int y, int length)
{
	mvhline(y, 0, WALL, length);
	for (int i = 0; i < length; i++)
	{
		tiles[y][i] = wall;
	}
	Sleep(500);
	refresh();
}

void buildVerticalWall(int x, int length)
{
	mvvline(0, x, WALL, length);
	for (int i = 0; i < length; i++)
	{
		tiles[i][x] = wall;
	}
	Sleep(500);
	refresh();
}

void drawWalls()
{
	const int totalWidth = width * 2 + 3;
	const int totalHeight = height * 2 + 3;
	attron(COLOR_PAIR(1));
	buildHorizontalWall(hWall1, totalWidth);
	buildHorizontalWall(hWall2, totalWidth);
	buildHorizontalWall(hWall3, totalWidth);
	buildVerticalWall(vWall1, totalHeight);
	buildVerticalWall(vWall2, totalHeight);
	buildVerticalWall(vWall3, totalHeight);
	attroff(COLOR_PAIR(1));
}

void DrawBlock(Coord& coord)
{
	pdcursesGuard.lock();
	attron(COLOR_PAIR(2));
	tiles[coord.y][coord.x] = block;
	move(coord.y, coord.x);
	addch(BLOCK);
	attroff(COLOR_PAIR(2));
	refresh();
	pdcursesGuard.unlock();
}

void EraseBlock(Coord& coord)
{
	pdcursesGuard.lock();
	tiles[coord.y][coord.x] = empty;
	mvaddch(coord.y, coord.x, BACKGROUND);
	refresh();
	pdcursesGuard.unlock();
}

#pragma endregion

#pragma region Block Generator

std::vector<Block> blocks;
bool generate = true;
int blocksInQueue;

void MoveBlocks()
{
	int loopBound = blocks.size();
	int i = 0;
	while(i < loopBound)
	{
		Block& block = blocks[i];
		EraseBlock(block.position);
		block.position.y++;
		if (block.position.y == hWall2)
		{
			blocks.erase(blocks.begin());
			loopBound--;	

			std::lock_guard<std::mutex> lock(blockQueueGuard);
			blocksInQueue++;
			blockQueueConditionVariable.notify_all();
		}
		else
		{
			DrawBlock(block.position);
			i++;
		}
	}
}

void GenerateBlock(int x)
{
	Block block;
	block.position.y = 1;
	block.position.x = x;	
	blocks.push_back(block);
	DrawBlock(block.position);
}

void BlockGeneratorThread()
{
	const std::uniform_int_distribution<int> random(startZoneLeft, startZoneRight);
	const int maxBlocks = 3 * width * height;
	int currentBlocks = 0;
	blocksInQueue = 0;		

	while(currentBlocks < maxBlocks + height)
	{
		if (generate)
		{
			if(currentBlocks < maxBlocks)
			{
				GenerateBlock(random(rng));				
			}			
			currentBlocks++;
		}
		else
		{
			MoveBlocks();
		}
		generate = !generate;
		Sleep(10);
	}
}

#pragma endregion

#pragma region Block Grabber

std::vector<Coord> CreateEmptyTileVector(int xOffset, int yOffset)
{
	std::vector<Coord> emptyTiles;
	for(int i = 0; i < height; i++)
	{
		for(int j = 0; j < width; j++)
		{
			Coord newCoord;
			newCoord.x = xOffset + j;
			newCoord.y = yOffset + i;
			emptyTiles.push_back(newCoord);
		}
	}

	std::random_shuffle(emptyTiles.begin(), emptyTiles.end());

	// Checking if it covers the space appropriately
	//for(int i = 0; i < emptyTiles.size(); i++)
	//{
	//	DrawBlock(emptyTiles[i]);		
	//}

	return emptyTiles;
}

void BlockGrabberThread(Coord startPosition)
{
	auto emptyTiles = CreateEmptyTileVector(startPosition.x, startPosition.y);
	
	int i = 0;
	while(i < emptyTiles.size())
	{		
		std::unique_lock<std::mutex> uniqueLock(blockQueueGuard);
		blockQueueConditionVariable.wait(uniqueLock, [] {return blocksInQueue > 0; });
		blocksInQueue--;
		DrawBlock(emptyTiles[i]);
		i++;
	}
}
#pragma endregion

#pragma region Initialize

void initialize()
{
	_setmode(_fileno(stdout), _O_U16TEXT); // Unicode in console.
	initscr();
	start_color();
	init_pair(1, COLOR_RED, COLOR_BLACK);
	init_pair(2, COLOR_GREEN, COLOR_BLACK);
}

#pragma endregion

int main()
{
	initialize();

	char decision = 'a';
	while(decision != 'x')
	{
		drawWalls();
		std::thread thread1(BlockGeneratorThread);

		Coord start1;
		start1.x = 1;
		start1.y = hWall2 + 1;
		std::thread thread2(BlockGrabberThread, std::ref(start1));
		Coord start2;
		start2.x = vWall2 + 1;
		start2.y = 1;
		std::thread thread3(BlockGrabberThread, std::ref(start2));
		Coord start3;
		start3.x = vWall2 + 1;
		start3.y = hWall2 + 1;
		std::thread thread4(BlockGrabberThread, std::ref(start3));

		thread1.join();
		thread2.join();
		thread3.join();
		thread4.join();

		decision = getch();
		clear();
	}

	endwin();
	return 0;
}

