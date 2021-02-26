#include "bitmap_font.h"
#include "fnt35.h"
#include <stdio.h>

#include "vpshader_bitmapfnt_vpo.h"
#include "fpshader_bitmapfnt_fpo.h"

/*
 * .fnt loadable font file format definition
 *
 * format               len         description								Start position
 * -------------------  ----------- --------------------------------------- ---------------------------------
 * char[4]  version     4           magic number and version bytes			[0]
 * u16      maxwidth    2           font max width in pixels				[4]
 * u16      height      2           font height in pixels					[6]
 * u16      ascent      2           font ascent (baseline) in pixels		[8]
 * char[2]   pad        2           depth of the font, 0=1bit and 1=4bit	[10]
 * s32      firstchar   4           first character code in font			[12]
 * s32      defaultchar 4           default character code in font			[16]
 * s32      size        4           # characters in font					[20]
 * s32      nbits       4           # bytes imagebits data in file			[24]
 * s32      noffset     4           # longs offset data in file				[28]
 * s32      nwidth      4           # bytes width data in file				[32]
 * u8*      bits        nbits       pointer to image bits					[36]
 * u8       pad         0～3        image padded to 16-bit boundary			[36 + nbits]
 * u32*		offset      noffset x4	offset variable data					[36 + nbits + pad]
 * u8*    width       nwidth      width variable data						[36 + nbits + pad + offset + pad]
 *
 */
void read_header_fnt(fnt_t* fnt_p)
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

void load_mem_fnt(const void* ptr, fnt_class* fntc)
{
	fntc->fnt.file_flag = 0;

	fntc->fnt.fnt_ptr = (void*)ptr;

	fntc->read_header(&fntc->fnt);
}

int load_file_fnt(const char* path, fnt_class* fntc)
{
	u8* buf;

	FILE* f = fopen(path, "rb");
	if (!f)
		return 1; // Err.
	fseek(f, 0, SEEK_END);
	s64 fsize = ftell(f);
	fseek(f, 0, SEEK_SET);  /* same as rewind(f); */

	buf = (unsigned char*)malloc(fsize + 1);
	if (!buf)
		return 1; // Err.
	fread(buf, 1, fsize, f);
	fclose(f);
	fntc->fnt.file_flag = 1;
	fntc->fnt.fnt_ptr = buf;

	fntc->read_header(&fntc->fnt);
	return 0;
}

void free_mem_fnt(fnt_class* fntc)
{

	if (fntc->fnt.file_flag == 1) {
		free(fntc->fnt.fnt_ptr);
		fntc->fnt.fnt_ptr = NULL;
	}
}

void printStart_fnt(fnt_class* fntc)
{
	rsxSetBlendFunc(fntc->mContext, GCM_SRC_ALPHA, GCM_ONE_MINUS_SRC_ALPHA, GCM_SRC_ALPHA, GCM_ONE_MINUS_SRC_ALPHA);
	rsxSetBlendEquation(fntc->mContext, GCM_FUNC_ADD, GCM_FUNC_ADD);
	rsxSetBlendEnable(fntc->mContext, GCM_TRUE);
	rsxSetLogicOpEnable(fntc->mContext, GCM_FALSE);

	rsxSetDepthTestEnable(fntc->mContext, GCM_FALSE);

	rsxLoadVertexProgram(fntc->mContext, fntc->mRSXVertexProgram, fntc->mVertexProgramUCode);
	rsxLoadFragmentProgramLocation(fntc->mContext, fntc->mRSXFragmentProgram, fntc->mFragmentProgramOffset, GCM_LOCATION_RSX);

}

u8* init_table_fnt(u8* texture, fnt_class* fntc)
{
	int n;

	fntc->r_use = 0;
	for (n = 0; n < MAX_CHARS_FNT; n++) {
		memset(&fntc->fnt_font_datas[n], 0, sizeof(fnt_dyn));
		fntc->fnt_font_datas[n].text = texture;

		texture += fntc->fnt_ux * fntc->fnt_uy;
	}

	return texture;

}

