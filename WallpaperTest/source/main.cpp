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
#include "wall2_png_bin.h"

#include <sys/process.h>

#include <io/pad.h>
#include <rsx/rsx.h>
#include <sysutil/sysutil.h>

#include "acid.h"
#include "mesh.h"
#include "geometry.h"
#include "rsxutil.h"

#include "diffuse_specular_shader_vpo.h"
#include "diffuse_specular_shader_fpo.h"

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
u32* texture_buffer2;
u32 texture_offset2;
u32* texture_buffer3;
u32 texture_offset3;
u32* texture_buffer4;
u32 texture_offset4;
u32* texture_buffer5;
u32 texture_offset5;

pngData *png, *png2;


SMeshBuffer* quad = NULL;
SMeshBuffer* quad2 = NULL;
SMeshBuffer* quad3 = NULL;
SMeshBuffer* quad4 = NULL;
SMeshBuffer* quad5 = NULL;

// vertex shader
rsxProgramConst  *projMatrix;

rsxProgramAttrib* mPosIndex = NULL;

rsxProgramAttrib* mColIndex = NULL;

// fragment shader
rsxProgramAttrib* textureUnit;


u32 color_index;
u32 position_index;

u32 selection = 0;

void *vp_ucode = NULL;
rsxVertexProgram *vpo = (rsxVertexProgram*)diffuse_specular_shader_vpo;

void *fp_ucode = NULL;
rsxFragmentProgram *fpo = (rsxFragmentProgram*)diffuse_specular_shader_fpo;

// setting default scanline parameters

SYS_PROCESS_PARAM(1001, 0x100000);

extern "C" {
static void program_exit_callback()
{
	gcmSetWaitFlip(context);
	rsxFinish(context,1);
}

static void sysutil_exit_callback(u64 status,u64 param,void *usrdata)
{
	switch(status) {
		case SYSUTIL_EXIT_GAME:
			running = 0;
			break;
		case SYSUTIL_DRAW_BEGIN:
		case SYSUTIL_DRAW_END:
			break;
		default:
			break;
	}
}
}

void getsubimage(u32* src, u32* dest, u32 x, u32 y, u32 w, u32 h, u32 w_src, u32 h_src) {

	printf("enter getsubimage... %d %d %d %d ... ", x,y,w,h);
	u32 idx = 0;
	for (u32 j = 0; j < h; j++) {
		for (u32 i = 0; i < w; i++) {
			dest[idx] = src[w_src * (y + j) + x + i];
			idx++;
		}
	}
	printf("exit getsubimage. IDX = %d\n", idx);
}

static void init_texture()
{

	//u32 i;
	//u8* buffer;

	//Init png texture
	//const u8* data = (u8*)png->bmp_out;
	texture_buffer = (u32*)rsxMemalign(128, 960 * 540 * 4);

	if (!texture_buffer) return;

	rsxAddressToOffset(texture_buffer, &texture_offset);

	//buffer = (u8*)texture_buffer;
	getsubimage((u32*)png->bmp_out, texture_buffer, 0,540, 960, 540, 1920, 1080);
	//memcpy(texture_buffer, png->bmp_out, png->height * png->pitch);
	/*for (i = 0; i < png->height * png->pitch; i += 4) {
		buffer[i + 0] = *data++;
		buffer[i + 1] = *data++;
		buffer[i + 2] = *data++;
		buffer[i + 3] = *data++;
	}*/

	texture_buffer2 = (u32*)rsxMemalign(128, 960*540*4);
	
	if (!texture_buffer2) return;

	rsxAddressToOffset(texture_buffer2, &texture_offset2);
	
	getsubimage((u32*)png->bmp_out, texture_buffer2, 0, 0, 960, 540, 1920, 1080);

	texture_buffer3 = (u32*)rsxMemalign(128, 960 * 540 * 4);

	if (!texture_buffer3) return;

	rsxAddressToOffset(texture_buffer3, &texture_offset3);

	getsubimage((u32*)png->bmp_out, texture_buffer3, 960, 0, 960, 540, 1920, 1080);

	texture_buffer4 = (u32*)rsxMemalign(128, 960 * 540 * 4);

	if (!texture_buffer4) return;

	rsxAddressToOffset(texture_buffer4, &texture_offset4);

	getsubimage((u32*)png->bmp_out, texture_buffer4, 960, 540, 960, 540, 1920, 1080);

	texture_buffer5 = (u32*)rsxMemalign(128, png2->height*png2->pitch);

	if (!texture_buffer5) return;

	rsxAddressToOffset(texture_buffer5, &texture_offset5);

	memcpy((void*)texture_buffer5, (void*)png2->bmp_out, png2->height * png2->pitch);

}




