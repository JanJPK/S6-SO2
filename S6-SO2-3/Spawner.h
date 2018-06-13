#pragma once
#ifndef SPAWNER_HPP
#define SPAWNER_HPP
#include "ProductPoint.h"

void SpawnerThread(ProductPoint& spawner)
{
	DrawBlock(spawner);
	while (run)
	{
		if (!spawner.full)
		{
			std::lock_guard<std::mutex> lock(spawner.m);
			spawner.symbol = '1';
			DrawBlock(spawner);
			spawner.full = true;
			(spawner.cv).notify_all();
		}
		std::this_thread::sleep_for(std::chrono::milliseconds(1000));
	}
}
#endif