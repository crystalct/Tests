#include <rsx/rsx.h>
#include "easyttfont.h"
#include "easyttfontrenderer.h"


#define NUM_VERTS_PER_GLYPH				4

extern u32* texture_buffer_app;

FT_Library EasyTTFont::freetype;
FT_Face EasyTTFont::face;
int EasyTTFont::f_face;

ttf_dyn EasyTTFont::ttf_font_datas[MAX_CHARS];

u32 EasyTTFont::r_use;
s32 EasyTTFont::ttf_inited;
s32 EasyTTFont::sXPos;
s32 EasyTTFont::sYPos;

s32 EasyTTFont::sXRes;
s32 EasyTTFont::sYRes;

s32 EasyTTFont::sLeftSafe;
s32 EasyTTFont::sRightSafe;
s32 EasyTTFont::sTopSafe;
s32 EasyTTFont::sBottomSafe;

f32 EasyTTFont::sR;
f32 EasyTTFont::sG;
f32 EasyTTFont::sB;
f32 EasyTTFont::sA;

u32* EasyTTFont::ntex_off;
u8 EasyTTFont::gly_w;
u8 EasyTTFont::gly_h;


EasyTTFont::Position *EasyTTFont::spPositions[2];
EasyTTFont::TexCoord *EasyTTFont::spTexCoords[2];
EasyTTFont::Color *EasyTTFont::spColors[2];

EasyTTFont::Color EasyTTFont::sDefaultColors[8] =
{
	{ 0.f, 0.f, 0.f, 1.f },	// black
	{ 0.f, 0.f, 1.f, 1.f },	// blue
	{ 1.f, 0.f, 0.f, 1.f },	// red
	{ 1.f, 0.f, 1.f, 1.f },	// magenta
	{ 0.f, 1.f, 0.f, 1.f },	// green
	{ 0.f, 1.f, 1.f, 1.f },	// cyan
	{ 1.f, 1.f, 0.f, 1.f },	// yellow
	{ 1.f, 1.f, 1.f, 1.f },	// white
};

EasyTTFontRenderer *EasyTTFont::spRenderer = NULL;

void EasyTTFont::init()
{
	if(!spRenderer) return;

	gly_w = EASYTTFONT_GLYPH_WIDTH;
	gly_h = EASYTTFONT_GLYPH_HEIGHT;
	r_use = 0;
	f_face = 0;

	spRenderer->init();
	ttf_inited = 0;
	sXPos = sYPos = 0;
	sLeftSafe = sRightSafe = sTopSafe = sBottomSafe = EASYTTFONT_DEFAULT_SAFE_AREA;
	sR = sG = sB = 1.0f;

	spPositions[0] = new Position[EASYTTFONT_MAX_CHAR_COUNT*NUM_VERTS_PER_GLYPH];
	spPositions[1] = new Position[EASYTTFONT_MAX_CHAR_COUNT*NUM_VERTS_PER_GLYPH];
	spTexCoords[0] = new TexCoord[EASYTTFONT_MAX_CHAR_COUNT*NUM_VERTS_PER_GLYPH];
	spTexCoords[1] = new TexCoord[EASYTTFONT_MAX_CHAR_COUNT*NUM_VERTS_PER_GLYPH];
	spColors[0] = new Color[EASYTTFONT_MAX_CHAR_COUNT*NUM_VERTS_PER_GLYPH];
	spColors[1] = new Color[EASYTTFONT_MAX_CHAR_COUNT*NUM_VERTS_PER_GLYPH];
	ntex_off = (u32 *) malloc(EASYTTFONT_MAX_CHAR_COUNT * sizeof(u32));
}

void EasyTTFont::shutdown()
{
	if(!spRenderer) return;

	spRenderer->shutdown();

	delete [] spPositions[0];
	delete [] spPositions[1];
	delete [] spTexCoords[0];
	delete [] spTexCoords[1];
	delete [] spColors[0];
	delete [] spColors[1];
	free(ntex_off);
}