void printPass_fnt(Position* pPositions, TexCoord* pTexCoords, Color* pColors, s32 numVerts, u32* textmem_off, u32 tex_w, u32 tex_h, fnt_class* fntc)
{


	while (*fntc->mLabel != fntc->mLabelValue)
		usleep(10);
	fntc->mLabelValue++;

	memcpy(fntc->mPosition, pPositions, numVerts * sizeof(f32) * 3);
	memcpy(fntc->mTexCoord, pTexCoords, numVerts * sizeof(f32) * 2);
	memcpy(fntc->mColor, pColors, numVerts * sizeof(f32) * 4);
	for (s32 i = 0; i < numVerts / 4; i++) {

		rsxBindVertexArrayAttrib(fntc->mContext, fntc->mPosIndex->index, 0, fntc->mPositionOffset + (i * (4 * sizeof(f32) * 3)), sizeof(f32) * 3, 3, GCM_VERTEX_DATA_TYPE_F32, GCM_LOCATION_RSX);
		rsxBindVertexArrayAttrib(fntc->mContext, fntc->mTexIndex->index, 0, fntc->mTexCoordOffset + (i * (4 * sizeof(f32) * 2)), sizeof(f32) * 2, 2, GCM_VERTEX_DATA_TYPE_F32, GCM_LOCATION_RSX);
		rsxBindVertexArrayAttrib(fntc->mContext, fntc->mColIndex->index, 0, fntc->mColorOffset + (i * (4 * sizeof(f32) * 4)), sizeof(f32) * 4, 4, GCM_VERTEX_DATA_TYPE_F32, GCM_LOCATION_RSX);

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
		rsxLoadTexture(fntc->mContext, fntc->mTexUnit->index, &tex);

		rsxTextureControl(fntc->mContext, fntc->mTexUnit->index, GCM_TRUE, 0 << 8, 12 << 8, GCM_TEXTURE_MAX_ANISO_1);
		rsxTextureFilter(fntc->mContext, fntc->mTexUnit->index, 0, GCM_TEXTURE_LINEAR, GCM_TEXTURE_LINEAR, GCM_TEXTURE_CONVOLUTION_QUINCUNX);
		rsxTextureWrapMode(fntc->mContext, fntc->mTexUnit->index, GCM_TEXTURE_CLAMP_TO_EDGE, GCM_TEXTURE_CLAMP_TO_EDGE, GCM_TEXTURE_CLAMP_TO_EDGE, 0, GCM_TEXTURE_ZFUNC_LESS, 0);

		rsxDrawVertexArray(fntc->mContext, BITMAPFNT_PRIMITIVE, 0, 4);

	}
	rsxInvalidateVertexCache(fntc->mContext);
	rsxSetWriteBackendLabel(fntc->mContext, fntc->sLabelId, fntc->mLabelValue);

	rsxFlushBuffer(fntc->mContext);
}

void printEnd_fnt(fnt_class* fntc)
{
	rsxSetDepthTestEnable(fntc->mContext, GCM_TRUE);
	rsxSetBlendEnable(fntc->mContext, GCM_FALSE);
}

void print_fnt(void* fntc, const char* pszText)
{
	((fnt_class*)fntc)->printn(pszText, strlen(pszText));
}

