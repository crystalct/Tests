#include "rsxbitmapfntrenderer.h"

#include "vpshader_bitmapfnt_vpo.h"
#include "fpshader_bitmapfnt_fpo.h"

u8* RSXBitmapFNTRenderer::spTextureData;

gcmContextData* RSXBitmapFNTRenderer::mContext = NULL;

u8* RSXBitmapFNTRenderer::mpTexture = NULL;
u8* RSXBitmapFNTRenderer::mPosition = NULL;
u8* RSXBitmapFNTRenderer::mTexCoord = NULL;
u8* RSXBitmapFNTRenderer::mColor = NULL;


rsxProgramAttrib* RSXBitmapFNTRenderer::mPosIndex = NULL;
rsxProgramAttrib* RSXBitmapFNTRenderer::mTexIndex = NULL;
rsxProgramAttrib* RSXBitmapFNTRenderer::mColIndex = NULL;
rsxProgramAttrib* RSXBitmapFNTRenderer::mTexUnit = NULL;

rsxVertexProgram* RSXBitmapFNTRenderer::mRSXVertexProgram;
rsxFragmentProgram* RSXBitmapFNTRenderer::mRSXFragmentProgram;

void* RSXBitmapFNTRenderer::mVertexProgramUCode;
void* RSXBitmapFNTRenderer::mFragmentProgramUCode;

vu32* RSXBitmapFNTRenderer::mLabel = NULL;
u32 RSXBitmapFNTRenderer::mLabelValue = 0;

u32 RSXBitmapFNTRenderer::mFragmentProgramOffset;
u32 RSXBitmapFNTRenderer::mTextureOffset;
u32 RSXBitmapFNTRenderer::mPositionOffset;
u32 RSXBitmapFNTRenderer::mTexCoordOffset;
u32 RSXBitmapFNTRenderer::mColorOffset;

BitmapFNT RSXBitmapFNTRenderer::fnt_font;

u8* RSXBitmapFNTRenderer::texture_mem;

u8* RSXBitmapFNTRenderer::free_mem;

RSXBitmapFNTRenderer::RSXBitmapFNTRenderer() : BitmapFNTRenderer()
{

}

RSXBitmapFNTRenderer::RSXBitmapFNTRenderer(gcmContextData *context) : BitmapFNTRenderer()
{
	mContext = context;
}

RSXBitmapFNTRenderer::~RSXBitmapFNTRenderer()
{

}

void RSXBitmapFNTRenderer::initShader()
{
	mRSXVertexProgram = (rsxVertexProgram*)vpshader_bitmapfnt_vpo;
	mRSXFragmentProgram = (rsxFragmentProgram*)fpshader_bitmapfnt_fpo;

	void *ucode;
	u32 ucodeSize;

	rsxFragmentProgramGetUCode(mRSXFragmentProgram, &ucode, &ucodeSize);

	mFragmentProgramUCode = rsxMemalign(64, ucodeSize);
	rsxAddressToOffset(mFragmentProgramUCode, &mFragmentProgramOffset);

	memcpy(mFragmentProgramUCode, ucode, ucodeSize);

	rsxVertexProgramGetUCode(mRSXVertexProgram, &mVertexProgramUCode, &ucodeSize);
}

void RSXBitmapFNTRenderer::init()
{
	mLabel = (vu32*) gcmGetLabelAddress(sLabelId);
	*mLabel = mLabelValue;

	texture_mem = (u8*)rsxMemalign(128, 64 * 1024 * 1024); // alloc 64MB of space for ttf textures (this pointer can be global)

	if (!texture_mem) return; // fail!

	free_mem = (u8*)fnt_font.init_fnt_table((u8*)texture_mem);

	initShader();

	mPosIndex = rsxVertexProgramGetAttrib(mRSXVertexProgram, "position");
	mTexIndex = rsxVertexProgramGetAttrib(mRSXVertexProgram, "texcoord");
	mColIndex = rsxVertexProgramGetAttrib(mRSXVertexProgram, "color");

	mTexUnit = rsxFragmentProgramGetAttrib(mRSXFragmentProgram, "texture");

	mPosition = (u8*)rsxMemalign(128, BITMAPFNT_MAX_CHAR_COUNT*NUM_VERTS_PER_GLYPH*sizeof(f32)*3);
	mTexCoord = (u8*)rsxMemalign(128, BITMAPFNT_MAX_CHAR_COUNT*NUM_VERTS_PER_GLYPH*sizeof(f32)*2);
	mColor = (u8*)rsxMemalign(128, BITMAPFNT_MAX_CHAR_COUNT*NUM_VERTS_PER_GLYPH*sizeof(f32)*4);

	rsxAddressToOffset(mPosition, &mPositionOffset);
	rsxAddressToOffset(mTexCoord, &mTexCoordOffset);
	rsxAddressToOffset(mColor, &mColorOffset);
}

void RSXBitmapFNTRenderer::shutdown()
{
	rsxFree(texture_mem);
	rsxFree(mPosition);
	rsxFree(mTexCoord);
	rsxFree(mColor);
}