u16* EasyTTFont::init_ttf_table(u16* texture)
{
	int n;

	r_use = 0;
	for (n = 0; n < MAX_CHARS; n++) {
		memset(&ttf_font_datas[n], 0, sizeof(ttf_dyn));
		ttf_font_datas[n].text = texture;

		texture += 32 * 32;
	}

	return texture;

}


inline u32 EasyTTFont::get_ttf_char(const char* string, u32* next_char, u32* ttf_y_start, u16* ttf_width, u16* ttf_height)
{
	//printf("CALLED\n");
	int l, n, m, ww, ww2;
	//printf("Char: %c ", string[0]);
	u8 colorc;
	u32 ttf_char;
	u8* ustring = (u8*)string;


	if (*ustring & 128) {
		//printf("*ustring & 128\n");
		m = 1;

		if ((*ustring & 0xf8) == 0xf0) { // 4 bytes
			ttf_char = (u32)(*(ustring++) & 3);
			m = 3;
			*next_char = 3;
		}
		else if ((*ustring & 0xE0) == 0xE0) { // 3 bytes
			ttf_char = (u32)(*(ustring++) & 0xf);
			m = 2;
			*next_char = 2;
		}
		else if ((*ustring & 0xE0) == 0xC0) { // 2 bytes
			ttf_char = (u32)(*(ustring++) & 0x1f);
			m = 1;
			*next_char = 1;
		}
		else { ustring++; *next_char = 1; } // error!

		for (n = 0; n < m; n++) {
			if (!*ustring) break; // error!
			if ((*ustring & 0xc0) != 0x80) break; // error!
			ttf_char = (ttf_char << 6) | ((u32)(*(ustring++) & 63));
		}

		if ((n != m) && !*ustring) return 0;

	}
	else {
		ttf_char = (u32) * (ustring++);
		*next_char = 0;
	}

	//printf("*next_char: %d\n", *next_char);

	// search ttf_char
	if (ttf_char < 128) n = ttf_char;
	else {
		//printf("ttf_char: 0x%x\n", ttf_char);
		m = 0;
		int rel = 0;

		for (n = 128; n < MAX_CHARS; n++) {
			if (!(ttf_font_datas[n].flags & 1)) m = n;

			if ((ttf_font_datas[n].flags & 3) == 1) {
				int trel = r_use - ttf_font_datas[n].r_use;
				if (m == 0) { m = n; rel = trel; }
				else if (rel > trel) { m = n; rel = trel; }

			}
			if (ttf_font_datas[n].ttf == ttf_char) break;
		}

		if (m == 0) m = 128;

	}

	if (n >= MAX_CHARS) { ttf_font_datas[m].flags = 0; l = m; }
	else { l = n; }

	u16* bitmap = ttf_font_datas[l].text;
	rsxAddressToOffset((u8*)bitmap, &ttf_font_datas[l].offset);

	texture_buffer_app = (u32*)bitmap;
	// building the character

	if (!(ttf_font_datas[l].flags & 1)) {

		if (f_face) FT_Set_Pixel_Sizes(face, TTF_UX, TTF_UY);


		FT_GlyphSlot slot = NULL;

		memset(bitmap, 0, 32 * 32 * 2);

		///////////

		FT_UInt index;

		if (f_face && (index = FT_Get_Char_Index(face, ttf_char)) != 0
			&& !FT_Load_Glyph(face, index, FT_LOAD_RENDER)) slot = face->glyph;
		else ttf_char = 0;

		if (ttf_char != 0) {
			ww = ww2 = 0;

			int y_correction = TTF_UY - 1 - slot->bitmap_top;
			if (y_correction < 0) y_correction = 0;

			ttf_font_datas[l].flags = 1;
			ttf_font_datas[l].y_start = y_correction;
			ttf_font_datas[l].height = slot->bitmap.rows;
			ttf_font_datas[l].width = slot->bitmap.width;
			ttf_font_datas[l].ttf = ttf_char;


			for (n = 0; n < slot->bitmap.rows; n++) {
				if (n >= 32) break;
				for (m = 0; m < slot->bitmap.width; m++) {

					if (m >= 32) continue;

					colorc = (u8)slot->bitmap.buffer[ww + m];

					if (colorc) bitmap[m + ww2] = (colorc << 8) | 0xfff;
				}

				ww2 += 32;

				ww += slot->bitmap.width;
			}

		}
	}

	// displaying the character
	ttf_font_datas[l].flags |= 2; // in use
	ttf_font_datas[l].r_use = r_use;
	/*printf("L: %d, Str: %s, delta: %d, offset: x0%x\n", l, string, *next_char, ttf_font_datas[l].offset);
	printf(" delta: %d - Ystart: %d - w: %d - h: %d\n",
		*next_char, ttf_font_datas[l].y_start, ttf_font_datas[l].width, ttf_font_datas[l].height);*/

	*ttf_y_start = ttf_font_datas[l].y_start;
	*ttf_width = ttf_font_datas[l].width;
	*ttf_height = ttf_font_datas[l].height;
	return ttf_font_datas[l].offset;
}