u32 get_char_fnt(const char* string, u32 aa_level, u32* next_char, u16* fnt_width, fnt_class* fntc)
{
	int l, n, m;
	
	u32 fnt_char = 0;
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
	*fnt_width = fntc->get_width_ucs2(ucs, fntc);
	
	if (*ustring & 128) {
		m = 1;

		if ((*ustring & 0xf8) == 0xf0) { // 4 bytes
			fnt_char = (u32)(*(ustring++) & 3);
			m = 3;
			*next_char = 3;
		}
		else if ((*ustring & 0xE0) == 0xE0) { // 3 bytes
			fnt_char = (u32)(*(ustring++) & 0xf);
			m = 2;
			*next_char = 2;
		}
		else if ((*ustring & 0xE0) == 0xC0) { // 2 bytes
			fnt_char = (u32)(*(ustring++) & 0x1f);
			m = 1;
			*next_char = 1;
		}
		else { ustring++; *next_char = 1; } // error!

		for (n = 0; n < m; n++) {
			if (!*ustring) break; // error!
			if ((*ustring & 0xc0) != 0x80) break; // error!
			fnt_char = (fnt_char << 6) | ((u32)(*(ustring++) & 63));
		}

		if ((n != m) && !*ustring) return 0;

	}
	else {
		fnt_char = (u32) * (ustring++);
		*next_char = 0;
	}

	// search fnt_char
	if (fnt_char < 128) n = fnt_char;
	else {
		m = 0;
		int rel = 0;

		for (n = 128; n < MAX_CHARS_FNT; n++) {
			if (!(fntc->fnt_font_datas[n].flags & 1)) m = n;

			if ((fntc->fnt_font_datas[n].flags & 3) == 1) {
				int trel = fntc->r_use - fntc->fnt_font_datas[n].r_use;
				if (m == 0) { m = n; rel = trel; }
				else if (rel > trel) { m = n; rel = trel; }

			}
			if (fntc->fnt_font_datas[n].ttf == fnt_char) break;
		}

		if (m == 0) m = 128;

	}

	if (n >= MAX_CHARS_FNT) { fntc->fnt_font_datas[m].flags = 0; l = m; }
	else { l = n; }
	
	u8* vram = fntc->fnt_font_datas[l].text;
	rsxAddressToOffset(vram, &fntc->fnt_font_datas[l].offset);

	// building the character

	if (!(fntc->fnt_font_datas[l].flags & 1)) {

		index = fntc->get_bits(ucs, fntc);
		vptr_tmp = vram;

		for (dx = 0; dx < *fnt_width; dx++) /* x loop */
		{

			index_tmp = index;
			shift = 0;
			vptr = vptr_tmp;
			pt = *index;

			for (dy = 0; dy < fntc->fnt.height; dy++) /* y loop */
			{
				if (shift >= 8)
				{
					shift = 0;
					index_tmp += *fnt_width;
					pt = *index_tmp;
				}

				if (pt & 0x01)
					*vptr = 0xFF;
				else
					if (pt & 0x2 && aa_level > 0)
						*vptr = 0x7F;
					else
						if (pt & 0x4 && aa_level > 1)
							*vptr = 0x3F;
				/*else
					if (pt & 0x8)
						*vptr = 0x1F;*/


				vptr += fntc->fnt.maxwidth;

				shift++;
				pt >>= 1;
			} /* y loop */

			vptr_tmp++;

			index++;
		} /* x loop */

		if (fnt_char != 0) {
			fntc->fnt_font_datas[l].flags = 1;
			fntc->fnt_font_datas[l].width = *fnt_width;
			fntc->fnt_font_datas[l].ttf = fnt_char;
		}


	}

	// displaying the character
	fntc->fnt_font_datas[l].flags |= 2; // in use
	fntc->fnt_font_datas[l].r_use = fntc->r_use;
	return fntc->fnt_font_datas[l].offset;
}

u8* get_bits_fnt(u32 ucs2, fnt_class* fntc)
{
	u8* bits;
	u8* tmp;

	if ((ucs2 < fntc->fnt.firstchar) || (ucs2 >= fntc->fnt.firstchar + fntc->fnt.size))
		ucs2 = fntc->fnt.defaultchar;

	ucs2 -= fntc->fnt.firstchar;

	if (fntc->fnt.long_offset == 0)
	{
		tmp = (u8*)fntc->fnt.offset + ucs2 * 2;
		bits = (u8*)fntc->fnt.bits + (fntc->fnt.offset ?
			(tmp[1] << 8) + tmp[0] :
			(u8)(((fntc->fnt.height + 7) / 8) * fntc->fnt.maxwidth * ucs2));
	}
	else
	{
		tmp = (u8*)fntc->fnt.offset + ucs2 * 4;
		bits = (u8*)fntc->fnt.bits + (fntc->fnt.offset ?
			(tmp[3] << 24) + (tmp[2] << 16) + (tmp[1] << 8) + tmp[0] :
			(u8)(((fntc->fnt.height + 7) / 8) * fntc->fnt.maxwidth * ucs2));
	}

	return bits;
}

