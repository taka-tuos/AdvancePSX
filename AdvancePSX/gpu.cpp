/*
PSEmu Plugin

Plugin Type: GPU
Plugin Name: Software Driver
Description: All drawing is performed with software routines and DirectX3 is
used only to obtain pointer to surface.

(C)1997-1998 Vision Thing Software Group

All rights reserved. Using this source or part of it without permission
is highly prohibited.

Changes, bug reports - plugin@psemu.com
*/

#include <stdlib.h>
#include <stdio.h>
#include <stdarg.h>
#include <string.h>
#define GPU_INTERNALS_DEF
#include "gpu_i.h"
#include "draw.h"
#include "prim.h"
#include "define.h"
#include "syslog.h"

// PPDK developer must change libraryName field and can change revision and build

const unsigned char version = PLUGIN_VERSION;	// do not touch - library for PSEmu 1.x

												// it is up to developer but values must be in range 0-255

const unsigned char revision = 1;
const unsigned char build = 0;

// to obtain library name for your plugin, mail: plugin@psemu.com
// this must be unique, and only we can provide this
char *libraryName = "AdvancePSX GPU Plugin";


// driver dependant variables

GpuConfS gpuConfig;

typedef struct
{
	unsigned char bpp;
	unsigned short width;
	unsigned short height;
} GpuDispModeS;

unsigned long found32, found16;

static int ScreenOpened = 0;
// internally used defines
static long initGPU = 0;

#define RED(x) (x & 0xff)
#define BLUE(x) ((x>>16) & 0xff)
#define GREEN(x) ((x>>8) & 0xff)

#define COLOR(x) (x & 0xffffff)

// macros for easy access to packet information
#define GPUCOMMAND(x) ((x>>24) & 0xff)

#define	GPU_RGB16(rgb)	\
	((((rgb)&0xF80000)>>9)|(((rgb)&0xF800)>>6)|(((rgb)&0xF8)>>3))

// memory image of the PSX vram 
unsigned char psxVub[1024 * 512 * 2];
signed char *psxVsb;
unsigned short *psxVuw;
signed short *psxVsw;
unsigned long *psxVul;
signed long *psxVsl;

long FrameToRead;
long FrameToWrite;
long FrameWidth;
long FrameCount;
long FrameIndex;

BYTE surf[640 * 480 * 4];

HDC WinDC;

long flip;

// internal GPU

static long GPUdataRet;
long GPUstatusRet;

static unsigned long gpuData[16777216];
static unsigned char gpuCommand = 0;
static long gpuDataC = 0;
long gpuDataP = 0;

long drawingLines;
long imageTransfer;


short imTYc, imTXc, imTY, imTX;
long imSize;
short imageX0, imageX1;
short imageY0, imageY1;

unsigned short textBuf[512 * 512];
long newTextX0, newTextX1, newTextX2, newTextX3;
long newTextY0, newTextY1, newTextY2, newTextY3;

long gpuDataX;

unsigned short *fb;

long HorizontalResolution[8] = {
	256, 368, 320, 384, 512, 512, 640, 640
};

long VerticalResolution[4] = {
	240, 480, 240, 480
};

long DisplayArea[8];

char * CALLBACK PSEgetLibName(void)
{
	return libraryName;
}

unsigned long CALLBACK PSEgetLibType(void)
{
	return  PSE_LT_GPU;
}

unsigned long CALLBACK PSEgetLibVersion(void)
{
	return version << 16 | revision << 8 | build;
}

void gpuReset(void)
{
	GPUstatusRet = 0x14802000;
	ZeroMemory(TextureWindow, sizeof TextureWindow);
	TextureWindow[2] = 255;
	TextureWindow[3] = 255;
	DrawingArea[2] = 256;
	DrawingArea[3] = 240;
	DisplayArea[2] = 256;
	DisplayArea[3] = 240;
	DisplayArea[6] = 256;
	DisplayArea[7] = 240;
}


void CALLBACK GPUmakeSnapshot(void)
{
	FILE *bmpfile;
	char filename[256];
	unsigned char header[0x36];
	long size;
	unsigned char line[1024 * 3];
	short i, j;
	unsigned char empty[2] = { 0,0 };
	unsigned short color;
	unsigned long snapshotnr = 0;

	size = 512 * 1024 * 3 + 0x38;

	// fill in proper values for BMP

	// hardcoded BMP header
	memset(header, 0, 0x36);
	header[0] = 'B';
	header[1] = 'M';
	header[2] = size & 0xff;
	header[3] = (size >> 8) & 0xff;
	header[4] = (size >> 16) & 0xff;
	header[5] = (size >> 24) & 0xff;
	header[0x0a] = 0x36;
	header[0x0e] = 0x28;
	header[0x12] = 1024 % 256;
	header[0x13] = 1024 / 256;
	header[0x16] = 512 % 256;
	header[0x17] = 512 / 256;
	header[0x1a] = 0x01;
	header[0x1c] = 0x18;
	header[0x26] = 0x12;
	header[0x27] = 0x0B;
	header[0x2A] = 0x12;
	header[0x2B] = 0x0B;


	// increment snapshot value
	// get filename
	do
	{
		snapshotnr++;
		sprintf(filename, "SNAP\\ps%d.bmp", snapshotnr);
		bmpfile = fopen(filename, "rb");
		if (bmpfile == NULL) break;
		fclose(bmpfile);
	} while (TRUE);

	// try opening new snapshot file
	if ((bmpfile = fopen(filename, "wb")) == NULL)
	{
		// could not open for writing !
		syslog(99, "PSE: Cannot snapshot to: [%s]\n", filename);
		return;
	}

	fwrite(header, 0x36, 1, bmpfile);
	for (i = 511; i >= 0; i--)
	{
		for (j = 0; j<1024; j++)
		{
			color = psxVuw[i * 1024 + j];
			line[j * 3 + 2] = (color << 3) & 0xf1;
			line[j * 3 + 1] = (color >> 2) & 0xf1;
			line[j * 3 + 0] = (color >> 7) & 0xf1;
		}
		fwrite(line, 1024 * 3, 1, bmpfile);
	}
	fwrite(empty, 0x2, 1, bmpfile);
	fclose(bmpfile);

}

long CALLBACK GPUinit()
{
	psxVsb = (signed char *)psxVub;
	psxVsw = (signed short *)psxVub;
	psxVsl = (signed long *)psxVub;
	psxVuw = (unsigned short *)psxVub;
	psxVul = (unsigned long *)psxVub;

	fillLookupTables();
	GPUstatusRet = 0x14802000;
	return 0;
}


