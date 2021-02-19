#ifndef __RSXEASYTTFONTRENDERER_H__
#define __RSXEASYTTFONTRENDERER_H__

#include "easyttfontrenderer.h"

#include <rsx/rsx.h>

#define EASYTTFONT_PRIMITIVE				GCM_TYPE_QUADS
#define NUM_VERTS_PER_GLYPH					4


class RSXEasyTTFontRenderer : public EasyTTFontRenderer
{
public:
	RSXEasyTTFontRenderer();
	RSXEasyTTFontRenderer(gcmContextData *context);
	virtual ~RSXEasyTTFontRenderer();

	virtual void init();
	virtual void shutdown();
	virtual void printStart(f32 r, f32 g, f32 b, f32 a);
	virtual void printPass(EasyTTFont::Position *pPositions, EasyTTFont::TexCoord *pTexCoords, EasyTTFont::Color *pColors, s32 numVerts);
	virtual void printPass(EasyTTFont::Position* pPositions, EasyTTFont::TexCoord* pTexCoords, EasyTTFont::Color* pColors, s32 numVerts, u32 *textmem_off);
	virtual void printEnd();

private:
	void initShader();
	static EasyTTFont ttf_font;
	static gcmContextData *mContext;
	static u32* texture_mem;		//ttf_textures

	static u32* free_mem;			// Pointer after last ttf texture

	static u8 *spTextureData;
	
	static u8 *mPosition;
	static u8 *mTexCoord;
	static u8 *mColor;

	static u32 mPositionOffset;
	static u32 mTexCoordOffset;
	static u32 mColorOffset;

	static u8 *mpTexture;
	static rsxProgramAttrib *mPosIndex;
	static rsxProgramAttrib *mTexIndex;
	static rsxProgramAttrib *mColIndex;
	static rsxProgramAttrib *mTexUnit;

	static rsxVertexProgram *mRSXVertexProgram;
	static rsxFragmentProgram *mRSXFragmentProgram;

	static u32 mFragmentProgramOffset;
	static u32 mTextureOffset;

	static void *mVertexProgramUCode;				// this is sysmem
	static void *mFragmentProgramUCode;			// this is vidmem

	static vu32 *mLabel;
	static u32 mLabelValue;

	static const u32 sLabelId = 254;
};

#endif // __RSXEASYTTFONTRENDERER_H__