int EasyTTFont::TTFLoadFont(char* path, void* from_memory, int size_from_memory)
{

	if (!ttf_inited)
		FT_Init_FreeType(&freetype);
	ttf_inited = 1;

	f_face = 0;

	if (path) {
		if (FT_New_Face(freetype, path, 0, &face) < 0) return -1;
	}
	else {
		if (FT_New_Memory_Face(freetype, (const unsigned char*)from_memory, size_from_memory, 0, &face)) return -1;
	}

	f_face = 1;

	return 0;
}

/* release all */

void EasyTTFont::TTFUnloadFont()
{
	if (!ttf_inited) return;
	FT_Done_FreeType(freetype);
	ttf_inited = 0;
}

void EasyTTFont::printf(const char *pszText, ...)
{
	va_list argList;
	char tempStr[1024];

	va_start(argList, pszText);
	vsprintf(tempStr, pszText, argList);
	va_end(argList);

	print(tempStr, strlen(tempStr));
}

void EasyTTFont::print(const char *pszText)
{
	print(pszText, strlen(pszText));
}

void EasyTTFont::print(const char *pszText, s32 count)
{
	s32 c, offset, pass, numPasses;
	u32 deltaUTF;
	u32 y_start;
	u16 gly_Height;
	u16 gly_Width;
	u32 idx = 0;

	if(!spRenderer) return;

	offset = 0;
	numPasses = (count/EASYTTFONT_MAX_CHAR_COUNT) + 1;

	spRenderer->printStart(sR, sG, sB, sA);

	//::printf("String: %s\n", pszText);
	for(pass=0;pass < numPasses;pass++) {
		s32 numVerts = 0;
		
		TexCoord *pTexCoords = spTexCoords[pass&1];
		Position *pPositions = spPositions[pass&1];
		Color *pColors = spColors[pass&1];

		for(c=0;(offset + c) < count && c < EASYTTFONT_MAX_CHAR_COUNT;c++) {
			if (isPrintable(pszText[offset + c])) {
				// top left
				ntex_off[idx] = get_ttf_char(pszText + offset + c, &deltaUTF, &y_start, &gly_Width, &gly_Height);
				
				//::printf("Char: %c - dy: %d \n", pszText[offset + c], dy);
				//pPositions[numVerts].x = calcPos(sXPos + sLeftSafe + (float)(GLYPH_WIDTH_REL * (TTF_UX - gly_Width) / 2), sXRes);
				pPositions[numVerts].x = calcPos(sXPos + sLeftSafe, sXRes);
				
				//pPositions[numVerts].y = calcPos((float)sYPos + sTopSafe + (TTF_UY - gly_Height) * GLYPH_HEIGHT_REL, sYRes) * -1.0f ;
				pPositions[numVerts].y = calcPos((float)sYPos + sTopSafe + y_start * (float)gly_h / TTF_UY, sYRes) * -1.0f;
				
				pPositions[numVerts].z = 0.0f;
				
				pTexCoords[numVerts].s = 0.0f; // calcS0((u8)pszText[offset + c]);
				pTexCoords[numVerts].t = 0.0f; // calcT0((u8)pszText[offset + c]);
				
				//::printf("REAL 1 X: %f", pPositions[numVerts].x);
				//::printf(" REAL 1 Y: %f\n", pPositions[numVerts].y);
				pColors[numVerts].r = sR;
				pColors[numVerts].g = sG;
				pColors[numVerts].b = sB;
				pColors[numVerts].a = sA;
				numVerts++;

				// top right
				//pPositions[numVerts].x = calcPos(sXPos + sLeftSafe + gly_Width * (float)GLYPH_WIDTH_REL + (float)(GLYPH_WIDTH_REL * (TTF_UX - gly_Width) / 2), sXRes);
				pPositions[numVerts].x = calcPos(sXPos + sLeftSafe + gly_w, sXRes);
				pPositions[numVerts].y = calcPos(sYPos + sTopSafe + y_start * (float)gly_h / TTF_UY, sYRes)*-1.0f ;
				
				pPositions[numVerts].z = 0.0f;
				
				pTexCoords[numVerts].s = 1.0f; //calcS1((u8)pszText[offset + c]);
				pTexCoords[numVerts].t = 0.0f; //calcT0((u8)pszText[offset + c]);
				
				//::printf("REAL 2 X: %f", pPositions[numVerts].x);
				//::printf(" REAL 2 Y: %f\n", pPositions[numVerts].y);
				pColors[numVerts].r = sR;
				pColors[numVerts].g = sG;
				pColors[numVerts].b = sB;
				pColors[numVerts].a = sA;
				numVerts++;

				// bottom right
				//pPositions[numVerts].x = calcPos(sXPos + sLeftSafe + gly_Width * (float)GLYPH_WIDTH_REL + (float)(GLYPH_WIDTH_REL * (TTF_UX - gly_Width) / 2), sXRes);
				pPositions[numVerts].x = calcPos(sXPos + sLeftSafe + gly_w, sXRes);
				pPositions[numVerts].y = calcPos((float)sYPos + sTopSafe + y_start * (float)gly_h / TTF_UY + gly_h, sYRes)*-1.0f;
				
				pPositions[numVerts].z = 0.0f;
				
				pTexCoords[numVerts].s = 1.0f; //calcS1((u8)pszText[offset + c]);
				pTexCoords[numVerts].t = 1.0f; //calcT1((u8)pszText[offset + c]);
				
				//::printf("REAL 3 X: %f", pPositions[numVerts].x);
				//::printf(" REAL 3 Y: %f\n", pPositions[numVerts].y);
				pColors[numVerts].r = sR;
				pColors[numVerts].g = sG;
				pColors[numVerts].b = sB;
				pColors[numVerts].a = sA;
				numVerts++;

				// bottom left
				//pPositions[numVerts].x = calcPos(sXPos + sLeftSafe + (float)(GLYPH_WIDTH_REL * (TTF_UX - gly_Width) / 2), sXRes);
				pPositions[numVerts].x = calcPos(sXPos + sLeftSafe, sXRes);
				pPositions[numVerts].y = calcPos((float)sYPos + sTopSafe + y_start * (float)gly_h / TTF_UY + gly_h, sYRes)*-1.0f;
				
				pPositions[numVerts].z = 0.0f;
				
				pTexCoords[numVerts].s = 0.0f; //calcS0((u8)pszText[offset + c]);
				pTexCoords[numVerts].t = 1.0f; //calcT1((u8)pszText[offset + c]);
				
				//::printf("REAL 4 X: %f", pPositions[numVerts].x);
				//::printf(" REAL 4 Y: %f\n", pPositions[numVerts].y);
				pColors[numVerts].r = sR;
				pColors[numVerts].g = sG;
				pColors[numVerts].b = sB;
				pColors[numVerts].a = sA;
				numVerts++;

				if (pszText[offset + c] == ' ')
					sXPos += gly_w/2;
				else
					sXPos += (int)((gly_Width * (float)gly_w / TTF_UX)+ 1.99f);
				
				//::printf("char: %c - gly_Width %d - dxPos %d - dY: %d - dyPos: %d\n", 
					//pszText[offset + c], gly_Width, (int)((gly_Width * (float)GLYPH_WIDTH_REL) + 1), gly_Height);

			}
			else if(pszText[offset + c] == '\n') {
				sXPos = 0;
				sYPos += (gly_Height * (float)(gly_h / TTF_UY) + (TTF_UY - gly_Height) * (float)(gly_h / TTF_UY) + 1);
			}
			else if(pszText[offset + c] == '\t') {
				sXPos += EASYTTFONT_TAB_SIZE*((int)((gly_Width * (float)gly_w / TTF_UX) + (float)((gly_w / TTF_UX) * (TTF_UX - gly_Width) / 2) + 1.99));
			}
			else if((s8)pszText[offset + c] < 0) {
				s32 iCode = (u8)pszText[offset + c] - 128;
				if(iCode >= 0 && iCode < 8) {
					sR = sDefaultColors[iCode].r;
					sG = sDefaultColors[iCode].g;
					sB = sDefaultColors[iCode].b;
				}
			}
			else {
				sXPos += (int)((gly_Width * (float)(gly_w / TTF_UX)) + (float)((gly_w / TTF_UX) * (TTF_UX - gly_Width) / 2) + 1.99);
			}

			if(sXPos + (int)((gly_Width * (float)(gly_w / TTF_UX)) + (float)((gly_w / TTF_UX) * (TTF_UX - gly_Width) / 2) + 1.99) >= sXRes - (sLeftSafe + sRightSafe)) {
				sXPos = 0;
				sYPos += (gly_Height * (gly_h / TTF_UY) + (TTF_UY - gly_Height) * (gly_h / TTF_UY) + 1);
			}
			//::printf("CALL get_ttf_char - IDX: %d - offset: %d - c: %d \n", idx, offset, c);
			
			idx++;
			c += deltaUTF;
		}

		
		offset += EASYTTFONT_MAX_CHAR_COUNT;
		
		spRenderer->printPass(pPositions, pTexCoords, pColors, numVerts, ntex_off);
		
		
	}

	spRenderer->printEnd();
}

/*
void EasyTTFont::getExtents(const char *pszText, s32 *pWidth, s32 *pHeight, s32 srcWidth)
{
	s32 len, x, y;
	bool lineFeed = true;

	x = y = 0;
	len = strlen(pszText);
	*pWidth = *pHeight = 0;

	for(s32 i=0;i < len;i++) {
		if(isPrintable(pszText[i])) {
			x += (getGlyphWidth() + 1);
			if(lineFeed) {
				lineFeed = false;
				y += (getGlyphHeight() + 1);
			}
		}
		else if(pszText[i] == '\n') {
			x = 0;
			lineFeed = true;
		}
		else if(pszText[i] == '\t') {
			x += EASYTTFONT_TAB_SIZE*(getGlyphWidth() + 1);
		}
		else if((s8)pszText[i] < 0) {}
		else {
			x += (getGlyphWidth() + 1);
		}

		if(x + (getGlyphWidth() + 1) >= srcWidth - (sLeftSafe + sRightSafe)) {
			x = 0;
			lineFeed = true;
		}

		if(x > *pWidth) *pWidth = x;
		if(y > *pHeight) *pHeight = y;
	}
}*/
