#include <stdio.h>
#include <stdarg.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <malloc.h>
#include <ppu-types.h>
#include <sysmodule/sysmodule.h>
#include <pngdec/pngdec.h>
#include "wall1_png_bin.h"

#include <sys/process.h>

#include <io/pad.h>
#include <rsx/rsx.h>
#include <sysutil/sysutil.h>
#include <sysutil/msg.h>
#include <lv2/sysfs.h>
#include <soundlib/spu_soundlib.h>
#include <soundlib/audioplayer.h>

#include "capp.h"
#include "acid.h"
#include "mesh.h"
#include "rsxutil.h"
#include "sqlite.h"
#include "misc.h"
#include "ftp.h"
#include "fnt_print.h"
#include "fnt35.h"


#include "diffuse_specular_shader_vpo.h"
#include "diffuse_specular_shader_fpo.h"

fnt_t fontM, fontL;
vs32 dialog_action = 0;

typedef struct
{
	float x, y, z;
	u32 rgba;
} Vertex_t;

Vertex_t* vertex_buffer;
u32 VertexBufferOffset;

u32 running = 0;

u32 fp_offset;
u32 *fp_buffer;

u32* texture_buffer;
u32 texture_offset;

pngData* png;

typedef struct {
	uint32_t total;
	uint32_t avail;
} smeminfo;

SMeshBuffer* quad = NULL;

// vertex shader
rsxProgramConst  *projMatrix;

rsxProgramAttrib* mPosIndex = NULL;

rsxProgramAttrib* mColIndex = NULL;

// fragment shader
rsxProgramAttrib* textureUnit;

u32 color_index;
u32 position_index;


void *vp_ucode = NULL;
rsxVertexProgram *vpo = (rsxVertexProgram*)diffuse_specular_shader_vpo;

void *fp_ucode = NULL;
rsxFragmentProgram *fpo = (rsxFragmentProgram*)diffuse_specular_shader_fpo;

// setting default scanline parameters
//Vector4 scanlineParams(200.0f, 2.0f, 0.7f, 0.0f);

SYS_PROCESS_PARAM(1001, 0x100000);
//SYS_PROCESS_PARAM(1001, 0x1000000);

// SPU
u32 spu = 0;
sysSpuImage spu_image;
#define SPU_SIZE(x) (((x)+127) & ~127)
u32 inited;
#include "spu_soundmodule_bin.h"

extern "C" {
	static void program_exit_callback()
	{
		//gcmSetWaitFlip(gGcmContext);
		//rsxFinish(gGcmContext,1);
		finish();
	}

int SND_Init_cpp(u32 spu);
void SND_Pause_cpp(int paused);
void SND_End_cpp();

	static void sysutil_exit_callback(uint64_t status, uint64_t param, void* userdata)
	{
		if (app.bRun && status == SYSUTIL_EXIT_GAME) {
			app.bRun = false;
		}
	}
}

static void getmem(smeminfo* m) {
	lv2syscall1(352, (uint64_t) m);
}

static void dialog_handler(msgButton button, void* usrData) {
	switch (button) {
	case MSG_DIALOG_BTN_OK:
		dialog_action = 1;
		break;
	case MSG_DIALOG_BTN_NO:
	case MSG_DIALOG_BTN_ESCAPE:
		dialog_action = 2;
		break;
	case MSG_DIALOG_BTN_NONE:
		dialog_action = -1;
		break;
	default:
		break;
	}
}

SMeshBuffer* createQuad(Point3 P1, Point3 P2, Point3 P3, Point3 P4)
{
	u32 i;
	SColor col(255, 255, 255, 255);
	SMeshBuffer* buffer = new SMeshBuffer();
	const u16 u[6] = { 0,1,2,   0,3,1 };

	buffer->indices.set_used(6);

	for (i = 0; i < 6; i++) buffer->indices[i] = u[i];

	buffer->vertices.set_used(4);

	//                              position, normal,    texture
	buffer->vertices[0] = S3DVertex(P1.getX(), P1.getY(), P1.getZ(), -1, -1, -1, col, 0, 1);
	buffer->vertices[1] = S3DVertex(P2.getX(), P2.getY(), P2.getZ(),  1, -1, -1, col, 1, 0);
	buffer->vertices[2] = S3DVertex(P3.getX(), P3.getY(), P3.getZ(),  1,  1, -1, col, 0, 0);
	buffer->vertices[3] = S3DVertex(P4.getX(), P4.getY(), P4.getZ(), -1,  1, -1, col, 1, 1);

	
	rsxAddressToOffset(&buffer->vertices[0].pos, &buffer->pos_off);
	rsxAddressToOffset(&buffer->vertices[0].nrm, &buffer->nrm_off);
	rsxAddressToOffset(&buffer->vertices[0].col, &buffer->col_off);
	rsxAddressToOffset(&buffer->vertices[0].u, &buffer->uv_off);
	rsxAddressToOffset(&buffer->indices[0], &buffer->ind_off);

	return buffer;
}
static void init_texture()
{

	u32 i;
	u8* buffer;

	//Init png texture
	const u8* data = (u8*)png->bmp_out;
	texture_buffer = (u32*)rsxMemalign(128, (png->height * png->pitch));

	if (!texture_buffer) return;

	rsxAddressToOffset(texture_buffer, &texture_offset);

	buffer = (u8*)texture_buffer;
	for (i = 0; i < png->height * png->pitch; i += 4) {
		buffer[i + 0] = *data++;
		buffer[i + 1] = *data++;
		buffer[i + 2] = *data++;
		buffer[i + 3] = *data++;
	}
}


