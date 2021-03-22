// =======================================================================
// DISPLAY SECTIONS (FRAME)

#include <unistd.h>
#include "capp.h"
//#include "fnt_print.h"
#include <rsx/rsx.h>
#include <ppu-lv2.h>
#include <lv2/memory.h>
#include "rsxutil.h"
#include <bdffnt.h>

extern smeminfo meminfo;

#define COLOR_WHITE		0xFFFFFF
#define COLOR_BLACK		0x000000
#define COLOR_BLUE		0x00FFFF
#define COLOR_GREEN		0x00FF00
#define COLOR_RED		0x0000FF
#define COLOR_YELLOW	0xFFFF00
#define COLOR_CYAN		0X00FFFF
#define COLOR_GREY		0x808080
#define COLOR_ORANGE	0xFF4500
#define NSHADOWCOLOR	0xFF050505
#define NSHADOWXYPOS2   0.00150f

//COLORS R,G,B,A
#define SHADOWCOLOR		0.1f, 0.1f, 0.1f, 1.0f
#define YELLOWCOLOR		1.0f, 1.0f, 0.0f, 1.0f
#define WHITECOLOR		1.0f, 1.0f, 1.0f, 1.0f
#define LIGHTBLUECOLOR  0.0f, 0.8f, 1.0f, 1.8f
#define REDCOLOR		1.0f, 0.0f, 0.0f, 1.0f
#define GRAYCOLOR		0.87f, 0.87f, 0.87f , 1.0f
#define GRAYCOLOR2		0.7f, 0.7f, 0.7f , 1.0f

#define NGAMELISTMAX    27
#define DRVMAP          ((hashmap_map *) app.drvMap)
#define GAMESMAP        ((hashmap_map *) app.gamesMap)
//#define PNG_SRC         (app.textures[TEX_GAME_LIST]->png)
#define FBADRV          ((FBA_DRV *)DRVMAP->data[hashmap_position].data)
#define FBAGAMES        ((FBA_GAMES *)GAMESMAP->data[hashmap_position].data)


//extern fnt_t fontM,fontL;
extern u32* texture_buffer;
extern u32 texture_offset;
extern u32 myLabelValue;
extern vu32* mLabel;


//static float nShadowXYpos		= 0.00250f;
//static float nShadowXYpos2		= 0.00200f;
//static uint32_t nShadowColor	= COLOR_BLACK;
//static uint32_t nTextColor		= COLOR_WHITE;
//static uint32_t nSelectColor	= COLOR_YELLOW;
//static uint32_t nMissingColor	= COLOR_RED;
//static float nBigSize		= 1.0500f;
//static float nSmallSize		= 0.5500f;
//static float nSmallSize2	= 0.6500f;
//static float nSmallSizeSel	= 0.6000f;
//static uint32_t nColor;
static uint32_t game_ord_id;
static float xPos;
static float yPos;
static float yPosDiff;
static uint32_t fontSize;
static int error_x, error_y;


static char txt[288];
extern char ipaddress[32];
extern fnt_class alpha;

void c_fbaRL::DisplayFrame()
{
	//printf("nSection: %d\n", nSection);
	if(nSection == SECTION_FILEBROWSER)	{
		FileBrowser_Frame();
	}
	if(nSection == SECTION_OPTIONS && !bProcessingGames) {
		Options_Frame();
	}
	if(nSection == SECTION_GAMELIST && !bProcessingGames) {
		GameList_Frame();
	}
	if(nSection == SECTION_ZIPINFO)	{
		ZipInfo_Frame();
	}
	if(nSection == SECTION_ROMINFO)	{
		RomInfo_Frame();
	}
	if(nSection == SECTION_MAIN) {
		MainMenu_Frame();
	}
}

void c_fbaRL::MainMenu_Frame()
{
	xPos		= 0.2600f;
	yPos		= 0.5885f;
	yPosDiff	= 0.0400f;
	
	switch (app.state.displayMode.resolution)
	{
		case VIDEO_RESOLUTION_1080:
		case VIDEO_RESOLUTION_1600x1080:
			SetCurrentFont_fnt(&alpha, 1);
			setDimension_fnt(&alpha, alpha.fnt[1].maxwidth, alpha.fnt[1].height);
			break;
		case VIDEO_RESOLUTION_720:
			SetCurrentFont_fnt(&alpha, 2);
			setDimension_fnt(&alpha, alpha.fnt[2].maxwidth, alpha.fnt[2].height);
			break;
	}

	if (nFrameStep == 0) 
	{
			//BitmapFNT::setColor(SHADOWCOLOR);
			setColor_fnt(&alpha, SHADOWCOLOR);
			//BitmapFNT::setPosition((int)(0.229 * display_width) + 1, (int)(0.135 * display_height) + 1);
			setPosition_fnt(&alpha, (int)(0.225 * display_width) + 1, (int)(0.135 * display_height) + 1);
	}
	else
	{
			//BitmapFNT::setColor(WHITECOLOR);
			setColor_fnt(&alpha, WHITECOLOR);
			//BitmapFNT::setPosition((int)(0.229 * display_width), (int)(0.135 * display_height));
			setPosition_fnt(&alpha, (int)(0.225 * display_width), (int)(0.135 * display_height));
	}

	snprintf(txt,sizeof(txt),"ver: %s (ip: %s)", _APP_VER, ipaddress);
	//BitmapFNT::print(txt);
	printf_fnt(&alpha, txt);
	
	yPos = 0.2050f;
	xPos = 0.0600f;
	yPosDiff = 0.0804f;
	
	int nMenuItem = 0;
	
	//BitmapFNT::setDimension(22, 26);
	switch (app.state.displayMode.resolution)
	{
	case VIDEO_RESOLUTION_1080:
	case VIDEO_RESOLUTION_1600x1080:
		SetCurrentFont_fnt(&alpha, 0);
		setDimension_fnt(&alpha, alpha.fnt[0].maxwidth, alpha.fnt[0].height);
		break;
	case VIDEO_RESOLUTION_720:
		SetCurrentFont_fnt(&alpha, 1);
		setDimension_fnt(&alpha, alpha.fnt[1].maxwidth, alpha.fnt[1].height);
		break;
	}
	
	while(nMenuItem < main_menu->nTotalItem)
	{
		if (nFrameStep == 0) {
			//BitmapFNT::setColor(SHADOWCOLOR);
			setColor_fnt(&alpha, SHADOWCOLOR);
			//BitmapFNT::setPosition((int)(xPos * display_width) + 1, (int)(int)(yPos * display_height) + 1);
			setPosition_fnt(&alpha, (int)(xPos * display_width) + 1, (int)(int)(yPos * display_height) + 1);
		}
		else
		{
			if (nMenuItem == main_menu->nSelectedItem) {  // selected
				//BitmapFNT::setColor(YELLOWCOLOR);
				setColor_fnt(&alpha, YELLOWCOLOR);
			}
			else
			{
				//BitmapFNT::setColor(WHITECOLOR);
				setColor_fnt(&alpha, WHITECOLOR);
			}
			//BitmapFNT::setPosition((int)(xPos * display_width), (int)(int)(yPos * display_height));
			setPosition_fnt(&alpha, (int)(xPos * display_width), (int)(int)(yPos * display_height));
		}

		//BitmapFNT::print(main_menu->item[nMenuItem]->szMenuLabel);
		printf_fnt(&alpha, main_menu->item[nMenuItem]->szMenuLabel);
		
		yPos += yPosDiff;

		nMenuItem++;
	}
	
}

