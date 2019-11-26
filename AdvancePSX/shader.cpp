#include <math.h>
#include <stdint.h>
#include "gpu_i.h"
#include "shader.h"

/*----------------------------------------------------------------------
Macro
----------------------------------------------------------------------*/

#define	GPU_RGB16(rgb)	\
	((((rgb)&0xF80000)>>9)|(((rgb)&0xF800)>>6)|(((rgb)&0xF8)>>3))
#define	FRAME_OFFSET(x,y)	(((y)<<10)+(x))

//#define CHECK_BUFFER int __p = (Pixel - psxVuw); int __x = __p % 1024; int __y = __p / 1024; if (((__x >= DrawingArea[0] && __y >= DrawingArea[1] && __x < DrawingArea[2] && __y < DrawingArea[3]) || CheckOverFlag) && __p >= 0 && __p < 1024 * 520)  
#define CHECK_BUFFER

/*----------------------------------------------------------------------
Global
----------------------------------------------------------------------*/

ushort	*Pixel;
ushort	PixelMSB;
ushort	PixelData;
ushort	*TextureBaseAddress;
ushort	*ClutBaseAddress;
uchar	TextureLocation[2];
uchar	LightColour[3];
uchar	CheckOverFlag = 0;

/*----------------------------------------------------------------------
Table
----------------------------------------------------------------------*/

uchar TextureMask[32] = {
	255, 7, 15, 7, 31, 7, 15, 7, 63, 7, 15, 7, 31, 7, 15, 7,	//
	127, 7, 15, 7, 31, 7, 15, 7, 63, 7, 15, 7, 31, 7, 15, 7	//
};

/*----------------------------------------------------------------------
Lighting
----------------------------------------------------------------------*/

void gpuLighting(void)
{
	short rgb, rr, gg, bb;
	rgb = PixelData;
	bb = ((rgb & 0x7C00) * _LB) >> (7 + 10);
	gg = ((rgb & 0x03E0) * _LG) >> (7 + 5);
	rr = ((rgb & 0x001F) * _LR) >> (7 + 0);
	bb -= 31;
	gg -= 31;
	rr -= 31;
	bb &= (bb >> 31);
	gg &= (gg >> 31);
	rr &= (rr >> 31);
	bb += 31;
	gg += 31;
	rr += 31;
	PixelData = (rgb & 0x8000) | (bb << 10) | (gg << 5) | (rr);
}

/*----------------------------------------------------------------------
Blending
----------------------------------------------------------------------*/

/*	0.5 x Back + 0.5 x Forward	*/
void gpuBlending00(void)
{
	CHECK_BUFFER *Pixel = PixelMSB | (((*Pixel & 0x7BDE) + (PixelData & 0x7BDE)) >> 1);
}

/*	1.0 x Back + 1.0 x Forward	*/
void gpuBlending01(void)
{
	CHECK_BUFFER
	{
		short bk, fr, rr, gg, bb;
		bk = *Pixel;
		fr = PixelData;
		bb = (bk & 0x7C00) + (fr & 0x7C00);
		gg = (bk & 0x03E0) + (fr & 0x03E0);
		rr = (bk & 0x001F) + (fr & 0x001F);
		bb -= 0x7C00;
		gg -= 0x03E0;
		rr -= 0x001F;
		bb &= (bb >> 31);
		gg &= (gg >> 31);
		rr &= (rr >> 31);
		bb += 0x7C00;
		gg += 0x03E0;
		rr += 0x001F;
		*Pixel = PixelMSB | bb | gg | rr;
	}
}

/*	1.0 x Back - 1.0 x Forward	*/
void gpuBlending02(void)
{
	CHECK_BUFFER
	{
		short bk, fr, rr, gg, bb;
		bk = *Pixel;
		fr = PixelData;
		bb = (bk & 0x7C00) - (fr & 0x7C00);
		gg = (bk & 0x03E0) - (fr & 0x03E0);
		rr = (bk & 0x001F) - (fr & 0x001F);
		bb &= ~(bb >> 31);
		gg &= ~(gg >> 31);
		rr &= ~(rr >> 31);
		*Pixel = PixelMSB | bb | gg | rr;
	}
}

