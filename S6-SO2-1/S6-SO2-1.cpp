// S6-SO2-1.cpp : Defines the entry point for the console application.
//

#include "stdafx.h"
#include "curses.h"
#include <io.h>
#include <fcntl.h>

#define SNAKE 'X'
#define BACKGROUND ' '
#define WALL L'\u2588'

enum Direction
{
	up, down, left, right
};

const int snakeSize = 5;
const int width = 15;
const int height = 10;

// Positions of walls, needed for drawing them and checking snake collisions.
const int hBar1 = 0;
const int hBar2 = height + 1;
const int hBar3 = height * 2 + 2;
const int vBar1 = 0;
const int vBar2 = width + 1;
const int vBar3 = width * 2 + 2;

#pragma region Snake

class Snake
{
	struct SnakePart
	{
		int x;
		int y;
	};

public:
	int offsetX;
	int offsetY;
	SnakePart* parts;

	Snake(int offsetX, int offsetY, int size)
	{
		this->offsetX = offsetX;
		this->offsetY = offsetY;
		parts = new SnakePart[size];
	}
};

#pragma endregion

#pragma region Snake Control

/*
 *	1	Choose direction
 *	2	Push all snake parts
 *			Make space for new head
 *			Remove tail
 */


Direction getDirection(Snake snake)
{
	return up;
	// TODO: randomly get a direction and make sure snake can do it
}

void removeTail(Snake snake)
{
	mvaddch(snake.parts[snakeSize].x, snake.parts[snakeSize].x, BACKGROUND);
}

void pushSnakeParts(Snake snake)
{
	for(int i = 1; i < snakeSize - 1; i++)
	{
		snake.parts[i] = snake.parts[i - 1];
	}

	snake.parts[0].x = snake.parts[1].x;
	snake.parts[0].y = snake.parts[1].y;
}

void addHead(Snake snake, Direction direction)
{
	switch (direction)
	{
	case up:
		snake.parts[0].y--;
		break;
	case down:
		snake.parts[0].y++;
		break;
	case left:
		snake.parts[0].x--;
		break;
	case right:
		snake.parts[0].x++;
		break;
	}
}



void refreshSnake(Snake snake)
{
	Direction direction = getDirection(snake);
	removeTail(snake);
	pushSnakeParts(snake);
	addHead(snake, direction);
}

#pragma endregion

#pragma region Drawing

void drawGrid()
{
	const int totalWidth = width * 2 + 3;
	const int totalHeight = height * 2 + 3;
	attron(COLOR_PAIR(1));
	mvhline(hBar1, 0, WALL, totalWidth);
	mvhline(hBar2, 0, WALL, totalWidth);
	mvhline(hBar3, 0, WALL, totalWidth);
	mvvline(0, vBar1, WALL, totalHeight);
	mvvline(0, vBar2, WALL, totalHeight);
	mvvline(0, vBar3, WALL, totalHeight);
	attroff(COLOR_PAIR(1));
}

void drawSnake(Snake snake)
{
	attron(COLOR_PAIR(2));
	for(int i = 0; i < snakeSize; i++ )
	{
		move(snake.parts[i].x, snake.parts[i].y);
		addch(SNAKE);
	}
	attroff(COLOR_PAIR(2));
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
	drawGrid();

	getch();
	endwin();
    return 0;
}

