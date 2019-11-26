#include "syslog.h"

#include "gpu_i.h"

// temporary only
#include "draw.h"
#include "shader.h"

#include <stdio.h>

uchar Masking;

#define	GPU_RGB16(rgb)	\
	((((rgb)&0xF80000)>>9)|(((rgb)&0xF800)>>6)|(((rgb)&0xF8)>>3))

#define CHKMAX_X 1024
#define CHKMAX_Y 512

#define	GPU_TESTRANGE(x)	\
{	/*if ((ulong)((x) + 1024) > (ulong)2047) return;*/	}

int  GPU_TESTRANGE_POLY(int x1, int y1, int x2, int y2, int x3, int y3, int x4, int y4)
{
	/*if (x1 > DrawingArea[2] && x2 > DrawingArea[2] && x3 > DrawingArea[2] && x4 > DrawingArea[2]) return 1;
	if (y1 > DrawingArea[3] && y2 > DrawingArea[3] && y3 > DrawingArea[3] && y4 > DrawingArea[3]) return 1;
	if (x1 < DrawingArea[0] && x2 < DrawingArea[0] && x3 < DrawingArea[0] && x4 < DrawingArea[0]) return 1;
	if (y1 < DrawingArea[1] && y2 < DrawingArea[1] && y3 < DrawingArea[1] && y4 < DrawingArea[1]) return 1;
	if (DrawingArea[1] >= DrawingArea[3]) return 1;
	if (DrawingArea[0] >= DrawingArea[2]) return 1;*/
	return 0;
}

BOOL CheckCoord4(int lx0, int ly0, int lx1, int ly1, int lx2, int ly2, int lx3, int ly3)
{
	if (lx0<0)
	{
		if (((lx1 - lx0)>CHKMAX_X) ||
			((lx2 - lx0)>CHKMAX_X))
		{
			if (lx3<0)
			{
				if ((lx1 - lx3)>CHKMAX_X) return TRUE;
				if ((lx2 - lx3)>CHKMAX_X) return TRUE;
			}
		}
	}
	if (lx1<0)
	{
		if ((lx0 - lx1)>CHKMAX_X) return TRUE;
		if ((lx2 - lx1)>CHKMAX_X) return TRUE;
		if ((lx3 - lx1)>CHKMAX_X) return TRUE;
	}
	if (lx2<0)
	{
		if ((lx0 - lx2)>CHKMAX_X) return TRUE;
		if ((lx1 - lx2)>CHKMAX_X) return TRUE;
		if ((lx3 - lx2)>CHKMAX_X) return TRUE;
	}
	if (lx3<0)
	{
		if (((lx1 - lx3)>CHKMAX_X) ||
			((lx2 - lx3)>CHKMAX_X))
		{
			if (lx0<0)
			{
				if ((lx1 - lx0)>CHKMAX_X) return TRUE;
				if ((lx2 - lx0)>CHKMAX_X) return TRUE;
			}
		}
	}


	if (ly0<0)
	{
		if ((ly1 - ly0)>CHKMAX_Y) return TRUE;
		if ((ly2 - ly0)>CHKMAX_Y) return TRUE;
	}
	if (ly1<0)
	{
		if ((ly0 - ly1)>CHKMAX_Y) return TRUE;
		if ((ly2 - ly1)>CHKMAX_Y) return TRUE;
		if ((ly3 - ly1)>CHKMAX_Y) return TRUE;
	}
	if (ly2<0)
	{
		if ((ly0 - ly2)>CHKMAX_Y) return TRUE;
		if ((ly1 - ly2)>CHKMAX_Y) return TRUE;
		if ((ly3 - ly2)>CHKMAX_Y) return TRUE;
	}
	if (ly3<0)
	{
		if ((ly1 - ly3)>CHKMAX_Y) return TRUE;
		if ((ly2 - ly3)>CHKMAX_Y) return TRUE;
	}

	return FALSE;
}

BOOL CheckCoord3(int lx0, int ly0, int lx1, int ly1, int lx2, int ly2)
{
	if (lx0<0)
	{
		if ((lx1 - lx0)>CHKMAX_X) return TRUE;
		if ((lx2 - lx0)>CHKMAX_X) return TRUE;
	}
	if (lx1<0)
	{
		if ((lx0 - lx1)>CHKMAX_X) return TRUE;
		if ((lx2 - lx1)>CHKMAX_X) return TRUE;
	}
	if (lx2<0)
	{
		if ((lx0 - lx2)>CHKMAX_X) return TRUE;
		if ((lx1 - lx2)>CHKMAX_X) return TRUE;
	}
	if (ly0<0)
	{
		if ((ly1 - ly0)>CHKMAX_Y) return TRUE;
		if ((ly2 - ly0)>CHKMAX_Y) return TRUE;
	}
	if (ly1<0)
	{
		if ((ly0 - ly1)>CHKMAX_Y) return TRUE;
		if ((ly2 - ly1)>CHKMAX_Y) return TRUE;
	}
	if (ly2<0)
	{
		if ((ly0 - ly2)>CHKMAX_Y) return TRUE;
		if ((ly1 - ly2)>CHKMAX_Y) return TRUE;
	}

	return FALSE;
}


BOOL CheckCoord2(int lx0, int ly0, int lx1, int ly1)
{
	if (lx0<0)
	{
		if ((lx1 - lx0)>CHKMAX_X) return TRUE;
	}
	if (lx1<0)
	{
		if ((lx0 - lx1)>CHKMAX_X) return TRUE;
	}
	if (ly0<0)
	{
		if ((ly1 - ly0)>CHKMAX_Y) return TRUE;
	}
	if (ly1<0)
	{
		if ((ly0 - ly1)>CHKMAX_Y) return TRUE;
	}

	return FALSE;
}

#include <time.h>

#define ABSI(n) ((n) > 0 ? (n) : -(n))

void arrayline(int *ay, int x1, int y1, int x2, int y2)
{
	int steep = ABSI(y2 - y1) > ABSI(x2 - x1);

	if (steep) {
		int tmp;
		tmp = x1;
		x1 = y1;
		y1 = tmp;

		tmp = x2;
		x2 = y2;
		y2 = tmp;
	}

	if (x1 > x2) {
		int tmp;
		tmp = x1;
		x1 = x2;
		x2 = tmp;

		tmp = y1;
		y1 = y2;
		y2 = tmp;
	}

	int dx = ABSI(x2 - x1);
	int dy = ABSI(y2 - y1);

	int err = dx / 2;
	int sy;

	sy = y1 < y2 ? 1 : -1;

	int y = y1;

	for (int x = x1; x <= x2; x++) {
		if (steep) {
			if (x >= 0 && x < 512) ay[x] = y;
		}
		else {
			if (y >= 0 && y < 512) ay[y] = x;
		}

		err -= dy;
		if (err < 0) {
			y += sy;
			err += dx;
		}
	}
}
extern unsigned short *fb;

#define INTERP(xi,xi1,yi,yi1,x) ((( (float)xi1 - (float)xi ) ? ((float)yi + ((( (float)yi1 - (float)yi ) * ( (float)x - (float)xi )) / ( (float)xi1 - (float)xi ))) : ( (float)yi )))

#define R_8(rgb) ((rgb >> 16) & 0xff)
#define G_8(rgb) ((rgb >>  8) & 0xff)
#define B_8(rgb) ((rgb >>  0) & 0xff)

#define SMOOTH 0
#define TEXTURE 0
#define VERTEX 3
void pp_drawpoly_F3(int *xv, int *yv, int *uv, int *vv, int *cv)
#include "pppoly.h"

#define SMOOTH 0
#define TEXTURE 1
#define VERTEX 3
void pp_drawpoly_FT3(int *xv, int *yv, int *uv, int *vv, int *cv)
#include "pppoly.h"

#define SMOOTH 1
#define TEXTURE 0
#define VERTEX 3
void pp_drawpoly_G3(int *xv, int *yv, int *uv, int *vv, int *cv)
#include "pppoly.h"

#define SMOOTH 1
#define TEXTURE 1
#define VERTEX 3
void pp_drawpoly_GT3(int *xv, int *yv, int *uv, int *vv, int *cv)
#include "pppoly.h"

#define POLYFUNC(name) pp_drawpoly_ ## name

#define FUNCTION_POLY POLYFUNC(F3)
void drawvertex_F3(short x1, short y1, short u1, short v1, int c1, short x2, short y2, short u2, short v2, int c2, short x3, short y3, short u3, short v3, int c3)
#include "prim_sub.h"

#define FUNCTION_POLY POLYFUNC(FT3)
void drawvertex_FT3(short x1, short y1, short u1, short v1, int c1, short x2, short y2, short u2, short v2, int c2, short x3, short y3, short u3, short v3, int c3)
#include "prim_sub.h"

