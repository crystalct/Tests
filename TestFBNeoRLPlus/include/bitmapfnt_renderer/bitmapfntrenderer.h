#ifndef __BITMAPFNTRENDERER_H__
#define __BITMAPFNTRENDERER_H__

#include "bitmapfnt.h"

class BitmapFNTRenderer
{
public:
	BitmapFNTRenderer();
	virtual ~BitmapFNTRenderer();

	virtual void init() = 0;
	virtual void shutdown() = 0;

	virtual void printStart(f32 r, f32 g, f32 b, f32 a) = 0;
	//virtual void printPass(BitmapFNT::Position *pPositions, BitmapFNT::TexCoord *pTexCoords, BitmapFNT::Color *pColors, s32 numVerts) = 0;
	virtual void printPass(BitmapFNT::Position* pPositions, BitmapFNT::TexCoord* pTexCoords, BitmapFNT::Color* pColors, s32 numVerts, u32 *textmem_off, u32 tex_w, u32 tex_h) = 0;
	virtual void printEnd() = 0;

protected:

	static f32 sR;
	static f32 sG;
	static f32 sB;
	static f32 sA;
};

#endif // __BITMAPFNTRENDERER_H__