long CALLBACK GPUshutdown()
{
	// shutdown device here....
	if (initGPU == 1)
	{
		initGPU = 0;
	}
	return 0;
}

#include <ddraw.h>

void DDError(HRESULT hErr)
{
	char dderr[256];
	char string[1024];

	switch (hErr)
	{
	case DDERR_DDSCAPSCOMPLEXREQUIRED: sprintf(dderr, "DDERR_DDSCAPSCOMPLEXREQUIRED: New for DirectX 7.0. The surface requires the DDSCAPS_COMPLEX flag."); break;
	case DDERR_DEVICEDOESNTOWNSURFACE: sprintf(dderr, "DDERR_DEVICEDOESNTOWNSURFACE: Surfaces created by one DirectDraw device cannot be used directly by another DirectDraw device."); break;
	case DDERR_EXPIRED: sprintf(dderr, "DDERR_EXPIRED: The data has expired and is therefore no longer valid."); break;
	case DDERR_INVALIDSTREAM: sprintf(dderr, "DDERR_INVALIDSTREAM: The specified stream contains invalid data."); break;
	case DDERR_MOREDATA: sprintf(dderr, "DDERR_MOREDATA: There is more data available than the specified buffer size can hold."); break;
	case DDERR_NEWMODE: sprintf(dderr, "DDERR_NEWMODE: New for DirectX 7.0. When IDirectDraw7::StartModeTest is called with the DDSMT_ISTESTREQUIRED flag, it may return this value to denote that some or all of the resolutions can and should be tested. IDirectDraw7::EvaluateMode returns this value to indicate that the test has switched to a new display mode."); break;
	case DDERR_NODRIVERSUPPORT: sprintf(dderr, "DDERR_NODRIVERSUPPORT: New for DirectX 7.0. Testing cannot proceed because the display adapter driver does not enumerate refresh rates."); break;
	case DDERR_NOFOCUSWINDOW: sprintf(dderr, "DDERR_NOFOCUSWINDOW: An attempt was made to create or set a device window without first setting the focus window."); break;
	case DDERR_NOMONITORINFORMATION: sprintf(dderr, "DDERR_NOMONITORINFORMATION: New for DirectX 7.0. Testing cannot proceed because the monitor has no associated EDID data."); break;
	case DDERR_NONONLOCALVIDMEM: sprintf(dderr, "DDERR_NONONLOCALVIDMEM: An attempt was made to allocate nonlocal video memory from a device that does not support nonlocal video memory."); break;
	case DDERR_NOOPTIMIZEHW: sprintf(dderr, "DDERR_NOOPTIMIZEHW: The device does not support optimized surfaces."); break;
	case DDERR_NOSTEREOHARDWARE: sprintf(dderr, "DDERR_NOSTEREOHARDWARE: There is no stereo hardware present or available."); break;
	case DDERR_NOSURFACELEFT: sprintf(dderr, "DDERR_NOSURFACELEFT: There is no hardware present that supports stereo surfaces."); break;
	case DDERR_NOTLOADED: sprintf(dderr, "DDERR_NOTLOADED: The surface is an optimized surface, but it has not yet been allocated any memory."); break;
	case DDERR_OVERLAPPINGRECTS: sprintf(dderr, "DDERR_OVERLAPPINGRECTS: The source and destination rectangles are on the same surface and overlap each other."); break;
	case DDERR_TESTFINISHED: sprintf(dderr, "DDERR_TESTFINISHED: New for DirectX 7.0. When returned by the IDirectDraw7::StartModeTest method, this value means that no test could be initiated because all the resolutions chosen for testing already have refresh rate information in the registry. When returned by IDirectDraw7::EvaluateMode, the value means that DirectDraw has completed a refresh rate test."); break;
	case DDERR_VIDEONOTACTIVE: sprintf(dderr, "DDERR_VIDEONOTACTIVE: The video port is not active."); break;
	case DDERR_ALREADYINITIALIZED: sprintf(dderr, "DDERR_ALREADYINITIALIZED: The object has already been initialized."); break;
	case DDERR_CANNOTATTACHSURFACE: sprintf(dderr, "DDERR_CANNOTATTACHSURFACE: A surface cannot be attached to another requested surface."); break;
	case DDERR_CANNOTDETACHSURFACE: sprintf(dderr, "DDERR_CANNOTDETACHSURFACE: A surface cannot be detached from another requested surface."); break;
	case DDERR_CURRENTLYNOTAVAIL: sprintf(dderr, "DDERR_CURRENTLYNOTAVAIL: No support is currently available"); break;
	case DDERR_EXCEPTION: sprintf(dderr, "DDERR_EXCEPTION: An exception was encountered while performing the requested operation."); break;
	case DDERR_GENERIC: sprintf(dderr, "DDERR_GENERIC: There is an undefined error condition."); break;
	case DDERR_HEIGHTALIGN: sprintf(dderr, "DDERR_HEIGHTALIGN: The height of the provided rectangle is not a multiple of the required alignment."); break;
	case DDERR_INCOMPATIBLEPRIMARY: sprintf(dderr, "DDERR_INCOMPATIBLEPRIMARY: The primary surface creation request does not match the existing primary surface."); break;
	case DDERR_INVALIDCAPS: sprintf(dderr, "DDERR_INVALIDCAPS: One or more of the capability bits passed to the callback function are incorrect."); break;
	case DDERR_INVALIDCLIPLIST: sprintf(dderr, "DDERR_INVALIDCLIPLIST: DirectDraw does not support the provided clip list."); break;
	case DDERR_INVALIDMODE: sprintf(dderr, "DDERR_INVALIDMODE: DirectDraw does not support the requested mode."); break;
	case DDERR_INVALIDOBJECT: sprintf(dderr, "DDERR_INVALIDOBJECT: DirectDraw received a pointer that was an invalid DirectDraw object."); break;
	case DDERR_INVALIDPARAMS: sprintf(dderr, "DDERR_INVALIDPARAMS: One or more of the parameters passed to the method are incorrect."); break;
	case DDERR_INVALIDPIXELFORMAT: sprintf(dderr, "DDERR_INVALIDPIXELFORMAT: The pixel format was invalid as specified."); break;
	case DDERR_INVALIDRECT: sprintf(dderr, "DDERR_INVALIDRECT: The provided rectangle was invalid."); break;
	case DDERR_LOCKEDSURFACES: sprintf(dderr, "DDERR_LOCKEDSURFACES: One or more surfaces are locked, causing the failure of the requested operation."); break;
	case DDERR_NO3D: sprintf(dderr, "DDERR_NO3D: No 3-D hardware or emulation is present."); break;
	case DDERR_NOALPHAHW: sprintf(dderr, "DDERR_NOALPHAHW: No alpha-acceleration hardware is present or available, causing the failure of the requested operation."); break;
	case DDERR_NOCLIPLIST: sprintf(dderr, "DDERR_NOCLIPLIST: No clip list is available."); break;
	case DDERR_NOCOLORCONVHW: sprintf(dderr, "DDERR_NOCOLORCONVHW: No color-conversion hardware is present or available."); break;
	case DDERR_NOCOOPERATIVELEVELSET: sprintf(dderr, "DDERR_NOCOOPERATIVELEVELSET: A create function was called without the IDirectDraw7::SetCooperativeLevel method."); break;
	case DDERR_NOCOLORKEY: sprintf(dderr, "DDERR_NOCOLORKEY: The surface does not currently have a color key."); break;
	case DDERR_NOCOLORKEYHW: sprintf(dderr, "DDERR_NOCOLORKEYHW: There is no hardware support for the destination color key."); break;
	case DDERR_NODIRECTDRAWSUPPORT: sprintf(dderr, "DDERR_NODIRECTDRAWSUPPORT: DirectDraw support is not possible with the current display driver."); break;
	case DDERR_NOEXCLUSIVEMODE: sprintf(dderr, "DDERR_NOEXCLUSIVEMODE: The operation requires the application to have exclusive mode, but the application does not have exclusive mode."); break;
	case DDERR_NOFLIPHW: sprintf(dderr, "DDERR_NOFLIPHW: Flipping visible surfaces is not supported."); break;
	case DDERR_NOGDI: sprintf(dderr, "DDERR_NOGDI: No GDI is present."); break;
	case DDERR_NOMIRRORHW: sprintf(dderr, "DDERR_NOMIRRORHW: No mirroring hardware is present or available."); break;
	case DDERR_NOTFOUND: sprintf(dderr, "DDERR_NOTFOUND: The requested item was not found."); break;
	case DDERR_NOOVERLAYHW: sprintf(dderr, "DDERR_NOOVERLAYHW: No overlay hardware is present or available."); break;
	case DDERR_NORASTEROPHW: sprintf(dderr, "DDERR_NORASTEROPHW: No appropriate raster-operation hardware is present or available."); break;
	case DDERR_NOROTATIONHW: sprintf(dderr, "DDERR_NOROTATIONHW: No rotation hardware is present or available."); break;
	case DDERR_NOSTRETCHHW: sprintf(dderr, "DDERR_NOSTRETCHHW: There is no hardware support for stretching."); break;
	case DDERR_NOT4BITCOLOR: sprintf(dderr, "DDERR_NOT4BITCOLOR: The DirectDrawSurface object is not using a 4-bit color palette, and the requested operation requires a 4-bit color palette."); break;
	case DDERR_NOT4BITCOLORINDEX: sprintf(dderr, "DDERR_NOT4BITCOLORINDEX: The DirectDrawSurface object is not using a 4-bit color index palette, and the requested operation requires a 4-bit color index palette."); break;
	case DDERR_NOT8BITCOLOR: sprintf(dderr, "DDERR_NOT8BITCOLOR: The DirectDrawSurface object is not using an 8-bit color palette, and the requested operation requires an 8-bit color palette."); break;
	case DDERR_NOTEXTUREHW: sprintf(dderr, "DDERR_NOTEXTUREHW: The operation cannot be carried out because no texture-mapping hardware is present or available."); break;
	case DDERR_NOVSYNCHW: sprintf(dderr, "DDERR_NOVSYNCHW: There is no hardware support for vertical blank synchronized operations."); break;
	case DDERR_NOZBUFFERHW: sprintf(dderr, "DDERR_NOZBUFFERHW: The operation to create a z-buffer in display memory or to perform a blit, using a z-buffer cannot be carried out because there is no hardware support for z-buffers."); break;
	case DDERR_NOZOVERLAYHW: sprintf(dderr, "DDERR_NOZOVERLAYHW: The overlay surfaces cannot be z-layered, based on the z-order because the hardware does not support z-ordering of overlays."); break;
	case DDERR_OUTOFCAPS: sprintf(dderr, "DDERR_OUTOFCAPS: The hardware needed for the requested operation has already been allocated."); break;
	case DDERR_OUTOFMEMORY: sprintf(dderr, "DDERR_OUTOFMEMORY: DirectDraw does not have enough memory to perform the operation."); break;
	case DDERR_OUTOFVIDEOMEMORY: sprintf(dderr, "DDERR_OUTOFVIDEOMEMORY: DirectDraw does not have enough display memory to perform the operation."); break;
	case DDERR_OVERLAYCANTCLIP: sprintf(dderr, "DDERR_OVERLAYCANTCLIP: The hardware does not support clipped overlays."); break;
	case DDERR_OVERLAYCOLORKEYONLYONEACTIVE: sprintf(dderr, "DDERR_OVERLAYCOLORKEYONLYONEACTIVE: An attempt was made to have more than one color key active on an overlay."); break;
	case DDERR_PALETTEBUSY: sprintf(dderr, "DDERR_PALETTEBUSY: Access to this palette is refused because the palette is locked by another thread."); break;
	case DDERR_COLORKEYNOTSET: sprintf(dderr, "DDERR_COLORKEYNOTSET: No source color key is specified for this operation."); break;
	case DDERR_SURFACEALREADYATTACHED: sprintf(dderr, "DDERR_SURFACEALREADYATTACHED: An attempt was made to attach a surface to another surface to which it is already attached."); break;
	case DDERR_SURFACEALREADYDEPENDENT: sprintf(dderr, "DDERR_SURFACEALREADYDEPENDENT: An attempt was made to make a surface a dependency of another surface on which it is already dependent."); break;
	case DDERR_SURFACEBUSY: sprintf(dderr, "DDERR_SURFACEBUSY: Access to the surface is refused because the surface is locked by another thread."); break;
	case DDERR_CANTLOCKSURFACE: sprintf(dderr, "DDERR_CANTLOCKSURFACE: Access to this surface is refused because an attempt was made to lock the primary surface without DCI support."); break;
	case DDERR_SURFACEISOBSCURED: sprintf(dderr, "DDERR_SURFACEISOBSCURED: Access to the surface is refused because the surface is obscured."); break;
	case DDERR_SURFACELOST: sprintf(dderr, "DDERR_SURFACELOST: Access to the surface is refused because the surface memory is gone. Call the IDirectDrawSurface7::Restore method on this surface to restore the memory associated with it."); break;
	case DDERR_SURFACENOTATTACHED: sprintf(dderr, "DDERR_SURFACENOTATTACHED: The requested surface is not attached."); break;
	case DDERR_TOOBIGHEIGHT: sprintf(dderr, "DDERR_TOOBIGHEIGHT: The height requested by DirectDraw is too large."); break;
	case DDERR_TOOBIGSIZE: sprintf(dderr, "DDERR_TOOBIGSIZE: The size requested by DirectDraw is too large. However, the individual height and width are valid sizes."); break;
	case DDERR_TOOBIGWIDTH: sprintf(dderr, "DDERR_TOOBIGWIDTH: The width requested by DirectDraw is too large."); break;
	case DDERR_UNSUPPORTED: sprintf(dderr, "DDERR_UNSUPPORTED: The operation is not supported."); break;
	case DDERR_UNSUPPORTEDFORMAT: sprintf(dderr, "DDERR_UNSUPPORTEDFORMAT: The pixel format requested is not supported by DirectDraw."); break;
	case DDERR_UNSUPPORTEDMASK: sprintf(dderr, "DDERR_UNSUPPORTEDMASK: The bitmask in the pixel format requested is not supported by DirectDraw."); break;
	case DDERR_VERTICALBLANKINPROGRESS: sprintf(dderr, "DDERR_VERTICALBLANKINPROGRESS: A vertical blank is in progress."); break;
	case DDERR_WASSTILLDRAWING: sprintf(dderr, "DDERR_WASSTILLDRAWING: The previous blit operation that is transferring information to or from this surface is incomplete."); break;
	case DDERR_XALIGN: sprintf(dderr, "DDERR_XALIGN: The provided rectangle was not horizontally aligned on a required boundary."); break;
	case DDERR_INVALIDDIRECTDRAWGUID: sprintf(dderr, "DDERR_INVALIDDIRECTDRAWGUID: The globally unique identifier (GUID) passed to the DirectDrawCreate function is not a valid DirectDraw driver identifier."); break;
	case DDERR_DIRECTDRAWALREADYCREATED: sprintf(dderr, "DDERR_DIRECTDRAWALREADYCREATED: A DirectDraw object representing this driver has already been created for this process."); break;
	case DDERR_NODIRECTDRAWHW: sprintf(dderr, "DDERR_NODIRECTDRAWHW: Hardware-only DirectDraw object creation is not possible; the driver does not support any hardware."); break;
	case DDERR_PRIMARYSURFACEALREADYEXISTS: sprintf(dderr, "DDERR_PRIMARYSURFACEALREADYEXISTS: This process has already created a primary surface."); break;
	case DDERR_NOEMULATION: sprintf(dderr, "DDERR_NOEMULATION: Software emulation is not available."); break;
	case DDERR_REGIONTOOSMALL: sprintf(dderr, "DDERR_REGIONTOOSMALL: The region passed to the IDirectDrawClipper::GetClipList method is too small."); break;
	case DDERR_CLIPPERISUSINGHWND: sprintf(dderr, "DDERR_CLIPPERISUSINGHWND: An attempt was made to set a clip list for a DirectDrawClipper object that is already monitoring a window handle."); break;
	case DDERR_NOCLIPPERATTACHED: sprintf(dderr, "DDERR_NOCLIPPERATTACHED: No DirectDrawClipper object is attached to the surface object."); break;
	case DDERR_NOHWND: sprintf(dderr, "DDERR_NOHWND: Clipper notification requires a window handle, or no window handle has been previously set as the cooperative level window handle."); break;
	case DDERR_HWNDSUBCLASSED: sprintf(dderr, "DDERR_HWNDSUBCLASSED: DirectDraw is prevented from restoring state because the DirectDraw cooperative-level window handle has been subclassed."); break;
	case DDERR_HWNDALREADYSET: sprintf(dderr, "DDERR_HWNDALREADYSET: The DirectDraw cooperative-level window handle has already been set. It cannot be reset while the process has surfaces or palettes created."); break;
	case DDERR_NOPALETTEATTACHED: sprintf(dderr, "DDERR_NOPALETTEATTACHED: No palette object is attached to this surface."); break;
	case DDERR_NOPALETTEHW: sprintf(dderr, "DDERR_NOPALETTEHW: There is no hardware support for 16- or 256-color palettes."); break;
	case DDERR_BLTFASTCANTCLIP: sprintf(dderr, "DDERR_BLTFASTCANTCLIP: A DirectDrawClipper object is attached to a source surface that has passed into a call to the IDirectDrawSurface7::BltFast method."); break;
	case DDERR_NOBLTHW: sprintf(dderr, "DDERR_NOBLTHW: No blitter hardware is present."); break;
	case DDERR_NODDROPSHW: sprintf(dderr, "DDERR_NODDROPSHW: No DirectDraw raster-operation (ROP) hardware is available."); break;
	case DDERR_OVERLAYNOTVISIBLE: sprintf(dderr, "DDERR_OVERLAYNOTVISIBLE: The IDirectDrawSurface7::GetOverlayPosition method was called on a hidden overlay."); break;
	case DDERR_NOOVERLAYDEST: sprintf(dderr, "DDERR_NOOVERLAYDEST: The IDirectDrawSurface7::GetOverlayPosition method is called on an overlay that the IDirectDrawSurface7::UpdateOverlay method has not been called on to establish as a destination."); break;
	case DDERR_INVALIDPOSITION: sprintf(dderr, "DDERR_INVALIDPOSITION: The position of the overlay on the destination is no longer legal."); break;
	case DDERR_NOTAOVERLAYSURFACE: sprintf(dderr, "DDERR_NOTAOVERLAYSURFACE: An overlay component is called for a nonoverlay surface."); break;
	case DDERR_EXCLUSIVEMODEALREADYSET: sprintf(dderr, "DDERR_EXCLUSIVEMODEALREADYSET: An attempt was made to set the cooperative level when it was already set to exclusive."); break;
	case DDERR_NOTFLIPPABLE: sprintf(dderr, "DDERR_NOTFLIPPABLE: An attempt was made to flip a surface that cannot be flipped."); break;
	case DDERR_CANTDUPLICATE: sprintf(dderr, "DDERR_CANTDUPLICATE: Primary and 3-D surfaces, or surfaces that are implicitly created, cannot be duplicated."); break;
	case DDERR_NOTLOCKED: sprintf(dderr, "DDERR_NOTLOCKED: An attempt was made to unlock a surface that was not locked."); break;
	case DDERR_CANTCREATEDC: sprintf(dderr, "DDERR_CANTCREATEDC: Windows cannot create any more device contexts (DCs), or a DC has requested a palette-indexed surface when the surface had no palette and the display mode was not palette-indexed (in this case DirectDraw cannot select a proper palette into the DC)."); break;
	case DDERR_NODC: sprintf(dderr, "DDERR_NODC: No device context (DC) has ever been created for this surface."); break;
	case DDERR_WRONGMODE: sprintf(dderr, "DDERR_WRONGMODE: This surface cannot be restored because it was created in a different mode."); break;
	case DDERR_IMPLICITLYCREATED: sprintf(dderr, "DDERR_IMPLICITLYCREATED: The surface cannot be restored because it is an implicitly created surface."); break;
	case DDERR_NOTPALETTIZED: sprintf(dderr, "DDERR_NOTPALETTIZED: The surface being used is not a palette-based surface."); break;
	case DDERR_UNSUPPORTEDMODE: sprintf(dderr, "DDERR_UNSUPPORTEDMODE: The display is currently in an unsupported mode."); break;
	case DDERR_NOMIPMAPHW: sprintf(dderr, "DDERR_NOMIPMAPHW: No mipmap-capable texture mapping hardware is present or available."); break;
	case DDERR_INVALIDSURFACETYPE: sprintf(dderr, "DDERR_INVALIDSURFACETYPE: The surface was of the wrong type."); break;
	case DDERR_DCALREADYCREATED: sprintf(dderr, "DDERR_DCALREADYCREATED: A device context (DC) has already been returned for this surface. Only one DC can be retrieved for each surface."); break;
	case DDERR_CANTPAGELOCK: sprintf(dderr, "DDERR_CANTPAGELOCK: An attempt to page-lock a surface failed. Page lock does not work on a display-memory surface or an emulated primary surface."); break;
	case DDERR_CANTPAGEUNLOCK: sprintf(dderr, "DDERR_CANTPAGEUNLOCK: An attempt to page-unlock a surface failed. Page unlock does not work on a display-memory surface or an emulated primary surface."); break;
	case DDERR_NOTPAGELOCKED: sprintf(dderr, "DDERR_NOTPAGELOCKED: An attempt was made to page-unlock a surface with no outstanding page locks."); break;
	case DDERR_NOTINITIALIZED: sprintf(dderr, "DDERR_NOTINITIALIZED: An attempt was made to call an interface method of a DirectDraw object created by CoCreateInstance before the object was initialized."); break;

	default: sprintf(dderr, "Unknown Error"); break;
	}

	sprintf(string, "DirectDraw Error %s\n", dderr);
	puts(string);
}