static void setTexture(u8 textureUnit)
{
	u32 width = png->width;
	u32 height = png->height;
	u32 pitch = png->pitch;
	gcmTexture texture;

	if (!texture_buffer) return;

	rsxInvalidateTextureCache(gGcmContext, GCM_INVALIDATE_TEXTURE);

	texture.format = (GCM_TEXTURE_FORMAT_A8R8G8B8 | GCM_TEXTURE_FORMAT_LIN);
	texture.mipmap = 1;
	texture.dimension = GCM_TEXTURE_DIMS_2D;
	texture.cubemap = GCM_FALSE;
	texture.remap = (
		(GCM_TEXTURE_REMAP_TYPE_REMAP << GCM_TEXTURE_REMAP_TYPE_B_SHIFT) |
		(GCM_TEXTURE_REMAP_TYPE_REMAP << GCM_TEXTURE_REMAP_TYPE_G_SHIFT) |
		(GCM_TEXTURE_REMAP_TYPE_REMAP << GCM_TEXTURE_REMAP_TYPE_R_SHIFT) |
		(GCM_TEXTURE_REMAP_TYPE_REMAP << GCM_TEXTURE_REMAP_TYPE_A_SHIFT) |
		(GCM_TEXTURE_REMAP_COLOR_B << GCM_TEXTURE_REMAP_COLOR_B_SHIFT) |
		(GCM_TEXTURE_REMAP_COLOR_G << GCM_TEXTURE_REMAP_COLOR_G_SHIFT) |
		(GCM_TEXTURE_REMAP_COLOR_R << GCM_TEXTURE_REMAP_COLOR_R_SHIFT) |
		(GCM_TEXTURE_REMAP_COLOR_A << GCM_TEXTURE_REMAP_COLOR_A_SHIFT));
	texture.width = width;
	texture.height = height;
	texture.depth = 1;
	texture.location = GCM_LOCATION_RSX;
	texture.pitch = pitch;
	texture.offset = texture_offset;
	rsxLoadTexture(gGcmContext, textureUnit, &texture);
	rsxTextureControl(gGcmContext, textureUnit, GCM_TRUE, 0 << 8, 12 << 8, GCM_TEXTURE_MAX_ANISO_1);
	rsxTextureFilter(gGcmContext, textureUnit, 0, GCM_TEXTURE_LINEAR, GCM_TEXTURE_LINEAR, GCM_TEXTURE_CONVOLUTION_QUINCUNX);
	rsxTextureWrapMode(gGcmContext, textureUnit, GCM_TEXTURE_CLAMP_TO_EDGE, GCM_TEXTURE_CLAMP_TO_EDGE, GCM_TEXTURE_CLAMP_TO_EDGE, 0, GCM_TEXTURE_ZFUNC_LESS, 0);
}


static void setDrawEnv()
{
	rsxSetColorMask(gGcmContext,GCM_COLOR_MASK_B |
							GCM_COLOR_MASK_G |
							GCM_COLOR_MASK_R |
							GCM_COLOR_MASK_A);

	rsxSetColorMaskMrt(gGcmContext,0);

	u16 x,y,w,h;
	f32 min, max;
	f32 scale[4],offset[4];

	x = 0;
	y = 0;
	w = display_width;
	h = display_height;
	min = 0.0f;
	max = 1.0f;
	scale[0] = w*0.5f;
	scale[1] = h*-0.5f;
	scale[2] = (max - min)*0.5f;
	scale[3] = 0.0f;
	offset[0] = x + w*0.5f;
	offset[1] = y + h*0.5f;
	offset[2] = (max + min)*0.5f;
	offset[3] = 0.0f;

	rsxSetViewport(gGcmContext,x, y, w, h, min, max, scale, offset);
	rsxSetScissor(gGcmContext,x,y,w,h);

	rsxSetDepthTestEnable(gGcmContext, GCM_FALSE);
	rsxSetDepthFunc(gGcmContext,GCM_LESS);
	rsxSetShadeModel(gGcmContext,GCM_SHADE_MODEL_SMOOTH);
	rsxSetDepthWriteEnable(gGcmContext,1);
	rsxSetFrontFace(gGcmContext,GCM_FRONTFACE_CCW);
	rsxSetBlendEnable(gGcmContext, GCM_FALSE);
	//rsxSetBlendFunc(gGcmContext, GCM_SRC_ALPHA, GCM_ONE_MINUS_SRC_ALPHA, GCM_SRC_COLOR, GCM_DST_COLOR);
	//rsxSetBlendEquation(gGcmContext, GCM_FUNC_ADD, GCM_FUNC_ADD);
}