static void setTexture(u8 textureUnit, u32 * tex_buffer, u32 tex_offset, u32 width, u32 height)
{
	
	u32 pitch = width*4;
	gcmTexture texture;

	if (!tex_buffer) return;

	rsxInvalidateTextureCache(context, GCM_INVALIDATE_TEXTURE);

	texture.format = (GCM_TEXTURE_FORMAT_A8R8G8B8 | GCM_TEXTURE_FORMAT_LIN);
	texture.mipmap = 1;
	texture.dimension = GCM_TEXTURE_DIMS_2D;
	texture.cubemap = GCM_FALSE;
	texture.remap = ((GCM_TEXTURE_REMAP_TYPE_REMAP << GCM_TEXTURE_REMAP_TYPE_B_SHIFT) |
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
	texture.offset = tex_offset;
	rsxLoadTexture(context, textureUnit, &texture);
	rsxTextureControl(context, textureUnit, GCM_TRUE, 0 << 8, 12 << 8, GCM_TEXTURE_MAX_ANISO_1);
	rsxTextureFilter(context, textureUnit, 0, GCM_TEXTURE_LINEAR, GCM_TEXTURE_LINEAR, GCM_TEXTURE_CONVOLUTION_QUINCUNX);
	rsxTextureWrapMode(context, textureUnit, GCM_TEXTURE_CLAMP_TO_EDGE, GCM_TEXTURE_CLAMP_TO_EDGE, GCM_TEXTURE_CLAMP_TO_EDGE, 0, GCM_TEXTURE_ZFUNC_LESS, 0);
}



static void setDrawEnv()
{
	rsxSetColorMask(context,GCM_COLOR_MASK_B |
							GCM_COLOR_MASK_G |
							GCM_COLOR_MASK_R |
							GCM_COLOR_MASK_A);

	rsxSetColorMaskMrt(context,0);

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

	rsxSetViewport(context,x, y, w, h, min, max, scale, offset);
	rsxSetScissor(context,x,y,w,h);

	rsxSetDepthTestEnable(context, GCM_FALSE);
	rsxSetDepthFunc(context, GCM_ALWAYS);
	rsxSetShadeModel(context, GCM_SHADE_MODEL_FLAT);
	rsxSetDepthWriteEnable(context, GCM_FALSE);
	rsxSetFrontFace(context,GCM_FRONTFACE_CCW);
	rsxSetBlendEnable(context, GCM_FALSE);
	//rsxSetBlendFunc(context, GCM_SRC_ALPHA, GCM_ONE_MINUS_SRC_ALPHA, GCM_SRC_COLOR, GCM_DST_COLOR);
	//rsxSetBlendEquation(context, GCM_FUNC_ADD, GCM_FUNC_ADD);
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
	
	
	//rsxSetClearColor(context,0);
	//rsxSetClearDepthStencil(context,0xffffff00);
	rsxClearSurface(context, GCM_CLEAR_R |
		GCM_CLEAR_G |
		GCM_CLEAR_B |
		GCM_CLEAR_A |
		GCM_CLEAR_S |
		GCM_CLEAR_Z);

	//rsxSetZControl(context,0,1,1);

	for(i=0;i<8;i++)
		rsxSetViewportClip(context,i,display_width,display_height);

	Matrix4 tempMatrix = transpose(Matrix4::identity());
	
	mesh = quad;
	setTexture(textureUnit->index, texture_buffer2, texture_offset2, 960, 540);

	rsxBindVertexArrayAttrib(context, GCM_VERTEX_ATTRIB_POS, 0, mesh->pos_off, sizeof(S3DVertex), 3, GCM_VERTEX_DATA_TYPE_F32, GCM_LOCATION_RSX);
	rsxBindVertexArrayAttrib(context, GCM_VERTEX_ATTRIB_NORMAL, 0, mesh->nrm_off, sizeof(S3DVertex), 3, GCM_VERTEX_DATA_TYPE_F32, GCM_LOCATION_RSX);
	rsxBindVertexArrayAttrib(context, GCM_VERTEX_ATTRIB_TEX0, 0, mesh->uv_off, sizeof(S3DVertex), 2, GCM_VERTEX_DATA_TYPE_F32, GCM_LOCATION_RSX);

	rsxLoadVertexProgram(context, vpo, vp_ucode);
	rsxSetVertexProgramParameter(context, vpo, projMatrix, (float*)&tempMatrix);

	rsxLoadFragmentProgramLocation(context, fpo, fp_offset, GCM_LOCATION_RSX);
	rsxAddressToOffset(&mesh->indices[0], &offset);
	rsxDrawIndexArray(context, GCM_TYPE_TRIANGLES, mesh->ind_off, mesh->getIndexCount(), GCM_INDEX_TYPE_32B, GCM_LOCATION_RSX);

	mesh = quad2;
	setTexture(textureUnit->index, texture_buffer, texture_offset, 960, 540);

	rsxBindVertexArrayAttrib(context, GCM_VERTEX_ATTRIB_POS, 0, mesh->pos_off, sizeof(S3DVertex), 3, GCM_VERTEX_DATA_TYPE_F32, GCM_LOCATION_RSX);
	rsxBindVertexArrayAttrib(context, GCM_VERTEX_ATTRIB_NORMAL, 0, mesh->nrm_off, sizeof(S3DVertex), 3, GCM_VERTEX_DATA_TYPE_F32, GCM_LOCATION_RSX);
	rsxBindVertexArrayAttrib(context, GCM_VERTEX_ATTRIB_TEX0, 0, mesh->uv_off, sizeof(S3DVertex), 2, GCM_VERTEX_DATA_TYPE_F32, GCM_LOCATION_RSX);

	rsxLoadVertexProgram(context, vpo, vp_ucode);
	rsxSetVertexProgramParameter(context, vpo, projMatrix, (float*)&tempMatrix);

	rsxLoadFragmentProgramLocation(context, fpo, fp_offset, GCM_LOCATION_RSX);
	rsxAddressToOffset(&mesh->indices[0], &offset);
	rsxDrawIndexArray(context, GCM_TYPE_TRIANGLES, mesh->ind_off, mesh->getIndexCount(), GCM_INDEX_TYPE_32B, GCM_LOCATION_RSX);

	mesh = quad3;
	setTexture(textureUnit->index, texture_buffer3, texture_offset3, 960, 540);

	rsxBindVertexArrayAttrib(context, GCM_VERTEX_ATTRIB_POS, 0, mesh->pos_off, sizeof(S3DVertex), 3, GCM_VERTEX_DATA_TYPE_F32, GCM_LOCATION_RSX);
	rsxBindVertexArrayAttrib(context, GCM_VERTEX_ATTRIB_NORMAL, 0, mesh->nrm_off, sizeof(S3DVertex), 3, GCM_VERTEX_DATA_TYPE_F32, GCM_LOCATION_RSX);
	rsxBindVertexArrayAttrib(context, GCM_VERTEX_ATTRIB_TEX0, 0, mesh->uv_off, sizeof(S3DVertex), 2, GCM_VERTEX_DATA_TYPE_F32, GCM_LOCATION_RSX);

	rsxLoadVertexProgram(context, vpo, vp_ucode);
	rsxSetVertexProgramParameter(context, vpo, projMatrix, (float*)&tempMatrix);

	rsxLoadFragmentProgramLocation(context, fpo, fp_offset, GCM_LOCATION_RSX);
	rsxAddressToOffset(&mesh->indices[0], &offset);
	rsxDrawIndexArray(context, GCM_TYPE_TRIANGLES, mesh->ind_off, mesh->getIndexCount(), GCM_INDEX_TYPE_32B, GCM_LOCATION_RSX);

	mesh = quad4;
	setTexture(textureUnit->index, texture_buffer4, texture_offset4, 960, 540);

	rsxBindVertexArrayAttrib(context, GCM_VERTEX_ATTRIB_POS, 0, mesh->pos_off, sizeof(S3DVertex), 3, GCM_VERTEX_DATA_TYPE_F32, GCM_LOCATION_RSX);
	rsxBindVertexArrayAttrib(context, GCM_VERTEX_ATTRIB_NORMAL, 0, mesh->nrm_off, sizeof(S3DVertex), 3, GCM_VERTEX_DATA_TYPE_F32, GCM_LOCATION_RSX);
	rsxBindVertexArrayAttrib(context, GCM_VERTEX_ATTRIB_TEX0, 0, mesh->uv_off, sizeof(S3DVertex), 2, GCM_VERTEX_DATA_TYPE_F32, GCM_LOCATION_RSX);

	rsxLoadVertexProgram(context, vpo, vp_ucode);
	rsxSetVertexProgramParameter(context, vpo, projMatrix, (float*)&tempMatrix);

	rsxLoadFragmentProgramLocation(context, fpo, fp_offset, GCM_LOCATION_RSX);
	rsxAddressToOffset(&mesh->indices[0], &offset);
	rsxDrawIndexArray(context, GCM_TYPE_TRIANGLES, mesh->ind_off, mesh->getIndexCount(), GCM_INDEX_TYPE_32B, GCM_LOCATION_RSX);

	if (selection) {
		mesh = quad5;
		setTexture(textureUnit->index, texture_buffer5, texture_offset5, png2->width, png2->height);

		rsxBindVertexArrayAttrib(context, GCM_VERTEX_ATTRIB_POS, 0, mesh->pos_off, sizeof(S3DVertex), 3, GCM_VERTEX_DATA_TYPE_F32, GCM_LOCATION_RSX);
		rsxBindVertexArrayAttrib(context, GCM_VERTEX_ATTRIB_NORMAL, 0, mesh->nrm_off, sizeof(S3DVertex), 3, GCM_VERTEX_DATA_TYPE_F32, GCM_LOCATION_RSX);
		rsxBindVertexArrayAttrib(context, GCM_VERTEX_ATTRIB_TEX0, 0, mesh->uv_off, sizeof(S3DVertex), 2, GCM_VERTEX_DATA_TYPE_F32, GCM_LOCATION_RSX);

		rsxLoadVertexProgram(context, vpo, vp_ucode);
		rsxSetVertexProgramParameter(context, vpo, projMatrix, (float*)&tempMatrix);

		rsxLoadFragmentProgramLocation(context, fpo, fp_offset, GCM_LOCATION_RSX);
		rsxAddressToOffset(&mesh->indices[0], &offset);
		rsxDrawIndexArray(context, GCM_TYPE_TRIANGLES, mesh->ind_off, mesh->getIndexCount(), GCM_INDEX_TYPE_32B, GCM_LOCATION_RSX);
	}
	
}


int main(int argc,const char *argv[])
{
	padInfo padinfo;
	padData paddata;
	void *host_addr = memalign(HOST_ADDR_ALIGNMENT,HOSTBUFFER_SIZE);

	printf("rsxtest started...\n");

	init_screen(host_addr,HOSTBUFFER_SIZE);
	ioPadInit(7);

	//Init background Image
	png = new pngData;
	pngLoadFromBuffer(wall1_png_bin, wall1_png_bin_size, png);
	png2 = new pngData;
	pngLoadFromBuffer(wall2_png_bin, wall2_png_bin_size, png2);
	

	//Create quad
	quad = createQuad(Point3(-1.0, 0.0, 0), Point3(0.0, 1.0, 0), Point3(-1.0, 1.0, 0), Point3(0.0, 0.0, 0));
	quad3 = createQuad(Point3(0.0, 0.0, 0), Point3(1.0, 1.0, 0), Point3(0.0, 1.0, 0), Point3(1.0, 0.0, 0));
	quad2 = createQuad(Point3(-1.0, -1.0, 0), Point3(0.0, 0.0, 0), Point3(-1.0, 0.0, 0), Point3(0.0, -1.0, 0));
	quad4 = createQuad(Point3(0.0, -1.0, 0), Point3(1.0, 0.0, 0), Point3(0.0, 0.0, 0), Point3(1.0, -1.0, 0));
	quad5 = createQuad(Point3(0.25, 0.25, 0), Point3(0.75, 0.75, 0), Point3(0.25, 0.75, 0), Point3(0.75, 0.25, 0));

	init_shader();
	init_texture();

	DebugFont::init();
	DebugFont::setScreenRes(display_width, display_height);

	atexit(program_exit_callback);
	sysUtilRegisterCallback(0,sysutil_exit_callback,NULL);

	setDrawEnv();
	setRenderTarget(curr_fb);

	running = 1;
	while(running) {
		sysUtilCheckCallback();

		ioPadGetInfo(&padinfo);
		for(int i=0; i < MAX_PADS; i++){
			if(padinfo.status[i]){
				ioPadGetData(i, &paddata);

				if(paddata.BTN_CIRCLE)
					goto done;
				if(paddata.BTN_CROSS)
					selection ^= 1;
			}

		}
		
		drawFrame();
		int ypos = 53;
		int xpos = 10;
		
		DebugFont::setColor(0.5f, 1.0f, 0.5f, 1.0f);
		DebugFont::setPosition(xpos, ypos);
		DebugFont::print("CROSS Button to enable horizontal scanlines");
		ypos += 12;
		DebugFont::setPosition(xpos, ypos);
		DebugFont::print("SQUARE Button to enable vertical scanlines");
		ypos += 12;
		DebugFont::setPosition(xpos, ypos);
		DebugFont::print("TRIANGLE Button to disable scanlines");
		ypos += 12;
		DebugFont::setPosition(xpos, ypos);
		DebugFont::print("CIRCLE Button to exit");

		flip();
	}

done:
    printf("rsxtest done...\n");
	DebugFont::shutdown();
	program_exit_callback();
    return 0;
}
