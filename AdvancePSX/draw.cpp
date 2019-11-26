#include <stdio.h>
#include <stdlib.h>

#include "gpu_i.h"
#include "shader.h"

//===============================================================

unsigned short colorR2C[256];
unsigned short colorG2C[256];
unsigned short colorB2C[256];

unsigned int lutBGR2RGB[65536];

short Ymin;
short Ymax;

short Xmin[512];
short Xmax[512];

long TXmax[512];
long TYmax[512];
long TXmin[512];
long TYmin[512];

long Rmax[512];
long Rmin[512];
long Gmax[512];
long Gmin[512];
long Bmax[512];
long Bmin[512];

short updateLace;
short dispMode;


void drawLineG(short x1, short y1, short x2, short y2, long rgb1, long rgb2)
{
	long deltax, deltay, numpixels;
	long i;
	long d, dinc1, dinc2;
	long x, y;
	long xinc1, xinc2;
	long yinc1, yinc2;
	long cR1, cG1, cB1, cR2, cG2, cB2;
	long difR, difB, difG;

	cR1 = (rgb1 >> 3) & 0x001f0000;
	cR2 = (rgb2 >> 3) & 0x001f0000;
	cG1 = (rgb1 << 5) & 0x001f0000;
	cG2 = (rgb2 << 5) & 0x001f0000;
	cB1 = (rgb1 << 13) & 0x001f0000;
	cB2 = (rgb2 << 13) & 0x001f0000;

	deltax = abs(x2 - x1);
	deltay = abs(y2 - y1);

	if (deltax >= deltay)
	{
		numpixels = deltax;
		d = (2 * deltay) - deltax;
		dinc1 = deltay << 1;
		dinc2 = (deltay - deltax) << 1;
		xinc1 = 1;
		xinc2 = 1;
		yinc1 = 0;
		yinc2 = 1;
	}
	else
	{
		numpixels = deltay;
		d = (2 * deltax) - deltay;
		dinc1 = deltax << 1;
		dinc2 = (deltax - deltay) << 1;
		xinc1 = 0;
		xinc2 = 1;
		yinc1 = 1;
		yinc2 = 1;
	}

	if (x1>x2)
	{
		xinc1 = (-xinc1);
		xinc2 = (-xinc2);
	}

	if (y1>y2)
	{
		yinc1 = (-yinc1);
		yinc2 = (-yinc2);
		if (y1>DrawingArea[1] && y1<DrawingArea[3] && y1>Ymax) Ymax = y1;
		if (y2>DrawingArea[1] && y2<DrawingArea[3] && y2<Ymin) Ymin = y2;
	}
	else
	{
		if (y1>DrawingArea[1] && y1<DrawingArea[3] && y1<Ymin) Ymin = y1;
		if (y2>DrawingArea[1] && y2<DrawingArea[3] && y2>Ymax) Ymax = y2;
	}

	x = x1; y = y1;
	if (numpixels>0)
	{
		difR = (cR2 - cR1) / numpixels;
		difG = (cG2 - cG1) / numpixels;
		difB = (cB2 - cB1) / numpixels;
	}
	else return;


	for (i = 0; i<numpixels; i++)
	{
		if ((y>DrawingArea[1] && y<DrawingArea[3]))
		{
			if (x>DrawingArea[0] && x<DrawingArea[2])
			{
				PixelData = ((cR1 >> 6) & 0x7c00) | ((cG1 >> 11) & 0x03e0) | ((cB1 >> 16) & 0x001f);
				Pixel = &psxVuw[y * 1024 + x];
				gpuDriver();
				//psxVuw[y * 1024 + x] = ((cR1 >> 6) & 0x7c00) | ((cG1 >> 11) & 0x03e0) | ((cB1 >> 16) & 0x001f);
			}
		}
		if (d<0)
		{
			d += dinc1;
			x += xinc1;
			y += yinc1;
		}
		else
		{
			d += dinc2;
			x += xinc2;
			y += yinc2;
		}
		cR1 += difR;
		cG1 += difG;
		cB1 += difB;

	}
}

// ------------------------------------------------
void drawLine(short x1, short y1, short x2, short y2, long rgb)
{
	short deltax, deltay, numpixels;
	short i;
	short d, dinc1, dinc2;
	short x, y;
	short xinc1, xinc2;
	short yinc1, yinc2;

	deltax = abs(x2 - x1);
	deltay = abs(y2 - y1);

	if (deltax >= deltay)
	{
		numpixels = deltax;
		d = (2 * deltay) - deltax;
		dinc1 = deltay << 1;
		dinc2 = (deltay - deltax) << 1;
		xinc1 = 1;
		xinc2 = 1;
		yinc1 = 0;
		yinc2 = 1;
	}
	else
	{
		numpixels = deltay;
		d = (2 * deltax) - deltay;
		dinc1 = deltax << 1;
		dinc2 = (deltax - deltay) << 1;
		xinc1 = 0;
		xinc2 = 1;
		yinc1 = 1;
		yinc2 = 1;
	}

	if (x1>x2)
	{
		xinc1 = (-xinc1);
		xinc2 = (-xinc2);
	}
	if (y1>y2)
	{
		yinc1 = (-yinc1);
		yinc2 = (-yinc2);
	}
	x = x1; y = y1;
	if (numpixels<0) return;
	for (i = numpixels; i >= 0; i--)
	{
		if ((DrawingArea[0] < x && x < DrawingArea[2]) && (DrawingArea[1] < y && y < DrawingArea[3])) {
			PixelData = rgb;
			Pixel = &psxVuw[y * 1024 + x];
			gpuDriver();
		}
		if (d<0)
		{
			d += dinc1;
			x += xinc1;
			y += yinc1;
		}
		else
		{
			d += dinc2;
			x += xinc2;
			y += yinc2;
		}
	}
}

//===============================================================
// routine that initializes lookup tables BGR16->RGB15
// also this routine initializes other parameters which are
// linked with display routines...
// the lookup table is used by updateDisplay() function

void fillLookupTables(void)
{
	int r, g, b;
	for (b = 0; b < 32; b++) {
		for (g = 0; g < 32; g++) {
			for (r = 0; r < 32; r++) {
				int r8 = (r << 3) | (r >> 2);
				int g8 = (g << 3) | (g >> 2);
				int b8 = (b << 3) | (b >> 2);
				lutBGR2RGB[(r << 10) | (g << 5) | (b << 0)] = (b8 << 16) | (g8 << 8) | (r8 << 0);
			}
		}
	}

	updateLace = 5; // update period when PSX don't want to change display (every 5th VSync)
	dispMode = 0; // holds current display mode (0 = no mode - assume 640)
}