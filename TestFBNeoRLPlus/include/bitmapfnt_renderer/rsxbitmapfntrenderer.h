#ifndef __RSXBITMAPFNTRENDERER_H__
#define __RSXBITMAPFNTRENDERER_H__

#include "bitmapfntrenderer.h"
#include <rsx/rsx.h>

#define BITMAPFNT_PRIMITIVE				GCM_TYPE_QUADS
#define NUM_VERTS_PER_GLYPH					4


class RSXBitmapFNTRenderer : public BitmapFNTRenderer
{
public:
	RSXBitmapFNTRenderer();
	RSXBitmapFNTRenderer(gcmContextData *context);
	virtual ~RSXBitmapFNTRenderer();

	virtual void init();
	virtual void shutdown();
	virtual void printStart(f32 r, f32 g, f32 b, f32 a);
	virtual void printPass(BitmapFNT::Position* pPositions, BitmapFNT::TexCoord* pTexCoords, BitmapFNT::Color* pColors, s32 numVerts, u32 *textmem_off, u32 tex_w, u32 tex_h);
	virtual void printEnd();

private:
	void initShader();
	static BitmapFNT fnt_font;
	
	static gcmContextData *mContext;
	static u8* texture_mem;		//fnt_textures

	static u8* free_mem;			// Pointer after last ttf texture

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

#endif // __RSXBITMAPFNTRENDERER_H__