void c_fbaRL::GameList_Frame()
{


	xPos = 0.3260;
	yPos = 0.2010;
	yPosDiff	= 0.025f;

	switch (app.state.displayMode.resolution)
	{
	case VIDEO_RESOLUTION_1080:
	case VIDEO_RESOLUTION_1600x1080:
		SetCurrentFont_fnt(&alpha, 1);
		setDimension_fnt(&alpha, alpha.fnt[1].maxwidth, alpha.fnt[1].height);
		break;
	case VIDEO_RESOLUTION_720:
		SetCurrentFont_fnt(&alpha, 2);
		setDimension_fnt(&alpha, alpha.fnt[2].maxwidth, alpha.fnt[2].height);
		break;
	}
	
    game_ord_id = 0;

    if(nFilteredGames >= 1)
	{
		game_ord_id = fgames[nSelectedGame]->GameID;
		if(nSelectedGame > NGAMELISTMAX || nGameListTop > 0)
		{
			if(nGameListTop < (nSelectedGame - NGAMELISTMAX)){
				nGameListTop = nSelectedGame - NGAMELISTMAX;
			}
			if((nGameListTop > 0) && (nSelectedGame < nGameListTop)) {
				nGameListTop = nSelectedGame;
			} else {
				nGameListTop = nSelectedGame - NGAMELISTMAX;
			}
		} else {
			nGameListTop = nSelectedGame - NGAMELISTMAX;
		}
		if(nGameListTop < 0) nGameListTop = 0;

		int nGame = nGameListTop;

		
		while(nGame <= (nGameListTop + NGAMELISTMAX))
		{
			if(nFilteredGames < 1) break;
			if(nGame == nFilteredGames) break;

			

			if (nFrameStep == 0) {
				//BitmapFNT::setColor(SHADOWCOLOR);
				setColor_fnt(&alpha, SHADOWCOLOR);
				//BitmapFNT::setPosition((int)(xPos * display_width) + 1, (int)(int)(yPos * display_height) + 1);
				setPosition_fnt(&alpha, (int)(xPos * display_width) + 1, (int)(int)(yPos * display_height) + 1);
			}
			else
			{
				if (nGame == nSelectedGame) {  // selected
					//BitmapFNT::setColor(YELLOWCOLOR);
					setColor_fnt(&alpha, YELLOWCOLOR);
				}
				else
				{	// missing
					if (!fgames[nGame]->bAvailable) {
						//BitmapFNT::setColor(REDCOLOR);
						setColor_fnt(&alpha, REDCOLOR);
					}
					else
						if (fgames[nGame]->isFavorite) // favorite
						{
							//BitmapFNT::setColor(LIGHTBLUECOLOR);
							setColor_fnt(&alpha, LIGHTBLUECOLOR);
						}
						else
						{
							//BitmapFNT::setColor(WHITECOLOR);
							setColor_fnt(&alpha, WHITECOLOR);
						}
				}
				
				//BitmapFNT::setPosition((int)(xPos * display_width), (int)(int)(yPos * display_height));
				setPosition_fnt(&alpha, (int)(xPos * display_width), (int)(int)(yPos * display_height));
			}
			

			/*if(fgames[nGame]->isFavorite) {
				nColor = 0xff00ccff;
			}*/

			//// selected
			//if(nGame == nSelectedGame)
			//{
			//	nColor = 0xffffcc00;
			//	//nfontLize = nSmallSize2;
			//}

			//if(nFrameStep == 0) { nColor = NSHADOWCOLOR; } // Shadow color

            hashmap_position = games[fgames[nGame]->GameID]->nSize;
			//fba_drv = (FBA_DRV *)DRVMAP->data[hashmap_position].data;
            //memcpy(pszFinalText, games[fgames[nGame]->GameID]->title, 104);
			//memcpy16(txt, FBADRV->szTitle, 128);
			//strcpy(txt, FBADRV->szTitle);
			//snprintf(txt, 128, "%s", FBADRV->szTitle );
            //memcpy(pszFinalText, fba_drv->szTitle, 104);
			
			//Main cycle
			//BitmapFNT::print(FBADRV->szTitle, 82);
			printf_fnt(&alpha, FBADRV->szTitle, 82);

			yPos += yPosDiff;

			nGame++;
		}


	}

	


	//BitmapFNT::setDimension(17, 16);
	xPos = 0.3050f;
	yPos = 0.9184f;
	yPosDiff = 0.0199f;
	if (nFrameStep == 0)
	{
		//BitmapFNT::setColor(SHADOWCOLOR);
		setColor_fnt(&alpha, SHADOWCOLOR);
		//BitmapFNT::setPosition((int)(xPos* display_width) + 1, (int)(yPos* display_height) + 1);
		setPosition_fnt(&alpha, (int)(xPos* display_width) + 1, (int)(int)(yPos* display_height) + 1);
	}
	else
	{
		//BitmapFNT::setColor(GRAYCOLOR2);
		setColor_fnt(&alpha, GRAYCOLOR2);
		//BitmapFNT::setPosition((int)(xPos* display_width), (int)(yPos* display_height));
		setPosition_fnt(&alpha, (int)(xPos* display_width), (int)(int)(yPos* display_height));
	}


	snprintf(txt,sizeof(txt),"GAMES AVAILABLE: %d - MISSING: %d",
                nTotalGames,
                app.nMissingGames);
	printf_fnt(&alpha, txt);
	

    yPos += yPosDiff;
	if (nFrameStep == 0)
	{
		setPosition_fnt(&alpha, (int)(xPos * display_width) + 1, (int)(yPos * display_height) + 1);
	}
	else
	{
		setPosition_fnt(&alpha, (int)(xPos * display_width), (int)(yPos * display_height));
	}
 	snprintf(txt,sizeof(txt),"FILTER: %s - FILTERED: %d",
                GetSystemFilter(g_opt_nActiveSysFilter),
                nFilteredGames);
	printf_fnt(&alpha, txt);

    yPos += yPosDiff;
	if (nFrameStep == 0)
	{
		setPosition_fnt(&alpha, (int)(xPos * display_width) + 1, (int)(yPos * display_height) + 1);
	}
	else
	{
		setPosition_fnt(&alpha, (int)(xPos * display_width), (int)(yPos * display_height));
	}
    if (app.ftrack != NULL) {
        snprintf(txt,sizeof(txt),"TRACK: %s", (*app.trackID).c_str());
		printf_fnt(&alpha, txt, 52);
    }


	xPos = 0.5422f;
	yPos = 0.0361f;
	//yPosDiff	= 0.0200f;

	if (nFrameStep == 0)
	{
		setColor_fnt(&alpha, SHADOWCOLOR);
		setPosition_fnt(&alpha, (int)(xPos * display_width) + 1, (int)(yPos * display_height) + 1);
	}
	else
	{
		setColor_fnt(&alpha, GRAYCOLOR);
		setPosition_fnt(&alpha, (int)(xPos * display_width), (int)(yPos * display_height));
	}


	if(nFilteredGames >= 1) //if(nBurnSelected >= 0)
	{

		hashmap_position = games[game_ord_id]->nSize;

		snprintf(txt,sizeof(txt)," TITLE: %s", FBADRV->szTitle);

		if (nFrameStep == 0)
		{
			setPosition_fnt(&alpha, (int)(xPos * display_width) + 1, (int)(yPos * display_height) + 1);
		}
		else
		{
			setPosition_fnt(&alpha, (int)(xPos * display_width), (int)(yPos * display_height));
		}
		printf_fnt(&alpha, txt,58);
		yPos += yPosDiff;

		snprintf(txt,sizeof(txt)," ROMSET:      %s  -  PARENT:      %s",
                    games[game_ord_id]->name,games[nBurnSelected]->parent_name);
		if (nFrameStep == 0)
		{
			setPosition_fnt(&alpha, (int)(xPos * display_width) + 1, (int)(yPos * display_height) + 1);
		}
		else
		{
			setPosition_fnt(&alpha, (int)(xPos * display_width), (int)(yPos * display_height));
		}
		printf_fnt(&alpha, txt);

		yPos += yPosDiff;


		snprintf(txt,sizeof(txt)," COMPANY:     %s", games[game_ord_id]->company);
		if (nFrameStep == 0)
		{
			setPosition_fnt(&alpha, (int)(xPos * display_width) + 1, (int)(yPos * display_height) + 1);
		}
		else
		{
			setPosition_fnt(&alpha, (int)(xPos * display_width), (int)(yPos * display_height));
		}
		printf_fnt(&alpha, txt);

		yPos += yPosDiff;

		snprintf(txt,sizeof(txt)," YEAR:        %s  -  SYSTEM:      %s",
           games[game_ord_id]->year, games[game_ord_id]->subsystem);
		if (nFrameStep == 0)
		{
			setPosition_fnt(&alpha, (int)(xPos * display_width) + 1, (int)(yPos * display_height) + 1);
		}
		else
		{
			setPosition_fnt(&alpha, (int)(xPos * display_width), (int)(yPos * display_height));
		}
		printf_fnt(&alpha, txt);

		yPos += yPosDiff;

		snprintf(txt,sizeof(txt)," MAX PLAYERS: %d  -  RESOLUTION:  %s (%s)",
                    games[game_ord_id]->players,games[nBurnSelected]->resolution,games[game_ord_id]->aspectratio);
		if (nFrameStep == 0)
		{
			setPosition_fnt(&alpha, (int)(xPos * display_width) + 1, (int)(yPos * display_height) + 1);
		}
		else
		{
			setPosition_fnt(&alpha, (int)(xPos * display_width), (int)(yPos * display_height));
		}
		printf_fnt(&alpha, txt);

		yPos += yPosDiff;


		if (games[game_ord_id]->bAvailable) {
                hashmap_position = fgames[nSelectedGame]->nSize;


                snprintf(txt,sizeof(txt)," PATH: %s",FBAGAMES->szPath);
				if (nFrameStep == 0)
				{
					setPosition_fnt(&alpha, (int)(xPos * display_width) + 1, (int)(yPos * display_height) + 1);
				}
				else
				{
					setPosition_fnt(&alpha, (int)(xPos * display_width), (int)(yPos * display_height));
				}
				printf_fnt(&alpha, txt, 58);

        }
        yPos += yPosDiff + 0.015;

        if (app.state.displayMode.resolution == 1)
            xPos = xPos + 0.15;
        else
            xPos = xPos + 0.18;
        lv2syscall1(352, (uint64_t) &meminfo);
        snprintf(txt,sizeof(txt),"MEMORY TOTAL: %d - AVAIL: %d", meminfo.total / 1024, meminfo.avail / 1024);
		if (nFrameStep == 0)
		{
			setPosition_fnt(&alpha, (int)(xPos * display_width) + 1, (int)(yPos * display_height) + 1);
		}
		else
		{
			setPosition_fnt(&alpha, (int)(xPos * display_width), (int)(yPos * display_height));
		}
		printf_fnt(&alpha, txt);
		yPos += yPosDiff;
	}


}



