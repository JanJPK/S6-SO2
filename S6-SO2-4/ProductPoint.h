#pragma once
#ifndef PRODUCTPOINT_HPP
#define PRODUCTPOINT_HPP
#include <mutex>
#include "Drawing.h"

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
#endif
