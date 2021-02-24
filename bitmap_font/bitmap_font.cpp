#include "bitmap_font.h"
#include "fnt35.h"
#include <stdio.h>

#include "vpshader_bitmapfnt_vpo.h"
#include "fpshader_bitmapfnt_fpo.h"

fnt_t fnt_font;

gcmContextData* mContext;
u8* texture_mem;		//fnt_textures

u8* free_mem;			// Pointer after last ttf texture

u8* spTextureData;

u8* mPosition;
u8* mTexCoord;
u8* mColor;

u32 mPositionOffset;
u32 mTexCoordOffset;
u32 mColorOffset;

u8* mpTexture;
rsxProgramAttrib* mPosIndex;
rsxProgramAttrib* mTexIndex;
rsxProgramAttrib* mColIndex;
rsxProgramAttrib* mTexUnit;

rsxVertexProgram* mRSXVertexProgram;
rsxFragmentProgram* mRSXFragmentProgram;

u32 mFragmentProgramOffset;
u32 mTextureOffset;

void* mVertexProgramUCode;				// this is sysmem
void* mFragmentProgramUCode;			// this is vidmem

vu32* mLabel;
u32 mLabelValue;

const u32 sLabelId = 254;

void fnt_read_header(fnt_t* fnt_p)
{
	u32 pad;
	u32 shift;
	u8* fnt_ptr = (u8 *)fnt_p->fnt_ptr;

	fnt_p->maxwidth = (fnt_ptr[5] << 8) + fnt_ptr[4];
	fnt_p->height = (fnt_ptr[7] << 8) + fnt_ptr[6];
	fnt_p->ascent = (fnt_ptr[9] << 8) + fnt_ptr[8];
	fnt_p->pad = (fnt_ptr[11] << 8) + fnt_ptr[10];
	fnt_p->firstchar = (fnt_ptr[15] << 24) + (fnt_ptr[14] << 16) + (fnt_ptr[13] << 8) + fnt_ptr[12];
	fnt_p->defaultchar = (fnt_ptr[19] << 24) + (fnt_ptr[18] << 16) + (fnt_ptr[17] << 8) + fnt_ptr[16];
	fnt_p->size = (fnt_ptr[23] << 24) + (fnt_ptr[22] << 16) + (fnt_ptr[21] << 8) + fnt_ptr[20];
	fnt_p->nbits = (fnt_ptr[27] << 24) + (fnt_ptr[26] << 16) + (fnt_ptr[25] << 8) + fnt_ptr[24];
	fnt_p->noffset = (fnt_ptr[31] << 24) + (fnt_ptr[30] << 16) + (fnt_ptr[29] << 8) + fnt_ptr[28];
	fnt_p->nwidth = (fnt_ptr[35] << 24) + (fnt_ptr[34] << 16) + (fnt_ptr[33] << 8) + fnt_ptr[32];

	fnt_p->bits = (const u8*)(&fnt_ptr[36]);

	if (fnt_p->nbits < 0xFFDB)
	{
		pad = 1;
		fnt_p->long_offset = 0;
		shift = 1;
	}
	else
	{
		pad = 3;
		fnt_p->long_offset = 1;
		shift = 2;
	}

	if (fnt_p->noffset != 0)
		fnt_p->offset = (u32*)((u64)(fnt_p->bits + fnt_p->nbits + pad) & ~pad);
	else
		fnt_p->offset = NULL;

	if (fnt_p->nwidth != 0)
		fnt_p->width = (u8*)((u64)fnt_p->offset + (fnt_p->noffset << shift));
	else
		fnt_p->width = NULL;
}

void fnt_load_mem(const void* ptr, fnt_t * fnt_p)
{
	fnt_p->file_flag = 0;

	fnt_p->fnt_ptr = (void*)ptr;

	fnt_read_header(fnt_p);
}