void RSXBitmapFNTRenderer::printStart(f32 r, f32 g, f32 b, f32 a)
{
	sR = r;
	sG = g;
	sB = b;
	sA = a;

	rsxSetBlendFunc(mContext, GCM_SRC_ALPHA, GCM_ONE_MINUS_SRC_ALPHA, GCM_SRC_ALPHA, GCM_ONE_MINUS_SRC_ALPHA);
	rsxSetBlendEquation(mContext, GCM_FUNC_ADD, GCM_FUNC_ADD);
	rsxSetBlendEnable(mContext, GCM_TRUE);
	rsxSetLogicOpEnable(mContext, GCM_FALSE);

	rsxSetDepthTestEnable(mContext, GCM_FALSE);

	rsxLoadVertexProgram(mContext, mRSXVertexProgram, mVertexProgramUCode);
	rsxLoadFragmentProgramLocation(mContext, mRSXFragmentProgram, mFragmentProgramOffset, GCM_LOCATION_RSX);

}

void RSXBitmapFNTRenderer::printPass(BitmapFNT::Position* pPositions, BitmapFNT::TexCoord* pTexCoords, BitmapFNT::Color* pColors, s32 numVerts, u32 *textmem_off, u32 tex_w, u32 tex_h)
{
	

	while (*mLabel != mLabelValue)
		usleep(10);
	mLabelValue++;

	memcpy(mPosition, pPositions, numVerts * sizeof(f32) * 3);
	memcpy(mTexCoord, pTexCoords, numVerts * sizeof(f32) * 2);
	memcpy(mColor, pColors, numVerts * sizeof(f32) * 4);
	for (s32 i=0; i < numVerts/4; i++) {
		
		rsxBindVertexArrayAttrib(mContext, mPosIndex->index, 0, mPositionOffset + (i * (4 * sizeof(f32) * 3)), sizeof(f32) * 3, 3, GCM_VERTEX_DATA_TYPE_F32, GCM_LOCATION_RSX);
		rsxBindVertexArrayAttrib(mContext, mTexIndex->index, 0, mTexCoordOffset + (i * (4 * sizeof(f32) * 2)), sizeof(f32) * 2, 2, GCM_VERTEX_DATA_TYPE_F32, GCM_LOCATION_RSX);
		rsxBindVertexArrayAttrib(mContext, mColIndex->index, 0, mColorOffset + (i * (4 * sizeof(f32) * 4)), sizeof(f32) * 4, 4, GCM_VERTEX_DATA_TYPE_F32, GCM_LOCATION_RSX);

		gcmTexture tex;
		tex.format = GCM_TEXTURE_FORMAT_B8 | GCM_TEXTURE_FORMAT_LIN;
		tex.mipmap = 1;
		tex.dimension = GCM_TEXTURE_DIMS_2D;
		tex.cubemap = GCM_FALSE;
		tex.remap = GCM_TEXTURE_REMAP_TYPE_REMAP << GCM_TEXTURE_REMAP_TYPE_B_SHIFT |
			GCM_TEXTURE_REMAP_TYPE_REMAP << GCM_TEXTURE_REMAP_TYPE_G_SHIFT |
			GCM_TEXTURE_REMAP_TYPE_REMAP << GCM_TEXTURE_REMAP_TYPE_R_SHIFT |
			GCM_TEXTURE_REMAP_TYPE_REMAP << GCM_TEXTURE_REMAP_TYPE_A_SHIFT |
			GCM_TEXTURE_REMAP_COLOR_B << GCM_TEXTURE_REMAP_COLOR_B_SHIFT |
			GCM_TEXTURE_REMAP_COLOR_B << GCM_TEXTURE_REMAP_COLOR_G_SHIFT |
			GCM_TEXTURE_REMAP_COLOR_B << GCM_TEXTURE_REMAP_COLOR_R_SHIFT |
			GCM_TEXTURE_REMAP_COLOR_B << GCM_TEXTURE_REMAP_COLOR_A_SHIFT;
		tex.width = tex_w;
		tex.height = tex_h;
		tex.depth = 1;
		tex.pitch = tex_w;  // -> tex_w * 4 if GCM_TEXTURE_FORMAT_A8R8G8B8
		tex.location = GCM_LOCATION_RSX;
		tex.offset = textmem_off[i];
		rsxLoadTexture(mContext, mTexUnit->index, &tex);

		rsxTextureControl(mContext, mTexUnit->index, GCM_TRUE, 0 << 8, 12 << 8, GCM_TEXTURE_MAX_ANISO_1);
		rsxTextureFilter(mContext, mTexUnit->index, 0, GCM_TEXTURE_LINEAR, GCM_TEXTURE_LINEAR, GCM_TEXTURE_CONVOLUTION_QUINCUNX);
		rsxTextureWrapMode(mContext, mTexUnit->index, GCM_TEXTURE_CLAMP_TO_EDGE, GCM_TEXTURE_CLAMP_TO_EDGE, GCM_TEXTURE_CLAMP_TO_EDGE, 0, GCM_TEXTURE_ZFUNC_LESS, 0);

		rsxDrawVertexArray(mContext, BITMAPFNT_PRIMITIVE, 0, 4);
		
	}
	rsxInvalidateVertexCache(mContext);
	rsxSetWriteBackendLabel(mContext, sLabelId, mLabelValue);

	rsxFlushBuffer(mContext);
}


void RSXBitmapFNTRenderer::printEnd()
{
	rsxSetDepthTestEnable(mContext, GCM_TRUE);
	rsxSetBlendEnable(mContext, GCM_FALSE);
}
