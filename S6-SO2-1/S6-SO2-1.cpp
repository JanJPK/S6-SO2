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

#define SNAKE 'X'
#define BACKGROUND ' '
#define WALL L'\u2588'

#pragma region Variables

// Definition of snake size and level area
const int snakeSize = 5;
const int width = 15;
const int height = 10;

// Positions of walls 
const int hWall1 = 0;
const int hWall2 = height + 1;
const int hWall3 = height * 2 + 2;
const int vWall1 = 0;
const int vWall2 = width + 1;
const int vWall3 = width * 2 + 2;

enum Direction
{
	left, right, up, down
};
Direction directions[] = { left, right, up, down };

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

// Parts of snake
struct Coord
{

	int y;
	int x;

};


std::mutex pdcursesGuard;

#pragma endregion

#pragma region Snake

class Snake
{

public:
	std::vector<Coord> parts;

};

#pragma endregion

#pragma region Drawing

void buildHorizontalWall(int y, int length)
{
	mvhline(y, 0, WALL, length);
	for(int i = 0; i < length; i++)
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
	addch(SNAKE);
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

void drawSnake(Snake& snake)
{
	for (int i = 0; i < snakeSize; i++)
	{
		DrawBlock(snake.parts[i]);
	}
}

#pragma endregion

#pragma region Snake Control

/*
*	1	Choose direction
*	2	Push all snake parts
*			Make space for new head
*			Remove tail
*/

Direction getDirection()
{
	return directions[randomDirection(rng)];
}

void eraseTail(Snake& snake)
{
	EraseBlock(snake.parts[snakeSize - 1]);
}

void drawHead(Snake& snake)
{
	DrawBlock(snake.parts[0]);
}

void pushSnakeParts(Snake& snake)
{
	for (int i = snakeSize - 1; i > 0; i--)
	{
		snake.parts[i] = snake.parts[i - 1];
	}
}

void addHead(Snake& snake)
{
	int critical = 0;
	while (true)
	{
		critical++;
		Coord newHead = snake.parts[0];
		switch (getDirection())
		{
		case left:
			newHead.x--;
			break;
		case right:
			newHead.x++;
			break;
		case up:
			newHead.y--;
			break;
		case down:
			newHead.y++;
			break;
		}

		if (newHead.y > hWall3 || newHead.x > vWall3)
			continue;
		if (tiles[newHead.y][newHead.x] != empty)
			continue;

		snake.parts[0] = newHead;
		return;
	}
}

void moveSnake(Snake& snake)
{	
	eraseTail(snake);
	pushSnakeParts(snake);
	addHead(snake);
	drawHead(snake);	
}

void createSnakeParts(Snake& snake, int y, int x)
{
	for (int i = 0; i < snakeSize; i++)
	{
		Coord part;
		part.y = y;
		part.x = x + i;
		snake.parts.push_back(part);
	}
}

void snakeThread(Snake& snake)
{
	drawSnake(snake);

	while (true)
	{
		Sleep(300);
		moveSnake(snake);
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
	drawWalls();

	Snake snake1;
	createSnakeParts(snake1, hWall2 - 1, vWall1 + 1);
	std::thread thread1(snakeThread, std::ref(snake1));

	Snake snake2;
	createSnakeParts(snake2, hWall2 - 1, vWall2 + 1);
	std::thread thread2(snakeThread, std::ref(snake2));

	Snake snake3;
	createSnakeParts(snake3, hWall3 - 1, vWall1 + 1);
	std::thread thread3(snakeThread, std::ref(snake3));

	Snake snake4;
	createSnakeParts(snake4, hWall3 - 1, vWall2 + 1);
	std::thread thread4(snakeThread, std::ref(snake4));

	thread1.join();
	thread2.join();
	thread3.join();
	thread4.join();

	getch();
	endwin();
	return 0;
}