int init_fnt(fnt_class* fntc) {
	//if (!spRenderer) return 1;

	fntc->gly_w = BITMAPFNT_GLYPH_WIDTH;
	fntc->gly_h = BITMAPFNT_GLYPH_HEIGHT;
	fntc->r_use = 0;

	fnt_load_mem(fnt35, &fntc->fnt);
	//if (internal_load_font("/dev_hdd0/game/FBNE00123/USRDIR/b14.fnt"))
		//return 1;	//Error

	//::printf("Font w: %d - h: %d\n", fontFNT.maxwidth, fontFNT.height);
	fntc->fnt_ux = fntc->fnt.maxwidth;
	fntc->fnt_uy = fntc->fnt.height;

	//spRenderer->init();

	mLabel = (vu32*)gcmGetLabelAddress(sLabelId);
	*mLabel = mLabelValue;
	::printf("2 ok\n");
	texture_mem = (u8*)rsxMemalign(128, 64 * 1024 * 1024); // alloc 64MB of space for ttf textures (this pointer can be global)

	if (!texture_mem) return 1; // fail!
	///////free_mem = (u8*)fnt_font.init_fnt_table((u8*)texture_mem);

	initShader();
	
	mPosIndex = rsxVertexProgramGetAttrib(mRSXVertexProgram, "position");
	mTexIndex = rsxVertexProgramGetAttrib(mRSXVertexProgram, "texcoord");
	mColIndex = rsxVertexProgramGetAttrib(mRSXVertexProgram, "color");

	mTexUnit = rsxFragmentProgramGetAttrib(mRSXFragmentProgram, "texture");

	mPosition = (u8*)rsxMemalign(128, BITMAPFNT_MAX_CHAR_COUNT * NUM_VERTS_PER_GLYPH * sizeof(f32) * 3);
	mTexCoord = (u8*)rsxMemalign(128, BITMAPFNT_MAX_CHAR_COUNT * NUM_VERTS_PER_GLYPH * sizeof(f32) * 2);
	mColor = (u8*)rsxMemalign(128, BITMAPFNT_MAX_CHAR_COUNT * NUM_VERTS_PER_GLYPH * sizeof(f32) * 4);

	rsxAddressToOffset(mPosition, &mPositionOffset);
	rsxAddressToOffset(mTexCoord, &mTexCoordOffset);
	rsxAddressToOffset(mColor, &mColorOffset);
	
	fntc->sXPos = fntc->sYPos = 0;
	fntc->sLeftSafe = fntc->sRightSafe = fntc->sTopSafe = fntc->sBottomSafe = BITMAPFNT_DEFAULT_SAFE_AREA;
	fntc->sR = fntc->sG = fntc->sB = 1.0f;

	fntc->spPositions[0] = new Position[BITMAPFNT_MAX_CHAR_COUNT * NUM_VERTS_PER_GLYPH];
	fntc->spPositions[1] = new Position[BITMAPFNT_MAX_CHAR_COUNT * NUM_VERTS_PER_GLYPH];
	fntc->spTexCoords[0] = new TexCoord[BITMAPFNT_MAX_CHAR_COUNT * NUM_VERTS_PER_GLYPH];
	fntc->spTexCoords[1] = new TexCoord[BITMAPFNT_MAX_CHAR_COUNT * NUM_VERTS_PER_GLYPH];
	fntc->spColors[0] = new Color[BITMAPFNT_MAX_CHAR_COUNT * NUM_VERTS_PER_GLYPH];
	fntc->spColors[1] = new Color[BITMAPFNT_MAX_CHAR_COUNT * NUM_VERTS_PER_GLYPH];
	fntc->ntex_off = (u32*)malloc(BITMAPFNT_MAX_CHAR_COUNT * sizeof(u32));

	return 0;
}

void initShader()
{
	mRSXVertexProgram = (rsxVertexProgram*)vpshader_bitmapfnt_vpo;
	mRSXFragmentProgram = (rsxFragmentProgram*)fpshader_bitmapfnt_fpo;

	void* ucode;
	u32 ucodeSize;

	rsxFragmentProgramGetUCode(mRSXFragmentProgram, &ucode, &ucodeSize);

	mFragmentProgramUCode = rsxMemalign(64, ucodeSize);
	rsxAddressToOffset(mFragmentProgramUCode, &mFragmentProgramOffset);

	memcpy(mFragmentProgramUCode, ucode, ucodeSize);

	rsxVertexProgramGetUCode(mRSXVertexProgram, &mVertexProgramUCode, &ucodeSize);
}