#define FUNCTION_POLY POLYFUNC(G3)
void drawvertex_G3(short x1, short y1, short u1, short v1, int c1, short x2, short y2, short u2, short v2, int c2, short x3, short y3, short u3, short v3, int c3)
#include "prim_sub.h"

#define FUNCTION_POLY POLYFUNC(GT3)
void drawvertex_GT3(short x1, short y1, short u1, short v1, int c1, short x2, short y2, short u2, short v2, int c2, short x3, short y3, short u3, short v3, int c3)
#include "prim_sub.h"

//#define syslog //syslog

void cmdSTP(unsigned char * baseAddr)
{
	unsigned long gdata = ((unsigned long*)baseAddr)[0];
	syslog(77, " CMD: STP: %x", gdata & 0xffffff);
	GPUstatusRet = (GPUstatusRet & ~0x00001800) | (gdata << 11);
	Masking = (gdata << 2) & 0x8;
	PixelMSB = gdata << 15;
}


void cmdTexturePage(unsigned char * baseAddr)
{
	unsigned long gdata = ((unsigned long*)baseAddr)[0];
	GPUstatusRet = (GPUstatusRet & ~0x000007FF) | (gdata & 0x000007FF);
	gpuSetTexture(gdata);
	return;
}

void cmdTextureWindow(unsigned char *baseAddr)
{
	unsigned long gdata = ((unsigned long*)baseAddr)[0];
	syslog(77, "CDM: TWin: %x", gdata & 0xffffff);
	TextureWindow[0] = ((gdata >> 10) & 0x1F) << 3;
	TextureWindow[1] = ((gdata >> 15) & 0x1F) << 3;
	TextureWindow[2] = TextureMask[(gdata >> 0) & 0x1F];
	TextureWindow[3] = TextureMask[(gdata >> 5) & 0x1F];
	TextureWindow[0] &= ~TextureWindow[2];
	TextureWindow[1] &= ~TextureWindow[3];
	gpuSetTexture(GPUstatusRet);
}

void cmdDrawAreaStart(unsigned char * baseAddr)
{
	unsigned long gdata = ((unsigned long*)baseAddr)[0];
	DrawingArea[0] = gdata & 0x3ff;
	DrawingArea[1] = (gdata >> 10) & 0x1ff;
	//DrawingArea[0] = (gdata >> 0) & 0x3FF;
	//DrawingArea[1] = (gdata >> 10) & 0x3FF;
	syslog(77, " CMD: DrawArea Start %d,%d %x", DrawingArea[0], DrawingArea[1], gdata);
}

void cmdDrawAreaEnd(unsigned char * baseAddr)
{
	unsigned long gdata = ((unsigned long*)baseAddr)[0];
	DrawingArea[2] = gdata & 0x3ff;
	DrawingArea[3] = (gdata >> 10) & 0x1ff;
	//DrawingArea[2] = ((gdata >> 0) & 0x3FF) + 1;
	//DrawingArea[3] = ((gdata >> 10) & 0x3FF) + 1;
	syslog(77, " CMD: DrawArea End %d,%d", DrawingArea[2], DrawingArea[3]);
}

void cmdDrawOffset(unsigned char * baseAddr)
{
	unsigned long gdata = ((unsigned long*)baseAddr)[0];
	//DrawingOffset[0] = gdata & 0x3ff;
	//DrawingOffset[1] = (gdata >> 11) & 0x1ff;
	DrawingOffset[0] = (gdata >> 0) & 0x7FF;
	DrawingOffset[1] = (gdata >> 11) & 0x7FF;
	syslog(77, " CMD: Draw Offset %d,%d", DrawingOffset[0], DrawingOffset[1], gdata);
}

void primLoadImage(unsigned char * baseAddr)
{
	unsigned long *gpuData = ((unsigned long *)baseAddr);
	ushort x0, y0, w0, h0;
	x0 = ((unsigned short *)gpuData)[2] & 1023;
	y0 = ((unsigned short *)gpuData)[3] & 511;
	w0 = ((unsigned short *)gpuData)[4];
	h0 = ((unsigned short *)gpuData)[5];
	FrameIndex = FRAME_OFFSET(x0, y0);
	if ((y0 + h0) > FRAME_HEIGHT) {
		h0 = FRAME_HEIGHT - y0;
	}
	FrameToWrite = w0 * h0;
	FrameCount = FrameWidth = w0;
}

void primStoreImage(unsigned char * baseAddr)
{
	unsigned long *gpuData = ((unsigned long *)baseAddr);

	ushort x0, y0, w0, h0;
	x0 = ((unsigned short *)gpuData)[2] & 1023;
	y0 = ((unsigned short *)gpuData)[3] & 511;
	w0 = ((unsigned short *)gpuData)[4];
	h0 = ((unsigned short *)gpuData)[5];
	FrameIndex = FRAME_OFFSET(x0, y0);
	if ((y0 + h0) > FRAME_HEIGHT) {
		h0 = FRAME_HEIGHT - y0;
	}
	FrameToRead = w0 * h0;
	FrameCount = FrameWidth = w0;
	GPUstatusRet |= 0x08000000;
}

void primMoveImage(unsigned char * baseAddr)
{
	unsigned short *gpuDataU2 = ((unsigned short *)baseAddr);
	long x0, y0, x1, y1, w0, h0;
	ushort *lpDst, *lpSrc;
	x0 = gpuDataU2[2] & 1023;
	y0 = gpuDataU2[3] & 511;
	x1 = gpuDataU2[4] & 1023;
	y1 = gpuDataU2[5] & 511;
	w0 = gpuDataU2[6] & 1023;
	h0 = gpuDataU2[7] & 511;

	syslog(1, " CMD: VRAM->VRAM %d,%d -> %d,%d (%d,%d)\n", x0, y0, x1, y1, w0, h0);

	if ((y0 + h0) > FRAME_HEIGHT)
		return;
	if ((y1 + h0) > FRAME_HEIGHT)
		return;
	lpDst = lpSrc = psxVuw;
	lpSrc += FRAME_OFFSET(x0, y0);
	lpDst += FRAME_OFFSET(x1, y1);
	x1 = FRAME_WIDTH - w0;
	for (; h0; h0--) {
		for (x0 = w0; x0; x0--)
			*lpDst++ = *lpSrc++;
		lpDst += x1;
		lpSrc += x1;
	}
}


void primBlkFill(unsigned char * baseAddr)
{
	unsigned short * baseAddrW = (unsigned short*)baseAddr;
	unsigned long *gpuData = ((unsigned long *)baseAddr);
	short *sgpuData = ((short *)baseAddr);

	syslog(2, "Blkfill %04x,%04x - %04x,%04x", baseAddrW[2], baseAddrW[3], baseAddrW[4], baseAddrW[5]);

	int cR1 = (gpuData[0] >> 3) & 0x001f0000;
	int cG1 = (gpuData[0] << 5) & 0x001f0000;
	int cB1 = (gpuData[0] << 13) & 0x001f0000;

	gpuDriver = gpuDrivers[Masking | ((*gpuData >> 24) & 2) | 1];

	long x0, y0, w0, h0;
	ushort *pixel, rgb;
	rgb = ((cR1 >> 6) & 0x7c00) | ((cG1 >> 11) & 0x03e0) | ((cB1 >> 16) & 0x001f);
	x0 = sgpuData[2];
	y0 = sgpuData[3];
	w0 = sgpuData[4];
	h0 = sgpuData[5];
	w0 += x0;
	if (x0 < 0)
		x0 = 0;
	if (w0 >= FRAME_WIDTH)
		w0 = FRAME_WIDTH-1;
	w0 -= x0;
	if (w0 < 0)
		w0 = 0;
	h0 += y0;
	if (y0 < 0)
		y0 = 0;
	if (h0 >= FRAME_HEIGHT)
		h0 = FRAME_HEIGHT-1;
	h0 -= y0;
	if (h0 < 0)
		h0 = 0;
	pixel = psxVuw + FRAME_OFFSET(x0, y0);
	y0 = FRAME_WIDTH - w0;
	for (; h0; h0--) {
		for (x0 = w0; x0; x0--)
			*pixel++ = rgb;
		pixel += y0;
	}

	return;
}