LPDIRECTDRAW7        g_pDD;         // DirectDraw object
LPDIRECTDRAWSURFACE7 g_pDDSPrimary; // DirectDraw primary surface
LPDIRECTDRAWSURFACE7 g_pDDSBack;    // DirectDraw back surface
LPDIRECTDRAWCLIPPER  g_pClipper;    // Clipper for windowed mode

long CALLBACK GPUopen(HWND hwndGPU)
{
	WinWnd = hwndGPU;

	RECT rw, rc;
	GetWindowRect(hwndGPU, &rw); // ウィンドウ全体のサイズ
	GetClientRect(hwndGPU, &rc); // クライアント領域のサイズ

	int new_width = (rw.right - rw.left) - (rc.right - rc.left) + 640;
	int new_height = (rw.bottom - rw.top) - (rc.bottom - rc.top) + 480;

	SetWindowPos(hwndGPU, HWND_TOP, 320, 240, new_width, new_height, SWP_NOZORDER | SWP_NOMOVE);
	gpuReset();

	WinDC = GetDC(WinWnd);
	//SetStretchBltMode(WinDC, HALFTONE);

	fb = psxVuw;

	/*DDError(DirectDrawCreateEx(NULL, (VOID**)&g_pDD, IID_IDirectDraw7, NULL));

	DDError(g_pDD->SetCooperativeLevel(hwndGPU, DDSCL_EXCLUSIVE | DDSCL_FULLSCREEN | DDSCL_ALLOWREBOOT));

	DDError(g_pDD->SetDisplayMode(1920, 1080, 32, 0, 0));

	DDSURFACEDESC2 ddsd;
	DDSCAPS2 ddscaps;

	ZeroMemory(&ddsd, sizeof(ddsd));
	ddsd.dwSize = sizeof(ddsd);
	ddsd.dwFlags = DDSD_CAPS | DDSD_BACKBUFFERCOUNT;
	ddsd.ddsCaps.dwCaps = DDSCAPS_PRIMARYSURFACE | DDSCAPS_FLIP |
		DDSCAPS_COMPLEX;
	ddsd.dwBackBufferCount = 1;

	DDError(g_pDD->CreateSurface(&ddsd, &g_pDDSPrimary, NULL));

	ZeroMemory(&ddscaps, sizeof(ddscaps));
	ddscaps.dwCaps = DDSCAPS_BACKBUFFER;
	g_pDDSPrimary->GetAttachedSurface(&ddscaps, &g_pDDSBack);*/

	return 0;
}