void init_shader()
{
	u32 fpsize = 0;
	u32 vpsize = 0;

	rsxVertexProgramGetUCode(vpo, &vp_ucode, &vpsize);

	projMatrix = rsxVertexProgramGetConst(vpo, "modelViewProj");
		
	mPosIndex = rsxVertexProgramGetAttrib(vpo, "position");
		
	mColIndex = rsxVertexProgramGetAttrib(vpo, "color");
	
	rsxFragmentProgramGetUCode(fpo, &fp_ucode, &fpsize);

	fp_buffer = (u32*)rsxMemalign(64,fpsize);
	memcpy(fp_buffer,fp_ucode,fpsize);

	rsxAddressToOffset(fp_buffer,&fp_offset);
	textureUnit = rsxFragmentProgramGetAttrib(fpo, "texture");
	if (textureUnit)
		printf("textureUnit OK\n");

}


void drawFrame()
{
	u32 i, offset;
	SMeshBuffer* mesh = NULL;

	setDrawEnv();
	
	
	rsxSetClearColor(gGcmContext,0);
	rsxSetClearDepthStencil(gGcmContext,0xffffff00);
	rsxClearSurface(gGcmContext,GCM_CLEAR_R |
							GCM_CLEAR_G |
							GCM_CLEAR_B |
							GCM_CLEAR_A |
							GCM_CLEAR_S |
							GCM_CLEAR_Z);

	rsxSetZControl(gGcmContext,0,1,1);

	for(i=0;i<8;i++)
		rsxSetViewportClip(gGcmContext,i,display_width,display_height);

	Matrix4 tempMatrix = transpose(Matrix4::identity());
	
	mesh = quad;
	setTexture(textureUnit->index);

	rsxBindVertexArrayAttrib(gGcmContext, GCM_VERTEX_ATTRIB_POS, 0, mesh->pos_off, sizeof(S3DVertex), 3, GCM_VERTEX_DATA_TYPE_F32, GCM_LOCATION_RSX);
	rsxBindVertexArrayAttrib(gGcmContext, GCM_VERTEX_ATTRIB_NORMAL, 0, mesh->nrm_off, sizeof(S3DVertex), 3, GCM_VERTEX_DATA_TYPE_F32, GCM_LOCATION_RSX);
	rsxBindVertexArrayAttrib(gGcmContext, GCM_VERTEX_ATTRIB_TEX0, 0, mesh->uv_off, sizeof(S3DVertex), 2, GCM_VERTEX_DATA_TYPE_F32, GCM_LOCATION_RSX);

	rsxLoadVertexProgram(gGcmContext, vpo, vp_ucode);
	rsxSetVertexProgramParameter(gGcmContext, vpo, projMatrix, (float*)&tempMatrix);

	rsxLoadFragmentProgramLocation(gGcmContext, fpo, fp_offset, GCM_LOCATION_RSX);
	rsxAddressToOffset(&mesh->indices[0], &offset);
	rsxDrawIndexArray(gGcmContext, GCM_TYPE_TRIANGLES, mesh->ind_off, mesh->getIndexCount(), GCM_INDEX_TYPE_32B, GCM_LOCATION_RSX);

	
}

CapApp app;

int main(int argc, char *argv[])
{
	app.onInit(argc, argv);
	return 0;

}

CapApp::CapApp()
{
	bRun = true;

	ftp_service = 0;
	fba_drv = NULL;
	drvMap = hashmap_new();
	gamesMap = hashmap_new();
	////renderWidth = 0;
	////renderHeight = 0;
	deviceWidth = 0;
	deviceHeight = 0;

	textures = (c_tex**)malloc(sizeof(c_tex) * 10);
	memset(textures, 0, sizeof(c_tex) * 10);

	mFrame = 0;
	for (int n = 0; n < 16; n++) {
		mIsButtPressed[n] = false;
		buttPressedNow[n] = false;
		nButtons[n] = (1 << n);		// Assign button values in order
	}
	mValRStickX = mValRStickY = 0;
	mValLStickX = mValLStickY = 0;
	tracks.clear();
	ftrack = NULL;
	nTotalBurnDrivers = TOTAL_DRV_GAMES;
}


void CapApp::onRender()
{
	if (fbaRL) { fbaRL->DlgDisplayFrame(); }

	// get render target buffer dimensions and set viewport


	/*////if(fbaRL) { fbaRL->RenderBackground(); }
	*/
	if (fbaRL) { fbaRL->nFrameStep = 0; fbaRL->DisplayFrame(); }
	if (fbaRL) { fbaRL->nFrameStep = 1; fbaRL->DisplayFrame(); }
}