void primTileS(unsigned char * baseAddr)
{
	unsigned short * baseAddrW = (unsigned short*)baseAddr;
	unsigned long *gpuData = ((unsigned long *)baseAddr);
	short *sgpuData = ((short *)baseAddr);

	syslog(2, "Tile S %04x,%04x - %04x,%04x", baseAddrW[2], baseAddrW[3], baseAddrW[4], baseAddrW[5]);

	gpuDriver = gpuDrivers[Masking | ((*gpuData >> 24) & 2) | 1];

	long temp;
	long xmin, xmax;
	long ymin, ymax;
	long x0, w0;
	long y0, h0;
	x0 = sgpuData[2];
	w0 = sgpuData[4];
	y0 = sgpuData[3];
	h0 = sgpuData[5];
	xmin = DrawingArea[0];
	xmax = DrawingArea[2];
	ymin = DrawingArea[1];
	ymax = DrawingArea[3];
	int cR1 = (gpuData[0] >> 3) & 0x001f0000;
	int cG1 = (gpuData[0] << 5) & 0x001f0000;
	int cB1 = (gpuData[0] << 13) & 0x001f0000;
	PixelData = ((cR1 >> 6) & 0x7c00) | ((cG1 >> 11) & 0x03e0) | ((cB1 >> 16) & 0x001f);
	temp = DrawingOffset[0];
	x0 += temp;
	temp = DrawingOffset[1];
	y0 += temp;
	h0 += y0;
	if (y0 < ymin)
		y0 = ymin;
	if (h0 > ymax)
		h0 = ymax;
	h0 -= y0;
	if (h0 < 0)
		h0 = 0;
	w0 += x0;
	if (x0 < xmin)
		x0 = xmin;
	if (w0 > xmax)
		w0 = xmax;
	w0 -= x0;
	if (w0 < 0)
		w0 = 0;
	Pixel = &psxVuw[FRAME_OFFSET(x0, y0)];
	temp = FRAME_WIDTH - w0;
	for (; h0; h0--) {
		for (x0 = w0; x0; x0--) {
			gpuDriver();
			Pixel++;
		}
		Pixel += temp;
	}
}



void primSprt8(unsigned char * baseAddr)
{
	unsigned long clutAddr;
	unsigned long spriteW;
	unsigned long spriteH;
	unsigned long textA;
	unsigned long textX;
	long sprtY, sprtX, sprtW, sprtH, sprCY, sprCX;
	unsigned long *gpuData = (unsigned long *)baseAddr;

	sprtY = (gpuData[1] >> 16) & 0xffff;
	sprtX = (gpuData[1] & 0xffff);

	sprtW = 8;
	sprtH = 8;

	sprtX += DrawingOffset[0];
	sprtY += DrawingOffset[1];

	syslog(2, "Sprt 8 %d,%d (%dx%d)", sprtX, sprtY, sprtW, sprtH);

	gpuSetCLUT(gpuData[2] >> 16);
	gpuSetTexture(GPUstatusRet);
	gpuDriver = gpuDrivers[Masking | ((*gpuData >> 24) & 7)];

	int cR1 = (gpuData[0] >> 0) & 0xff;
	int cG1 = (gpuData[0] >> 8) & 0xff;
	int cB1 = (gpuData[0] >> 16) & 0xff;

	int xmin = DrawingArea[0];
	int xmax = DrawingArea[2];
	int ymin = DrawingArea[1];
	int ymax = DrawingArea[3];

	sprtH += sprtY;
	if (sprtY < ymin)
		sprtY = ymin;
	if (sprtH > ymax)
		sprtH = ymax;
	sprtH -= sprtY;
	if (sprtH < 0)
		sprtH = 0;
	sprtW += sprtX;
	if (sprtX < xmin)
		sprtX = xmin;
	if (sprtW > xmax)
		sprtW = xmax;
	sprtW -= sprtX;
	if (sprtW < 0)
		sprtW = 0;

	for (sprCY = 0; sprCY < sprtH; sprCY++) {
		for (sprCX = 0; sprCX < sprtW; sprCX++) {
			Pixel = &psxVuw[(sprtY + sprCY) * 1024 + (sprtX + sprCX)];
			_TU = sprCX + ((uchar *)gpuData)[8];
			_TV = sprCY + ((uchar *)gpuData)[9];
			_LR = cR1;
			_LG = cG1;
			_LB = cB1;
			gpuDriver();
		}
	}
}

void primSprtS(unsigned char * baseAddr)
{
	unsigned long clutAddr;
	unsigned long spriteW;
	unsigned long spriteH;
	unsigned long textA;
	unsigned long textX;
	long sprtY, sprtX, sprtW, sprtH, sprCY, sprCX;
	unsigned long *gpuData = (unsigned long *)baseAddr;

	sprtY = (gpuData[1] >> 16) & 0xffff;
	sprtX = (gpuData[1] & 0xffff);

	sprtH = (gpuData[3] >> 16) & 0x1ff;
	sprtW = (gpuData[3] & 0x3ff);

	sprtX += DrawingOffset[0];
	sprtY += DrawingOffset[1];

	syslog(2, "Sprt S %d,%d (%dx%d)", sprtX, sprtY, sprtW, sprtH);

	gpuSetCLUT(gpuData[2] >> 16);
	gpuSetTexture(GPUstatusRet);
	gpuDriver = gpuDrivers[Masking | ((*gpuData >> 24) & 7)];

	int cR1 = (gpuData[0] >> 0) & 0xff;
	int cG1 = (gpuData[0] >> 8) & 0xff;
	int cB1 = (gpuData[0] >> 16) & 0xff;

	int xmin = DrawingArea[0];
	int xmax = DrawingArea[2];
	int ymin = DrawingArea[1];
	int ymax = DrawingArea[3];

	sprtH += sprtY;
	if (sprtY < ymin)
		sprtY = ymin;
	if (sprtH > ymax)
		sprtH = ymax;
	sprtH -= sprtY;
	if (sprtH < 0)
		sprtH = 0;
	sprtW += sprtX;
	if (sprtX < xmin)
		sprtX = xmin;
	if (sprtW > xmax)
		sprtW = xmax;
	sprtW -= sprtX;
	if (sprtW < 0)
		sprtW = 0;

	for (sprCY = 0; sprCY < sprtH; sprCY++) {
		for (sprCX = 0; sprCX < sprtW; sprCX++) {
			Pixel = &psxVuw[(sprtY + sprCY) * 1024 + (sprtX + sprCX)];
			_TU = sprCX + ((uchar *)gpuData)[8];
			_TV = sprCY + ((uchar *)gpuData)[9];
			_LR = cR1;
			_LG = cG1;
			_LB = cB1;
			gpuDriver();
		}
	}
}

void primSprt16(unsigned char * baseAddr)
{
	unsigned long clutAddr;
	unsigned long spriteW;
	unsigned long spriteH;
	unsigned long textA;
	unsigned long textX;
	long sprtY, sprtX, sprtW, sprtH, sprCY, sprCX;
	unsigned long *gpuData = (unsigned long *)baseAddr;

	sprtY = (gpuData[1] >> 16) & 0xffff;
	sprtX = (gpuData[1] & 0xffff);

	sprtW = 16;
	sprtH = 16;

	sprtX += DrawingOffset[0];
	sprtY += DrawingOffset[1];

	syslog(2, "Sprt 16 %d,%d (%dx%d)", sprtX, sprtY, sprtW, sprtH);

	gpuSetCLUT(gpuData[2] >> 16);
	gpuSetTexture(GPUstatusRet);
	gpuDriver = gpuDrivers[Masking | ((*gpuData >> 24) & 7)];
	
	int cR1 = (gpuData[0] >> 0) & 0xff;
	int cG1 = (gpuData[0] >> 8) & 0xff;
	int cB1 = (gpuData[0] >> 16) & 0xff;

	int xmin = DrawingArea[0];
	int xmax = DrawingArea[2];
	int ymin = DrawingArea[1];
	int ymax = DrawingArea[3];

	sprtH += sprtY;
	if (sprtY < ymin)
		sprtY = ymin;
	if (sprtH > ymax)
		sprtH = ymax;
	sprtH -= sprtY;
	if (sprtH < 0)
		sprtH = 0;
	sprtW += sprtX;
	if (sprtX < xmin)
		sprtX = xmin;
	if (sprtW > xmax)
		sprtW = xmax;
	sprtW -= sprtX;
	if (sprtW < 0)
		sprtW = 0;

	for (sprCY = 0; sprCY < sprtH; sprCY++) {
		for (sprCX = 0; sprCX < sprtW; sprCX++) {
			Pixel = &psxVuw[(sprtY + sprCY) * 1024 + (sprtX + sprCX)];
			_TU = sprCX + ((uchar *)gpuData)[8];
			_TV = sprCY + ((uchar *)gpuData)[9];
			_LR = cR1;
			_LG = cG1;
			_LB = cB1;
			gpuDriver();
		}
	}
}