/*	1.0 x Back + 0.25 x Forward	*/
void gpuBlending03(void)
{
	CHECK_BUFFER
	{
		short bk, fr, rr, gg, bb;
		bk = *Pixel;
		fr = PixelData >> 2;
		bb = (bk & 0x7C00) + (fr & 0x1C00);
		gg = (bk & 0x03E0) + (fr & 0x00E0);
		rr = (bk & 0x001F) + (fr & 0x0007);
		bb -= 0x7C00;
		gg -= 0x03E0;
		rr -= 0x001F;
		bb &= (bb >> 31);
		gg &= (gg >> 31);
		rr &= (rr >> 31);
		bb += 0x7C00;
		gg += 0x03E0;
		rr += 0x001F;
		*Pixel = PixelMSB | bb | gg | rr;
	}
}

/*	Function Pointer	*/
void(*gpuBlending) (void);

/* Table */
void (*gpuBlendings[4])(void) = {
	gpuBlending00, gpuBlending01, gpuBlending02, gpuBlending03
};

/*----------------------------------------------------------------------
Texture Mapping
----------------------------------------------------------------------*/

/*	4bitCLUT	*/
void gpuTextureMapping00(void)
{
	uchar tu, tv;
	ushort rgb;
	tu = _TU & _TUM;
	tv = _TV & _TVM;
	rgb = _TBA[FRAME_OFFSET(tu >> 2, tv)];
	tu = (tu & 3) << 2;
	PixelData = _CBA[(rgb >> tu) & 15];
}

/*	8bitCLUT	*/
void gpuTextureMapping01(void)
{
	uchar tu, tv;
	ushort rgb;
	tu = _TU & _TUM;
	tv = _TV & _TVM;
	rgb = _TBA[FRAME_OFFSET(tu >> 1, tv)];
	tu = (tu & 1) << 3;
	PixelData = _CBA[(rgb >> tu) & 255];
}

/*	15bitDirect	*/
void gpuTextureMapping02(void)
{
	uchar tu, tv;
	tu = _TU & _TUM;
	tv = _TV & _TVM;
	PixelData = _TBA[FRAME_OFFSET(tu, tv)];
}

void gpuTextureMapping03(void)
{

}

/*	4bitCLUT(2)	*/
void gpuTextureMapping04(void)
{
	uchar tu, tv;
	ushort rgb;
	tu = _TU;
	tv = _TV;
	rgb = _TBA[FRAME_OFFSET(tu >> 2, tv)];
	tu = (tu & 3) << 2;
	PixelData = _CBA[(rgb >> tu) & 15];
}

/*	8bitCLUT(2)	*/
void gpuTextureMapping05(void)
{
	uchar tu, tv;
	ushort rgb;
	tu = _TU;
	tv = _TV;
	rgb = _TBA[FRAME_OFFSET(tu >> 1, tv)];
	tu = (tu & 1) << 3;
	PixelData = _CBA[(rgb >> tu) & 255];
}

/*	15bitDirect(2)	*/
void gpuTextureMapping06(void)
{
	PixelData = _TBA[FRAME_OFFSET(_TU, _TV)];
}

/*	Function Pointer	*/
void(*gpuTextureMapping) (void);

/*	Table	*/
void (*gpuTextureMappings[8])(void) = {
	gpuTextureMapping00,	//
	gpuTextureMapping01,	//
	gpuTextureMapping02,	//
	gpuTextureMapping00,	//
	gpuTextureMapping04,	//
	gpuTextureMapping05,	//
	gpuTextureMapping06,	//
	gpuTextureMapping00		//
};

/*----------------------------------------------------------------------
Driver
----------------------------------------------------------------------*/

/* Function Pointer */
void(*gpuDriver) (void);

#define	GPU_MASKING()													\
{																		\
	CHECK_BUFFER if (*Pixel & 0x8000) return;										\
}

#define	GPU_TEXTUREMAPPING()											\
{																		\
	gpuTextureMapping();												\
	if (!PixelData) return;												\
}

#define	GPU_LIGHTING()													\
{																		\
	gpuLighting();														\
}

