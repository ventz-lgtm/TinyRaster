/*---------------------------------------------------------------------
*
* Copyright © 2016  Minsi Chen
* E-mail: m.chen@derby.ac.uk
*
* The source is written for the Graphics I and II modules. You are free
* to use and extend the functionality. The code provided here is functional
* however the author does not guarantee its performance.
---------------------------------------------------------------------*/
#include <algorithm>
#include <math.h>
#include <iostream>

#include "Rasterizer.h"
#include "ColourUtil.h"

using namespace ColourUtil;

Rasterizer::Rasterizer(void)
{
	mFramebuffer = NULL;
	mScanlineLUT = NULL;
}

void Rasterizer::ClearScanlineLUT()
{
	Scanline *pScanline = mScanlineLUT;

	for (int y = 0; y < mHeight; y++)
	{
		(pScanline + y)->clear();
		(pScanline + y)->shrink_to_fit();
	}
}

unsigned int Rasterizer::ComputeOutCode(const Vector2 & p, const ClipRect& clipRect)
{
	unsigned int CENTRE = 0x0;
	unsigned int LEFT = 0x1;
	unsigned int RIGHT = 0x1 << 1;
	unsigned int BOTTOM = 0x1 << 2;
	unsigned int TOP = 0x1 << 3;
	unsigned int outcode = CENTRE;
	
	if (p[0] < clipRect.left)
		outcode |= LEFT;
	else if (p[0] >= clipRect.right)
		outcode |= RIGHT;

	if (p[1] < clipRect.bottom)
		outcode |= BOTTOM;
	else if (p[1] >= clipRect.top)
		outcode |= TOP;

	return outcode;
}

bool Rasterizer::ClipLine(const Vertex2d & v1, const Vertex2d & v2, const ClipRect& clipRect, Vector2 & outP1, Vector2 & outP2)
{
	//TODO: EXTRA This is not directly prescribed as an assignment exercise. 
	//However, if you want to create an efficient and robust rasteriser, clipping is a usefull addition.
	//The following code is the starting point of the Cohen-Sutherland clipping algorithm.
	//If you complete its implementation, you can test it by calling prior to calling any DrawLine2D .

	const Vector2 p1 = v1.position;
	const Vector2 p2 = v2.position;
	unsigned int outcode1 = ComputeOutCode(p1, clipRect);
	unsigned int outcode2 = ComputeOutCode(p2, clipRect);

	outP1 = p1;
	outP2 = p2;
	
	bool draw = false;

	return true;
}

void Rasterizer::WriteRGBAToFramebuffer(int x, int y, const Colour4 & colour)
{
	if (x >= mWidth || y >= mHeight)
	{
		return;
	}

	PixelRGBA *pixel = mFramebuffer->GetBuffer();
	
	pixel[y*mWidth + x] = colour;
}

Rasterizer::Rasterizer(int width, int height)
{
	//Initialise the rasterizer to its initial state
	mFramebuffer = new Framebuffer(width, height);
	mScanlineLUT = new Scanline[height];
	mWidth = width;
	mHeight = height;

	mBGColour.SetVector(0.0, 0.0, 0.0, 1.0);	//default bg colour is black
	mFGColour.SetVector(1.0, 1.0, 1.0, 1.0);    //default fg colour is white

	mGeometryMode = LINE;
	mFillMode = UNFILLED;
	mBlendMode = NO_BLEND;

	SetClipRectangle(0, mWidth, 0, mHeight);
}

Rasterizer::~Rasterizer()
{
	delete mFramebuffer;
	delete[] mScanlineLUT;
}

void Rasterizer::Clear(const Colour4& colour)
{
	PixelRGBA *pixel = mFramebuffer->GetBuffer();

	SetBGColour(colour);

	int size = mWidth*mHeight;
	
	for(int i = 0; i < size; i++)
	{
		//fill all pixels in the framebuffer with background colour
		*(pixel + i) = mBGColour;
	}
}

void Rasterizer::DrawPoint2D(const Vector2& pt, int size)
{
	int x = pt[0];
	int y = pt[1];
	
	if (x < 0 || y < 0) { return; }

	if (mBlendMode == Rasterizer::NO_BLEND) {
		WriteRGBAToFramebuffer(x, y, mFGColour);
	}
	else if (mBlendMode == Rasterizer::ALPHA_BLEND) {
		// Retrieve current colour of frame buffer at location

		PixelRGBA *pixel = mFramebuffer->GetBuffer();
		Colour4 c = pixel[y*mWidth + x];

		// Write the interpolated alpha blend with the two colours instead
		WriteRGBAToFramebuffer(x, y, ColourUtil::Interpolate(c, mFGColour, mFGColour[3]));
	}
}