void primPolyF4(unsigned char *baseAddr)
{
	unsigned long *gpuData = ((unsigned long *)baseAddr);
	short ly0, ly1, ly2, ly3, lx0, lx1, lx2, lx3;

	ly0 = (short)(gpuData[1] >> 16) & 0xffff;
	lx0 = (short)(gpuData[1] & 0xffff);
	ly1 = (short)((gpuData[2] >> 16) & 0xffff);
	lx1 = (short)(gpuData[2] & 0xffff);
	ly2 = (short)((gpuData[3] >> 16) & 0xffff);
	lx2 = (short)(gpuData[3] & 0xffff);
	ly3 = (short)((gpuData[4] >> 16) & 0xffff);
	lx3 = (short)(gpuData[4] & 0xffff);
	lx0 += (short)DrawingOffset[0];
	ly0 += (short)DrawingOffset[1];
	lx1 += (short)DrawingOffset[0];
	ly1 += (short)DrawingOffset[1];
	lx2 += (short)DrawingOffset[0];
	ly2 += (short)DrawingOffset[1];
	lx3 += (short)DrawingOffset[0];
	ly3 += (short)DrawingOffset[1];
	syslog(77, "!GPU! P4 F %d,%d %d,%d %d,%d %d,%d %6x\n", lx0, ly0, lx1, ly1, lx2, ly2, lx3, ly3, gpuData[0]);
	//drawPoly4F(lx0, ly0, lx1, ly1, lx2, ly2, lx3, ly3, gpuData[0]);

	if (CheckCoord4(lx0, ly0, lx1, ly1, lx2, ly2, lx3, ly3)) return;

	GPU_TESTRANGE(lx0);
	GPU_TESTRANGE(ly0);
	GPU_TESTRANGE(lx1);
	GPU_TESTRANGE(ly1);
	GPU_TESTRANGE(lx2);
	GPU_TESTRANGE(ly2);
	GPU_TESTRANGE(lx3);
	GPU_TESTRANGE(ly3);

	gpuDriver = gpuDrivers[Masking | ((gpuData[0] >> 24) & 2) | 1];

	int rgb = (gpuData[0]);

	//PixelShaderA = PS_F3;

	drawvertex_F3(lx0, ly0, 0, 0, rgb, lx1, ly1, 0, 0, rgb, lx2, ly2, 0, 0, rgb);
	drawvertex_F3(lx1, ly1, 0, 0, rgb, lx2, ly2, 0, 0, rgb, lx3, ly3, 0, 0, rgb);
	return;
}

void primPolyG4(unsigned char * baseAddr)
{
	short ly0, ly1, ly2, ly3, lx0, lx1, lx2, lx3;

	unsigned long *gpuData = (unsigned long *)baseAddr;
	ly0 = (short)(gpuData[1] >> 16) & 0xffff;
	lx0 = (short)(gpuData[1] & 0xffff);
	ly1 = (short)((gpuData[3] >> 16) & 0xffff);
	lx1 = (short)(gpuData[3] & 0xffff);
	ly2 = (short)((gpuData[5] >> 16) & 0xffff);
	lx2 = (short)(gpuData[5] & 0xffff);
	ly3 = (short)((gpuData[7] >> 16) & 0xffff);
	lx3 = (short)(gpuData[7] & 0xffff);
	syslog(77, "!GPU! P4 G %d,%d %d,%d %d,%d %d,%d\n",lx0,ly0,lx1,ly1,lx2,ly2,lx3,ly3);
	lx0 += DrawingOffset[0];
	ly0 += DrawingOffset[1];
	lx1 += DrawingOffset[0];
	ly1 += DrawingOffset[1];
	lx2 += DrawingOffset[0];
	ly2 += DrawingOffset[1];
	lx3 += DrawingOffset[0];
	ly3 += DrawingOffset[1];
	//drawPoly4G(lx0, ly0, lx1, ly1, lx2, ly2, lx3, ly3, gpuData[0], gpuData[2], gpuData[4], gpuData[6]);

	GPU_TESTRANGE(lx0);
	GPU_TESTRANGE(ly0);
	GPU_TESTRANGE(lx1);
	GPU_TESTRANGE(ly1);
	GPU_TESTRANGE(lx2);
	GPU_TESTRANGE(ly2);
	GPU_TESTRANGE(lx3);
	GPU_TESTRANGE(ly3);

	if (CheckCoord4(lx0, ly0, lx1, ly1, lx2, ly2, lx3, ly3)) return;

	gpuDriver = gpuDrivers[Masking | ((gpuData[0] >> 24) & 2)];

	int rgb1 = (gpuData[0]);
	int rgb2 = (gpuData[2]);
	int rgb3 = (gpuData[4]);
	int rgb4 = (gpuData[6]);

	//PixelShaderA = PS_G3;

	drawvertex_G3(lx0, ly0, 0, 0, rgb1, lx1, ly1, 0, 0, rgb2, lx2, ly2, 0, 0, rgb3);
	drawvertex_G3(lx1, ly1, 0, 0, rgb2, lx2, ly2, 0, 0, rgb3, lx3, ly3, 0, 0, rgb4);

	return;
}

void primPolyFT3(unsigned char * baseAddr)
{
	unsigned long *gpuData = ((unsigned long *)baseAddr);
	short ly0, ly1, ly2, ly3, lx0, lx1, lx2, lx3;
	unsigned short gpuDataX;
	ly0 = (short)((gpuData[1] >> 16) & 0xffff);
	lx0 = (short)((gpuData[1] & 0xffff));
	ly1 = (short)((gpuData[3] >> 16) & 0xffff);
	lx1 = (short)(gpuData[3] & 0xffff);
	ly2 = (short)((gpuData[5] >> 16) & 0xffff);
	lx2 = (short)(gpuData[5] & 0xffff);

	// now we gonna add offset to this...
	lx0 += DrawingOffset[0];
	ly0 += DrawingOffset[1];
	lx1 += DrawingOffset[0];
	ly1 += DrawingOffset[1];
	lx2 += DrawingOffset[0];
	ly2 += DrawingOffset[1];

	if (CheckCoord3(lx0, ly0, lx1, ly1, lx2, ly2)) return;

	GPU_TESTRANGE(lx0);
	GPU_TESTRANGE(ly0);
	GPU_TESTRANGE(lx1);
	GPU_TESTRANGE(ly1);
	GPU_TESTRANGE(lx2);
	GPU_TESTRANGE(ly2);

	syslog(77, "!GPU! P3 T %d,%d %d,%d %d,%d\n",lx0,ly0,lx1,ly1,lx2,ly2);

	gpuDataX = gpuData[4] >> 16;
	/*switch (textTP)   // depending on texture mode
	{
	case 0:
		fillText4Buffer3V((gpuData[2] & 0x000000ff), ((gpuData[2] >> 8) & 0x000000ff), (gpuData[4] & 0x000000ff), ((gpuData[4] >> 8) & 0x000000ff), (gpuData[6] & 0x000000ff), ((gpuData[6] >> 8) & 0x000000ff), ((gpuData[2] >> 12) & 0x3f0), ((gpuData[2] >> 22) & 0x1ff));
		drawPoly3T(lx0, ly0, lx1, ly1, lx2, ly2, newTextX0, newTextY0, newTextX1, newTextY1, newTextX2, newTextY2);
		return;
	case 1:
		fillText8Buffer3V((gpuData[2] & 0x000000ff), ((gpuData[2] >> 8) & 0x000000ff), (gpuData[4] & 0x000000ff), ((gpuData[4] >> 8) & 0x000000ff), (gpuData[6] & 0x000000ff), ((gpuData[6] >> 8) & 0x000000ff), ((gpuData[2] >> 12) & 0x3f0), ((gpuData[2] >> 22) & 0x1ff));
		drawPoly3T(lx0, ly0, lx1, ly1, lx2, ly2, newTextX0, newTextY0, newTextX1, newTextY1, newTextX2, newTextY2);
		return;
	case 2:
		drawPoly3TD(lx0, ly0, lx1, ly1, lx2, ly2, (gpuData[2] & 0x000000ff), ((gpuData[2] >> 8) & 0x000000ff), (gpuData[4] & 0x000000ff), ((gpuData[4] >> 8) & 0x000000ff), (gpuData[6] & 0x000000ff), ((gpuData[6] >> 8) & 0x000000ff));
		return;
	}*/

	gpuSetCLUT(gpuData[2] >> 16);
	gpuSetTexture(gpuData[4] >> 16);
	gpuDriver = gpuDrivers[Masking | ((gpuData[0] >> 24) & 7)];

	int rgb = (gpuData[0]);

	short u0 = (gpuData[2] & 0x000000ff), v0 = ((gpuData[2] >> 8) & 0x000000ff);
	short u1 = (gpuData[4] & 0x000000ff), v1 = ((gpuData[4] >> 8) & 0x000000ff);
	short u2 = (gpuData[6] & 0x000000ff), v2 = ((gpuData[6] >> 8) & 0x000000ff);

	//PixelShaderA = PS_FT3;

	drawvertex_FT3(lx0, ly0, u0, v0, rgb, lx1, ly1, u1, v1, rgb, lx2, ly2, u2, v2, rgb);
}


