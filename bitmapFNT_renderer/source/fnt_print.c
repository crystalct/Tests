// fnt_print.c

#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include "fnt_print.h"
//#include "graphics.h"




#if 0
static void *fnt_malloc(int size);
static int fnt_mfree(void *ptr);
#endif



//int internal_load_font(fnt_t *font, const char *path) { //, size_t buf_size) {
//	unsigned char *buf;
//	 //483.549 +1
//
//	FILE *f = fopen(path, "rb");
//	if (!f)
//		return 1; // Err.
//	fseek(f, 0, SEEK_END);
//	long fsize = ftell(f);
//	fseek(f, 0, SEEK_SET);  /* same as rewind(f); */
//
//	buf = (unsigned char *) malloc(fsize + 1);
//	if (!buf)
//		return 1; // Err.
//	fread(buf, 1, fsize, f);
//	fclose(f);
//	font->file_flag = 1;
//	font->fnt_ptr = buf;
//    printf("fnt_read_header\n");
//	fnt_read_header(font->fnt_ptr, font);
//	return 0;
//}


#if 0
/*---------------------------------------------------------------------------
  フォントをファイルからロード
    *path: フォントファイルのパス
    *font: font_tへのポインタ
    return: ファイルサイズ
            -1 指定ファイル無し / -2 メモリ確保失敗 / -3 読込失敗
---------------------------------------------------------------------------*/
int fnt_load_file(const void* path, fnt_t *font)
{
  SceIoStat stat;
  SceUID fp;
  int size;

  if(sceIoGetstat(path, &stat) < 0)
    return -1;

  font->fnt_ptr = fnt_malloc(stat.st_size);
  if(font->fnt_ptr <0)
    return -2;

  font->file_flag = 1;

  fp = sceIoOpen(path, PSP_O_RDONLY, 0777);
  if(fp < 0)
    return -3;

  size = sceIoRead(fp, font->fnt_ptr, stat.st_size);
  sceIoClose(fp);

  if(size != stat.st_size)
    return -3;

  fnt_read_header(font->fnt_ptr, font);
  return stat.st_size;

}
#endif

/*---------------------------------------------------------------------------
  メモリ上のフォントをロード
    *ptr: フォントデータへのポインタ
    *font: fnt_tへのポインタ
---------------------------------------------------------------------------*/






#if 0
/*---------------------------------------------------------------------------
  メモリ確保
    size: 利用メモリサイズ
    return: 確保したメモリへのポインタ
            エラーの場合はNULLを返す
---------------------------------------------------------------------------*/
static void *fnt_malloc(int size)
{
  int *p;
  int h_block;

  if(size == 0) return NULL;

  h_block = sceKernelAllocPartitionMemory(2, "block", 0, size + sizeof(h_block), NULL);

  if(h_block < 0) return NULL;

  p = (int *)sceKernelGetBlockHeadAddr(h_block);
  *p = h_block;

  return (void *)(p + 1);
}
#endif

#if 0
/*---------------------------------------------------------------------------
  メモリ解放
    *ptr: 確保したメモリへのポインタ
    return: エラーの場合は負の値を返す
---------------------------------------------------------------------------*/
static int fnt_mfree(void *ptr)
{
  return sceKernelFreePartitionMemory((SceUID)*((int *)ptr - 1));
}
#endif

/*---------------------------------------------------------------------------
  フォント ヘッダ読込み
    *fnt_ptr: フォントデーターへのポインタ
    *font: fnt_tへのポインタ
---------------------------------------------------------------------------*/


/*---------------------------------------------------------------------------
  文字列の幅を得る
    *font: fnt_tへのポインタ
    *str: 文字列へのポインタ(UTF-8N)
    return: 文字列の幅
---------------------------------------------------------------------------*/
//uint32_t fnt_get_width(const fnt_t* font, const char *str, int mx)
//{
//  uint16_t i;
//  uint16_t len;
//  uint16_t width = 0;
//  uint16_t ucs[MAX_STR];
//
//  utf8_to_utf16(ucs, str);
//  len = utf16len(ucs);
//
//  for (i = 0; i < len; i++)
//    width += fnt_get_width_ucs2(font ,ucs[i]);
//
//  return width * mx;
//}

/*---------------------------------------------------------------------------
  文字の幅を得る
    *font: fnt_tへのポインタ
    ucs2: 文字(UCS2)
    return: 文字の幅
---------------------------------------------------------------------------*/
//uint32_t fnt_get_width_ucs2(const fnt_t* font, uint32_t ucs2)
//{
//  uint16_t width = 0;
//
//  if((ucs2 < font->firstchar) || (ucs2 >= font->firstchar + font->size))
//    ucs2 = font->defaultchar;
//
//  if(font->nwidth != 0)
//    width = font->width[ucs2 - font->firstchar];
//  else
//    width = font->maxwidth;
//
//  return width;
//}

/*---------------------------------------------------------------------------
  グリフデータを得る
    *font: fnt_tへのポインタ
    ucs2: 文字(UCS2)
    return: グリフデータへのポインタ
---------------------------------------------------------------------------*/


