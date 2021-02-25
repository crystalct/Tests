#ifndef __FONTS_BITMAP_H__
#define __FONTS_BITMAP_H__


#define MAX_CHARS_FNT							1600  //max char per session

typedef struct fnt_dyn {
	u32 ttf;
	u8* text;
	u32 offset;
	u32 r_use;
	u16 width;
	u16 height;
	u16 flags;

} fnt_dyn;

typedef struct {
	uint16_t maxwidth;
	uint16_t height;
	uint16_t ascent;
	uint16_t pad;
	uint32_t firstchar;
	uint32_t size;
	uint32_t defaultchar;
	uint32_t nbits;
	uint32_t noffset;
	uint32_t nwidth;
	const uint8_t* bits;
	const uint32_t* offset;
	const uint8_t* width;
	uint32_t long_offset;
	uint32_t file_flag;
	void* fnt_ptr;
} fnt_t;

struct Position
{
	f32 x, y, z;
};

struct TexCoord
{
	f32 s, t;
};

struct Color
{
	f32 r, g, b, a;
};

void printn_fnt(void* fnt, const char* pszText, s32 count);
void initShader_fnt(void* fntc);
void print_fnt(void* fnt, const char* pszText);

typedef struct fnt_class
{
	fnt_class* self;
	fnt_t fnt;
	s32 sXPos;
	s32 sYPos;
	f32 sR;
	f32 sG;
	f32 sB;
	f32 sA;
	u8 gly_w;
	u8 gly_h;
	s32 sXRes;
	s32 sYRes;
	s32 sLeftSafe;
	s32 sRightSafe;
	s32 sTopSafe;
	s32 sBottomSafe;
	u32 aa_level;
	u32 fnt_ux;
	u32 fnt_uy;
	u32 r_use;
	u32* ntex_off;
	fnt_dyn fnt_font_datas[MAX_CHARS_FNT];

	gcmContextData* mContext;
	u8* texture_mem;		//fnt_textures

	u8* next_mem;			// Pointer after last ttf texture

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

	Position* spPositions[2];
	TexCoord* spTexCoords[2];
	Color* spColors[2];

	Color sDefaultColors[8] = {
	{ 0.f, 0.f, 0.f, 1.f },	// black
	{ 0.f, 0.f, 1.f, 1.f },	// blue
	{ 1.f, 0.f, 0.f, 1.f },	// red
	{ 1.f, 0.f, 1.f, 1.f },	// magenta
	{ 0.f, 1.f, 0.f, 1.f },	// green
	{ 0.f, 1.f, 1.f, 1.f },	// cyan
	{ 1.f, 1.f, 0.f, 1.f },	// yellow
	{ 1.f, 1.f, 1.f, 1.f },	// white
	};


	inline void setPosition(s32 x, s32 y)
	{
		sXPos = x;
		sYPos = y;
	}


	inline bool isPrintable(char c)
	{
		return ((u8)c & 0x7f) > 31;
	}



	inline void setColor(f32 r, f32 g, f32 b, f32 a)
	{
		sR = r;
		sG = g;
		sB = b;
		sA = a;
	}


	inline void setSafeArea(s32 left, s32 right, s32 top, s32 bottom)
	{
		sLeftSafe = left;
		sRightSafe = right;
		sTopSafe = top;
		sBottomSafe = bottom;
	}

	inline void getSafeArea(s32* pLeft, s32* pRight, s32* pTop, s32* pBottom)
	{
		*pLeft = sLeftSafe;
		*pRight = sRightSafe;
		*pTop = sTopSafe;
		*pBottom = sBottomSafe;
	}

	inline void setDimension(u32 w, u32 h)
	{
		gly_w = w;
		gly_h = h;
	}

	void (*read_header)(fnt_t* fnt_p);
	void (*load_mem)(const void* ptr, fnt_class* fntc);
	int (*load_file)(const char* path, fnt_class* fntc);
	void (*free_mem)(fnt_class* fnt);
	//void (*initShader)(fnt_class* fntc);
	inline void initShader() {
		initShader_fnt((void*)self);
	}
	void (*printStart)(fnt_class* fnt);
	u8* (*init_table)(u8* texture, fnt_class* fnt);
	u32(*get_char)(const char* string, u32 aa_level, u32* next_char, u16* fnt_width, fnt_class* fntc);
	u32(*get_width)(const char* str, int mx, fnt_class* fntc);
	u32(*get_width_ucs2)(u32 ucs2, fnt_class* fntc);
	u8* (*get_bits)(u32 ucs2, fnt_class* fntc);
	void (*printPass)(Position* pPositions, TexCoord* pTexCoords, Color* pColors, s32 numVerts, u32* textmem_off, u32 tex_w, u32 tex_h, fnt_class* fnt);
	void (*printEnd)(fnt_class* fnt);
	int (*vsprintf)(char* buffer, const char* format, va_list arg);
	//void (*printf)(fnt_class* fnt, const char* pszText, ...);
	void (*shutdown)(fnt_class* fntc);

	inline void print(const char* pszText)
	{
		print_fnt((void*)self, pszText);
	}

	inline void printn(const char* pszText, s32 count)
	{
		printn_fnt((void*)self, pszText, count);
	}

	inline void printf(const char* pszText, ...)
	{
		va_list argList;
		char tempStr[1024];

		va_start(argList, pszText);
		vsprintf(tempStr, pszText, argList);
		va_end(argList);
		printn_fnt((void*)self, tempStr, strlen(tempStr));
	}

} fnt_class;

int init_fnt(gcmContextData* context, u32 display_width, u32 display_height, u32 aalevel, fnt_class* fnt);
int init_fnt(gcmContextData* context, u32 display_width, u32 display_height, const char* path, u32 aalevel, fnt_class* fntc);

#endif // __FONTS_BITMAP_H__