void primPolyFT4(unsigned char * baseAddr)
{
	unsigned long *gpuData = ((unsigned long *)baseAddr);
	short ly0, ly1, ly2, ly3, lx0, lx1, lx2, lx3;
	unsigned short gpuDataX;

	//return;

	ly0 = (short)((gpuData[1] >> 16) & 0xffff);
	lx0 = (short)((gpuData[1] & 0xffff));
	ly1 = (short)(((gpuData[3] >> 16) & 0xffff));
	lx1 = (short)((gpuData[3] & 0xffff));
	ly2 = (short)(((gpuData[5] >> 16) & 0xffff));
	lx2 = (short)((gpuData[5] & 0xffff));
	ly3 = (short)(((gpuData[7] >> 16) & 0xffff));
	lx3 = (short)((gpuData[7] & 0xffff));

	lx0 += (short)DrawingOffset[0];
	ly0 += (short)DrawingOffset[1];
	lx1 += (short)DrawingOffset[0];
	ly1 += (short)DrawingOffset[1];
	lx2 += (short)DrawingOffset[0];
	ly2 += (short)DrawingOffset[1];
	lx3 += (short)DrawingOffset[0];
	ly3 += (short)DrawingOffset[1];

	GPU_TESTRANGE(lx0);
	GPU_TESTRANGE(ly0);
	GPU_TESTRANGE(lx1);
	GPU_TESTRANGE(ly1);
	GPU_TESTRANGE(lx2);
	GPU_TESTRANGE(ly2);
	GPU_TESTRANGE(lx3);
	GPU_TESTRANGE(ly3);

	if (CheckCoord4(lx0, ly0, lx1, ly1, lx2, ly2, lx3, ly3)) return;

	syslog(77, "!GPU! P4 T %d,%d %d,%d %d,%d %d,%d\n", lx0, ly0, lx1, ly1, lx2, ly2, lx3, ly3);
	/*gpuDataX = gpuData[4] >> 16;
	textAddrX = (gpuDataX & 0xf) << 6;
	textAddrX2 = (gpuDataX & 0xf) << 7;
	textAddrY = (((gpuDataX) << 4) & 0x100) + (((gpuDataX) >> 2) & 0x200);
	textTP = (gpuDataX & 0x180) >> 7;
	textABR = (gpuDataX & 0x6) >> 5;
	syslog(22, "!GPU! TPage %d,%d TP=%d ABR=%d REST=%x\n", textAddrX, textAddrY, textTP, textABR, (gpuDataX & 0x00ffffff) >> 9);*/

	/*switch (textTP)
	{
	case 0:
		fillText4Buffer4((gpuData[2] & 0x000000ff), ((gpuData[2] >> 8) & 0x000000ff), (gpuData[4] & 0x000000ff), ((gpuData[4] >> 8) & 0x000000ff), (gpuData[6] & 0x000000ff), ((gpuData[6] >> 8) & 0x000000ff), (gpuData[8] & 0x000000ff), ((gpuData[8] >> 8) & 0x000000ff), ((gpuData[2] >> 12) & 0x3f0), ((gpuData[2] >> 22) & 0x1ff));
		drawPoly4T(lx0, ly0, lx1, ly1, lx3, ly3, lx2, ly2, newTextX0, newTextY0, newTextX1, newTextY1, newTextX3, newTextY3, newTextX2, newTextY2);
		return;
	case 1:
		fillText8Buffer4(
			(gpuData[2] & 0x000000ff), ((gpuData[2] >> 8) & 0x000000ff),
			(gpuData[4] & 0x000000ff), ((gpuData[4] >> 8) & 0x000000ff),
			(gpuData[6] & 0x000000ff), ((gpuData[6] >> 8) & 0x000000ff),
			(gpuData[8] & 0x000000ff), ((gpuData[8] >> 8) & 0x000000ff),
			((gpuData[2] >> 12) & 0x3f0), ((gpuData[2] >> 22) & 0x1ff));
		drawPoly4T(lx0, ly0, lx1, ly1, lx3, ly3, lx2, ly2, newTextX0, newTextY0, newTextX1, newTextY1, newTextX3, newTextY3, newTextX2, newTextY2);
		return;
	case 2:
		drawPoly4TD(lx0, ly0, lx1, ly1, lx3, ly3, lx2, ly2,

			(gpuData[2] & 0x000000ff), ((gpuData[2] >> 8) & 0x000000ff),
			(gpuData[4] & 0x000000ff), ((gpuData[4] >> 8) & 0x000000ff),
			(gpuData[8] & 0x000000ff), ((gpuData[8] >> 8) & 0x000000ff),
			(gpuData[6] & 0x000000ff), ((gpuData[6] >> 8) & 0x000000ff));
		return;
	}*/

	gpuSetCLUT(gpuData[2] >> 16);
	gpuSetTexture(gpuData[4] >> 16);
	gpuDriver = gpuDrivers[Masking | ((gpuData[0] >> 24) & 7)];

	int rgb = (gpuData[0]);

	short u0 = (gpuData[2] & 0x000000ff), v0 = ((gpuData[2] >> 8) & 0x000000ff);
	short u1 = (gpuData[4] & 0x000000ff), v1 = ((gpuData[4] >> 8) & 0x000000ff);
	short u2 = (gpuData[6] & 0x000000ff), v2 = ((gpuData[6] >> 8) & 0x000000ff);
	short u3 = (gpuData[8] & 0x000000ff), v3 = ((gpuData[8] >> 8) & 0x000000ff);

	//PixelShaderA = PS_FT3;

	drawvertex_FT3(lx0, ly0, u0, v0, rgb, lx1, ly1, u1, v1, rgb, lx2, ly2, u2, v2, rgb);
	drawvertex_FT3(lx1, ly1, u1, v1, rgb, lx2, ly2, u2, v2, rgb, lx3, ly3, u3, v3, rgb);

	return;

}

void primPolyGT3(unsigned char *baseAddr)
{
	unsigned long *gpuData = ((unsigned long *)baseAddr);
	short ly0, ly1, lx0, lx1, ly2, lx2;
	ly0 = (gpuData[1] >> 16) & 0xffff;
	lx0 = (gpuData[1] & 0xffff);
	ly1 = ((gpuData[4] >> 16) & 0xffff);
	lx1 = (gpuData[4] & 0xffff);
	ly2 = ((gpuData[7] >> 16) & 0xffff);
	lx2 = (gpuData[7] & 0xffff);
	syslog(22, "!GPU! P3 GT %d,%d %d,%d %d,%d\n",lx0,ly0,lx1,ly1,lx2,ly2);
	// now we gonna add offset to this...
	lx0 += DrawingOffset[0];
	ly0 += DrawingOffset[1];
	lx1 += DrawingOffset[0];
	ly1 += DrawingOffset[1];
	lx2 += DrawingOffset[0];
	ly2 += DrawingOffset[1];

	/*
	gpuDataX= gpuData[5]>>16;
	textAddrX= (gpuDataX&0xf)<<6;
	textAddrY= (((gpuDataX)<<4)&0x100)+(((gpuDataX)>>2)&0x200);
	textTP= (gpuDataX & 0x180) >> 7;
	*/

	GPU_TESTRANGE(lx0);
	GPU_TESTRANGE(ly0);
	GPU_TESTRANGE(lx1);
	GPU_TESTRANGE(ly1);
	GPU_TESTRANGE(lx2);
	GPU_TESTRANGE(ly2);

	if (CheckCoord3(lx0, ly0, lx1, ly1, lx2, ly2)) return;

	gpuSetCLUT(gpuData[2] >> 16);
	gpuSetTexture(gpuData[5] >> 16);
	gpuDriver = gpuDrivers[Masking | ((((int *)gpuData)[0] >> 24) & 7)];

	int rgb1 = (gpuData[0]);
	int rgb2 = (gpuData[3]);
	int rgb3 = (gpuData[6]);

	short u0 = (gpuData[2] & 0x000000ff), v0 = ((gpuData[2] >> 8) & 0x000000ff);
	short u1 = (gpuData[5] & 0x000000ff), v1 = ((gpuData[5] >> 8) & 0x000000ff);
	short u2 = (gpuData[8] & 0x000000ff), v2 = ((gpuData[8] >> 8) & 0x000000ff);

	//PixelShaderA = PS_GT3;

	drawvertex_GT3(lx0, ly0, u0, v0, rgb1, lx1, ly1, u1, v1, rgb2, lx2, ly2, u2, v2, rgb3);

	//drawFlatTriangleGoraud(lx0, ly0, lx1, ly1, lx2, ly2, gpuData[0], gpuData[3], gpuData[6]);
}

