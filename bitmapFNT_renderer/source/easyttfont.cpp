#include <rsx/rsx.h>
#include "easyttfont.h"
#include "easyttfontrenderer.h"


#define NUM_VERTS_PER_GLYPH				4

ttf_dyn EasyTTFont::ttf_font_datas[MAX_CHARS];

fnt_t EasyTTFont::fontFNT;
u32 EasyTTFont::fnt_ux;
u32 EasyTTFont::fnt_uy;
u32 EasyTTFont::r_use;
//s32 EasyTTFont::ttf_inited;
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

int EasyTTFont::init()
{
	if(!spRenderer) return 1;

	gly_w = EASYTTFONT_GLYPH_WIDTH;
	gly_h = EASYTTFONT_GLYPH_HEIGHT;
	r_use = 0;

	if (internal_load_font("/dev_hdd0/game/FBNE00123/USRDIR/b24_b.fnt"))
		return 1;	//Error
	
	::printf("Font w: %d - h: %d\n", fontFNT.maxwidth, fontFNT.height);
	fnt_ux = fontFNT.maxwidth;
	fnt_uy = fontFNT.height;

	spRenderer->init();
	
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
	return 0;
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
	fnt_free();
}

u8* EasyTTFont::fnt_get_bits(u32 ucs2)
{
	u8* bits;
	u8* tmp;

	if ((ucs2 < fontFNT.firstchar) || (ucs2 >= fontFNT.firstchar + fontFNT.size))
		ucs2 = fontFNT.defaultchar;

	ucs2 -= fontFNT.firstchar;

	if (fontFNT.long_offset == 0)
	{
		tmp = (u8*)fontFNT.offset + ucs2 * 2;
		bits = (u8*)fontFNT.bits + (fontFNT.offset ?
			(tmp[1] << 8) + tmp[0] :
			(u8)(((fontFNT.height + 7) / 8) * fontFNT.maxwidth * ucs2));
	}
	else
	{
		tmp = (u8*)fontFNT.offset + ucs2 * 4;
		bits = (u8*)fontFNT.bits + (fontFNT.offset ?
			(tmp[3] << 24) + (tmp[2] << 16) + (tmp[1] << 8) + tmp[0] :
			(u8)(((fontFNT.height + 7) / 8) * fontFNT.maxwidth * ucs2));
	}

	return bits;
}

void EasyTTFont::fnt_read_header(const u8* fnt_ptr)
{
	u32 pad;
	u32 shift;

	fontFNT.maxwidth = (fnt_ptr[5] << 8) + fnt_ptr[4];
	fontFNT.height = (fnt_ptr[7] << 8) + fnt_ptr[6];
	fontFNT.ascent = (fnt_ptr[9] << 8) + fnt_ptr[8];
	fontFNT.pad = (fnt_ptr[11] << 8) + fnt_ptr[10];
	fontFNT.firstchar = (fnt_ptr[15] << 24) + (fnt_ptr[14] << 16) + (fnt_ptr[13] << 8) + fnt_ptr[12];
	fontFNT.defaultchar = (fnt_ptr[19] << 24) + (fnt_ptr[18] << 16) + (fnt_ptr[17] << 8) + fnt_ptr[16];
	fontFNT.size = (fnt_ptr[23] << 24) + (fnt_ptr[22] << 16) + (fnt_ptr[21] << 8) + fnt_ptr[20];
	fontFNT.nbits = (fnt_ptr[27] << 24) + (fnt_ptr[26] << 16) + (fnt_ptr[25] << 8) + fnt_ptr[24];
	fontFNT.noffset = (fnt_ptr[31] << 24) + (fnt_ptr[30] << 16) + (fnt_ptr[29] << 8) + fnt_ptr[28];
	fontFNT.nwidth = (fnt_ptr[35] << 24) + (fnt_ptr[34] << 16) + (fnt_ptr[33] << 8) + fnt_ptr[32];

	fontFNT.bits = (const u8*)(&fnt_ptr[36]);

	if (fontFNT.nbits < 0xFFDB)
	{
		pad = 1;
		fontFNT.long_offset = 0;
		shift = 1;
	}
	else
	{
		pad = 3;
		fontFNT.long_offset = 1;
		shift = 2;
	}

	if (fontFNT.noffset != 0)
		fontFNT.offset = (u32*)((u64)(fontFNT.bits + fontFNT.nbits + pad) & ~pad);
	else
		fontFNT.offset = NULL;

	if (fontFNT.nwidth != 0)
		fontFNT.width = (u8*)((u64)fontFNT.offset + (fontFNT.noffset << shift));
	else
		fontFNT.width = NULL;
}

u8* EasyTTFont::init_ttf_table(u8* texture)
{
	int n;

	r_use = 0;
	for (n = 0; n < MAX_CHARS; n++) {
		memset(&ttf_font_datas[n], 0, sizeof(ttf_dyn));
		ttf_font_datas[n].text = texture;

		texture += fnt_ux * fnt_uy;
	}

	return texture;

}


