#include "a3d.h"
#include <math.h>
#include <stdint.h>

#include "LFloat.h"

#include "shader.h"

#define INTERP(xi,xi1,x) (((x - xi) << QUOL) / (xi1 - xi))

#define	GPU_RGB16(rgb)	\
	((((rgb)&0xF80000)>>9)|(((rgb)&0xF800)>>6)|(((rgb)&0xF8)>>3))

#define R_8(rgb) ((rgb >> 16) & 0xff)
#define G_8(rgb) ((rgb >>  8) & 0xff)
#define B_8(rgb) ((rgb >>  0) & 0xff)

int a3d_lnstep(a3d_liner *o)
{
	//o->dx++;
	/*if (o->x1 - o->x0 == 0)
		o->y0 = o->y0;
	else
		o->y0 = INTERP(o->x0, o->x1, o->dx);*/

	o->y1 += o->dy;
	o->y0 = (o->y1 + (QUOL2_NUM >> 1)) >> QUOL2;

	return 0;
}

void a3d_lninit(a3d_liner *o, int x0, int x1)
{
	int a = x0;
	int b = x1;
	int l = (b - a);

	o->x0 = a;
	o->x1 = b;

	o->y0 = 0;
	o->y1 = 0;

	if (l) o->dy = (QUOL_NUM << QUOL2) / l;
	else o->dy = 0;
}
//----------------------------------------------------------------

a3d_bitmap a3d_createbit(void *pix, int w, int h)
{
	a3d_bitmap obj;
	obj.pixel = pix;
	obj.w = w;
	obj.h = h;

	return obj;
}

#define SMOOTH 0
#define TEXTURE 0
#define VERTEX 3
void a3d_drawpoly_F3(a3d_bitmap dst, a3d_vertex *v)
#include "a3d_sub.h"

#define SMOOTH 0
#define TEXTURE 1
#define VERTEX 3
void a3d_drawpoly_FT3(a3d_bitmap dst, a3d_vertex *v)
#include "a3d_sub.h"

#define SMOOTH 1
#define TEXTURE 0
#define VERTEX 3
void a3d_drawpoly_G3(a3d_bitmap dst, a3d_vertex *v)
#include "a3d_sub.h"

#define SMOOTH 1
#define TEXTURE 1
#define VERTEX 3
void a3d_drawpoly_GT3(a3d_bitmap dst, a3d_vertex *v)
#include "a3d_sub.h"