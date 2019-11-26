#ifndef __SHADER_H__
#define __SHADER_H__

/* Short Name of Global */
#define	_TBA	TextureBaseAddress
#define	_CBA	ClutBaseAddress
#define	_TU		TextureLocation[0]
#define	_TV		TextureLocation[1]
#define	_TUM	TextureWindow[2]
#define	_TVM	TextureWindow[3]
#define	_LR		LightColour[0]
#define	_LG		LightColour[1]
#define	_LB		LightColour[2]

#define DISABLE_CHECK CheckOverFlag=1;
#define ENABLE_CHECK CheckOverFlag=0;

extern void(*gpuDrivers[])(void);
extern void(*gpuDriver)(void);
//extern void gpuDriver(void);

typedef unsigned char uchar;
typedef signed char schar;
typedef unsigned short ushort;
typedef unsigned long ulong;

void gpuSetTexture(long tpage);
void gpuSetCLUT(ushort clut);

extern ushort	*Pixel;
extern ushort	PixelMSB;
extern ushort	PixelData;
extern ushort	*TextureBaseAddress;
extern ushort	*ClutBaseAddress;
extern uchar	TextureLocation[2];
extern uchar	LightColour[3];
extern uchar	TextureMask[];
extern uchar	CheckOverFlag;
extern long		DrawingArea[4];

#endif