bool CapApp::onUpdate()
{
	if (!mFrame) mFrame = 0;
	mFrame++;

	InputFrameStart();
	if (fbaRL) fbaRL->InputFrame();
	InputFrameEnd();

	return true;
}

void CapApp::onShutdown()
{
	printf("Shutdown\n");
	iniWrite(); // save settings



	hashmap_free(drvMap);
	hashmap_free(gamesMap);
		
	deinitSPUSound();

	InputExit();
	program_exit_callback();
	printf("RSX End.\n");
}

void CapApp::deinitSPUSound()
{
	StopAudio();
	printf("Audio stopped\n");
	SND_End_cpp();
	printf("Sound end\n");
	if (ftrack != NULL)
		fclose(ftrack);
	sysSpuRawDestroy(spu);
	printf("sysSpuRawDestroy\n");
	sysSpuImageClose(&spu_image);
	printf("sysSpuImageClose\n");
}

void CapApp::FontInit()
{
	//printf("STart font init\n");
	int ret1 = 0, ret2 = 0;

	/*if (state.displayMode.resolution == 2) {
		//printf("Display mode 2\n");
		ret2 = internal_load_font(&fontM, "/dev_hdd0/game/FBNE00123/USRDIR/b14.fnt");
		//printf("Loaded fontM\n");
		ret1 = internal_load_font(&fontL, "/dev_hdd0/game/FBNE00123/USRDIR/b24_b.fnt");
		//printf("Loaded fontL\n");
	}
	else {*/
		ret2 = internal_load_font(&fontM, "/dev_hdd0/game/FBNE00123/USRDIR/b12.fnt");
		//ret1 = internal_load_font(&fontL, "/dev_hdd0/game/FBNE00123/USRDIR/b24_b.fnt");
		ret1 = 0;
		fnt_load_mem(fnt35, &fontL);
	//}
	if (ret1 || ret2) {
		printf("Error load fonts\n");
		msgDialogOpen2((msgType)((MSG_DIALOG_BTN_TYPE_OK | MSG_DIALOG_BTN_OK)), "Error! Cant load Font file.", dialog_handler, NULL, NULL);
		dialog_action = 0;
		while (dialog_action == 0) {
			flip();
			sysUtilCheckCallback();
		}

		//printf("Exit while with: %d\n", dialog_action);
		//msgDialogAbort();
		onShutdown();
	}
	//printf("END font init\n");
}


void CapApp::initGraphics()
{
	//host_addr = memalign(HOST_ADDR_ALIGNMENT, HOSTBUFFER_SIZE);
	printf("rsxtest started...\n");

	//init_screen(host_addr, HOSTBUFFER_SIZE);
	initScreen();
	//Init background Image
	png = new pngData;
	pngLoadFromFile("/dev_hdd0/game/FBNE00123/USRDIR/LOADING1.PNG", png);

	
	//Create quad
	quad = createQuad(Point3(-1.0, -1.0, 0), Point3(1.0, 1.0, 0), Point3(-1.0, 1.0, 0), Point3(1.0, -1.0, 0));


	init_shader();
	init_texture();


	setDrawEnv();

	setRenderTarget(curr_fb);
	for (u32 i=0; i < 4; i++) {
		usleep(250);
		drawFrame();
		flip();
	}
	SAFE_FREE(png->bmp_out);

	textures[TEX_MAIN_MENU] = new c_tex(nTextures, 1);
	nTextures++;

	textures[TEX_GAME_LIST] = new c_tex(nTextures, 1);
	nTextures++;

	textures[TEX_ZIP_INFO] = new c_tex(nTextures, 1);
	nTextures++;

	textures[TEX_ROM_INFO] = new c_tex(nTextures, 1);
	nTextures++;

	textures[TEX_OPTIONS] = new c_tex(nTextures, 1);
	nTextures++;

	textures[TEX_FILEBROWSER] = new c_tex(nTextures, 1);
	nTextures++;

	textures[TEX_PREVIEW] = new c_tex(TEX_PREVIEW, g_opt_szTextures[TEX_PREVIEW]);
}



