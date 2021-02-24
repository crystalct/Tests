#ifndef __BITMAPFONT_H__
#define __BITMAPFONT_H__

#include <ppu-types.h>
#include <rsx/rsx.h>
#include <string.h>
#include <unistd.h>
#include <malloc.h>
#include "mesh.h"

#define BITMAPFNT_PRIMITIVE					GCM_TYPE_QUADS
#define NUM_VERTS_PER_GLYPH					4
#define BITMAPFNT_GLYPH_WIDTH				16
#define BITMAPFNT_GLYPH_HEIGHT				18

#define BITMAPFNT_TAB_SIZE					4
#define BITMAPFNT_MAX_CHAR_COUNT			1024 //max char per frame
#define BITMAPFNT_DEFAULT_SAFE_AREA			40
#define MAX_CHARS							1600  //max char per session
#define MAX_STR								2048

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

typedef struct 
{
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
	u32 fnt_ux;
	u32 fnt_uy;
	u32 r_use;
	u32* ntex_off;
	fnt_dyn fnt_font_datas[MAX_CHARS];

	Position* spPositions[2];
	TexCoord* spTexCoords[2];
	Color* spColors[2];

	Color sDefaultColors[8];

	inline void setPosition(s32 x, s32 y)
	{
		sXPos = x;
		sYPos = y;
	}

	inline f32 calcPos(f32 pos, s32 dim)
	{
		return (pos - ((f32)dim * 0.5f)) / ((f32)dim * 0.5f);
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

	inline void setScreenRes(s32 x, s32 y)
	{
		sXRes = x;
		sYRes = y;
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

} fnt_class;


int init_fnt(fnt_class * fnt);
void shutdown(fnt_class* fnt);
void printStart(f32 r, f32 g, f32 b, f32 a);
void printPass(Position* pPositions, TexCoord* pTexCoords, Color* pColors, s32 numVerts, u32* textmem_off, u32 tex_w, u32 tex_h);
void printEnd();
void initShader();
void print_fnt(const char* pszText);
void printf_fnt(const char* pszText, ...);
void print_fnt(const char* pszText, s32 count);
u32 get_fnt_char(const char* string, u32 aa_level, u32* next_char, u16* fnt_width);
void fnt_read_header(fnt_t* fnt_p);
u8* fnt_get_bits(u32 ucs2);
u16 utf16len(const u16* utf16);
int internal_load_font(const char* path, fnt_t* fnt_p);
void fnt_load_mem(const void* ptr, fnt_t* fnt_p);
int fnt_free(fnt_class* fnt);
u32 fnt_get_width_ucs2(u32 ucs2);
char* utf8_utf16(u16* utf16, const char* utf8);
void utf8_to_utf16(u16* utf16, const char* utf8);
u32 fnt_get_width(const char* str, int mx);

u8* init_fnt_table(u8* texture);



#endif // __BITMAPFONT_H__