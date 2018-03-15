#include "EdgeBucket.h"

EdgeBucket::EdgeBucket(Vector2* a, Vector2* b) {
	Vector2 va = *a;
	Vector2 vb = *b;

	yMin = va[1];
	if (yMin > vb[1]) {
		yMin = vb[1];
		yMax = va[1];
	}
	else {
		yMax = vb[1];
	}

	x = yMin;

	dX = abs(va[0] - vb[0]);
	dY = abs(va[1] - vb[1]);

	sum = 0;
}