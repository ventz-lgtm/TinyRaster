#pragma once

#include "Vector2.h"

class EdgeBucket {

public:
	EdgeBucket(Vector2* a, Vector2* b);
	int yMax;
	int yMin;
	int x;
	int sign;
	int dX;
	int dY;
	int sum;
};