void Rasterizer::DrawLine2D(const Vertex2d & v1, const Vertex2d & v2, int thickness)
{
	Vector2 pt1 = v1.position;
	Vector2 pt2 = v2.position;

	bool swap_x = pt2[0] < pt1[0];
	bool swap_y = pt2[1] < pt1[1];

	float dx = swap_x ? pt1[0] - pt2[0] : pt2[0] - pt1[0];
	float dy = swap_x ? pt1[1] - pt2[1] : pt2[1] - pt1[1];

	int reflect = dy < 0 ? -1 : 1;
	bool swap_xy = dy*reflect > dx;
	int epsilon = 0;

	int sx = swap_xy ? reflect < 0 ? swap_x ? pt1[1] : pt2[1] : swap_x ? pt2[1] : pt1[1] : swap_x ? pt2[0] : pt1[0];
	int sy = swap_xy ? reflect < 0 ? swap_x ? pt1[0] : pt2[0] : swap_x ? pt2[0] : pt1[0] : swap_x ? pt2[1] : pt1[1];
	int ex = swap_xy ? reflect < 0 ? swap_x ? pt2[1] : pt1[1] : swap_x ? pt1[1] : pt2[1] : swap_x ? pt1[0] : pt2[0];

	int y = sy;
	int x = sx;
	y *= reflect;

	Vector2 temp;

	while (x <= ex)
	{
		temp[0] = swap_xy ? y*reflect : x;
		temp[1] = swap_xy ? x : y*reflect;

		Colour4 colour;

		// Interpolated fill
		if (mFillMode == Rasterizer::INTERPOLATED_FILLED) {
			float diff = swap_xy ? abs(dy) : abs(dx);
			int curX = x - sx;

			float i = abs((curX / diff) * reflect);
			
			if (swap_x) {
				i = 1 - i;
			}

			if ((swap_xy && !swap_y && swap_x) || (swap_xy && swap_y && !swap_x)) {
				colour = ColourUtil::Interpolate(v2.colour, v1.colour, i);
			}
			else {
				colour = ColourUtil::Interpolate(v1.colour, v2.colour, i);
			}
		}
		else {
			colour = v1.colour;
		}

		SetFGColour(colour);
		DrawPoint2D(temp);

		// Line thickness
		if (thickness > 1) {
			float dt = abs(dx) + abs(dy);

			for (int i = 1; i < thickness; i += 2) {
				float xp = dx / dt;
				float yp = dy / dt;

				int tx = -((i / 2) * yp);
				int ty = ((i / 2) * xp);

				if (swap_y) {
					int newY = temp[1] - ty;
					int newX = temp[0] - tx;

					Vector2 temp2(newX, newY);
					DrawPoint2D(temp2);

					newY = temp[1] + ty;
					newX = temp[0] + tx;

					Vector2 temp3(newX, newY);
					DrawPoint2D(temp3);
				}
				else {

					int newY = temp[1] + ty;
					int newX = temp[0] + tx;

					Vector2 temp2(newX, newY);
					DrawPoint2D(temp2);

					newY = temp[1] - ty;
					newX = temp[0] - tx;

					Vector2 temp3(newX, newY);
					DrawPoint2D(temp3);
				}
				
			}
		}

		epsilon += swap_xy ? dx : dy*reflect;

		if ((epsilon << 1) >= (swap_xy ? dy*reflect : dx))
		{
			y++;

			epsilon -= swap_xy ? dy*reflect : dx;
		}
		x++;
	}
}

void Rasterizer::DrawUnfilledPolygon2D(const Vertex2d * vertices, int count)
{
	for (int i = 0; i < count - 1; i++) {
		DrawLine2D(vertices[i], vertices[i + 1]);
	}

	DrawLine2D(vertices[0], vertices[count - 1]);
}

struct less_than_key
{
	inline bool operator() (const ScanlineLUTItem& struct1, const ScanlineLUTItem& struct2)
	{
		return (struct1.pos_x < struct2.pos_x);
	}
};

