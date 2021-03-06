#ifndef TTF_RENDER_H
#define TTF_RENDER_H

#include <ppu-types.h>

// define window mode and size

#define WIN_AUTO_LF 1
#define WIN_SKIP_LF 2
#define WIN_DOUBLE_LF  4
#define TTF_UX 30 //30
#define TTF_UY 24 //24

extern float Y_ttf;
extern float Z_ttf;

#ifdef __cplusplus
extern "C" {
#endif
int display_ttf_string(int posx, int posy, const char* string, u32 color, u32 bkcolor, int sw, int sh, int (*DrawIcon)(int, int, char));
u32 get_ttf_char(const char* string, u32* next_char, u16 *ttf_y_start, u16* ttf_width, u16* ttf_height);
int TTFLoadFont(int set, char* path, void* from_memory, int size_from_memory);
void TTFUnloadFont();
void TTF_to_Bitmap(u8 chr, u8* bitmap, short* w, short* h, short* y_correction);
int Render_String_UTF8(u16* bitmap, int w, int h, u8* string, int sw, int sh);

// initialize and create textures slots for ttf
u16* init_ttf_table(u16* texture);

// do one time per frame to reset the character use flag
void reset_ttf_frame(void);

// display UTF8 string int posx/posy position. With color 0 don't display and refresh/calculate the width. 
// color is the character color and sw/sh the width/height of the characters

int width_ttf_string(const char* string, int sw, int sh);
void set_ttf_window(int x, int y, int width, int height, u32 mode);
#ifdef __cplusplus
}
#endif

#endif
