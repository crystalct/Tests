#ifndef __BITMAPFNT_H__
#define __BITMAPFNT_H__

#include <stdio.h>
#include <stdlib.h>
#include <stddef.h>
#include <stdarg.h>
#include <string.h>
#include <unistd.h>
#include <malloc.h>
#include <math.h>
#include <setjmp.h>
#include <ppu-asm.h>
#include <ppu-types.h>

#define BITMAPFNT_GLYPH_WIDTH				16
#define BITMAPFNT_GLYPH_HEIGHT				18

#define BITMAPFNT_TAB_SIZE					4
#define BITMAPFNT_MAX_CHAR_COUNT			1024 //max char per frame
#define BITMAPFNT_DEFAULT_SAFE_AREA			0
#define MAX_CHARS							1600  //max char per session
#define MAX_STR								2048

typedef struct fnt_dyn {
	u32 ttf;
	u8* text;
	u32 offset;
	u32 r_use;
	//u16 y_start;
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

class BitmapFNTRenderer;

class BitmapFNT
{
    friend class BitmapFNTRenderer;

public:
    static int init();
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
	static void print(const char* pszText, s32 count);

	u8* init_fnt_table(u8* texture);
	

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
	static u32 fnt_ux;
	static u32 fnt_uy;

private:
	static inline u32 get_fnt_char(const char* string, u32* next_char, u16* fnt_width);
	static void fnt_read_header(const u8* fnt_ptr);
	static u8* fnt_get_bits(u32 ucs2);
	static u16 utf16len(const u16* utf16)
	{
		u16 len = 0;
		while (utf16[len] != '\0')
			len++;
		return len;
	}

	static int internal_load_font(const char* path) 
	{ 
		unsigned char* buf;
		//483.549 +1

		FILE* f = fopen(path, "rb");
		if (!f)
			return 1; // Err.
		fseek(f, 0, SEEK_END);
		long fsize = ftell(f);
		fseek(f, 0, SEEK_SET);  /* same as rewind(f); */

		buf = (unsigned char*)malloc(fsize + 1);
		if (!buf)
			return 1; // Err.
		fread(buf, 1, fsize, f);
		fclose(f);
		fontFNT.file_flag = 1;
		fontFNT.fnt_ptr = buf;

		fnt_read_header((u8*)fontFNT.fnt_ptr);
		return 0;
	}

	static void fnt_load_mem(const void* ptr)
	{
		fontFNT.file_flag = 0;

		fontFNT.fnt_ptr = (void*)ptr;

		fnt_read_header((u8*)fontFNT.fnt_ptr);
	}

	static int fnt_free()
	{
		
		if (fontFNT.file_flag == 1) {
			free(fontFNT.fnt_ptr);
			fontFNT.fnt_ptr = NULL;
		}

		return 0;
	}

	static u32 fnt_get_width_ucs2(u32 ucs2)
	{
		u16 width = 0;

		if ((ucs2 < fontFNT.firstchar) || (ucs2 >= fontFNT.firstchar + fontFNT.size))
			ucs2 = fontFNT.defaultchar;

		if (fontFNT.nwidth != 0)
			width = fontFNT.width[ucs2 - fontFNT.firstchar];
		else
			width = fontFNT.maxwidth;

		return width;
	}
	
	static char* utf8_utf16(u16* utf16, const char* utf8)
	{
		u8 c = *utf8++;
		u32 code;
		u32 tail = 0;

		if ((c <= 0x7f) || (c >= 0xc2))
		{
			/* Start of new character. */
			if (c < 0x80)
			{
				/* U-00000000 - U-0000007F, 1 byte */
				code = c;
			}
			else if (c < 0xe0)   /* U-00000080 - U-000007FF, 2 bytes */
			{
				tail = 1;
				code = c & 0x1f;
			}
			else if (c < 0xf0)   /* U-00000800 - U-0000FFFF, 3 bytes */
			{
				tail = 2;
				code = c & 0x0f;
			}
			else if (c < 0xf5)   /* U-00010000 - U-001FFFFF, 4 bytes */
			{
				tail = 3;
				code = c & 0x07;
			}
			else                /* Invalid size. */
			{
				code = 0xfffd;
			}

			while (tail-- && ((c = *utf8++) != 0))
			{
				if ((c & 0xc0) == 0x80)
				{
					/* Valid continuation character. */
					code = (code << 6) | (c & 0x3f);

				}
				else
				{
					/* Invalid continuation char */
					code = 0xfffd;
					utf8--;
					break;
				}
			}
		}
		else
		{
			/* Invalid UTF-8 char */
			code = 0xfffd;
		}
		/* currently we don't support chars above U-FFFF */
		*utf16 = (code < 0x10000) ? code : 0xfffd;
		return (char*)utf8;
	}
	
	static void utf8_to_utf16(u16* utf16, const char* utf8)
	{
		while (*utf8 != '\0')
		{
			utf8 = utf8_utf16(utf16++, utf8);
		}
		*utf16 = '\0';
	}

	static u32 fnt_get_width(const char* str, int mx)
	{
		u16 i;
		u16 len;
		u16 width = 0;
		u16 ucs[MAX_STR];

		utf8_to_utf16(ucs, str);
		len = utf16len(ucs);

		for (i = 0; i < len; i++)
			width += fnt_get_width_ucs2(ucs[i]);

		return width * mx;
	}

	static inline f32 calcPos(f32 pos, s32 dim)
	{
		return (pos - ((f32)dim*0.5f))/((f32)dim*0.5f);
	}


	static inline bool isPrintable(char c)
	{
		return ((u8)c&0x7f) > 31;
	}

	static fnt_t fontFNT;
	static u32 r_use;
	
	static fnt_dyn fnt_font_datas[MAX_CHARS];
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

	static BitmapFNTRenderer *spRenderer;
	static u32* ntex_off;
};

#endif // __BITMAPFNT_H__