void c_fbaRL::DrawIMG(int x, int y, pngData *png1){
    //rsxBuffer *buf;
	//buf = &(app.buffers[app.currentBuffer]);
	if(png1->bmp_out){
		u32 *scr = (u32 *)texture_buffer;
		u32 *png = (u32 *)png1->bmp_out;


		if(x<0){
			png = png-x;
			error_x=-x;
			x=0;
		}else
			error_x=0;

		if(y<0){
			error_y=-y;
			png = png + y*png1->width;
			y=0;
		}else
			error_y=0;

		scr += y* MAX_WIDTH+x;
		int height = png1->height;
		if((y+height) >= MAX_HEIGHT)
			height = MAX_HEIGHT-1-y;

		int width = png1->width;
		if((x+width) >= MAX_WIDTH)
			width = MAX_WIDTH-x-1;

		width-=error_x;
		height-=error_y;

		if(width>0)
        while(height>0){
			memcpy(scr,png,width*sizeof(u32));
			png+=png1->pitch>>2;
			scr+= MAX_WIDTH;
			height--;
		}
	}
}

void c_fbaRL::Options_Frame()
{
	xPos		= 0.0600f;
	yPos		= 0.0460f;
	yPosDiff	= 0.0410f;
	int X, Y;
	//float nfontLize = nBigSize; // big text
	//uint32_t nColor = nTextColor;
    //pngData*    png_src;
    //rsxbuffer = &(app.buffers[app.currentBuffer]);

    if (app.state.displayMode.resolution == 1)
        fontSize = 2;
    else
        fontSize = 1;
    //rsxBuffer *buffer = &buffers[currentBuffer];
	

	//png_src = app.textures[TEX_OPTIONS]->png;
	//memcpy(texture_buffer, png_src->bmp_out, png_src->width * png_src->height * sizeof(uint32_t));


	//snprintf(txt,sizeof(txt),"ver: %s (ip: %s)", _APP_VER, ipaddress);

	////if(nFrameStep == 0) { yPos += nShadowXYpos; xPos +=nShadowXYpos; }

	yPos += (yPosDiff * 4);
	yPosDiff = 0.0800f;

	int nMenuItem = options_menu->UpdateTopItem();

	while(nMenuItem <= (options_menu->nTopItem + options_menu->nListMax))
	{
		if(nMenuItem == options_menu->nTotalItem) break;

		// normal
		//nColor	= nTextColor;
		setColor_fnt(&alpha, WHITECOLOR);
		
		////if(nFrameStep == 0) { nColor = nShadowColor; } // Shadow color

		// selected
		if(nMenuItem == options_menu->nSelectedItem)
		{
			SetCurrentFont_fnt(&alpha, 2);
			setDimension_fnt(&alpha, alpha.fnt[2].maxwidth, alpha.fnt[2].height);

			if (nFrameStep == 0)
			{
				X = 0.0600f * display_width + 1;
				Y = 0.8900f * display_height + 1;
				setColor_fnt(&alpha, SHADOWCOLOR);
			}
			else
			{
				X = 0.0600f * display_width;
				Y = 0.8900f * display_height;
			}

			//float size = nSmallSize2; // small text 2
			setPosition_fnt(&alpha, X,Y);

			if(nMenuItem == MENU_OPT_AUTO_CFG) {
				snprintf(txt,sizeof(txt),"Auto create basic Input Preset CFG files for all systems.");
                /*fnt_print_vram(&fontM, (u32 *)texture_buffer, png_src->width, (int)(x * png_src->width),
                           (int)(y * png_src->height),
                            txt, nColor, 0x00000000, fontSize, fontSize);*/
				printf_fnt(&alpha, txt);
			}

			if(nMenuItem == MENU_OPT_MUSIC) {
				snprintf(txt,sizeof(txt),"Enable / Disable background MP3 music.");
                /*fnt_print_vram(&fontM, (u32 *)texture_buffer, png_src->width, (int)(x * png_src->width),
                           (int)(y * png_src->height),
                            txt, nColor, 0x00000000, fontSize, fontSize);*/
				printf_fnt(&alpha, txt);
			}

			if(nMenuItem == MENU_OPT_RETROARCH_MENU) {
				snprintf(txt,sizeof(txt),"Change Retro Arch menu driver.");
                /*fnt_print_vram(&fontM, (u32 *)texture_buffer, png_src->width, (int)(x * png_src->width),
                           (int)(y * png_src->height),
                            txt, nColor, 0x00000000, fontSize, fontSize);*/
				printf_fnt(&alpha, txt);
			}

			if(nMenuItem == MENU_OPT_DISP_CLONES) {
				snprintf(txt,sizeof(txt),"Enable / Disable display of clone games.");
                /*fnt_print_vram(&fontM, (u32 *)texture_buffer, png_src->width, (int)(x * png_src->width),
                           (int)(y * png_src->height),
                            txt, nColor, 0x00000000, fontSize, fontSize);*/
				printf_fnt(&alpha, txt);
			}

			if(nMenuItem == MENU_OPT_USE_UNIBIOS) {
				snprintf(txt,sizeof(txt),"Use UNI-BIOS when playing Neo-Geo games.");
                /*fnt_print_vram(&fontM, (u32 *)texture_buffer, png_src->width, (int)(x * png_src->width),
                           (int)(y * png_src->height),
                            txt, nColor, 0x00000000, fontSize, fontSize);*/
				printf_fnt(&alpha, txt);
			}

			if(nMenuItem == MENU_OPT_DISP_MISS_GMS) {
				snprintf(txt,sizeof(txt),"Enable / Disable display of missing games.");
                /*fnt_print_vram(&fontM, (u32 *)texture_buffer, png_src->width, (int)(x * png_src->width),
                           (int)(y * png_src->height),
                            txt, nColor, 0x00000000, fontSize, fontSize);*/
				printf_fnt(&alpha, txt);
			}

			if (nMenuItem == MENU_OPT_MD_DEF_CORE) {
				snprintf(txt, sizeof(txt), "Select default core for MegaDrive games.");
				/*fnt_print_vram(&fontM, (u32*)texture_buffer, png_src->width, (int)(x * png_src->width),
					(int)(y * png_src->height),
					txt, nColor, 0x00000000, fontSize, fontSize);*/
				printf_fnt(&alpha, txt);
			}

			if(nMenuItem >= MENU_OPT_FILTER_START && nMenuItem <=  MASKFAVORITE+MENU_OPT_FILTER_START) {
				snprintf(txt,sizeof(txt),"Choose the emulated system(s) you want to be displayed / filtered.");
                /*fnt_print_vram(&fontM, (u32 *)texture_buffer, png_src->width, (int)(x * png_src->width),
                           (int)(y * png_src->height),
                            txt, nColor, 0x00000000, fontSize, fontSize);*/
				printf_fnt(&alpha, txt);
			}

			// Rom Paths (directories)
			if(nMenuItem > MASKFAVORITE+MENU_OPT_FILTER_START && nMenuItem <= MASKFAVORITE+MENU_OPT_FILTER_START+NDIRPATH) {
				int nRomPath = nMenuItem-(MASKFAVORITE+MENU_OPT_FILTER_START+1);
				snprintf(txt,sizeof(txt),"Current: %s", g_opt_szROMPaths[nRomPath]);
                /*fnt_print_vram(&fontM, (u32 *)texture_buffer, png_src->width, (int)(x * png_src->width),
                           (int)(y * png_src->height),
                            txt, nColor, 0x00000000, fontSize, fontSize);*/
				printf_fnt(&alpha, txt);
			}

			// Input Preset Paths (CFG)
			if(nMenuItem > MASKFAVORITE+MENU_OPT_FILTER_START+12 && nMenuItem <= MASKFAVORITE+MENU_OPT_FILTER_START+NDIRPATH+MASKFAVORITE) {
				int nCfgPath = nMenuItem-(MASKFAVORITE+MENU_OPT_FILTER_START+12+1);
				snprintf(txt,sizeof(txt),"Current: %s", g_opt_szInputCFG[nCfgPath]);
                /*fnt_print_vram(&fontM, (u32 *)texture_buffer, png_src->width, (int)(x * png_src->width),
                           (int)(y * png_src->height),
                            txt, nColor, 0x00000000, fontSize, fontSize);*/
				printf_fnt(&alpha, txt);
			}

			//nColor = nSelectColor;
			setColor_fnt(&alpha, YELLOWCOLOR);

			////if(nFrameStep == 0) { nColor = nShadowColor; } // Shadow color
		}

		SetCurrentFont_fnt(&alpha, 1);
		setDimension_fnt(&alpha, alpha.fnt[1].maxwidth, alpha.fnt[1].height);

		if (nFrameStep == 0)
		{
			X = xPos * display_width + 1;
			Y = yPos * display_height + 1;
			setColor_fnt(&alpha, SHADOWCOLOR);
		}
		else
		{
			X = xPos * display_width;
			Y = yPos * display_height;
		}
		
		setPosition_fnt(&alpha, X,Y);

		if(nMenuItem == MENU_OPT_AUTO_CFG) {
                snprintf(txt,sizeof(txt),"%s: [%s]", options_menu->item[nMenuItem]->szMenuLabel,
                    g_opt_bAutoInputCfgCreate ? "ON" : "OFF");
                /*fnt_print_vram(&fontL, (u32 *)texture_buffer, png_src->width, (int)(xPos * png_src->width),
                           (int)(yPos * png_src->height),
                            txt, nColor, 0x00000000, 1, 1);*/
				printf_fnt(&alpha, txt);
		}

		if(nMenuItem == MENU_OPT_MUSIC) {
                snprintf(txt,sizeof(txt),"%s: [%s]", options_menu->item[nMenuItem]->szMenuLabel,
                    //g_opt_bUseAltMenuKeyCombo ? "ON" : "OFF");
                    g_opt_bMusic ? "ON" : "OFF");
                /*fnt_print_vram(&fontL, (u32 *)texture_buffer, png_src->width, (int)(xPos * png_src->width),
                           (int)(yPos * png_src->height),
                            txt, nColor, 0x00000000, 1, 1);*/
				printf_fnt(&alpha, txt);
		}

		if(nMenuItem == MENU_OPT_RETROARCH_MENU) {
                snprintf(txt,sizeof(txt),"%s: [%s]", options_menu->item[nMenuItem]->szMenuLabel,
                    g_opt_sRetroArchMenu[g_opt_nRetroArchMenu]);
                /*fnt_print_vram(&fontL, (u32 *)texture_buffer, png_src->width, (int)(xPos * png_src->width),
                           (int)(yPos * png_src->height),
                            txt, nColor, 0x00000000, 1, 1);*/
				printf_fnt(&alpha, txt);
		}

		if(nMenuItem == MENU_OPT_DISP_CLONES) { //CRYSTAL
                snprintf(txt,sizeof(txt),"%s: [%s]", options_menu->item[nMenuItem]->szMenuLabel,
                    g_opt_bDisplayCloneGames ? "ON" : "OFF");
                /*fnt_print_vram(&fontL, (u32 *)texture_buffer, png_src->width, (int)(xPos * png_src->width),
                           (int)(yPos * png_src->height),
                            txt, nColor, 0x00000000, 1, 1);*/
				printf_fnt(&alpha, txt);
		}

		if(nMenuItem == MENU_OPT_USE_UNIBIOS) {
                snprintf(txt,sizeof(txt),"%s: [%s]", options_menu->item[nMenuItem]->szMenuLabel,
                    g_opt_bUseUNIBIOS ? "ON" : "OFF");
                /*fnt_print_vram(&fontL, (u32 *)texture_buffer, png_src->width, (int)(xPos * png_src->width),
                           (int)(yPos * png_src->height),
                            txt, nColor, 0x00000000, 1, 1);*/
				printf_fnt(&alpha, txt);
		}

		if(nMenuItem == MENU_OPT_DISP_MISS_GMS) {
                snprintf(txt,sizeof(txt),"%s: [%s]", options_menu->item[nMenuItem]->szMenuLabel,
                    g_opt_bDisplayMissingGames ? "ON" : "OFF");
                /*fnt_print_vram(&fontL, (u32 *)texture_buffer, png_src->width, (int)(xPos * png_src->width),
                           (int)(yPos * png_src->height),
                            txt, nColor, 0x00000000, 1, 1);*/
				printf_fnt(&alpha, txt);
		}

		if (nMenuItem == MENU_OPT_MD_DEF_CORE) {
			snprintf(txt, sizeof(txt), "%s: [%s]", options_menu->item[nMenuItem]->szMenuLabel,
				g_opt_sMegaDriveCores[g_opt_nMegaDriveDefaultCore]);
			/*fnt_print_vram(&fontL, (u32*)texture_buffer, png_src->width, (int)(xPos * png_src->width),
				(int)(yPos * png_src->height),
				txt, nColor, 0x00000000, 1, 1);*/
			printf_fnt(&alpha, txt);
		}

		// Custom System Filters
		if(nMenuItem >= MENU_OPT_FILTER_START && nMenuItem <=  MASKFAVORITE+MENU_OPT_FILTER_START)
		{
			int nSysFilter = nMenuItem-MENU_OPT_FILTER_START;

                snprintf(txt,sizeof(txt),"%s: [%s]", options_menu->item[nMenuItem]->szMenuLabel,
                    g_opt_bCustomSysFilter[nSysFilter] ? "ON" : "OFF");
                /*fnt_print_vram(&fontL, (u32 *)texture_buffer, png_src->width, (int)(xPos * png_src->width),
                           (int)(yPos * png_src->height),
                            txt, nColor, 0x00000000, 1, 1);*/
				printf_fnt(&alpha, txt);
//			cellDbgFontPrintf(xPos, yPos, nfontLize, nColor, "%s: [%s]",
//				options_menu->item[nMenuItem]->szMenuLabel,
//				g_opt_bCustomSysFilter[nSysFilter] ? "ON" : "OFF"
//			);
		}

		// Rom Paths (directories)
		if(nMenuItem > MASKFAVORITE+MENU_OPT_FILTER_START && nMenuItem <= MASKFAVORITE+MENU_OPT_FILTER_START+NDIRPATH)
		{
                snprintf(txt,sizeof(txt),"%s: [...]", options_menu->item[nMenuItem]->szMenuLabel);
                /*fnt_print_vram(&fontL, (u32 *)texture_buffer, png_src->width, (int)(xPos * png_src->width),
                           (int)(yPos * png_src->height),
                            txt, nColor, 0x00000000, 1, 1);*/
				printf_fnt(&alpha, txt);
//			cellDbgFontPrintf(xPos, yPos, nfontLize, nColor, "%s: [...]", options_menu->item[nMenuItem]->szMenuLabel);
		}

		// Input Preset Paths (CFG)
		if(nMenuItem > MASKFAVORITE+MENU_OPT_FILTER_START+12 && nMenuItem <= MASKFAVORITE+MENU_OPT_FILTER_START+NDIRPATH+MASKFAVORITE)
		{
                snprintf(txt,sizeof(txt),"%s: [...]", options_menu->item[nMenuItem]->szMenuLabel);
                /*fnt_print_vram(&fontL, (u32 *)texture_buffer, png_src->width, (int)(xPos * png_src->width),
                           (int)(yPos * png_src->height),
                            txt, nColor, 0x00000000, 1, 1);*/
				printf_fnt(&alpha, txt);
//			cellDbgFontPrintf(xPos, yPos, nfontLize, nColor, "%s: [...]", options_menu->item[nMenuItem]->szMenuLabel);
		}

		yPos += yPosDiff;

		nMenuItem++;
	}
}