long CALLBACK GPUclose()
{
	if (g_pDD != NULL)
	{
		if (g_pDDSPrimary != NULL)
		{
			g_pDDSPrimary->Release();
			g_pDDSPrimary = NULL;
		}
		g_pDD->Release();
		g_pDD = NULL;
	}
	ReleaseDC(WinWnd, WinDC);
	return 0;
}

void updateDisplay(void)
{
	int x, y;

	int x0 = DisplayArea[0];
	int y0 = DisplayArea[1];
	int w0 = DisplayArea[2];
	int h0 = DisplayArea[3];
	int h1 = DisplayArea[7] - DisplayArea[5];

	float a = (float)DisplayArea[2] / (float)640;
	float b = (float)DisplayArea[3] / (float)480;

	if (h0 > 240)
		h1 += h1;

	BITMAPINFOHEADER bmpHeader;
	ZeroMemory(&bmpHeader, sizeof BITMAPINFOHEADER);
	bmpHeader.biSize = sizeof BITMAPINFOHEADER;
	bmpHeader.biWidth = w0;
	bmpHeader.biHeight = h0;
	bmpHeader.biPlanes = 1;
	bmpHeader.biBitCount = GPUstatusRet & 0x00200000 ? 24 : 16;
	bmpHeader.biCompression = BI_RGB;//RGB555是BI_RGB; 
	bmpHeader.biSizeImage = w0 * h0 * (GPUstatusRet & 0x00200000 ? 3 : 2);

	BITMAPINFO bmpInfo;

	ZeroMemory(&bmpInfo, sizeof BITMAPINFO);

	bmpInfo.bmiHeader = bmpHeader;

	//bmpHeader.biHeight = h0;
	//bmpHeader.biWidth = w0;

	/*static char bmih[] = {
		0x28, 0x00, 0x00, 0x00,					//
		0x00, 0x00, 0x00, 0x00,					//
		0x00, 0x00, 0x00, 0x00,					//
		0x01, 0x00,								//
		0x00, 0x00,								//
		0x00, 0x00, 0x00, 0x00,					//
		0x00, 0x00, 0x00, 0x00,					//
		0x00, 0x00, 0x00, 0x00,					//
		0x00, 0x00, 0x00, 0x00,					//
		0x00, 0x00, 0x00, 0x00,					//
		0x00, 0x00, 0x00, 0x00					//
	};*/

	//*(unsigned int *)& bmih[4] = w0;
	//*(unsigned int *)& bmih[8] = h0;

	if (GPUstatusRet & 0x00200000) {
		/*for (y = 0; y < 480; y++) {
			for (x = 0; x < 640; x++) {
				BYTE *pack0 = (BYTE *)&psxVub[(int)((((float)y*b) + dispPosY) * 2048 + (int)(((float)x*a) + dispPosX)) * 3];

				int r0 = pack0[0];
				int g0 = pack0[1];
				int b0 = pack0[2];

				int r1 = pack0[3];
				int g1 = pack0[4];
				int b1 = pack0[5];

				surf[(y + 0) * 640 + (x + 0)] = (r0 << 16) | (g0 << 8) | b0;
			}
		}*/
		//*(unsigned short *)& bmih[14] = 24;
		//*(unsigned int *)& bmih[20] = w0 * h0 * 3;
		unsigned char *lpDst = (unsigned char *)&surf[w0 * (h0 - 1) * 3];
		unsigned char *lpSrc = (unsigned char *)&psxVuw[FRAME_OFFSET(x0, y0)];
		long nDst = (w0 * 6);
		long nSrc = (FRAME_WIDTH * 2 - w0 * 3);
		for (y0 = h1; y0; y0--) {
			for (x0 = w0; x0; x0 -= 4) {
				lpDst[0] = lpSrc[2];
				lpDst[1] = lpSrc[1];
				lpDst[2] = lpSrc[0];
				lpDst[3] = lpSrc[5];
				lpDst[4] = lpSrc[4];
				lpDst[5] = lpSrc[3];
				lpDst[6] = lpSrc[8];
				lpDst[7] = lpSrc[7];
				lpDst[8] = lpSrc[6];
				lpDst[9] = lpSrc[11];
				lpDst[10] = lpSrc[10];
				lpDst[11] = lpSrc[9];
				lpDst += 12;
				lpSrc += 12;
			}
			lpDst -= nDst;
			lpSrc += nSrc;
		}
		for (y0 = h0 - h1; y0 > 0; y0--) {
			for (x0 = w0; x0; x0 -= 4) {
				((unsigned int *)lpDst)[0] = 0;
				((unsigned int *)lpDst)[1] = 0;
				((unsigned int *)lpDst)[2] = 0;
				lpDst += 12;
			}
			lpDst -= nDst;
		}
	}
	else {
		/*for (y = 0; y < 480; y++) {
			for (x = 0; x < 640; x++) {
				surf[y * 640 + x] = lutBGR2RGB[psxVuw[(int)(((float)y*b) + dispPosY) * 1024 + (int)(((float)x*a) + dispPosX)] & 0x7fff];
			}
		}*/
		int data;
		//*(unsigned short *)& bmih[14] = 16;
		//*(unsigned int *)& bmih[20] = w0 * h0 * 2;
		unsigned int *lpDst = (unsigned int *)&surf[w0 * (h0 - 1) * 2];
		unsigned int *lpSrc = (unsigned int *)&psxVuw[FRAME_OFFSET(x0, y0)];
		int nDst = w0;
		int nSrc = ((FRAME_WIDTH - w0) >> 1);
		for (y0 = h1; y0; y0--) {
			for (x0 = w0; x0; x0 -= 8) {
				data = lpSrc[0];
				data =
					((data & 0x001F001F) << 10) | (data & 0x03E003E0) |
					((data & 0x7C007C00) >> 10);
				lpDst[0] = data;
				data = lpSrc[1];
				data =
					((data & 0x001F001F) << 10) | (data & 0x03E003E0) |
					((data & 0x7C007C00) >> 10);
				lpDst[1] = data;
				data = lpSrc[2];
				data =
					((data & 0x001F001F) << 10) | (data & 0x03E003E0) |
					((data & 0x7C007C00) >> 10);
				lpDst[2] = data;
				data = lpSrc[3];
				data =
					((data & 0x001F001F) << 10) | (data & 0x03E003E0) |
					((data & 0x7C007C00) >> 10);
				lpDst[3] = data;
				lpDst += 4;
				lpSrc += 4;
			}
			lpDst -= nDst;
			lpSrc += nSrc;
		}
		for (y0 = h0 - h1; y0 > 0; y0--) {
			for (x0 = w0; x0; x0 -= 16) {
				((unsigned int *)lpDst)[0] = 0;
				((unsigned int *)lpDst)[1] = 0;
				((unsigned int *)lpDst)[2] = 0;
				((unsigned int *)lpDst)[3] = 0;
				lpDst += 8;
			}
			lpDst -= nDst;
		}
	}

	/*HDC hdc;

	hdc = GetDC(WinWnd);
	SetStretchBltMode(WinDC, HALFTONE);*/
	/*SetDIBitsToDevice(
		hdc, 0, 0,
		640, 480,
		0, 0, 0, abs(bmpInfo.bmiHeader.biHeight),
		surf, &bmpInfo, DIB_RGB_COLORS
	);*/

	//bmpInfo.bmiHeader = bmpHeader;

	//printf("%d,%d,%x\n", w0, h0, WinDC);

	/*SetDIBitsToDevice(WinDC, 0, 0, w0, h0, 0, 0, 0, h0, surf,
		&bmpInfo, DIB_RGB_COLORS);*/

	StretchDIBits(WinDC, 0, 0, 640, 480, 0, 0, w0, h0,
		surf, &bmpInfo, DIB_RGB_COLORS,
		SRCCOPY);
	/*DDSURFACEDESC2 desc;

	ZeroMemory(&desc, sizeof(DDSURFACEDESC));
	desc.dwSize = sizeof(DDSURFACEDESC);
	g_pDDSBack->Lock(NULL, &desc, DDLOCK_WAIT | DDLOCK_SURFACEMEMORYPTR, NULL);
	unsigned int *p = (unsigned int *)desc.lpSurface;

	for (y = 0; y < h0; y++) {
		for (x = 0; x < w0; x++) {
			p[y * (desc.lPitch/4) + x] = surf[y * w0 + x];
		}
	}

	g_pDDSBack->Unlock(NULL);

	HRESULT hRet;
	while (1) {
		hRet = g_pDDSPrimary->Flip(NULL, 0);
		if (hRet == DD_OK)
			break;
		if (hRet == DDERR_SURFACELOST)
		{
			hRet = g_pDDSPrimary->Restore();
			if (hRet != DD_OK)
				break;
		}
		if (hRet != DDERR_WASSTILLDRAWING)
			break;
	}*/
	//ReleaseDC(WinWnd, hdc);
}

