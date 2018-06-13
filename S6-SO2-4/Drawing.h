#pragma once
#ifndef DRAWING_HPP
#define DRAWING_HPP 
#include <curses.h>
#include "Drawable.h"
#include <mutex>

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

void EraseBlock(Coord& coord)
{
	mCurses.lock();
	mvaddch(coord.y, coord.x, BACKGROUND);
	refresh();
	mCurses.unlock();
}

#endif 