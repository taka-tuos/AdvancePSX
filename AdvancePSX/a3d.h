#include <string.h>
#include <stdlib.h>
#include <stdint.h>

#ifndef __A3D__
#define __A3D__

#define QUOL 10
#define QUOL_NUM (1 << QUOL)

#define QUOL2 10
#define QUOL2_NUM (1 << QUOL2)

#define FIX_MIX_EX(a1,a2,f) (a2 * f + a1 * (QUOL_NUM - f))
#define FIX_MIX_NM(a1,a2,f) (a2 * f + a1 * (QUOL_NUM - f) + (QUOL_NUM >> 1)) >> QUOL
#define FIX_MIX(a1,a2,f) (a2 * f + a1 * (QUOL_NUM - f) + (1 << ((QUOL * 2)-1))) >> (QUOL * 2)

typedef struct {
	int x,y,u,v,c;
} a3d_vertex;

typedef struct {
	int dx,dy,err,a,e2,sx,sy;
	int x0,y0,x1,y1,x2,y2;
} a3d_liner;

typedef struct {
	int w,h;
	void *pixel;
} a3d_bitmap;

typedef void (*A3D_DRAWPOLY)(a3d_bitmap dst, a3d_vertex *v, void(*PixelShader)(int u, int v, int r, int g, int b));

void a3d_drawpoly_F3(a3d_bitmap dst, a3d_vertex *v);
void a3d_drawpoly_FT3(a3d_bitmap dst, a3d_vertex *v);
void a3d_drawpoly_G3(a3d_bitmap dst, a3d_vertex *v);
void a3d_drawpoly_GT3(a3d_bitmap dst, a3d_vertex *v);

void a3d_drawpoly(a3d_bitmap dst, a3d_vertex *v, int vnum, void (*PixelShader)(int u, int v, int r, int g, int b));
a3d_bitmap a3d_createbit(void *pix, int w, int h);

extern void PixelShader(int u, int v, int r, int g, int b);

#endif