void primPolyG3(unsigned char *baseAddr)
{
	unsigned long *gpuData = ((unsigned long *)baseAddr);
	short ly0, ly1, lx0, lx1, ly2, lx2;

	ly0 = (short)(gpuData[1] >> 16) & 0xffff;
	lx0 = (short)(gpuData[1] & 0xffff);
	ly1 = (short)((gpuData[3] >> 16) & 0xffff);
	lx1 = (short)(gpuData[3] & 0xffff);
	ly2 = (short)((gpuData[5] >> 16) & 0xffff);
	lx2 = (short)(gpuData[5] & 0xffff);
	lx0 += DrawingOffset[0];
	ly0 += DrawingOffset[1];
	lx1 += DrawingOffset[0];
	ly1 += DrawingOffset[1];
	lx2 += DrawingOffset[0];
	ly2 += DrawingOffset[1];
	syslog(22,"!GPU! P3 G %d,%d %d,%d %d,%d\n",lx0,ly0,lx1,ly1,lx2,ly2);
	//drawFlatTriangleGoraud(lx0, ly0, lx1, ly1, lx2, ly2, gpuData[0], gpuData[2], gpuData[4]);

	GPU_TESTRANGE(lx0);
	GPU_TESTRANGE(ly0);
	GPU_TESTRANGE(lx1);
	GPU_TESTRANGE(ly1);
	GPU_TESTRANGE(lx2);
	GPU_TESTRANGE(ly2);

	if (CheckCoord3(lx0, ly0, lx1, ly1, lx2, ly2)) return;

	gpuDriver = gpuDrivers[Masking | ((gpuData[0] >> 24) & 2)];

	int rgb1 = (gpuData[0]);
	int rgb2 = (gpuData[2]);
	int rgb3 = (gpuData[4]);

	//PixelShaderA = PS_G3;

	drawvertex_G3(lx0, ly0, 0, 0, rgb1, lx1, ly1, 0, 0, rgb2, lx2, ly2, 0, 0, rgb3);
}

void primPolyGT4(unsigned char *baseAddr)
{
	unsigned long *gpuData = ((unsigned long *)baseAddr);
	short ly0, ly1, lx0, lx1, ly2, lx2, ly3, lx3;
	ly0 = (gpuData[1] >> 16) & 0xffff;
	lx0 = (gpuData[1] & 0xffff);
	ly1 = ((gpuData[4] >> 16) & 0xffff);
	lx1 = (gpuData[4] & 0xffff);
	ly2 = ((gpuData[7] >> 16) & 0xffff);
	lx2 = (gpuData[7] & 0xffff);
	ly3 = ((gpuData[10] >> 16) & 0xffff);
	lx3 = (gpuData[10] & 0xffff);

	syslog(22, "!GPU! P4 GT %d,%d %d,%d %d,%d %d,%d\n",lx0,ly0,lx1,ly1,lx2,ly2,lx3,ly3);

	/*
	gpuDataX=gpuData[5]>>16;
	textAddrX= (gpuDataX&0xf)<<6;
	textAddrY = (((gpuDataX)<<4)&0x100)+(((gpuDataX)>>2)&0x200);
	textTP = (gpuDataX & 0x180) >> 7;
	*/
	lx0 += DrawingOffset[0];
	ly0 += DrawingOffset[1];
	lx1 += DrawingOffset[0];
	ly1 += DrawingOffset[1];
	lx2 += DrawingOffset[0];
	ly2 += DrawingOffset[1];
	lx3 += DrawingOffset[0];
	ly3 += DrawingOffset[1];

	GPU_TESTRANGE(lx0);
	GPU_TESTRANGE(ly0);
	GPU_TESTRANGE(lx1);
	GPU_TESTRANGE(ly1);
	GPU_TESTRANGE(lx2);
	GPU_TESTRANGE(ly2);
	GPU_TESTRANGE(lx3);
	GPU_TESTRANGE(ly3);

	if (CheckCoord4(lx0, ly0, lx1, ly1, lx2, ly2, lx3, ly3)) return;

	//drawPoly4G(lx0, ly0, lx1, ly1, lx2, ly2, lx3, ly3, gpuData[0], gpuData[3], gpuData[6], gpuData[9]);

	gpuSetCLUT(gpuData[2] >> 16);
	gpuSetTexture(gpuData[5] >> 16);
	gpuDriver = gpuDrivers[Masking | ((gpuData[0] >> 24) & 7)];

	int rgb1 = (gpuData[0]);
	int rgb2 = (gpuData[3]);
	int rgb3 = (gpuData[6]);
	int rgb4 = (gpuData[9]);

	short u0 = (gpuData[2] & 0x000000ff), v0 = ((gpuData[2] >> 8) & 0x000000ff);
	short u1 = (gpuData[5] & 0x000000ff), v1 = ((gpuData[5] >> 8) & 0x000000ff);
	short u2 = (gpuData[8] & 0x000000ff), v2 = ((gpuData[8] >> 8) & 0x000000ff);
	short u3 = (gpuData[11] & 0x000000ff), v3 = ((gpuData[11] >> 8) & 0x000000ff);

	//PixelShaderA = PS_GT3;

	drawvertex_GT3(lx0, ly0, u0, v0, rgb1, lx1, ly1, u1, v1, rgb2, lx2, ly2, u2, v2, rgb3);
	drawvertex_GT3(lx1, ly1, u1, v1, rgb2, lx2, ly2, u2, v2, rgb3, lx3, ly3, u3, v3, rgb4);
}

void primPolyF3(unsigned char *baseAddr)
{
	unsigned long *gpuData = ((unsigned long *)baseAddr);
	short ly0, ly1, lx0, lx1, ly2, lx2;
	ly0 = (short)(gpuData[1] >> 16) & 0xffff;
	lx0 = (short)(gpuData[1] & 0xffff);
	ly1 = (short)((gpuData[2] >> 16) & 0xffff);
	lx1 = (short)(gpuData[2] & 0xffff);
	ly2 = (short)((gpuData[3] >> 16) & 0xffff);
	lx2 = (short)(gpuData[3] & 0xffff);
	syslog(22, "!GPU! P3 F %d,%d %d,%d %d,%d\n", lx0, ly0, lx1, ly1, lx2, ly2);
	lx0 += DrawingOffset[0];
	ly0 += DrawingOffset[1];
	lx1 += DrawingOffset[0];
	ly1 += DrawingOffset[1];
	lx2 += DrawingOffset[0];
	ly2 += DrawingOffset[1];

	GPU_TESTRANGE(lx0);
	GPU_TESTRANGE(ly0);
	GPU_TESTRANGE(lx1);
	GPU_TESTRANGE(ly1);
	GPU_TESTRANGE(lx2);
	GPU_TESTRANGE(ly2);

	if(CheckCoord3(lx0, ly0, lx1, ly1, lx2, ly2)) return;

	gpuDriver = gpuDrivers[Masking | ((gpuData[0] >> 24) & 2) | 1];

	int rgb = (gpuData[0]);

	//PixelShaderA = PS_F3;

	drawvertex_F3(lx0, ly0, 0, 0, rgb, lx1, ly1, 0, 0, rgb, lx2, ly2, 0, 0, rgb);
}


void primLineF2(unsigned char *baseAddr)
{
	unsigned long *gpuData = ((unsigned long *)baseAddr);
	short ly0, ly1, lx0, lx1;
	ly0 = (short)((gpuData[1] >> 16) & 0xffff);
	lx0 = (short)((gpuData[1] & 0xffff));
	ly1 = (short)(((gpuData[2] >> 16) & 0xffff));
	lx1 = (short)((gpuData[2] & 0xffff));
	lx0 += (short)DrawingOffset[0];
	ly0 += (short)DrawingOffset[1];
	lx1 += (short)DrawingOffset[0];
	ly1 += (short)DrawingOffset[1];

	syslog(77, "!GPU! P2 F %d,%d %d,%d\n", lx0, ly0, lx1, ly1);

	int cR1 = (gpuData[0] >> 3) & 0x001f0000;
	int cG1 = (gpuData[0] << 5) & 0x001f0000;
	int cB1 = (gpuData[0] << 13) & 0x001f0000;

	gpuDriver = gpuDrivers[Masking | ((*gpuData >> 24) & 2) | 1];

	drawLine(lx0, ly0, lx1, ly1, ((cR1 >> 6) & 0x7c00) | ((cG1 >> 11) & 0x03e0) | ((cB1 >> 16) & 0x001f));
}