void Rasterizer::ScanlineFillPolygon2D(const Vertex2d * vertices, int count)
{
	//TODO:
	//Ex 2.2 Implement the Rasterizer::ScanlineFillPolygon2D method method so that it is capable of drawing a solidly filled polygon.
	//Note: You can implement floodfill for this exercise however scanline fill is considered a more efficient and robust solution.
	//		You should be able to reuse DrawUnfilledPolygon2D here.
	//
	//Use Test 4 (Press F4) to test your solution, this is a simple test case as all polygons are convex.
	//Use Test 5 (Press F5) to test your solution, this is a complex test case with one non-convex polygon.

	//Ex 2.3 Extend Rasterizer::ScanlineFillPolygon2D method so that it is capable of alpha blending, i.e. draw translucent polygons.
	//Note: The variable mBlendMode indicates if the blend mode is set to alpha blending.
	//To do alpha blending during filling, the new colour of a point should be combined with the existing colour in the framebuffer using the alpha value.
	//Use Test 6 (Press F6) to test your solution
	
	int minShapeY = INT_MAX;
	int maxShapeY = INT_MIN;
	int minShapeX = INT_MAX;
	int maxShapeX = INT_MIN;

	SetFGColour(vertices[0].colour);

	for (int i = 0; i < count; i++) {
		if (minShapeY > vertices[i].position[1]) {
			minShapeY = vertices[i].position[1];
		}
		else if (maxShapeY < vertices[i].position[1]) {
			maxShapeY = vertices[i].position[1];
		}

		if (minShapeX > vertices[i].position[0]) {
			minShapeX = vertices[i].position[0];
		}
		else if (maxShapeX < vertices[i].position[0]) {
			maxShapeX = vertices[i].position[0];
		}
	}

	ScanlineLUTItem l;
	Vector2 point;

	for (int y = minShapeY; y < maxShapeY; y++) {
		if (y < 0) { y = 0;  continue; }

		std::vector<ScanlineLUTItem> *lutTable = new std::vector<ScanlineLUTItem>;

		for (int v = 0; v < count;) {
			Vector2 v1 = vertices[v++].position;
			Vector2 v2;
			
			if (v == count) {
				v2 = vertices[0].position;
			}
			else {
				v2 = vertices[v].position;
			}
			
			
			if((y >= v1[1] && y < v2[1]) || (y >= v2[1] && y < v1[1])){
				int x = v1[0] + (y - v1[1]) * ((v1[0] - v2[0]) / (v1[1] - v2[1]));

				l.colour = Colour4();
				l.pos_x = x;
				lutTable->push_back(l);
			}
		}

		std::sort(lutTable->begin(), lutTable->end(), less_than_key());

		if (lutTable->size() != 0)
		{
			bool draw = true;

			for (int i = 0; i < lutTable->size() - 1;)
			{
				int start = lutTable->at(i++).pos_x;
				int end = lutTable->at(i).pos_x;

				if (draw) {
					if (start > end) {
						int temp = start;
						start = end;
						end = temp;
					}

					while (start < end)
					{
						if (start > 0 && y > 0) {
							point[0] = start;
							point[1] = y;

							DrawPoint2D(point);
						}

						start++;
					}
				}

				draw = !draw;
			}
		}

		delete lutTable;
	}
}

void Rasterizer::ScanlineInterpolatedFillPolygon2D(const Vertex2d * vertices, int count)
{
	//TODO:
	//Ex 2.4 Implement Rasterizer::ScanlineInterpolatedFillPolygon2D method so that it is capable of performing interpolated filling.
	//Note: mFillMode is set to INTERPOLATED_FILL
	//		This exercise will be more straightfoward if Ex 1.3 has been implemented in DrawLine2D
	//Use Test 7 to test your solution

	int minShapeY = INT_MAX;
	int maxShapeY = INT_MIN;
	int minShapeX = INT_MAX;
	int maxShapeX = INT_MIN;

	for (int i = 0; i < count; i++) {
		if (minShapeY > vertices[i].position[1]) {
			minShapeY = vertices[i].position[1];
		}
		else if (maxShapeY < vertices[i].position[1]) {
			maxShapeY = vertices[i].position[1];
		}

		if (minShapeX > vertices[i].position[0]) {
			minShapeX = vertices[i].position[0];
		}
		else if (maxShapeX < vertices[i].position[0]) {
			maxShapeX = vertices[i].position[0];
		}
	}

	ScanlineLUTItem l;
	Vector2 point;

	for (int y = minShapeY; y < maxShapeY; y++) {
		if (y < 0) { y = 0;  continue; }

		std::vector<ScanlineLUTItem> *lutTable = new std::vector<ScanlineLUTItem>;

		for (int v = 0; v < count;) {
			Vector2 v1 = vertices[v++].position;
			Vector2 v2;

			if (v == count) {
				v2 = vertices[0].position;
			}
			else {
				v2 = vertices[v].position;
			}


			if ((y >= v1[1] && y < v2[1]) || (y >= v2[1] && y < v1[1])) {
				int x = v1[0] + (y - v1[1]) * ((v1[0] - v2[0]) / (v1[1] - v2[1]));

				float slope = (v2[1] - v1[1]) / (v2[0] - v1[0]);
				float lerp = (v2[1] - y) / (v2[1] - v1[1]);

				Colour4 colour;

				if (v == count) {
					colour = ColourUtil::Interpolate(vertices[0].colour, vertices[v - 1].colour, lerp);
				}
				else {
					colour = ColourUtil::Interpolate(vertices[v].colour, vertices[v - 1].colour, lerp);
				}

				l.colour = colour;// vertices[v - 1].colour;
				l.pos_x = x;
				lutTable->push_back(l);
			}
		}

		std::sort(lutTable->begin(), lutTable->end(), less_than_key());

		if (lutTable->size() != 0)
		{
			bool draw = true;

			for (int i = 0; i < lutTable->size() - 1;)
			{
				ScanlineLUTItem startLUT = lutTable->at(i++);
				ScanlineLUTItem endLUT = lutTable->at(i++);

				int start = startLUT.pos_x;
				int end = endLUT.pos_x;

				if (draw) {
					if (start > end) {
						int temp = start;
						start = end;
						end = temp;
					}

					float initial = start;

					while (start < end)
					{
						if (start > 0 && y > 0) {
							point[0] = start;
							point[1] = y;

							if (mFillMode == Rasterizer::INTERPOLATED_FILLED) {
								float total = end - initial;
								float diffX = end - start;
								float lerp = diffX / total;

								Colour4 colour = ColourUtil::Interpolate(endLUT.colour, startLUT.colour, lerp);

								SetFGColour(colour);
							}
							DrawPoint2D(point);
						}

						start++;
					}
				}

				draw = !draw;
			}
		}

		delete lutTable;
	}
}