#if 0
/*---------------------------------------------------------------------------
  文字列表示(表示VRAMへの描画)
    *font: fnt_tへのポインタ
    x: 横方向位置
    y: 縦方向位置
    *str: 文字列(UTF8-N)へのポインタ('\n'は改行を行う)
    color: 文字色
    back: 背景色
    fill: 書込フラグ FNT_FONT_FILL(0x01) 文字部, FNT_BACK_FILL(0x10) 背景部
    rate: 混合比(-100～100)
    mx: 横方向倍率
    my: 縦方向倍率
---------------------------------------------------------------------------*/
int fnt_print_xy(const fnt_t* font, int x, int y, void *str, int color, int back, char fill, int rate, int mx, int my)
{
  void *vram;
  int  bufferwidth;
  int  pixelformat;
  int  pwidth;
  int  pheight;
  int  unk;

  sceDisplayGetMode(&unk, &pwidth, &pheight);
  sceDisplayGetFrameBuf(&vram, &bufferwidth, &pixelformat, unk);

  if(vram == NULL)
    vram = (void*) (0x40000000 | (int) sceGeEdramGetAddr());

  return fnt_print_vram(font, vram, bufferwidth, pixelformat, x, y, str, color, back, fill, rate, mx, my);
}
#endif

/*---------------------------------------------------------------------------
  文字列表示(指定したRAM/VRAMへの描画)
    *font: fnt_tへのポインタ
    *vram: VRAMアドレス
    bufferwidth: VRAM横幅
    pixelformat: ピクセルフォーマット (0=16bit, 1=15bit, 2=12bit, 3=32bit)
    x: 横方向位置
    y: 縦方向位置
    *str: 文字列(UTF8-N)へのポインタ('\n'は改行を行う)
    color: 文字色
    back: 背景色
    fill: 書込フラグ FNT_FONT_FILL(0x01) 文字部, FNT_BACK_FILL(0x10) 背景部
    rate: 混合比(-100～100) ※負の場合は減色
    mx: 横方向倍率
    my: 縦方向倍率
    return: エラー時は-1
---------------------------------------------------------------------------*/
//uint32_t fnt_print_vram(const fnt_t* font, uint8_t *vram, const void *str)
//{
//  uint16_t ucs;
//  uint32_t dx, dy;
//  uint8_t* index;
//  uint8_t* index_tmp;
//  uint16_t shift;
//  uint8_t pt;
//  uint8_t *vptr_tmp;
//  uint8_t *vptr;
//  
//  uint32_t width;
//
//  utf8_utf16(&ucs, str);
//  
//
//      width = fnt_get_width_ucs2(font, ucs);
//
//      index = fnt_get_bits(font, ucs);
//      vptr_tmp = vram;
//
//      for (dx = 0; dx < width; dx++) /* x loop */
//      {
//        
//          index_tmp = index;
//          shift = 0;
//          vptr = vptr_tmp;
//          pt = *index;
//
//          for(dy = 0; dy < font->height; dy++) /* y loop */
//          {
//            if(shift >= 8)
//            {
//              shift = 0;
//              index_tmp += width;
//              pt = *index_tmp;
//            }
//
//            if(pt & 0x01)
//                *vptr = 0xFF;
//            else 
//                if ((pt & 0x2) > 0) 
//                    *vptr = 0x7F;
//            
//            
//              vptr += font->maxwidth;
//              printf("0x%.2x ", pt);
//            shift++;
//            pt >>= 1;
//          } /* y loop */
//          printf("\n");
//          vptr_tmp++;
//        
//        index++;
//      } /* x loop */
//      printf("\nEND\n");
//
//return 0;
//}

/*---------------------------------------------------------------------------
  UFT-8Nの一文字をUTF-16に変換する
    *utf16: 変換後の文字(UTF-16)へのポインタ
    *utf8: UFT-8N文字へのポインタ
    return: UTF-8Nの次の文字へのポインタ
---------------------------------------------------------------------------*/
//char* utf8_utf16(uint16_t *utf16, const char *utf8)
//{
//  uint8_t c = *utf8++;
//  uint32_t code;
//  int32_t tail = 0;
//
//  if((c <= 0x7f) || (c >= 0xc2))
//  {
//    /* Start of new character. */
//    if(c < 0x80)
//    {
//      /* U-00000000 - U-0000007F, 1 byte */
//      code = c;
//    }
//    else if(c < 0xe0)   /* U-00000080 - U-000007FF, 2 bytes */
//    {
//      tail = 1;
//      code = c & 0x1f;
//    }
//    else if(c < 0xf0)   /* U-00000800 - U-0000FFFF, 3 bytes */
//    {
//      tail = 2;
//      code = c & 0x0f;
//    }
//    else if(c < 0xf5)   /* U-00010000 - U-001FFFFF, 4 bytes */
//    {
//      tail = 3;
//      code = c & 0x07;
//    }
//    else                /* Invalid size. */
//    {
//      code = 0xfffd;
//    }
//
//    while(tail-- && ((c = *utf8++) != 0))
//    {
//      if((c & 0xc0) == 0x80)
//      {
//        /* Valid continuation character. */
//        code = (code << 6) | (c & 0x3f);
//
//      }
//      else
//      {
//        /* Invalid continuation char */
//        code = 0xfffd;
//        utf8--;
//        break;
//      }
//    }
//  }
//  else
//  {
//    /* Invalid UTF-8 char */
//    code = 0xfffd;
//  }
//  /* currently we don't support chars above U-FFFF */
//  *utf16 = (code < 0x10000) ? code : 0xfffd;
//  return (char*)utf8;
//}

/*---------------------------------------------------------------------------
  UFT-8Nの文字列をUTF-16に変換する
    *utf16: 変換後の文字列(UTF-16)へのポインタ
    *utf8: UFT-8N文字列へのポインタ
---------------------------------------------------------------------------*/
//void utf8_to_utf16(uint16_t *utf16, const char *utf8)
//{
//  while(*utf8 !='\0')
//  {
//    utf8 = utf8_utf16(utf16++, utf8);
//  }
//  *utf16 = '\0';
//}

/*---------------------------------------------------------------------------
  UTF-16の文字列の長さを得る
    *utf16: 文字列(UTF-16)へのポインタ
    return: 文字数
---------------------------------------------------------------------------*/