// update lace is called evry VSync
void CALLBACK GPUupdateLace(void)
{
	static int backtime2 = timeGetTime();
	static int backtime = timeGetTime();
	static int f = 0;
	int time = 16 - (timeGetTime() - backtime);
	int ftime = timeGetTime() - backtime;
	//if(time > 0) Sleep(time);
	backtime = timeGetTime();

	int time2 = timeGetTime() - backtime2;

	if (time2 >= 1000) {
		char sz[1024];
		float fps = (50.0f * (float)f) / (3.0f * (float)time2) * 60.0f;
		sprintf(sz, "%.2f fps | %.2f%%", fps, fps / 60.0f * 100.0f);
		SetWindowText(WinWnd, sz);
		backtime2 = timeGetTime();
		f = 0;
	}
	f++;


	drawingLines ^= 1;
	if (DisplayArea[3]>0 && DisplayArea[2]>0)
	{
		syslog(0, "UPDATING LACE");
		updateDisplay();
	}
}


// process read request from GPU status register
unsigned long CALLBACK GPUreadStatus(void)
{
	// return the status of the GPU
	/*
	- gp1 - GPU Status
	Mask
	04000000 - 1:Idle 0:Busy
	10000000 - 1:Ready 0:Not ready , to receive commands.
	80000000 - GPU is drawing 1:Odd 0:Even lines, in interlaced mode.
	*/
	//just'0x74000000' for now ! (idle, ready, even lines)
	if (drawingLines == 1)
		return GPUstatusRet & ~0x80000000;
	else
		return GPUstatusRet | 0x80000000;
	//return 0x74000000;
}