void Rasterizer::DrawCircle2D(const Circle2D & inCircle, bool filled)
{
	//TODO:
	//Ex 2.5 Implement Rasterizer::DrawCircle2D method so that it can draw a filled circle.
	//Note: For a simple solution, you can first attempt to draw an unfilled circle in the same way as drawing an unfilled polygon.
	//Use Test 8 to test your solution

	float radius = inCircle.radius;

	float x = radius;
	float y = 0;
	float dx = 1;
	float dy = 1;
	int err = dx - ((int)radius << 1);

	while (x >= y) {
		if (filled) {
			Vertex2d start;
			Vertex2d end;
			
			start.colour = inCircle.colour;
			end.colour = inCircle.colour;

			start.position = Vector2(inCircle.centre[0] + x, inCircle.centre[1] + y);
			end.position = Vector2(inCircle.centre[0] - x, inCircle.centre[1] + y);
			Rasterizer::DrawLine2D(start, end, 1);

			start.position = Vector2(inCircle.centre[0] + x, inCircle.centre[1] - y);
			end.position = Vector2(inCircle.centre[0] - x, inCircle.centre[1] - y);
			Rasterizer::DrawLine2D(start, end, 1);

			start.position = Vector2(inCircle.centre[0] + y, inCircle.centre[1] + x);
			end.position = Vector2(inCircle.centre[0] - y, inCircle.centre[1] + x);
			Rasterizer::DrawLine2D(start, end, 1);

			start.position = Vector2(inCircle.centre[0] + y, inCircle.centre[1] - x);
			end.position = Vector2(inCircle.centre[0] - y, inCircle.centre[1] - x);
			Rasterizer::DrawLine2D(start, end, 1);
		}
		else {
			DrawPoint2D(Vector2(inCircle.centre[0] + x, inCircle.centre[1] + y));
			DrawPoint2D(Vector2(inCircle.centre[0] + y, inCircle.centre[1] + x));
			DrawPoint2D(Vector2(inCircle.centre[0] - y, inCircle.centre[1] + x));
			DrawPoint2D(Vector2(inCircle.centre[0] - x, inCircle.centre[1] + y));
			DrawPoint2D(Vector2(inCircle.centre[0] - x, inCircle.centre[1] - y));
			DrawPoint2D(Vector2(inCircle.centre[0] - y, inCircle.centre[1] - x));
			DrawPoint2D(Vector2(inCircle.centre[0] + y, inCircle.centre[1] - x));
			DrawPoint2D(Vector2(inCircle.centre[0] + x, inCircle.centre[1] - y));
		}

		if (err <= 0) {
			y++;
			err += dy;
			dy += 2;
		}

		if (err > 0) {
			x--;
			dx += 2;
			err += dx - ((int)radius << 1);
		}
	}
}

Framebuffer *Rasterizer::GetFrameBuffer() const
{
	return mFramebuffer;
}