void primLineFEx(unsigned char *baseAddr)
{
	unsigned long *gpuData = ((unsigned long *)baseAddr);
	int iMax;
	short lx0, ly0, lx1, ly1; int i;
	syslog(77, "PRIM: LineFEx");
	iMax = 255;

	int cR1 = (gpuData[0] >> 3) & 0x001f0000;
	int cG1 = (gpuData[0] << 5) & 0x001f0000;
	int cB1 = (gpuData[0] << 13) & 0x001f0000;

	ly1 = (short)((gpuData[1] >> 16) & 0xffff);
	lx1 = (short)(gpuData[1] & 0xffff);

	i = 2;

	gpuDriver = gpuDrivers[Masking | ((*gpuData >> 24) & 2) | 1];

	// while(!(gpuData[i]&0x40000000)) 
	// while((gpuData[i]>>24)!=0x55)
	// while((gpuData[i]&0x50000000)!=0x50000000) 
	// currently best way to check for poly line end:
	while (!(((gpuData[i] & 0xF000F000) == 0x50005000) && i >= 3))
	{
		ly0 = ly1; lx0 = lx1;
		ly1 = (short)((gpuData[i] >> 16) & 0xffff);
		lx1 = (short)(gpuData[i] & 0xffff);

		syslog(77, "!GPU! P2 FEx(%d) %d,%d %d,%d\n", i, lx0, ly0, lx1, ly1);

		drawLine(lx0, ly0, lx1, ly1, ((cR1 >> 6) & 0x7c00) | ((cG1 >> 11) & 0x03e0) | ((cB1 >> 16) & 0x001f));

		i++; if (i > iMax) break;
	}
}

void primLineG2(unsigned char *baseAddr)
{
	unsigned long *gpuData = ((unsigned long *)baseAddr);
	short *sgpuData = ((short *)baseAddr);

	short lx0 = sgpuData[2];
	short ly0 = sgpuData[3];
	short lx1 = sgpuData[6];
	short ly1 = sgpuData[7];

	int cR1 = (gpuData[0] >> 3) & 0x001f0000;
	int cG1 = (gpuData[0] << 5) & 0x001f0000;
	int cB1 = (gpuData[0] << 13) & 0x001f0000;

	int cR2 = (gpuData[2] >> 3) & 0x001f0000;
	int cG2 = (gpuData[2] << 5) & 0x001f0000;
	int cB2 = (gpuData[2] << 13) & 0x001f0000;

	syslog(77, "!GPU! P2 G %d,%d %d,%d\n", lx0, ly0, lx1, ly1);

	if ((lx0 == lx1) && (ly0 == ly1)) return;

	gpuDriver = gpuDrivers[Masking | ((*gpuData >> 24) & 2)];

	//if(ClipVertexList4())
	drawLineG(lx0, ly0, lx1, ly1, ((cR1 >> 6) & 0x7c00) | ((cG1 >> 11) & 0x03e0) | ((cB1 >> 16) & 0x001f), ((cR2 >> 6) & 0x7c00) | ((cG2 >> 11) & 0x03e0) | ((cB2 >> 16) & 0x001f));
}

void primLineGEx(unsigned char *baseAddr)
{
	unsigned long *gpuData = ((unsigned long *)baseAddr);
	int iMax = 255;
	short lx0, lx1, ly0, ly1; int i; BOOL bDraw = TRUE;


	ly1 = (short)((gpuData[1] >> 16) & 0xffff);
	lx1 = (short)(gpuData[1] & 0xffff);

	int cR1 = (gpuData[0] >> 3) & 0x001f0000;
	int cG1 = (gpuData[0] << 5) & 0x001f0000;
	int cB1 = (gpuData[0] << 13) & 0x001f0000;

	int cR2 = (gpuData[0] >> 3) & 0x001f0000;
	int cG2 = (gpuData[0] << 5) & 0x001f0000;
	int cB2 = (gpuData[0] << 13) & 0x001f0000;

	i = 2;

	gpuDriver = gpuDrivers[Masking | ((*gpuData >> 24) & 2)];

	//while((gpuData[i]>>24)!=0x55)
	//while((gpuData[i]&0x50000000)!=0x50000000) 
	// currently best way to check for poly line end:
	while (!(((gpuData[i] & 0xF000F000) == 0x50005000) && i >= 4))
	{
		ly0 = ly1; lx0 = lx1;

		cR1 = cR2;
		cG1 = cG2;
		cB1 = cB2;

		cR2 = (gpuData[i] >> 3) & 0x001f0000;
		cG2 = (gpuData[i] << 5) & 0x001f0000;
		cB2 = (gpuData[i] << 13) & 0x001f0000;

		i++;

		ly1 = (short)((gpuData[i] >> 16) & 0xffff);
		lx1 = (short)(gpuData[i] & 0xffff);

		syslog(77, "!GPU! P2 GEx(%d) %d,%d %d,%d\n", i, lx0, ly0, lx1, ly1);

		if ((lx0 != lx1) || (ly0 != ly1)) {
			drawLineG(lx0, ly0, lx1, ly1, ((cR1 >> 6) & 0x7c00) | ((cG1 >> 11) & 0x03e0) | ((cB1 >> 16) & 0x001f), ((cR2 >> 6) & 0x7c00) | ((cG2 >> 11) & 0x03e0) | ((cB2 >> 16) & 0x001f));
		}

		i++;

		if (i>iMax) break;
	}

}

void primTile1(unsigned char * baseAddr)
{
	unsigned long *gpuData = ((unsigned long*)baseAddr);
	short *sgpuData = ((short *)baseAddr);

	int cR1 = (gpuData[0] >> 3) & 0x001f0000;
	int cG1 = (gpuData[0] << 5) & 0x001f0000;
	int cB1 = (gpuData[0] << 13) & 0x001f0000;

	syslog(2, "Tile 1 %d,%d", sgpuData[2], sgpuData[3]);

	gpuDriver = gpuDrivers[Masking | ((*gpuData >> 24) & 2) | 1];

	int xmin = DrawingArea[0];
	int xmax = DrawingArea[2];
	int ymin = DrawingArea[1];
	int ymax = DrawingArea[3];

	if (sgpuData[3] < ymin || sgpuData[3] > ymax || sgpuData[2] < xmin || sgpuData[2] > xmax) return;

	Pixel = &psxVuw[sgpuData[3] * 1024 + sgpuData[2]];
	PixelData = ((cR1 >> 6) & 0x7c00) | ((cG1 >> 11) & 0x03e0) | ((cB1 >> 16) & 0x001f);
	gpuDriver();
}

void primTile8(unsigned char * baseAddr)
{
	unsigned long *gpuData = ((unsigned long*)baseAddr);
	short *sgpuData = ((short *)baseAddr);

	int cR1 = (gpuData[0] >> 3) & 0x001f0000;
	int cG1 = (gpuData[0] << 5) & 0x001f0000;
	int cB1 = (gpuData[0] << 13) & 0x001f0000;

	int vy = sgpuData[3];
	int vx = sgpuData[2];

	syslog(2, "Tile 8 %d,%d", sgpuData[2], sgpuData[3]);

	int x, y;

	int w = 8, h = 8;
	int i = 0, j = 0;

	int xmin = DrawingArea[0];
	int xmax = DrawingArea[2];
	int ymin = DrawingArea[1];
	int ymax = DrawingArea[3];

	w += vx;
	if (w > xmax) w = xmax;
	w -= vx;

	h += vy;
	if (h > ymax) h = ymax;
	h -= vy;

	if (vx < xmin) i = xmin - vx;
	if (vy < ymin) j = ymin - vy;

	gpuDriver = gpuDrivers[Masking | ((*gpuData >> 24) & 2) | 1];

	for (y = j; y < h; y++) {
		for (x = i; x < w; x++) {
			Pixel = &psxVuw[(vy + y) * 1024 + (vx + x)];
			PixelData = ((cR1 >> 6) & 0x7c00) | ((cG1 >> 11) & 0x03e0) | ((cB1 >> 16) & 0x001f);
			gpuDriver();
		}
	}
}