inline u32 EasyTTFont::get_fnt_char(const char* string, u32* next_char, u16* ttf_width)
{
	//printf("CALLED\n");
	int l, n, m;
	//printf("Char: %c ", string[0]);
	
	u32 ttf_char;
	u8* ustring = (u8*)string;

	u32 dx, dy;
	u8* index;
	u8* index_tmp;
	u16 shift;
	u8 pt;
	u8* vptr_tmp;
	u8* vptr;

	u16 ucs;
	utf8_utf16(&ucs, string);
	*ttf_width = fnt_get_width_ucs2(ucs);
	//::printf("char: 0x%x - w: %d\n", ucs, *ttf_width);


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
	//::printf("l: %d\n", l);
	u8* vram = ttf_font_datas[l].text;
	rsxAddressToOffset(vram, &ttf_font_datas[l].offset);

	// building the character

	if (!(ttf_font_datas[l].flags & 1)) {

		index = fnt_get_bits(ucs);
		vptr_tmp = vram;

		for (dx = 0; dx < *ttf_width; dx++) /* x loop */
		{

			index_tmp = index;
			shift = 0;
			vptr = vptr_tmp;
			pt = *index;

			for (dy = 0; dy < fontFNT.height; dy++) /* y loop */
			{
				if (shift >= 8)
				{
					shift = 0;
					index_tmp += *ttf_width;
					pt = *index_tmp;
				}

				if (pt & 0x01)
					*vptr = 0xFF;
				else
					if ((pt & 0x2) > 0)
						*vptr = 0x7F;


				vptr += fontFNT.maxwidth;
				
				shift++;
				pt >>= 1;
			} /* y loop */
			
			vptr_tmp++;

			index++;
		} /* x loop */
		
		//if (f_face) FT_Set_Pixel_Sizes(face, fnt_ux, fnt_uy);
		if (ttf_char != 0) {
			ttf_font_datas[l].flags = 1;
			//ttf_font_datas[l].y_start = 0;
			//ttf_font_datas[l].height = fontFNT.height;
			ttf_font_datas[l].width = *ttf_width;
			ttf_font_datas[l].ttf = ttf_char;
		}

		/*printf("L: %d, Str: %s, delta: %d, offset: x0%x\n", l, string, *next_char, ttf_font_datas[l].offset);
		printf(" delta: %d - Ystart: %d - w: %d - h: %d\n",
			*next_char, ttf_font_datas[l].y_start, ttf_font_datas[l].width, ttf_font_datas[l].height);*/

	}

	// displaying the character
	ttf_font_datas[l].flags |= 2; // in use
	ttf_font_datas[l].r_use = r_use;
	/*printf("L: %d, Str: %s, delta: %d, offset: x0%x\n", l, string, *next_char, ttf_font_datas[l].offset);
	printf(" delta: %d - Ystart: %d - w: %d - h: %d\n",
		*next_char, ttf_font_datas[l].y_start, ttf_font_datas[l].width, ttf_font_datas[l].height);*/

	//*ttf_y_start = ttf_font_datas[l].y_start;
	
	//*ttf_height = ttf_font_datas[l].height;
	return ttf_font_datas[l].offset;
}


/* release all */

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
	//u32 y_start;
	//u16 gly_Height;
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
				ntex_off[idx] = get_fnt_char(pszText + offset + c, &deltaUTF, &gly_Width);
				
				//::printf("Char: %c - dy: %d \n", pszText[offset + c], dy);
				//pPositions[numVerts].x = calcPos(sXPos + sLeftSafe + (float)(GLYPH_WIDTH_REL * (TTF_UX - gly_Width) / 2), sXRes);
				pPositions[numVerts].x = calcPos(sXPos + sLeftSafe, sXRes);
				
				//pPositions[numVerts].y = calcPos((float)sYPos + sTopSafe + (TTF_UY - gly_Height) * GLYPH_HEIGHT_REL, sYRes) * -1.0f ;
				pPositions[numVerts].y = calcPos((float)sYPos + sTopSafe, sYRes) * -1.0f;
				
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
				pPositions[numVerts].y = calcPos(sYPos + sTopSafe, sYRes)*-1.0f ;
				
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
				pPositions[numVerts].y = calcPos((float)sYPos + sTopSafe + gly_h, sYRes)*-1.0f;
				
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
				pPositions[numVerts].y = calcPos((float)sYPos + sTopSafe + gly_h, sYRes)*-1.0f;
				
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
					sXPos += (int)((gly_Width * (float)gly_w / fnt_ux)+ 1.99f);
				
				//::printf("char: %c - gly_Width %d - dxPos %d - dY: %d - dyPos: %d\n", 
					//pszText[offset + c], gly_Width, (int)((gly_Width * (float)GLYPH_WIDTH_REL) + 1), gly_Height);

			}
			else if(pszText[offset + c] == '\n') {
				sXPos = 0;
				sYPos += (fontFNT.height * (float)(gly_h / fnt_uy) + (fnt_uy - fontFNT.height) * (float)(gly_h / fnt_uy) + 1);
			}
			else if(pszText[offset + c] == '\t') {
				sXPos += EASYTTFONT_TAB_SIZE*((int)((gly_Width * (float)gly_w / fnt_ux) + (float)((gly_w / fnt_ux) * (fnt_ux - gly_Width) / 2) + 1.99));
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
				sXPos += (int)((gly_Width * (float)(gly_w / fnt_ux)) + (float)((gly_w / fnt_ux) * (fnt_ux - gly_Width) / 2) + 1.99);
			}

			if(sXPos + (int)((gly_Width * (float)(gly_w / fnt_ux)) + (float)((gly_w / fnt_ux) * (fnt_ux - gly_Width) / 2) + 1.99) >= sXRes - (sLeftSafe + sRightSafe)) {
				sXPos = 0;
				sYPos += (fontFNT.height * (gly_h / fnt_uy) + (fnt_uy - fontFNT.height) * (gly_h / fnt_uy) + 1);
			}
			//::printf("CALL get_fnt_char - IDX: %d - offset: %d - c: %d \n", idx, offset, c);
			
			idx++;
			c += deltaUTF;
		}

		
		offset += EASYTTFONT_MAX_CHAR_COUNT;
		
		spRenderer->printPass(pPositions, pTexCoords, pColors, numVerts, ntex_off, fnt_ux, fnt_uy);
		
		
	}

	spRenderer->printEnd();
}