void c_fbaRL::FileBrowser_Frame()
{
	xPos		= 0.0750f;
	yPos		= 0.0500f;
	yPosDiff	= 0.0265f;
	//float nfontLize = nSmallSize2; // small text
    //rsxbuffer = &(app.buffers[app.currentBuffer]);
    //pngData*    png_src;

    if (app.state.displayMode.resolution == 1)
        fontSize = 2;
    else
        fontSize = 1;

	
    //rsxBuffer *buffer = &buffers[currentBuffer];
	//png_src = app.textures[TEX_FILEBROWSER]->png;
	//memcpy(texture_buffer, png_src->bmp_out, png_src->width * png_src->height * sizeof(uint32_t));


	//if(nFrameStep == 0) { yPos += nShadowXYpos2; xPos +=nShadowXYpos2; }

	yPos = 0.1250L;

	float yHeadPos = 0.8900f;
	////if(nFrameStep == 0) { yHeadPos += nShadowXYpos2; }

	//nColor	= nTextColor;
	SetCurrentFont_fnt(&alpha, 2);
	setDimension_fnt(&alpha, alpha.fnt[2].maxwidth, alpha.fnt[2].height);
	setColor_fnt(&alpha, WHITECOLOR);
	setPosition_fnt(&alpha, (int)(xPos * display_width), (int)(int)(yHeadPos * display_height));
	////if(nFrameStep == 0) { nColor = nShadowColor; } // Shadow color

	////cellDbgFontPrintf(xPos, yHeadPos, nfontLize, nColor, "CURRENT DIR: %s", filebrowser->pszCurrentDir);
	snprintf(txt,sizeof(txt),"CURRENT DIR: %s", filebrowser->pszCurrentDir);
	printf_fnt(&alpha, txt);
    /*fnt_print_vram(&fontM, (u32 *)texture_buffer, png_src->width, (int)(xPos * png_src->width),
                           (int)(yHeadPos * png_src->height),
                            txt, nColor, 0x00000000, fontSize, fontSize);*/
	yPos += yPosDiff;

	SetCurrentFont_fnt(&alpha, 1);
	setDimension_fnt(&alpha, alpha.fnt[1].maxwidth, alpha.fnt[1].height);

	if(filebrowser->nTotalItem >= 1)
	{
		yPos += yPosDiff;

		int nMenuItem = filebrowser->UpdateTopItem();

		while(nMenuItem <= (filebrowser->nTopItem + filebrowser->nListMax))
		{
			if(nMenuItem == filebrowser->nTotalItem) break;

			// normal
			//nColor = nTextColor;
			setColor_fnt(&alpha, WHITECOLOR);
			// selected
			if(nMenuItem == filebrowser->nSelectedItem) {
				//nColor = nSelectColor;
				setColor_fnt(&alpha, YELLOWCOLOR);
			}

			//if(nFrameStep == 0) { nColor = nShadowColor; } // Shadow color

			////cellDbgFontPrintf(xPos, yPos, nfontLize, nColor, "%s", filebrowser->item[nMenuItem]->szMenuLabel);
            snprintf(txt,sizeof(txt),"%s", filebrowser->item[nMenuItem]->szMenuLabel);
			setPosition_fnt(&alpha, (int)(xPos * display_width), (int)(int)(yPos * display_height));
			printf_fnt(&alpha, txt);
            /*fnt_print_vram(&fontL, (u32 *)texture_buffer, png_src->width, (int)(xPos * png_src->width),
                           (int)(yPos * png_src->height),
                            txt, nColor, 0x00000000, 1, 1);*/
			yPos += yPosDiff;

			nMenuItem++;
		}
	}
}


