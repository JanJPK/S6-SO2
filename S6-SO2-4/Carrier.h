#pragma once
#ifndef CARRIER_HPP
#define CARRIER_HPP
#include <mutex>
#include "ProductPoint.h"
#include "Global.h"

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
			DrawBlock(spawner);
			carrier.state = carrying;
			carrier.symbol = '1';
			DrawBlock(carrier.position, carrier.symbol, carrier.colorIndex);
			break;
		}

		case carrying:
		{
			if (carrier.position.x == receiver.position.x - 1)
			{
				std::unique_lock<std::mutex> uniqueLock(receiver.m);
				(receiver.cv).wait(uniqueLock, [&receiver] {return !receiver.full; });
				receiver.symbol = carrier.symbol;
				receiver.full = true;
				DrawBlock(receiver);
				carrier.state = returning;
				carrier.symbol = 'C';
				DrawBlock(carrier);
				(receiver.cv).notify_all();
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
#endif