bool CapApp::onInit(int argc, char* argv[])
{
	smeminfo meminfo;
#ifdef FDEBUG
	FILE* fdebug = NULL;
	fdebug = fopen("/dev_hdd0/game/FBNE00123/USRDIR/fdebug.log", "w");
	if (fdebug == 0) {
		printf("Errore nel write\n");
		return 0;
	}
	fprintf(fdebug, "Start FB Neo Rl Plus\n");
	fflush(fdebug);
#endif // FDEBUG
	getmem(&meminfo);
	printf("MEMORY TOTAL: %d - AVAIL: %d\n", meminfo.total / 1024, meminfo.avail / 1024);
	if (sysModuleLoad(SYSMODULE_FS) != 0) exit(0);
	if (sysModuleLoad(SYSMODULE_PNGDEC) != 0) exit(0);
	if (sysModuleLoad(SYSMODULE_NETCTL) != 0) exit(0);
	
#ifdef FDEBUG
	fprintf(fdebug, "SYSMODULE_PNGDEC loaded\n");
	fflush(fdebug);
#endif // FDEBUG

	if (sysUtilRegisterCallback(0, sysutil_exit_callback, NULL) < 0) {
		printf("sysUtilRegisterCallback error.\n");
	}
#ifdef FDEBUG
	fprintf(fdebug, "sysUtilRegisterCallback OK\n");
	fflush(fdebug);
#endif // FDEBUG

	atexit(program_exit_callback);

	if (!iniRead()) {
#ifdef FDEBUG
		fprintf(fdebug, "iniRead OK\n");
		fflush(fdebug);
#endif
		iniWrite(); // create settings file...
#ifdef FDEBUG
		fprintf(fdebug, "iniWrite OK\n");
		fflush(fdebug);
#endif
	}

	//padInfo padinfo;
	//padData paddata;

	//ioPadInit(7);
	getmem(&meminfo);
	printf("MEMORY2 TOTAL: %d - AVAIL: %d\n", meminfo.total / 1024, meminfo.avail / 1024);
	InputInit();
#ifdef FDEBUG
	fprintf(fdebug, "InputInit OK\n");
	fflush(fdebug);
#endif
	getmem(&meminfo);
	printf("MEMORY3 TOTAL: %d - AVAIL: %d\n", meminfo.total / 1024, meminfo.avail / 1024);
	initSPUSound();
	getmem(&meminfo);
	printf("MEMORY4 TOTAL: %d - AVAIL: %d\n", meminfo.total / 1024, meminfo.avail / 1024);
	initGraphics();
	getmem(&meminfo);
	printf("MEMORY5 TOTAL: %d - AVAIL: %d\n", meminfo.total / 1024, meminfo.avail / 1024);

	
#ifdef FDEBUG
	fprintf(fdebug, "initSPUSound OK\n");
	fflush(fdebug);
#endif

	//Check for Upgrade installs
	if (fileExist("/dev_hdd0/game/FBNE00123/USRDIR/GENESIS.INST"))
	{
		g_opt_nMegaDriveDefaultCore = 1;
		iniWrite();
		sysFsUnlink("/dev_hdd0/game/FBNE00123/USRDIR/GENESIS.INST");
		dialog_action = 0;
		msgDialogOpen2((msgType)((MSG_DIALOG_NORMAL | MSG_DIALOG_BTN_TYPE_OK)),
			"MEGADRIVE expansion was correctly installed. Now you can play Sega/Mega CD(J) games. \nDefault core for Mega Drive becomes Genesis Plus FX, but you can change it in the Options.",
			dialog_handler, (void*)&dialog_action, NULL);
		while (dialog_action == 0)
		{
			//waitflip();
			flip();
			sysUtilCheckCallback();
		}
	}
	if (fileExist("/dev_hdd0/game/FBNE00123/USRDIR/SNES.INST"))
	{
		sysFsUnlink("/dev_hdd0/game/FBNE00123/USRDIR/SNES.INST");
		dialog_action = 0;
		msgDialogOpen2((msgType)((MSG_DIALOG_NORMAL | MSG_DIALOG_BTN_TYPE_OK)),
			"SNES expansion was correctly installed. Now you can play SNES games.",
			dialog_handler, (void*)&dialog_action, NULL);
		while (dialog_action == 0)
		{
			//waitflip();
			flip();
			sysUtilCheckCallback();
		}
	}
	if (fileExist("/dev_hdd0/game/FBNE00123/USRDIR/AMIGA.INST"))
	{
		sysFsUnlink("/dev_hdd0/game/FBNE00123/USRDIR/AMIGA.INST");
		dialog_action = 0;
		msgDialogOpen2((msgType)((MSG_DIALOG_NORMAL | MSG_DIALOG_BTN_TYPE_OK)),
			"AMIGA expansion was correctly installed. Now you can play Amiga games.",
			dialog_handler, (void*)&dialog_action, NULL);
		while (dialog_action == 0)
		{
			//waitflip();
			flip();
			sysUtilCheckCallback();
		}
	}

	if (InitDB() == 1)
		return true;
#ifdef FDEBUG
	fprintf(fdebug, "InitDB OK\n");
	fflush(fdebug);
#endif

	int r = ftp_init();
#ifdef FDEBUG
	fprintf(fdebug, "ftp_init OK\n");
	fflush(fdebug);
#endif

	if (r == 0)
	{
		ftp_service = 1; //printf("OpenPS3FTP v2.3 by jjolano started.\n");

	}
	else {
		if (r == -1) printf("Error in netInitialize()\n");
		else if (r == -2) printf("Error in netCtlInit()\n");
		else if (r == -3) printf("Error in netCtlGetInfo()\n");
		else if (r == -4) printf("Net Disconnected or Connection not Established\n");
		else printf("Another FTP service present!\n");
	}

	FontInit();
#ifdef FDEBUG
	fprintf(fdebug, "FontInit OK\n");
	fflush(fdebug);
#endif

#ifdef FDEBUG
	fprintf(fdebug, "InputInit OK\n");

	lv2syscall1(352, (uint64_t)&meminfo);
	fprintf(fdebug, "MEMORY TOTAL: %d - AVAIL: %d\n", meminfo.total / 1024, meminfo.avail / 1024);
	fclose(fdebug);
#endif

	if (argc > 1) {
		printf("onInit argv[1]: %s\n",argv[1]);
		if (strcmp(argv[1], "gamesList") == 0) {
			fbaRL = new c_fbaRL(true);
			printf("StartwithFileSection = true\n");
		}
		else {
			fbaRL = new c_fbaRL(false);

		}
	}
	else {
		fbaRL = new c_fbaRL(false);
	}
#ifdef FDEBUG
	fdebug = fopen("/dev_hdd0/game/FBNE00123/USRDIR/fdebug.log", "a");
	fprintf(fdebug, "fbaRL class OK\n");
	fflush(fdebug);
#endif

	fbaRL->InitGameList();
#ifdef FDEBUG
	fprintf(fdebug, "InitGameList OK\n");
	fflush(fdebug);
#endif
	//fbaRL->InitGameList();

	//printf("fbaRL->InitFilterList()\n");
	fbaRL->InitFilterList();
#ifdef FDEBUG
	fprintf(fdebug, "InitFilterList OK\n");
	fclose(fdebug);
#endif

	//pngLoadFromBuffer(wall1_png_bin, wall1_png_bin_size, png);
	memcpy(texture_buffer, textures[TEX_MAIN_MENU]->png->bmp_out, textures[TEX_MAIN_MENU]->png->height * textures[TEX_MAIN_MENU]->png->pitch);

/*	running = 1;
	while (running) {
		sysUtilCheckCallback();

		ioPadGetInfo(&padinfo);
		for (int i = 0; i < MAX_PADS; i++) {
			if (padinfo.status[i]) {
				ioPadGetData(i, &paddata);

				if (paddata.BTN_CIRCLE)
					goto done;

			}

		}

		drawFrame();
		flip();
	}*/

	while (bRun)
	{
		sysUtilCheckCallback();
		onRender();
		onUpdate();
		drawFrame();
		flip();
		
	}

	return false;
}