void c_fbaRL::ZipInfo_Frame()
{
	xPos		= 0.0400f;
	yPos		= 0.0440f;
	yPosDiff	= 0.0200f;
	//float nfontLize = nSmallSize; // small text
    //rsxbuffer = &(app.buffers[app.currentBuffer]);
   
    if (app.state.displayMode.resolution == 1)
        fontSize = 2;
    else
        fontSize = 1;
    //rsxBuffer *buffer = &buffers[currentBuffer];
	//png_src = app.textures[TEX_ZIP_INFO]->png;
	//memcpy(texture_buffer, png_src->bmp_out, png_src->width * png_src->height * sizeof(uint32_t));


	//if(nFrameStep == 0) { yPos += nShadowXYpos2; xPos +=nShadowXYpos2; }

	// normal
	//nColor	= nTextColor;
	SetCurrentFont_fnt(&alpha, 2);
	setDimension_fnt(&alpha, alpha.fnt[2].maxwidth, alpha.fnt[2].height);
	setColor_fnt(&alpha, WHITECOLOR);
	setPosition_fnt(&alpha, (int)(xPos * display_width), (int)(int)(yPos * display_height));
	//if(nFrameStep == 0) { nColor = nShadowColor; } // Shadow color

	if(nFilteredGames >= 1)
	{
		char* pszZipName = NULL;
		pszZipName = (char*)malloc(33);
		memset(pszZipName, 0, 33);
		memcpy(pszZipName, fgames[nSelectedGame]->zipname, 32);

		snprintf(txt,sizeof(txt),"ZIP: %s / TOTAL FILES FOUND: %d", pszZipName, zipinfo_menu->nTotalItem);
        /*fnt_print_vram(&fontM, (u32 *)texture_buffer, png_src->width, (int)(xPos * png_src->width),
                           (int)(yPos * png_src->height),
                            txt, nColor, 0x00000000, fontSize, fontSize);*/
		//cellDbgFontPrintf(xPos, yPos, nfontLize, nColor, "ZIP: %s / TOTAL FILES FOUND: %d", pszZipName, zipinfo_menu->nTotalItem);
		printf_fnt(&alpha, txt);
		yPos += yPosDiff;

		SAFE_FREE(pszZipName)

		xPos = 0.0650f;
		//if(nFrameStep == 0) { xPos +=nShadowXYpos2; }

		yPos += (yPosDiff * 3);

		int nMenuItem = zipinfo_menu->UpdateTopItem();

		while(nMenuItem <= (zipinfo_menu->nTopItem + zipinfo_menu->nListMax))
		{
			if(nMenuItem == zipinfo_menu->nTotalItem) break;

			// normal
			//nColor = nTextColor;
			setColor_fnt(&alpha, WHITECOLOR);
			// selected
			if(nMenuItem == zipinfo_menu->nSelectedItem) {
				//nColor = nSelectColor;
				setColor_fnt(&alpha, YELLOWCOLOR);
			}

			//if(nFrameStep == 0) { nColor = nShadowColor; } // Shadow color
			setPosition_fnt(&alpha, (int)(xPos * display_width), (int)(int)(yPos * display_height));
            snprintf(txt,sizeof(txt),"%s", zipinfo_menu->item[nMenuItem]->szMenuLabel);
            /*fnt_print_vram(&fontM, (u32 *)texture_buffer, png_src->width, (int)(xPos * png_src->width),
                           (int)(yPos * png_src->height),
                            txt, nColor, 0x00000000, fontSize, fontSize);*/
			//cellDbgFontPrintf(xPos, yPos, nfontLize, nColor, "[%d] %s", nMenuItem+1, zipinfo_menu->item[nMenuItem]->szMenuLabel);
			printf_fnt(&alpha, txt);
			yPos += yPosDiff;

			nMenuItem++;
		}
	}
}

