#pragma once
#ifndef LOADER_HPP
#define LOADER_HPP
#include "Global.h"
#include "LoadingZone.h"

void LoaderThread(LoadingZone& lz)
{
	while (run)
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
#endif