void printn_fnt(void* fntc_void, const char* pszText, s32 count)
{
	fnt_class* fntc = (fnt_class*)fntc_void;
	s32 c, offset, pass, numPasses;
	u32 deltaUTF;
	u16 gly_Width;
	u32 idx = 0;


	offset = 0;
	numPasses = (count / BITMAPFNT_MAX_CHAR_COUNT) + 1;

	fntc->printStart(fntc);

	for (pass = 0; pass < numPasses; pass++) {
		s32 numVerts = 0;

		TexCoord* pTexCoords = fntc->spTexCoords[pass & 1];
		Position* pPositions = fntc->spPositions[pass & 1];
		Color* pColors = fntc->spColors[pass & 1];

		for (c = 0; (offset + c) < count && c < BITMAPFNT_MAX_CHAR_COUNT; c++) {
			if (pszText[offset + c] == 0)
				break;
			if (fntc->isPrintable(pszText[offset + c])) {
				// top left
				fntc->ntex_off[idx] = fntc->get_char(pszText + offset + c, fntc->aa_level, &deltaUTF, &gly_Width, fntc);

				pPositions[numVerts].x = calcPos(fntc->sXPos + fntc->sLeftSafe, fntc->sXRes);
				pPositions[numVerts].y = calcPos((float)fntc->sYPos + fntc->sTopSafe, fntc->sYRes) * -1.0f;
				pPositions[numVerts].z = 0.0f;

				pTexCoords[numVerts].s = 0.0f;
				pTexCoords[numVerts].t = 0.0f;

				pColors[numVerts].r = fntc->sR;
				pColors[numVerts].g = fntc->sG;
				pColors[numVerts].b = fntc->sB;
				pColors[numVerts].a = fntc->sA;
				numVerts++;

				// top right
				pPositions[numVerts].x = calcPos(fntc->sXPos + fntc->sLeftSafe + fntc->gly_w, fntc->sXRes);
				pPositions[numVerts].y = calcPos(fntc->sYPos + fntc->sTopSafe, fntc->sYRes) * -1.0f;
				pPositions[numVerts].z = 0.0f;

				pTexCoords[numVerts].s = 1.0f;
				pTexCoords[numVerts].t = 0.0f;

				pColors[numVerts].r = fntc->sR;
				pColors[numVerts].g = fntc->sG;
				pColors[numVerts].b = fntc->sB;
				pColors[numVerts].a = fntc->sA;
				numVerts++;

				// bottom right
				pPositions[numVerts].x = calcPos(fntc->sXPos + fntc->sLeftSafe + fntc->gly_w, fntc->sXRes);
				pPositions[numVerts].y = calcPos((float)fntc->sYPos + fntc->sTopSafe + fntc->gly_h, fntc->sYRes) * -1.0f;
				pPositions[numVerts].z = 0.0f;

				pTexCoords[numVerts].s = 1.0f;
				pTexCoords[numVerts].t = 1.0f;

				pColors[numVerts].r = fntc->sR;
				pColors[numVerts].g = fntc->sG;
				pColors[numVerts].b = fntc->sB;
				pColors[numVerts].a = fntc->sA;
				numVerts++;

				// bottom left
				pPositions[numVerts].x = calcPos(fntc->sXPos + fntc->sLeftSafe, fntc->sXRes);
				pPositions[numVerts].y = calcPos((float)fntc->sYPos + fntc->sTopSafe + fntc->gly_h, fntc->sYRes) * -1.0f;
				pPositions[numVerts].z = 0.0f;

				pTexCoords[numVerts].s = 0.0f; 
				pTexCoords[numVerts].t = 1.0f; 

				pColors[numVerts].r = fntc->sR;
				pColors[numVerts].g = fntc->sG;
				pColors[numVerts].b = fntc->sB;
				pColors[numVerts].a = fntc->sA;
				numVerts++;

				if (pszText[offset + c] == ' ')
					fntc->sXPos += fntc->gly_w / 2;
				else
					fntc->sXPos += (int)((gly_Width * (float)fntc->gly_w / fntc->fnt_ux) + 1.99f);

			}
			else if (pszText[offset + c] == '\n') {
				fntc->sXPos = 0;
				fntc->sYPos += (fntc->fnt.height * (float)(fntc->gly_h / fntc->fnt_uy) + (fntc->fnt_uy - fntc->fnt.height) * (float)(fntc->gly_h / fntc->fnt_uy) + 1);
			}
			else if (pszText[offset + c] == '\t') {
				fntc->sXPos += BITMAPFNT_TAB_SIZE * ((int)((gly_Width * (float)fntc->gly_w / fntc->fnt_ux) + (float)((fntc->gly_w / fntc->fnt_ux) * (fntc->fnt_ux - gly_Width) / 2) + 1.99));
			}
			else if ((s8)pszText[offset + c] < 0) {
				s32 iCode = (u8)pszText[offset + c] - 128;
				if (iCode >= 0 && iCode < 8) {
					fntc->sR = fntc->sDefaultColors[iCode].r;
					fntc->sG = fntc->sDefaultColors[iCode].g;
					fntc->sB = fntc->sDefaultColors[iCode].b;
				}
			}
			else {
			fntc->sXPos += (int)((gly_Width * (float)(fntc->gly_w / fntc->fnt_ux)) + (float)((fntc->gly_w / fntc->fnt_ux) * (fntc->fnt_ux - gly_Width) / 2) + 1.99);
			}

			if (fntc->sXPos + (int)((gly_Width * (float)(fntc->gly_w / fntc->fnt_ux)) + (float)((fntc->gly_w / fntc->fnt_ux) * (fntc->fnt_ux - gly_Width) / 2) + 1.99) >= fntc->sXRes - (fntc->sLeftSafe + fntc->sRightSafe)) {
				fntc->sXPos = 0;
				fntc->sYPos += (fntc->fnt.height * (fntc->gly_h / fntc->fnt_uy) + (fntc->fnt_uy - fntc->fnt.height) * (fntc->gly_h / fntc->fnt_uy) + 1);
			}

			idx++;
			c += deltaUTF;
		}


		offset += BITMAPFNT_MAX_CHAR_COUNT;

		fntc->printPass(pPositions, pTexCoords, pColors, numVerts, fntc->ntex_off, fntc->fnt_ux, fntc->fnt_uy, fntc);


	}

	fntc->printEnd(fntc);
}