int CapApp::InitDB()
{
	//printf("LOAD ARRAY\n");
	int rc = 1;
	//int row = 0;
	sqlite3* mdb;
	char buf[512];
	char cache_path[100];
	char* errmsg;
	//char sql[256];

	snprintf(cache_path, sizeof(cache_path), "/dev_hdd0/game/FBNE00123/USRDIR");



	rc = db_init_cpp(cache_path);
	if (rc != SQLITE_OK) {
		printf("DB Init failed!\n");
		return 1;
	}

	snprintf(buf, sizeof(buf), "/dev_hdd0/game/FBNE00123/USRDIR/retro_local.db");
	if (!fileExist(buf)) {  //CREATE DB LOCAL
		rc = sqlite3_open_v2(buf, &mdb,
			SQLITE_OPEN_READWRITE | SQLITE_OPEN_CREATE,
			NULL);
		if (mdb == NULL) {
			printf("Open DB failed!\n");
			return 1;
		}
		snprintf(buf, sizeof(buf), "CREATE TABLE games_favorite ( ord_game_id INTEGER PRIMARY KEY );");
		rc = sqlite3_exec(mdb, buf, NULL, NULL, &errmsg);
		if (rc) {
			printf("CREATE TABLE games_favorite failed - %s: failed -- %s\n",
				buf, errmsg);
			sqlite3_free(errmsg);
			sqlite3_close(mdb);
		}
		snprintf(buf, sizeof(buf), "CREATE INDEX ord_game_id_idx ON games_favorite (ord_game_id)");
		rc = sqlite3_exec(mdb, buf, NULL, NULL, &errmsg);
		if (rc) {
			printf("CREATE INDEX ord_game_id_idx ON games_favorite failed - %s: failed -- %s\n",
				buf, errmsg);
			sqlite3_free(errmsg);
			sqlite3_close(mdb);
		}


		rc = sqlite3_close(mdb);
		if (rc)
			printf("CLOSE PROBLEM POST CREATE DB retro_local: %d\n", rc);

	}

	
	sqlite3_stmt* stmt;
	bool fav[app.nTotalBurnDrivers];
	for (unsigned int i = 0; i < app.nTotalBurnDrivers; i++)
		fav[i] = false;
	snprintf(buf, sizeof(buf), "/dev_hdd0/game/FBNE00123/USRDIR/retro_local.db");

	//mdb = db_open_cpp(buf);
	rc = sqlite3_open_v2(buf, &mdb,
		SQLITE_OPEN_READONLY,
		NULL);
	if (mdb == NULL) {
		printf("Open DB failed!\n");
		return 1;
	}
	snprintf(buf, sizeof(buf), "SELECT ord_game_id COL00 FROM games_favorite");
	rc = sqlite3_prepare_v2(mdb, buf, -1, &stmt, 0);
	if (rc != SQLITE_OK) {
		printf("Query failed!\n");
		sqlite3_finalize(stmt);
		sqlite3_close(mdb);
		return 1;
	}
	rc = sqlite3_step(stmt);
	int row = 0;
	while (rc == SQLITE_ROW) {
		row = sqlite3_column_int(stmt, COL00);
		fav[row] = true;
		rc = sqlite3_step(stmt);
	}
	sqlite3_finalize(stmt);
	rc = sqlite3_close(mdb);

	if (rc)
		printf("CLOSE PROBLEM:END ParseGameListCache %d\n", rc);

	snprintf(buf, sizeof(buf), "/dev_hdd0/game/FBNE00123/USRDIR/retro.db");

	rc = sqlite3_open_v2(buf, &mdb,
		SQLITE_OPEN_READONLY,
		NULL);
	if (mdb == NULL) {
		printf("Open DB failed!\n");
		return 1;
	}

	snprintf(buf, sizeof(buf), "SELECT corename COL00 FROM cores order by id");
	rc = sqlite3_prepare_v2(mdb, buf, -1, &stmt, COL00);
	if (rc != SQLITE_OK) {
		printf("Query failed!\n");
		return 1;
	}
	rc = sqlite3_step(stmt);
	int i = 0;
	while (rc == SQLITE_ROW) {
		snprintf(cores[i], 32, "%s", (char*)sqlite3_column_text(stmt, COL00));
		//printf("Core: %s\n", cores[i]);
		i++;
		rc = sqlite3_step(stmt);
	}
	sqlite3_finalize(stmt);

	snprintf(buf, sizeof(buf), "select g.id COL00, g.title COL01, g.order_id  COL02, ga.path COL03, "
		"s.sysmask COL04, g.cloneof COL05, g.aspectratio COL06, g.name COL07, "
		"g.publisher COL08, g.year COL09, g.system COL10, g.players COL11, g.resolution COL12, "
		"g.aspectratio COL13, g.def_core_id COL14, ga.coreid COL15, g.subsystem COL16 "
		"from games g inner join systems s on g.subsystem = s.name left join games_available ga on g.id = ga.game_id ");
	//"left join games_favorite fa on g.order_id = fa.ord_game_id order by order_id");
	//"order by title");

	rc = sqlite3_prepare_v2(mdb, buf, -1, &stmt, 0);
	if (rc != SQLITE_OK) {
		printf("Query failed!\n");
		sqlite3_finalize(stmt);
		sqlite3_close(mdb);
		return 1;
	}
	rc = sqlite3_step(stmt);

	row = 0;
	nMissingGames = 0;

	int error = 0;
	while (rc == SQLITE_ROW) {
		fba_drv = (FBA_DRV*)malloc(sizeof(FBA_DRV));
		if (fba_drv == NULL) {
			printf("Exit.... memory error malloc(sizeof(FBA_DRV)\n");
			exit(0);
		}
		snprintf(fba_drv->szTitle, 256, "%s", (char*)sqlite3_column_text(stmt, COL01));
		snprintf(fba_drv->szName, 128, "%s", (char*)sqlite3_column_text(stmt, COL07));		// title
		snprintf(fba_drv->szSystemFilter, 32, "%s", (char*)sqlite3_column_text(stmt, COL04));	// sys filter
		snprintf(fba_drv->szCompany, 96, "%s", (char*)sqlite3_column_text(stmt, COL08) == NULL ? "" : (char*)sqlite3_column_text(stmt, COL08));	//
		snprintf(fba_drv->szYear, 5, "%s", (char*)sqlite3_column_text(stmt, COL09));	//
		snprintf(fba_drv->szSystem, 32, "%s", (char*)sqlite3_column_text(stmt, COL10));	//
		snprintf(fba_drv->szSubSystem, 32, "%s", (char*)sqlite3_column_text(stmt, COL16));	//
		snprintf(fba_drv->szResolution, 10, "%s", (char*)sqlite3_column_text(stmt, COL12));	//
		snprintf(fba_drv->szAspectRatio, 7, "%s", (char*)sqlite3_column_text(stmt, COL13));	//
		fba_drv->nMaxPlayers = sqlite3_column_int(stmt, COL11);
		fba_drv->nDefCoreID = sqlite3_column_int(stmt, COL14);
		fba_drv->nCoreID = sqlite3_column_int(stmt, COL15);
		fba_drv->nGameID = sqlite3_column_int(stmt, COL00);
		fba_drv->nGameOrderID = sqlite3_column_int(stmt, COL02) - 1;
		//                if (sqlite3_column_text(stmt, COL16) != NULL)
		//                                fba_drv->isFavorite = true;
		//                else
		fba_drv->isFavorite = fav[fba_drv->nGameOrderID + 1];
		if (sqlite3_column_text(stmt, COL05) != NULL) { // is a clone?
			fba_drv->isClone = true;
			snprintf(fba_drv->szParent, 128, "%s", (char*)sqlite3_column_text(stmt, COL05));

		}
		else
			fba_drv->isClone = false;

		if (sqlite3_column_text(stmt, COL03) != NULL) { // is available?
			fba_games = (FBA_GAMES*)malloc(sizeof(FBA_GAMES));
			if (fba_games == NULL) {
				printf("Exit.... memory error malloc(sizeof(FBA_GAMES)\n");
				exit(0);
			}
			snprintf(fba_games->szPath, 256, "%s", (char*)sqlite3_column_text(stmt, COL03));
			snprintf(fba_games->key_string, KEY_MAX_LENGTH, "%s%s%d",
				(char*)sqlite3_column_text(stmt, COL10), (char*)sqlite3_column_text(stmt, COL07), sqlite3_column_int(stmt, COL15));
			error = hashmap_put(gamesMap, fba_games->key_string, fba_games);
			//                        if (error !=MAP_OK) {
//                            printf("Error: %d - %s\n", error, fba_games->szPath);
//                        }
			fba_drv->isAvailable = true;

		}
		else {
			fba_drv->isAvailable = false;
			nMissingGames++;
		}
		snprintf(fba_drv->key_string, KEY_MAX_LENGTH, "%s%s", (char*)sqlite3_column_text(stmt, COL10), (char*)sqlite3_column_text(stmt, COL07));

		error = hashmap_put(drvMap, fba_drv->key_string, fba_drv);
		//				if (error !=MAP_OK) {
		//                    printf("Error: %d - %s\n", error, fba_drv->szName);
		//				}
		row++;
		rc = sqlite3_step(stmt);

	}
	sqlite3_finalize(stmt);

	rc = sqlite3_close(mdb);


	if (rc)
		printf("CLOSE PROBLEM:load cores %d\n", rc);


	//printf("fba_games->size %d\n", ((hashmap_map *)gamesMap)->size);

	//printf("Missing games: %d\n", nMissingGames);
	
	return 0;
}