void c_fbaRL::RomInfo_Frame()
{
	xPos		= 0.0400f;
	yPos		= 0.0430f;
	yPosDiff	= 0.0220f;
	//float nfontLize = nSmallSize; // small text
    //rsxbuffer = &(app.buffers[app.currentBuffer]);
    //pngData*    png_src;

    if (app.state.displayMode.resolution == 1)
        fontSize = 2;
    else
        fontSize = 1;
    //rsxBuffer *buffer = &buffers[currentBuffer];
	//png_src = app.textures[TEX_ROM_INFO]->png;
	//memcpy(texture_buffer, png_src->bmp_out, png_src->width * png_src->height * sizeof(uint32_t));
    //u16 X,Y;
    //X = round(1269.0f * app.width /1920.0f + (551.0f * app.width /1920.0f - app.textures[TEX_PREVIEW]->pngSec->width)/2.0f);
	//Y = round(532.0f * app.height/1080.0f + (360.0f * app.height/1080.0f - app.textures[TEX_PREVIEW]->pngSec->height)/2.0f);
//	printf("Draw to: %d:%d\n",X,Y);
	//fbaRL->DrawIMG(X,Y, app.textures[TEX_PREVIEW]->pngSec);

	//if(nFrameStep == 0) { yPos += nShadowXYpos2; xPos +=nShadowXYpos2; }

	// normal
	//nColor	= nTextColor;
	SetCurrentFont_fnt(&alpha, 2);
	setDimension_fnt(&alpha, alpha.fnt[2].maxwidth, alpha.fnt[2].height);
	setColor_fnt(&alpha, WHITECOLOR);
	//if(nFrameStep == 0) { nColor = nShadowColor; } // Shadow color

	if(nFilteredGames >= 1)
	{
		char* pszZipName = NULL;
		pszZipName = (char*)malloc(33);
		memset(pszZipName, 0, 33);
		memcpy(pszZipName, fgames[nSelectedGame]->zipname, 32);

		snprintf(txt,sizeof(txt),"ROMSET: %s ", pszZipName);
		setPosition_fnt(&alpha, (int)(xPos * display_width), (int)(int)(yPos * display_height));
		printf_fnt(&alpha, txt);
        /*fnt_print_vram(&fontM, (u32 *)texture_buffer, png_src->width, (int)(xPos * png_src->width),
                           (int)(yPos * png_src->height),
                            txt, nColor, 0x00000000, fontSize, fontSize);*/
		//cellDbgFontPrintf(xPos, yPos, nfontLize, nColor, "ROMSET: %s / TOTAL ROMS: %d", pszZipName, rominfo_menu->nTotalItem);
		yPos += yPosDiff;

		SAFE_FREE(pszZipName)

		xPos = 0.0650f;
		//if(nFrameStep == 0) { xPos +=nShadowXYpos2; }

		yPos += (yPosDiff * 3);

		int nMenuItem = rominfo_menu->UpdateTopItem();

		while(nMenuItem <= (rominfo_menu->nTopItem + rominfo_menu->nListMax))
		{
			if(nMenuItem == rominfo_menu->nTotalItem) break;

			// normal
			//nColor = nTextColor;
			setColor_fnt(&alpha, WHITECOLOR);
			// selected
			if(nMenuItem == rominfo_menu->nSelectedItem) {
				//nColor = nSelectColor;
				setColor_fnt(&alpha, YELLOWCOLOR);
			}

			//if(nFrameStep == 0) { nColor = nShadowColor; } // Shadow color

			snprintf(txt,sizeof(txt),"%s", rominfo_menu->item[nMenuItem]->szMenuLabel);
            /*fnt_print_vram(&fontM, (u32 *)texture_buffer, png_src->width, (int)(xPos * png_src->width),
                           (int)(yPos * png_src->height),
                            txt, nColor, 0x00000000, fontSize, fontSize);*/
			setPosition_fnt(&alpha, (int)(xPos * display_width), (int)(int)(yPos * display_height));
			printf_fnt(&alpha, txt);
			//cellDbgFontPrintf(xPos, yPos, nfontLize, nColor, "[%d] %s", nMenuItem+1, rominfo_menu->item[nMenuItem]->szMenuLabel);
			yPos += yPosDiff;

			nMenuItem++;
		}
	}
}