#define	GPU_BLENDING()													\
{																		\
	gpuBlending();														\
}

#define	GPU_BLENDING_STP()												\
{																		\
	if (PixelData & 0x8000) {											\
		gpuBlending();													\
	} else {															\
		CHECK_BUFFER *Pixel = PixelMSB | (PixelData & 0x7FFF);			\
	}																	\
}

#define	GPU_NOBLENDING()												\
{																		\
	CHECK_BUFFER *Pixel = PixelMSB | (PixelData & 0x7FFF);				\
}

void gpuDriver00(void)
{
	GPU_LIGHTING();
	GPU_NOBLENDING();
}
void gpuDriver01(void)
{
	GPU_NOBLENDING();
}
void gpuDriver02(void)
{
	GPU_LIGHTING();
	GPU_BLENDING();
}
void gpuDriver03(void)
{
	GPU_BLENDING();
}
void gpuDriver04(void)
{
	GPU_TEXTUREMAPPING();
	GPU_LIGHTING();
	GPU_NOBLENDING();
}
void gpuDriver05(void)
{
	GPU_TEXTUREMAPPING();
	GPU_NOBLENDING();
}
void gpuDriver06(void)
{
	GPU_TEXTUREMAPPING();
	GPU_LIGHTING();
	GPU_BLENDING_STP();
}
void gpuDriver07(void)
{
	GPU_TEXTUREMAPPING();
	GPU_BLENDING_STP();
}
void gpuDriver08(void)
{
	GPU_MASKING();
	GPU_LIGHTING();
	GPU_NOBLENDING();
}
void gpuDriver09(void)
{
	GPU_MASKING();
	GPU_NOBLENDING();
}
void gpuDriver0A(void)
{
	GPU_MASKING();
	GPU_LIGHTING();
	GPU_BLENDING();
}
void gpuDriver0B(void)
{
	GPU_MASKING();
	GPU_BLENDING();
}
void gpuDriver0C(void)
{
	GPU_MASKING();
	GPU_TEXTUREMAPPING();
	GPU_LIGHTING();
	GPU_NOBLENDING();
}
void gpuDriver0D(void)
{
	GPU_MASKING();
	GPU_TEXTUREMAPPING();
	GPU_NOBLENDING();
}
void gpuDriver0E(void)
{
	GPU_MASKING();
	GPU_TEXTUREMAPPING();
	GPU_LIGHTING();
	GPU_BLENDING_STP();
}
void gpuDriver0F(void)
{
	GPU_MASKING();
	GPU_TEXTUREMAPPING();
	GPU_BLENDING_STP();
}

void (*gpuDrivers[16])(void) = {
	gpuDriver00,	//
	gpuDriver01,	//
	gpuDriver02,	//
	gpuDriver03,	//
	gpuDriver04,	//
	gpuDriver05,	//
	gpuDriver06,	//
	gpuDriver07,	//
	gpuDriver08,	//
	gpuDriver09,	//
	gpuDriver0A,	//
	gpuDriver0B,	//
	gpuDriver0C,	//
	gpuDriver0D,	//
	gpuDriver0E,	//
	gpuDriver0F	//
};

/*----------------------------------------------------------------------
Texture Setting
----------------------------------------------------------------------*/

void gpuSetTexture(long tpage)
{
	long tp;
	long tx, ty;
	GPUstatusRet = (GPUstatusRet & ~0x1FF) | (tpage & 0x1FF);
	gpuBlending = gpuBlendings[(tpage >> 5) & 3];
	tp = (tpage >> 7) & 3;
	tx = (tpage & 0x0F) << 6;
	ty = (tpage & 0x10) << 4;
	tx += (TextureWindow[0] >> (2 - tp));
	ty += TextureWindow[1];
	_TBA = &psxVuw[FRAME_OFFSET(tx, ty)];
	tp |= (((_TUM & _TVM) >> 5) & 4);
	gpuTextureMapping = gpuTextureMappings[tp];
}

/*----------------------------------------------------------------------
CLUT Setting
----------------------------------------------------------------------*/

void gpuSetCLUT(ushort clut)
{
	_CBA = &psxVuw[(clut & 0x7FFF) << 4];
}