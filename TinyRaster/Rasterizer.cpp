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
	
	WriteRGBAToFramebuffer(x, y, mFGColour);
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
	int y = swap_xy ? reflect < 0 ? swap_x ? pt1[0] : pt2[0] : swap_x ? pt2[0] : pt1[0] : swap_x ? pt2[1] : pt1[1];
	int ex = swap_xy ? reflect < 0 ? swap_x ? pt2[1] : pt1[1] : swap_x ? pt1[1] : pt2[1] : swap_x ? pt1[0] : pt2[0];

	int x = sx;
	y *= reflect;

	Vector2 temp;

	while (x <= ex)
	{
		temp[0] = swap_xy ? y*reflect : x;
		temp[1] = swap_xy ? x : y*reflect;

		if (temp[0] < 0 || temp[1] < 0) { return; }

		Colour4 colour;

		// Interpolated fill
		if (mFillMode == Rasterizer::INTERPOLATED_FILLED) {
			float i = (x - pt1[0]) / dx;
			colour = ColourUtil::Interpolate(v1.colour, v2.colour, i);
		}
		else {
			colour = v1.colour;
		}

		SetFGColour(colour);
		DrawPoint2D(temp);

		// Line thickness
		if (thickness > 1) {
			float dt = abs(dx) + abs(dy);

			for (int i = 1; i < thickness; i++) {
				float xp = dx / dt;
				float yp = dy / dt;

				int tx = -(i * yp);
				int ty = (i * xp);

				if (swap_y) {
					int newY = temp[1] - ty;
					int newX = temp[0] - tx;

					if (newX > 0 && newY > 0) {
						Vector2 temp2(newX, newY);
						DrawPoint2D(temp2);
					}
				}
				else {

					int newY = temp[1] + ty;
					int newX = temp[0] + tx;

					if (newX > 0 && newY > 0) {
						Vector2 temp2(newX, newY);
						DrawPoint2D(temp2);
					}
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
	//TODO:
	//Ex 2.1 Implement the Rasterizer::DrawUnfilledPolygon2D method so that it is capable of drawing an unfilled polygon, i.e. only the edges of a polygon are rasterised. 
	//Please note, in order to complete this exercise, you must first complete Ex1.1 since DrawLine2D method is reusable here.
	//Note: The edges of a given polygon can be found by conntecting two adjacent vertices in the vertices array.
	//Use Test 3 (Press F3) to test your solution.
	for (int i = 0; i < count; i++) {
		//DrawLine2D(*(vertices + i), *(vertices + i + 1));
		DrawLine2D(vertices[i], vertices[i + 1], 3);
	}
}

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
}

void Rasterizer::ScanlineInterpolatedFillPolygon2D(const Vertex2d * vertices, int count)
{
	//TODO:
	//Ex 2.4 Implement Rasterizer::ScanlineInterpolatedFillPolygon2D method so that it is capable of performing interpolated filling.
	//Note: mFillMode is set to INTERPOLATED_FILL
	//		This exercise will be more straightfoward if Ex 1.3 has been implemented in DrawLine2D
	//Use Test 7 to test your solution
}

void Rasterizer::DrawCircle2D(const Circle2D & inCircle, bool filled)
{
	//TODO:
	//Ex 2.5 Implement Rasterizer::DrawCircle2D method so that it can draw a filled circle.
	//Note: For a simple solution, you can first attempt to draw an unfilled circle in the same way as drawing an unfilled polygon.
	//Use Test 8 to test your solution
}

Framebuffer *Rasterizer::GetFrameBuffer() const
{
	return mFramebuffer;
}