// processes data send to GPU status register
// these are always single packet commands.
void CALLBACK GPUwriteStatus(unsigned long gdata)
{
	switch ((gdata >> 24) & 0xff)
	{
	case 0x00:
		gpuReset();
		break;
	case 0x01:
		GPUstatusRet &= ~0x08000000;
		gpuDataC = FrameToRead = FrameToWrite = 0;
		break;
	case 0x02:
		GPUstatusRet &= ~0x08000000;
		gpuDataC = FrameToRead = FrameToWrite = 0;
		break;
	case 0x03:
		GPUstatusRet = (GPUstatusRet & ~0x00800000) | ((gdata & 1) << 23);
		break;
	case 0x04:
		if (gdata == 0x04000000)
			gpuDataC = 0;
		GPUstatusRet = (GPUstatusRet & ~0x60000000) | ((gdata & 3) << 29);
		break;
	case 0x05:
		DisplayArea[0] = gdata & 0x000003FF;
		DisplayArea[1] = (gdata & 0x000FFC00) >> 10;
		syslog(77, " GPU_S: DISPLAY SET: X=%d Y=%d\n", DisplayArea[0], DisplayArea[1]);
		return;
	case 0x06:
		DisplayArea[4] = gdata & 0x00000FFF;
		DisplayArea[6] = (gdata & 0x00FFF000) >> 12;
		break;
	case 0x07:
		DisplayArea[5] = gdata & 0x000003FF;
		DisplayArea[7] = (gdata & 0x000FFC00) >> 10;
		break;
	case 0x08:
		GPUstatusRet =
			(GPUstatusRet & ~0x007F0000) | ((gdata & 0x3F) << 17) |
			((gdata & 0x40) << 10);
		DisplayArea[2] = HorizontalResolution[(GPUstatusRet >> 16) & 7];
		DisplayArea[3] = VerticalResolution[(GPUstatusRet >> 19) & 3];
		//syslog(77, "!GPU! DISPLAY SET: W=%d Wo=%d H=%d TRUE=%d LACE=%d\n", dispHorNew, dispWidths[gdata & 0x3], dispVerNew, dispColorNew, dispLaceNew);
		return;

	case 0x10: // ask about GPU version
		/*switch (gdata & 0xffff) {
		case 0:
		case 1:
		case 3:
			GPUdataRet = (DrawingArea[1] << 10) | DrawingArea[0];
			break;
		case 4:
			GPUdataRet =
				((DrawingArea[3] - 1) << 10) | (DrawingArea[2] - 1);
			break;
		case 6:
		case 5:
			GPUdataRet = (DrawingOffset[1] << 11) | DrawingOffset[0];
			break;
		case 7:
			GPUdataRet = 2;
			break;
		default:
			GPUdataRet = 0;
		}*/
		GPUdataRet = 2;
		return;

	default:
		syslog(77, "STATUS=%08x\n", gdata);
		return;
	}

	return;
}