void primTile16(unsigned char * baseAddr)
{
	unsigned long *gpuData = ((unsigned long*)baseAddr);
	short *sgpuData = ((short *)baseAddr);

	int cR1 = (gpuData[0] >> 3) & 0x001f0000;
	int cG1 = (gpuData[0] << 5) & 0x001f0000;
	int cB1 = (gpuData[0] << 13) & 0x001f0000;

	int vy = sgpuData[3];
	int vx = sgpuData[2];

	syslog(2, "Tile 16 %d,%d", sgpuData[2], sgpuData[3]);

	int x, y;

	int w = 16, h = 16;
	int i = 0, j = 0;

	int xmin = DrawingArea[0];
	int xmax = DrawingArea[2];
	int ymin = DrawingArea[1];
	int ymax = DrawingArea[3];

	w += vx;
	if (w > xmax) w = xmax;
	w -= vx;

	h += vy;
	if (h > ymax) h = ymax;
	h -= vy;

	if (vx < xmin) i = xmin - vx;
	if (vy < ymin) j = ymin - vy;

	gpuDriver = gpuDrivers[Masking | ((*gpuData >> 24) & 2) | 1];

	for (y = j; y < h; y++) {
		for (x = i; x < w; x++) {
			Pixel = &psxVuw[(vy + y) * 1024 + (vx + x)];
			PixelData = ((cR1 >> 6) & 0x7c00) | ((cG1 >> 11) & 0x03e0) | ((cB1 >> 16) & 0x001f);
			gpuDriver();
		}
	}
}

void primNI(unsigned char *bA)
{
	if (bA[3]) syslog(255, "PRIM: *NI* %02x", bA[3]);
}

unsigned char primTableC[256] =
{
	// 00
	0,0,3,0,0,0,0,0,
	// 08
	0,0,0,0,0,0,0,0,
	// 10
	0,0,0,0,0,0,0,0,
	// 18
	0,0,0,0,0,0,0,0,
	// 20
	4,4,4,4,7,7,7,7,
	// 28
	5,5,5,5,9,9,9,9,
	// 30
	6,6,6,6,9,9,9,9,
	// 38
	8,8,8,8,12,12,12,12,
	// 40
	3,3,3,3,0,0,0,0,
	// 48
	5,5,5,5,6,6,6,6,
	// 50
	4,4,4,4,0,0,0,0,
	// 58
	7,7,7,7,9,9,9,9,
	// 60
	3,3,3,3,4,4,4,4,
	// 68
	2,2,2,2,0,0,0,0,
	// 70
	2,2,2,2,3,3,3,3,
	// 78
	2,2,2,2,3,3,3,3,
	// 80
	4,0,0,0,0,0,0,0,
	// 88
	0,0,0,0,0,0,0,0,
	// 90
	0,0,0,0,0,0,0,0,
	// 98
	0,0,0,0,0,0,0,0,
	// a0
	3,0,0,0,0,0,0,0,
	// a8
	0,0,0,0,0,0,0,0,
	// b0
	0,0,0,0,0,0,0,0,
	// b8
	0,0,0,0,0,0,0,0,
	// c0
	3,0,0,0,0,0,0,0,
	// c8
	0,0,0,0,0,0,0,0,
	// d0
	0,0,0,0,0,0,0,0,
	// d8
	0,0,0,0,0,0,0,0,
	// e0
	0,1,1,1,1,1,1,0,
	// e8
	0,0,0,0,0,0,0,0,
	// f0
	0,0,0,0,0,0,0,0,
	// f8
	0,0,0,0,0,0,0,0

};

unsigned char primTableCX[256] =
{
	// 00
	0,0,3,0,0,0,0,0,
	// 08
	0,0,0,0,0,0,0,0,
	// 10
	0,0,0,0,0,0,0,0,
	// 18
	0,0,0,0,0,0,0,0,
	// 20
	4,4,4,4,7,7,7,7,
	// 28
	5,5,5,5,9,9,9,9,
	// 30
	6,6,6,6,9,9,9,9,
	// 38
	8,8,8,8,12,12,12,12,
	// 40
	3,3,3,3,0,0,0,0,
	// 48
	254,254,254,254,254,254,254,254,
	// 50
	4,4,4,4,0,0,0,0,
	// 58
	255,255,255,255,255,255,255,255,
	// 60
	3,3,3,3,4,4,4,4,
	// 68
	2,2,2,2,3,3,3,3,
	// 70
	2,2,2,2,3,3,3,3,
	// 78
	2,2,2,2,3,3,3,3,
	// 80
	4,0,0,0,0,0,0,0,
	// 88
	0,0,0,0,0,0,0,0,
	// 90
	0,0,0,0,0,0,0,0,
	// 98
	0,0,0,0,0,0,0,0,
	// a0
	3,0,0,0,0,0,0,0,
	// a8
	0,0,0,0,0,0,0,0,
	// b0
	0,0,0,0,0,0,0,0,
	// b8
	0,0,0,0,0,0,0,0,
	// c0
	3,0,0,0,0,0,0,0,
	// c8
	0,0,0,0,0,0,0,0,
	// d0
	0,0,0,0,0,0,0,0,
	// d8
	0,0,0,0,0,0,0,0,
	// e0
	0,1,1,1,1,1,1,0,
	// e8
	0,0,0,0,0,0,0,0,
	// f0
	0,0,0,0,0,0,0,0,
	// f8
	0,0,0,0,0,0,0,0
};

void(*primTableJ[256])(unsigned char *) =
{
	// 00
	primNI,primNI,primBlkFill,primNI,primNI,primNI,primNI,primNI,
	// 08
	primNI,primNI,primNI,primNI,primNI,primNI,primNI,primNI,
	// 10
	primNI,primNI,primNI,primNI,primNI,primNI,primNI,primNI,
	// 18
	primNI,primNI,primNI,primNI,primNI,primNI,primNI,primNI,
	// 20
	primPolyF3,primPolyF3,primPolyF3,primPolyF3,primPolyFT3,primPolyFT3,primPolyFT3,primPolyFT3,
	// 28
	primPolyF4,primPolyF4,primPolyF4,primPolyF4,primPolyFT4,primPolyFT4,primPolyFT4,primPolyFT4,
	// 30
	primPolyG3,primPolyG3,primPolyG3,primPolyG3,primPolyGT3,primPolyGT3,primPolyGT3,primPolyGT3,
	// 38
	primPolyG4,primPolyG4,primPolyG4,primPolyG4,primPolyGT4,primPolyGT4,primPolyGT4,primPolyGT4,
	// 40
	primLineF2,primLineF2,primLineF2,primLineF2,primNI,primNI,primNI,primNI,
	// 48
	primLineFEx,primLineFEx,primLineFEx,primLineFEx,primLineFEx,primLineFEx,primLineFEx,primLineFEx,
	// 50
	primLineG2,primLineG2,primLineG2,primLineG2,primNI,primNI,primNI,primNI,
	// 58
	primLineGEx,primLineGEx,primLineGEx,primLineGEx,primLineGEx,primLineGEx,primLineGEx,primLineGEx,
	// 60
	primTileS,primTileS,primTileS,primTileS,primSprtS,primSprtS,primSprtS,primSprtS,
	// 68
	primTile1,primTile1,primTile1,primTile1,primNI,primNI,primNI,primNI,
	// 70
	primTile8,primTile8,primTile8,primTile8,primSprt8,primSprt8,primSprt8,primSprt8,
	// 78
	primTile16,primTile16,primTile16,primTile16,primSprt16,primSprt16,primSprt16,primSprt16,
	// 80
	primMoveImage,primNI,primNI,primNI,primNI,primNI,primNI,primNI,
	// 88
	primNI,primNI,primNI,primNI,primNI,primNI,primNI,primNI,
	// 90
	primNI,primNI,primNI,primNI,primNI,primNI,primNI,primNI,
	// 98
	primNI,primNI,primNI,primNI,primNI,primNI,primNI,primNI,
	// a0
	primLoadImage,primNI,primNI,primNI,primNI,primNI,primNI,primNI,
	// a8
	primNI,primNI,primNI,primNI,primNI,primNI,primNI,primNI,
	// b0
	primNI,primNI,primNI,primNI,primNI,primNI,primNI,primNI,
	// b8
	primNI,primNI,primNI,primNI,primNI,primNI,primNI,primNI,
	// c0
	primStoreImage,primNI,primNI,primNI,primNI,primNI,primNI,primNI,
	// c8
	primNI,primNI,primNI,primNI,primNI,primNI,primNI,primNI,
	// d0
	primNI,primNI,primNI,primNI,primNI,primNI,primNI,primNI,
	// d8
	primNI,primNI,primNI,primNI,primNI,primNI,primNI,primNI,
	// e0
	primNI,cmdTexturePage,cmdTextureWindow,cmdDrawAreaStart,cmdDrawAreaEnd,cmdDrawOffset,cmdSTP,primNI,
	// e8
	primNI,primNI,primNI,primNI,primNI,primNI,primNI,primNI,
	// f0
	primNI,primNI,primNI,primNI,primNI,primNI,primNI,primNI,
	// f8
	primNI,primNI,primNI,primNI,primNI,primNI,primNI,primNI
};

