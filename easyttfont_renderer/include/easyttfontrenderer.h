#ifndef __EASYTTFONTRENDERER_H__
#define __EASYTTFONTRENDERER_H__

#include "easyttfont.h"

class EasyTTFontRenderer
{
public:
	EasyTTFontRenderer();
	virtual ~EasyTTFontRenderer();

	virtual void init() = 0;
	virtual void shutdown() = 0;

	virtual void printStart(f32 r, f32 g, f32 b, f32 a) = 0;
	//virtual void printPass(EasyTTFont::Position *pPositions, EasyTTFont::TexCoord *pTexCoords, EasyTTFont::Color *pColors, s32 numVerts) = 0;
	virtual void printPass(EasyTTFont::Position* pPositions, EasyTTFont::TexCoord* pTexCoords, EasyTTFont::Color* pColors, s32 numVerts, u32 *textmem_off) = 0;
	virtual void printEnd() = 0;

protected:

	static f32 sR;
	static f32 sG;
	static f32 sB;
	static f32 sA;
};

#endif // __EASYTTFONTRENDERER_H__