void CapApp::initSPUSound() {
	u32 entry = 0;
	u32 segmentcount = 0;
	sysSpuSegment* segments;
	printf("Initializing SPUs... %08x\n", sysSpuInitialize(6, 5));
	printf("Initializing raw SPU... %08x\n", sysSpuRawCreate(&spu, NULL));
	printf("Getting ELF information... %08x\n",
		sysSpuElfGetInformation(spu_soundmodule_bin, &entry, &segmentcount));
	printf("    Entry Point: %08x    Segment Count: %08x\n", entry, segmentcount);
	size_t segmentsize = sizeof(sysSpuSegment) * segmentcount;
	segments = (sysSpuSegment*)memalign(128, SPU_SIZE(segmentsize)); // must be aligned to 128 or it break malloc() allocations
	memset(segments, 0, segmentsize);
	printf("Getting ELF segments... %08x\n",
		sysSpuElfGetSegments(spu_soundmodule_bin, segments, segmentcount));
	printf("Loading ELF image... %08x\n",
		sysSpuImageImport(&spu_image, spu_soundmodule_bin, 0));
	printf("Loading image into SPU... %08x\n",
		sysSpuRawImageLoad(spu, &spu_image));
	SND_Init_cpp(spu);

	SND_Pause_cpp(0);
	std::vector <std::string> tmp;
	tracks.clear();
	readDir("/dev_hdd0/game/FBNE00123/USRDIR/soundtracks", DIR_NO_DOT_AND_DOTDOT | DIR_FILES, tracks, tmp);
	trackID = tracks.begin() + g_opt_nTrackID;
	if (trackID < tracks.end() && g_opt_bMusic) {
		char tpath[256];
		sprintf(tpath, "/dev_hdd0/game/FBNE00123/USRDIR/soundtracks/%s", (*trackID).c_str());
		ftrack = fopen(tpath, "r");
		if (ftrack) {
			//printf("Start track: %s\n", (*trackID).c_str());
			PlayAudiofd(ftrack, 0, AUDIO_INFINITE_TIME);
		}

	}

}

void CapApp::remakegamesMaps() {
	hashmap_free(gamesMap);
	gamesMap = hashmap_new();
	if (gamesMap == NULL)
		printf("ERRORE - remakegamesMaps: gamesMap = hashmap_new()\n");
}
