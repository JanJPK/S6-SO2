#pragma once
#ifndef ASSEMBLER_HPP
#define ASSEMBLER_HPP
#include "ProductPoint.h"
#include "Global.h"

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
				break;
			}

			case hasB:
			{
				std::lock_guard<std::mutex> lockGuard(spawner.m);
				spawner.ChangeState(true, '3');
				(spawner.cv).notify_all();
				state = hasNone;
				break;
			}
		}
		std::this_thread::sleep_for(std::chrono::milliseconds(waitTime));
	}
}
#endif
