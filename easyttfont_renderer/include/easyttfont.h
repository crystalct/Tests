#ifndef __EASYTTFONT_H__
#define __EASYTTFONT_H__

#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <string.h>
#include <unistd.h>
#include <malloc.h>
#include <math.h>
#include <setjmp.h>
#include <ppu-asm.h>
#include <ppu-types.h>

#define EASYTTFONT_TEXTURE_WIDTH			128
#define EASYTTFONT_TEXTURE_HEIGHT			128
#define EASYTTFONT_GLYPH_WIDTH				16
#define EASYTTFONT_GLYPH_HEIGHT				18
#define TTF_UX								32 
#define TTF_UY								32 
#define GLYPH_WIDTH_REL						((float)EASYTTFONT_GLYPH_WIDTH / TTF_UX)
#define GLYPH_HEIGHT_REL					((float)EASYTTFONT_GLYPH_HEIGHT / TTF_UY)
#define DELTA_GLYPH_WIDTH					EASYTTFONT_GLYPH_WIDTH

#define EASYTTFONT_TAB_SIZE					4
#define EASYTTFONT_MAX_CHAR_COUNT			1024 //max char per frame
#define EASYTTFONT_DEFAULT_SAFE_AREA			40
#define MAX_CHARS							1600  //max char per session

typedef struct ttf_dyn {
	u32 ttf;
	u16* text;
	u32 offset;
	u32 r_use;
	u16 y_start;
	u16 width;
	u16 height;
	u16 flags;

} ttf_dyn;

class EasyTTFontRenderer;

class EasyTTFont
{
    friend class EasyTTFontRenderer;

public:
    static void init();
    static void shutdown();

	static inline void setPosition(s32 x, s32 y)
	{
		sXPos = x;
		sYPos = y;
	}

	static inline void setColor(f32 r, f32 g, f32 b, f32 a)
	{
		sR = r;
		sG = g;
		sB = b;
		sA = a;
	}
	

	static void print(const char *pszText);
	static void printf(const char *pszText, ...);

	int TTFLoadFont(char* path, void* from_memory, int size_from_memory);
	void TTFUnloadFont();
	u16* init_ttf_table(u16* texture);
	/*static void getExtents(const char *pszText, s32 *pWidth, s32 *pHeight, s32 srcWidth);*/

	static inline void setScreenRes(s32 x, s32 y)
	{
		sXRes = x;
		sYRes = y;
	}

	static inline void setSafeArea(s32 left, s32 right, s32 top, s32 bottom)
	{
		sLeftSafe = left;
		sRightSafe = right;
		sTopSafe = top;
		sBottomSafe = bottom;
	}

	static inline void getSafeArea(s32 *pLeft, s32 *pRight, s32 *pTop, s32 *pBottom)
	{
		*pLeft = sLeftSafe;
		*pRight = sRightSafe;
		*pTop = sTopSafe;
		*pBottom = sBottomSafe;
	}

	static void setDimension(u32 w, u32 h)
	{
		gly_w = w;
		gly_h = h;
	}

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

private:
	static inline u32 get_ttf_char(const char* string, u32* next_char, u32* ttf_y_start, u16* ttf_width, u16* ttf_height);
	static void print(const char *pszText, s32 count);
	

	static inline f32 calcPos(f32 pos, s32 dim)
	{
		return (pos - ((f32)dim*0.5f))/((f32)dim*0.5f);
	}


	static inline bool isPrintable(char c)
	{
		return ((u8)c&0x7f) > 31;
	}

	static u32 r_use;
	static s32 ttf_inited;
	static ttf_dyn ttf_font_datas[MAX_CHARS];
	static s32 sXPos;
	static s32 sYPos;
	static s32 sXRes;
	static s32 sYRes;
	static s32 sLeftSafe;
	static s32 sRightSafe;
	static s32 sTopSafe;
	static s32 sBottomSafe;
	static f32 sR;
	static f32 sG;
	static f32 sB;
	static f32 sA;
	static u8 gly_w;
	static u8 gly_h;

	static Position *spPositions[2];
	static TexCoord *spTexCoords[2];
	static Color *spColors[2];

	static Color sDefaultColors[8];

	static EasyTTFontRenderer *spRenderer;
	static u32* ntex_off;
};

#endif // __EASYTTFONT_H__