void shutdown_fnt(fnt_class* fntc)
{
	delete[] fntc->spPositions[0];
	delete[] fntc->spPositions[1];
	delete[] fntc->spTexCoords[0];
	delete[] fntc->spTexCoords[1];
	delete[] fntc->spColors[0];
	delete[] fntc->spColors[1];
	rsxFree(fntc->texture_mem);
	rsxFree(fntc->mPosition);
	rsxFree(fntc->mTexCoord);
	rsxFree(fntc->mColor);
	free(fntc->ntex_off);
	fntc->free_mem(fntc);
}

int init_fnt(gcmContextData* context, u32 display_width, u32 display_height, u32 aalevel, fnt_class* fntc)
{
	return init_fnt(context, display_width, display_height, NULL, aalevel, fntc);
}

int init_fnt(gcmContextData* context, u32 display_width, u32 display_height, const char * path, u32 aalevel, fnt_class* fntc)
{
	if (!context)
		return 1;  //error
	
	fntc->self = fntc;
	fntc->sLabelId = 254;
	fntc->sXRes = display_width;
	fntc->sYRes = display_height;
	fntc->aa_level = aalevel;
	fntc->read_header = &read_header_fnt;
	fntc->load_mem = &load_mem_fnt;
	fntc->load_file = &load_file_fnt;
	fntc->free_mem = &free_mem_fnt;
	fntc->printStart = &printStart_fnt;
	fntc->init_table = &init_table_fnt;
	fntc->printPass = &printPass_fnt;
	fntc->printEnd = &printEnd_fnt;
	fntc->get_char = &get_char_fnt;
	fntc->get_width = &get_width_fnt;
	fntc->get_width_ucs2 = &get_width_ucs2_fnt;
	fntc->get_bits = &get_bits_fnt;
	fntc->vsprintf = &vsprintf;

	fntc->mContext = context;
	fntc->gly_w = BITMAPFNT_GLYPH_WIDTH;
	fntc->gly_h = BITMAPFNT_GLYPH_HEIGHT;
	fntc->r_use = 0;
	
	fntc->sDefaultColors[0] = { 0.f, 0.f, 0.f, 1.f };	// black
	fntc->sDefaultColors[1] = { 0.f, 0.f, 1.f, 1.f };	// blue
	fntc->sDefaultColors[2] = { 1.f, 0.f, 0.f, 1.f };	// red
	fntc->sDefaultColors[3] = { 1.f, 0.f, 1.f, 1.f };	// magenta
	fntc->sDefaultColors[4] = { 0.f, 1.f, 0.f, 1.f };	// green
	fntc->sDefaultColors[5] = { 0.f, 1.f, 1.f, 1.f };	// cyan
	fntc->sDefaultColors[6] = { 1.f, 1.f, 0.f, 1.f };	// yellow
	fntc->sDefaultColors[7] = { 1.f, 1.f, 1.f, 1.f };	// white
	

	if (path)
	{ 
		if (fntc->load_file(path, fntc))
			return 1;	//Error
	}
	else
		fntc->load_mem(fnt35, fntc);
	
	fntc->fnt_ux = fntc->fnt.maxwidth;
	fntc->fnt_uy = fntc->fnt.height;

	fntc->mLabel = (vu32*)gcmGetLabelAddress(fntc->sLabelId);
	*fntc->mLabel = fntc->mLabelValue;
	
	fntc->texture_mem = (u8*)rsxMemalign(128, RSX_FNT_MEM * 1024 * 1024); // alloc 32MB of space for font textures
	if (!fntc->texture_mem) return 1; // fail!

	fntc->next_mem = (u8*)fntc->init_table((u8*)fntc->texture_mem, fntc);
	
	fntc->initShader();
	
	fntc->mPosIndex = rsxVertexProgramGetAttrib(fntc->mRSXVertexProgram, "position");
	fntc->mTexIndex = rsxVertexProgramGetAttrib(fntc->mRSXVertexProgram, "texcoord");
	fntc->mColIndex = rsxVertexProgramGetAttrib(fntc->mRSXVertexProgram, "color");

	fntc->mTexUnit = rsxFragmentProgramGetAttrib(fntc->mRSXFragmentProgram, "texture");
	
	fntc->mPosition = (u8*)rsxMemalign(128, BITMAPFNT_MAX_CHAR_COUNT * NUM_VERTS_PER_GLYPH * sizeof(f32) * 3);
	fntc->mTexCoord = (u8*)rsxMemalign(128, BITMAPFNT_MAX_CHAR_COUNT * NUM_VERTS_PER_GLYPH * sizeof(f32) * 2);
	fntc->mColor = (u8*)rsxMemalign(128, BITMAPFNT_MAX_CHAR_COUNT * NUM_VERTS_PER_GLYPH * sizeof(f32) * 4);
	
	rsxAddressToOffset(fntc->mPosition, &fntc->mPositionOffset);
	rsxAddressToOffset(fntc->mTexCoord, &fntc->mTexCoordOffset);
	rsxAddressToOffset(fntc->mColor, &fntc->mColorOffset);
	
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

u32 get_width_fnt(const char* str, int mx, fnt_class* fntc)
{
	u16 i;
	u16 len;
	u16 width = 0;
	u16 ucs[MAX_STR];

	utf8_to_utf16(ucs, str);
	len = utf16len(ucs);

	for (i = 0; i < len; i++)
		width += get_width_ucs2_fnt(ucs[i], fntc);

	return width * mx;
}

u32 get_width_ucs2_fnt(u32 ucs2, fnt_class* fntc)
{
	u16 width = 0;

	if ((ucs2 < fntc->fnt.firstchar) || (ucs2 >= fntc->fnt.firstchar + fntc->fnt.size))
		ucs2 = fntc->fnt.defaultchar;

	if (fntc->fnt.nwidth != 0)
		width = fntc->fnt.width[ucs2 - fntc->fnt.firstchar];
	else
		width = fntc->fnt.maxwidth;

	return width;
}

void initShader_fnt(void* fntc_void)
{
	fnt_class* fntc = ((fnt_class*) fntc_void);
	fntc->mRSXVertexProgram = (rsxVertexProgram*)vpshader_bitmapfnt_vpo;
	fntc->mRSXFragmentProgram = (rsxFragmentProgram*)fpshader_bitmapfnt_fpo;

	void* ucode;
	u32 ucodeSize;

	rsxFragmentProgramGetUCode(fntc->mRSXFragmentProgram, &ucode, &ucodeSize);

	fntc->mFragmentProgramUCode = rsxMemalign(64, ucodeSize);
	rsxAddressToOffset(fntc->mFragmentProgramUCode, &fntc->mFragmentProgramOffset);

	memcpy(fntc->mFragmentProgramUCode, ucode, ucodeSize);

	rsxVertexProgramGetUCode(fntc->mRSXVertexProgram, &fntc->mVertexProgramUCode, &ucodeSize);
}

char* utf8_utf16(u16* utf16, const char* utf8)
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

void utf8_to_utf16(u16* utf16, const char* utf8)
{
	while (*utf8 != '\0')
	{
		utf8 = utf8_utf16(utf16++, utf8);
	}
	*utf16 = '\0';
}

