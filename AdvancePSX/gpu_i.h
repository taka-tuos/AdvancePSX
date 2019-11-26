#ifndef _GPU_INTERNALS_H
#define _GPU_INTERNALS_H

#ifdef GPU_INTERNALS_DEF
#define GPUVAR_DEF
#else
#define GPUVAR_DEF extern
#endif

#define INITGUID
#include <windows.h>

void syslog(int level, char *fmt,...);

extern unsigned int lutBGR2RGB[65536];

extern unsigned short textBuf[];

extern long GPUstatusRet;

#define	FRAME_WIDTH			1024
#define	FRAME_HEIGHT		512

#define	FRAME_OFFSET(x,y)	(((y)<<10)+(x))
/*
GPUVAR_DEF long textAddrY,textAddrX, textAddrX2;

GPUVAR_DEF long drawY;
GPUVAR_DEF long drawX;
GPUVAR_DEF long drawW;
GPUVAR_DEF long drawH;
GPUVAR_DEF long ofsX;
GPUVAR_DEF long ofsY;
GPUVAR_DEF long textTP;
GPUVAR_DEF long textREST;
GPUVAR_DEF long textABR;
*/
GPUVAR_DEF HWND WinWnd;
GPUVAR_DEF long TextureWindow[4];
GPUVAR_DEF long DrawingArea[4];
GPUVAR_DEF long DrawingOffset[2];

extern long imageTransfer;
extern long drawingLines;

extern short imTYc,imTXc,imTY,imTX;
extern long imSize;
extern short imageX0,imageX1;
extern short imageY0,imageY1;

extern long newTextX0,newTextX1,newTextX2,newTextX3;
extern long newTextY0,newTextY1,newTextY2,newTextY3;
extern unsigned short textBuf[];

extern unsigned char psxVub[];
extern signed char *psxVsb;
extern unsigned short *psxVuw;
extern signed short *psxVsw;
extern unsigned long *psxVul;
extern signed long *psxVsl;

extern long FrameToRead;
extern long FrameToWrite;
extern long FrameWidth;
extern long FrameCount;
extern long FrameIndex;

typedef struct
{
	char scanH;
	char scanV;
} GpuConfS;

#endif // _GPU_INTERNALS_H