void ReadFrameBuffer(unsigned short * lpBuffer, long nCount)
{
	long temp, nCount2, nCount3;
	unsigned short *lpBuffer2;
	temp = FrameToRead;
	if (temp <= nCount) {
		nCount = temp;
		GPUstatusRet &= ~0x08000000;
	}
	FrameToRead = temp - nCount;
	lpBuffer2 = &psxVuw[FrameIndex];
	temp = FrameCount;
	while (nCount) {
		if (nCount >= temp) {
			nCount2 = temp;
			nCount3 = temp + 1024 - FrameWidth;
			temp = FrameWidth;
		}
		else {
			nCount3 = nCount2 = nCount;
			temp -= nCount;
		}
		memcpy(lpBuffer, lpBuffer2, nCount2 + nCount2);
		lpBuffer += nCount2;
		lpBuffer2 += nCount3;
		nCount -= nCount2;
	}
	FrameCount = temp;
	FrameIndex = lpBuffer2 - psxVuw;
}

/* WriteFrameBuffer */
void WriteFrameBuffer(unsigned short * lpBuffer, long nCount)
{
	long temp, nCount2, nCount3;
	unsigned short *lpBuffer2;
	temp = FrameToWrite;
	if (temp <= nCount) {
		nCount = temp;
		GPUstatusRet &= ~0x08000000;
	}
	FrameToWrite = temp - nCount;
	lpBuffer2 = &psxVuw[FrameIndex];
	temp = FrameCount;
	while (nCount) {
		if (nCount >= temp) {
			nCount2 = temp;
			nCount3 = temp + 1024 - FrameWidth;
			temp = FrameWidth;
		}
		else {
			nCount3 = nCount2 = nCount;
			temp -= nCount;
		}
		memcpy(lpBuffer2, lpBuffer, nCount2 + nCount2);
		lpBuffer += nCount2;
		lpBuffer2 += nCount3;
		nCount -= nCount2;
	}
	FrameCount = temp;
	FrameIndex = lpBuffer2 - psxVuw;
}

unsigned long CALLBACK GPUreadData(void)
{
	GPUstatusRet &= ~0x14000000;
	if (FrameToRead)
		ReadFrameBuffer((unsigned short *)&GPUdataRet, 2);
	GPUstatusRet |= 0x14000000;
	return GPUdataRet;
}

void CALLBACK GPUreadDataMem(unsigned long * pMem, int iSize)
{
	unsigned long temp, temp2;
	GPUstatusRet &= ~0x14000000;
	temp = FrameToRead;
	if (temp) {
		temp2 = iSize << 1;
		if (temp2 > temp)
			temp2 = temp;
		ReadFrameBuffer((unsigned short *)pMem, temp2);
		temp2 = (temp2 + 1) >> 1;
		pMem += temp2;
		iSize -= temp2;
	}
	for (; iSize; iSize--) {
		*pMem++ = GPUdataRet;
	}
	GPUstatusRet = (GPUstatusRet | 0x14000000) & ~0x60000000;
}

// processes data send to GPU data register
// user should not take care about that
void CALLBACK GPUwriteData(unsigned long gdata)
{
	unsigned char command;
	GPUdataRet = gdata;
	//syslog(87,"%08x",gdata);
	if (FrameToWrite) {
		WriteFrameBuffer((unsigned short *)&gdata, 2);
	}
	else {
		//syslog(77,"!GPU! DATA = %08x\n",gdata);
		if (gpuDataC == 0)
		{
			command = (unsigned char)(gdata >> 24) & 0xff;
			if (primTableCX[command])
			{
				gpuDataC = primTableCX[command];
				gpuCommand = command;
				gpuData[0] = gdata;
				gpuDataP = 1;
			}
			else
			{
				if (gdata)
				{
					syslog(77, "!GPU! ERROR: COMMAND? %02x, %06x\n", command, gdata);
				}
				return;
			}
		}
		else
		{
			gpuData[gpuDataP] = gdata;
			if (gpuDataC > 128)
			{
				if ((gpuDataC == 254 && gpuDataP >= 3) ||
					(gpuDataC == 255 && gpuDataP >= 4 && !(gpuDataP & 1)))
				{
					if ((gpuData[gpuDataP] & 0xF000F000) == 0x50005000)
						gpuDataP = gpuDataC - 1;
				}
			}
			gpuDataP++;
		}
		if (gpuDataP == gpuDataC)
		{
			gpuDataC = gpuDataP = 0;
			primTableJ[gpuCommand]((unsigned char *)gpuData);
		}
	}
	return;
}

void CALLBACK GPUwriteDataMem(unsigned long * pMem, int iSize)
{
	unsigned long temp, temp2;
	GPUstatusRet &= ~0x14000000;
	while (iSize) {
		temp = *pMem++;
		iSize--;
		GPUwriteData(temp);
	}
	GPUstatusRet = (GPUstatusRet | 0x14000000) & ~0x60000000;
}

// this function will be removed soon
void CALLBACK GPUsetMode(unsigned long gdata)
{
	imageTransfer = gdata;
	return;
}

// this function will be removed soon
long CALLBACK GPUgetMode(void)
{
	return imageTransfer;
}


long CALLBACK GPUconfigure(void)
{
	MessageBox(WinWnd, "設定する項目はありません", "Setting", MB_OK);
	return 0;
}

long CALLBACK GPUdmaChain(unsigned long * baseAddrL, unsigned long addr)
{
	unsigned long temp, data, pkts, *address, count, offset;
	GPUstatusRet &= ~0x14000000;
	addr &= 0x00FFFFFF;
	pkts = 0;

	temp = FrameToWrite;
	FrameToWrite = 0;
	while (addr != 0xFFFFFF) {
		address = (baseAddrL + (addr >> 2));
		data = *address++;
		count = (data >> 24);
		offset = data & 0x00FFFFFF;
		if (addr != offset)
			addr = offset;
		else
			addr = 0xFFFFFF;
		while (count) {
			data = *address++;
			count--;
			GPUwriteData(data);
		}
		pkts++;
		if (pkts > 0x100000) return(addr);
	}
	GPUstatusRet = (GPUstatusRet | 0x14000000) & ~0x60000000;
	FrameToWrite = temp;
	return (0);
}



void CALLBACK GPUabout(void)
{
	// do not edit this function
	// modify only Resource Dialog (IDD_ABTDLG)
	MessageBox(WinWnd, "AdvancePSX GPU Plugin\n2016-2017 daretoku_taka", "About", MB_OK);
}

long CALLBACK GPUtest(void)
{
	// if test fails this function should return negative value for error (unable to continue)
	// and positive value for warning (can continue but output might be crappy)
	return 0;
}


typedef struct
{
	unsigned long ulFreezeVersion;      // should be always 1 for now (set by main emu)
	unsigned long ulStatus;             // current gpu status
	unsigned long ulControl[256];       // latest control register values
	unsigned char psxVRam[1024 * 512 * 2];  // current VRam image
} GPUFreeze_t;

long CALLBACK GPUfreeze(unsigned long ulGetFreezeData, GPUFreeze_t * pF)
{
	if (!pF)                    return 0;
	if (pF->ulFreezeVersion != 1) return 0;
	if (ulGetFreezeData != 0) return 0;
	return 1;
}