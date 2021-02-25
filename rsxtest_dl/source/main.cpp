﻿#include <stdio.h>
#include <stdarg.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <malloc.h>
#include <ppu-types.h>

#include <io/pad.h>
#include <sys/process.h>
#include <sysutil/sysutil.h>

#include "acid.h"
#include "mesh.h"
#include "rsxutil.h"
#include "fonts_bitmap.h"

#include "diffuse_specular_shader_vpo.h"
#include "diffuse_specular_shader_fpo.h"

#define GCM_APP_WAIT_LABEL_INDEX		128

#define HOSTBUFFER_SIZE		(128*1024*1024)

#define DEGTORAD(a)			( (a) *  0.01745329252f )
#define RADTODEG(a)			( (a) * 57.29577951f )

SYS_PROCESS_PARAM(1001, 0x100000);

u32 running = 0;

vu32 *wait_label = NULL;

u32 fp_offset;
u32 *fp_buffer;

u32 *texture_buffer, *textTemp;
u32 texture_offset;



// vertex shader
rsxProgramConst *projMatrix;
rsxProgramConst *mvMatrix;

// fragment shader
rsxProgramAttrib *textureUnit;
rsxProgramConst *eyePosition;
rsxProgramConst *globalAmbient;
rsxProgramConst *litPosition;
rsxProgramConst *litColor;
rsxProgramConst *Kd;
rsxProgramConst *Ks;
rsxProgramConst *spec;

Point3 eye_pos = Point3(0.0f,0.0f,20.0f);
Point3 eye_dir = Point3(0.0f,0.0f,0.0f);
Vector3 up_vec = Vector3(0.0f,1.0f,0.0f);

void *vp_ucode = NULL;
rsxVertexProgram *vpo = (rsxVertexProgram*)diffuse_specular_shader_vpo;

void *fp_ucode = NULL;
rsxFragmentProgram *fpo = (rsxFragmentProgram*)diffuse_specular_shader_fpo;

static Matrix4 P;
static SMeshBuffer * quad = NULL;

static u32 sLabelValue = 0;

extern "C" {
static void program_exit_callback()
{
    finish();
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

static void init_texture()
{
	u32 i;
	u8 *buffer;
	const u8 *data = acid.pixel_data;

	texture_buffer = (u32*)rsxMemalign(128,(acid.width*acid.height*4));
	if(!texture_buffer) return;

	rsxAddressToOffset(texture_buffer,&texture_offset);

	buffer = (u8*)texture_buffer;
	for(i=0;i<acid.width*acid.height*4;i+=4) {
		buffer[i + 1] = *data++;
		buffer[i + 2] = *data++;
		buffer[i + 3] = *data++;
		buffer[i + 0] = *data++;
	}
}

static SMeshBuffer* createCube(f32 size)
{
	u32 i;
	SMeshBuffer* buffer = new SMeshBuffer();
	const u16 u[36] = { 0,1,2,   0,2,3,   1,4,5,   1,5,2,   4,7,6,	 4,6,5,
						  7,0,3,   7,3,6,   9,2,5,   9,5,8,   0,10,11,   0,7,10 };

	buffer->cnt_indices = 36;
	buffer->indices = (u16*)rsxMemalign(128, buffer->cnt_indices * sizeof(u16));

	for (i = 0; i < 36; i++) buffer->indices[i] = u[i];

	buffer->cnt_vertices = 12;
	buffer->vertices = (S3DVertex*)rsxMemalign(128, buffer->cnt_vertices * sizeof(S3DVertex));

	buffer->vertices[0] = S3DVertex(0, 0, 0, -1, -1, -1, 1, 0);
	buffer->vertices[1] = S3DVertex(1, 0, 0, 1, -1, -1, 1, 1);
	buffer->vertices[2] = S3DVertex(1, 1, 0, 1, 1, -1, 0, 1);
	buffer->vertices[3] = S3DVertex(0, 1, 0, -1, 1, -1, 0, 0);
	buffer->vertices[4] = S3DVertex(1, 0, 1, 1, -1, 1, 1, 0);
	buffer->vertices[5] = S3DVertex(1, 1, 1, 1, 1, 1, 0, 0);
	buffer->vertices[6] = S3DVertex(0, 1, 1, -1, 1, 1, 0, 1);
	buffer->vertices[7] = S3DVertex(0, 0, 1, -1, -1, 1, 1, 1);
	buffer->vertices[8] = S3DVertex(0, 1, 1, -1, 1, 1, 1, 0);
	buffer->vertices[9] = S3DVertex(0, 1, 0, -1, 1, -1, 1, 1);
	buffer->vertices[10] = S3DVertex(1, 0, 1, 1, -1, 1, 0, 1);
	buffer->vertices[11] = S3DVertex(1, 0, 0, 1, -1, -1, 0, 0);

	for (i = 0; i < 12; i++) {
		buffer->vertices[i].pos -= Vector3(0.5f, 0.5f, 0.5f);
		buffer->vertices[i].pos *= size;
	}

	return buffer;
}

static void setTexture(u8 textureUnit)
{
	u32 width = 14;
	u32 height = 14;
	u32 pitch = 14;
	gcmTexture texture;

	if(!texture_buffer) return;

	rsxInvalidateTextureCache(gGcmContext,GCM_INVALIDATE_TEXTURE);

	texture.format		= (GCM_TEXTURE_FORMAT_B8 | GCM_TEXTURE_FORMAT_LIN);
	texture.mipmap		= 1;
	texture.dimension	= GCM_TEXTURE_DIMS_2D;
	texture.cubemap		= GCM_FALSE;
	texture.remap		= ((GCM_TEXTURE_REMAP_TYPE_REMAP << GCM_TEXTURE_REMAP_TYPE_B_SHIFT) |
						   (GCM_TEXTURE_REMAP_TYPE_REMAP << GCM_TEXTURE_REMAP_TYPE_G_SHIFT) |
						   (GCM_TEXTURE_REMAP_TYPE_REMAP << GCM_TEXTURE_REMAP_TYPE_R_SHIFT) |
						   (GCM_TEXTURE_REMAP_TYPE_REMAP << GCM_TEXTURE_REMAP_TYPE_A_SHIFT) |
						   (GCM_TEXTURE_REMAP_COLOR_B << GCM_TEXTURE_REMAP_COLOR_B_SHIFT) |
						   (GCM_TEXTURE_REMAP_COLOR_B << GCM_TEXTURE_REMAP_COLOR_G_SHIFT) |
						   (GCM_TEXTURE_REMAP_COLOR_B << GCM_TEXTURE_REMAP_COLOR_R_SHIFT) |
						   (GCM_TEXTURE_REMAP_COLOR_B << GCM_TEXTURE_REMAP_COLOR_A_SHIFT));
	texture.width		= width;
	texture.height		= height;
	texture.depth		= 1;
	texture.location	= GCM_LOCATION_RSX;
	texture.pitch		= pitch;
	texture.offset		= texture_offset;
	rsxLoadTexture(gGcmContext,textureUnit,&texture);
	rsxTextureControl(gGcmContext,textureUnit,GCM_TRUE,0<<8,12<<8,GCM_TEXTURE_MAX_ANISO_1);
	rsxTextureFilter(gGcmContext,textureUnit,0,GCM_TEXTURE_LINEAR,GCM_TEXTURE_LINEAR,GCM_TEXTURE_CONVOLUTION_QUINCUNX);
	rsxTextureWrapMode(gGcmContext,textureUnit,GCM_TEXTURE_CLAMP_TO_EDGE,GCM_TEXTURE_CLAMP_TO_EDGE,GCM_TEXTURE_CLAMP_TO_EDGE,0,GCM_TEXTURE_ZFUNC_LESS,0);
}

static void setDrawEnv()
{
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

	rsxSetCallCommand(gGcmContext, state_offset);
	while(*wait_label != sLabelValue)
		usleep(10);
	sLabelValue++;

	rsxSetViewport(gGcmContext,x, y, w, h, min, max, scale, offset);
	rsxSetScissor(gGcmContext,x,y,w,h);
}

void init_shader()
{
	u32 fpsize = 0;
	u32 vpsize = 0;

	rsxVertexProgramGetUCode(vpo, &vp_ucode, &vpsize);
	printf("vpsize: %d\n", vpsize);

	projMatrix = rsxVertexProgramGetConst(vpo,"projMatrix");
	mvMatrix = rsxVertexProgramGetConst(vpo,"modelViewMatrix");

	rsxFragmentProgramGetUCode(fpo, &fp_ucode, &fpsize);
	printf("fpsize: %d\n", fpsize);

	fp_buffer = (u32*)rsxMemalign(64,fpsize);
	memcpy(fp_buffer,fp_ucode,fpsize);
	rsxAddressToOffset(fp_buffer,&fp_offset);

	textureUnit = rsxFragmentProgramGetAttrib(fpo,"texture");
	eyePosition = rsxFragmentProgramGetConst(fpo,"eyePosition");
	globalAmbient = rsxFragmentProgramGetConst(fpo,"globalAmbient");
	litPosition = rsxFragmentProgramGetConst(fpo,"lightPosition");
	litColor = rsxFragmentProgramGetConst(fpo,"lightColor");
	spec = rsxFragmentProgramGetConst(fpo,"shininess");
	Ks = rsxFragmentProgramGetConst(fpo,"Ks");
	Kd = rsxFragmentProgramGetConst(fpo,"Kd");
}

void drawFrame()
{
	u32 i,offset,color = 0;
	Matrix4 rotX,rotY;
	Vector4 objEyePos,objLightPos;
	Matrix4 viewMatrix,modelMatrix,modelMatrixIT,modelViewMatrix;
	Point3 lightPos = Point3(250.0f,150.0f,150.0f);
	f32 globalAmbientColor[3] = {0.1f,0.1f,0.1f};
	f32 lightColor[3] = {0.95f,0.95f,0.95f};
	f32 materialColorDiffuse[3] = {0.5f,0.0f,0.0f};
	f32 materialColorSpecular[3] = {0.7f,0.6f,0.6f};
	f32 shininess = 17.8954f;
	static f32 rot = 0.0f;
	SMeshBuffer *mesh = NULL;

	setDrawEnv();
	setTexture(textureUnit->index);

	rsxSetClearColor(gGcmContext,color);
	rsxSetClearDepthStencil(gGcmContext,0xffffff00);
	rsxClearSurface(gGcmContext,GCM_CLEAR_R |
							GCM_CLEAR_G |
							GCM_CLEAR_B |
							GCM_CLEAR_A |
							GCM_CLEAR_S |
							GCM_CLEAR_Z);

	rsxSetZControl(gGcmContext,GCM_FALSE, GCM_TRUE, GCM_TRUE);

	for(i=0;i<8;i++)
		rsxSetViewportClip(gGcmContext,i,display_width,display_height);

	viewMatrix = Matrix4::lookAt(eye_pos,eye_dir,up_vec);

	mesh = quad;
	rotX = Matrix4::rotationX(DEGTORAD(30.0f));
	rotY = Matrix4::rotationY(DEGTORAD(rot));
	modelMatrix = rotX*rotY;
	modelMatrixIT = inverse(modelMatrix);
	modelViewMatrix = transpose(viewMatrix*modelMatrix);

	objEyePos = modelMatrixIT*eye_pos;
	objLightPos = modelMatrixIT*lightPos;

	rsxAddressToOffset(&mesh->vertices[0].pos,&offset);
	rsxBindVertexArrayAttrib(gGcmContext,GCM_VERTEX_ATTRIB_POS,0,offset,sizeof(S3DVertex),3,GCM_VERTEX_DATA_TYPE_F32,GCM_LOCATION_RSX);

	rsxAddressToOffset(&mesh->vertices[0].nrm,&offset);
	rsxBindVertexArrayAttrib(gGcmContext,GCM_VERTEX_ATTRIB_NORMAL,0,offset,sizeof(S3DVertex),3,GCM_VERTEX_DATA_TYPE_F32,GCM_LOCATION_RSX);

	rsxAddressToOffset(&mesh->vertices[0].u,&offset);
	rsxBindVertexArrayAttrib(gGcmContext,GCM_VERTEX_ATTRIB_TEX0,0,offset,sizeof(S3DVertex),2,GCM_VERTEX_DATA_TYPE_F32,GCM_LOCATION_RSX);

	rsxLoadVertexProgram(gGcmContext,vpo,vp_ucode);
	rsxSetVertexProgramParameter(gGcmContext,vpo,projMatrix,(float*)&P);
	rsxSetVertexProgramParameter(gGcmContext,vpo,mvMatrix,(float*)&modelViewMatrix);

	rsxSetFragmentProgramParameter(gGcmContext,fpo,eyePosition,(float*)&objEyePos,fp_offset,GCM_LOCATION_RSX);
	rsxSetFragmentProgramParameter(gGcmContext,fpo,globalAmbient,globalAmbientColor,fp_offset,GCM_LOCATION_RSX);
	rsxSetFragmentProgramParameter(gGcmContext,fpo,litPosition,(float*)&objLightPos,fp_offset,GCM_LOCATION_RSX);
	rsxSetFragmentProgramParameter(gGcmContext,fpo,litColor,lightColor,fp_offset,GCM_LOCATION_RSX);
	rsxSetFragmentProgramParameter(gGcmContext,fpo,spec,&shininess,fp_offset,GCM_LOCATION_RSX);

	rsxSetFragmentProgramParameter(gGcmContext,fpo,Kd,materialColorDiffuse,fp_offset,GCM_LOCATION_RSX);
	rsxSetFragmentProgramParameter(gGcmContext,fpo,Ks,materialColorSpecular,fp_offset,GCM_LOCATION_RSX);

	rsxLoadFragmentProgramLocation(gGcmContext,fpo,fp_offset,GCM_LOCATION_RSX);

	rsxSetUserClipPlaneControl(gGcmContext,GCM_USER_CLIP_PLANE_DISABLE,
									   GCM_USER_CLIP_PLANE_DISABLE,
									   GCM_USER_CLIP_PLANE_DISABLE,
									   GCM_USER_CLIP_PLANE_DISABLE,
									   GCM_USER_CLIP_PLANE_DISABLE,
									   GCM_USER_CLIP_PLANE_DISABLE);

	rsxAddressToOffset(&mesh->indices[0],&offset);
	rsxDrawIndexArray(gGcmContext,GCM_TYPE_TRIANGLES,offset,mesh->cnt_indices,GCM_INDEX_TYPE_16B,GCM_LOCATION_RSX);


	rsxSetWriteTextureLabel(gGcmContext, GCM_APP_WAIT_LABEL_INDEX, sLabelValue);
	rsxFlushBuffer(gGcmContext);
	
	rot += 2.0f;
	if(rot >= 360.0f) rot = fmodf(rot, 360.0f);
}

int main(int argc,const char *argv[])
{
	padInfo padinfo;
	padData paddata;
	
	printf("rsxtest_dl started...\n");

	ioPadInit(7);
	initScreen(HOSTBUFFER_SIZE);

	atexit(program_exit_callback);
	sysUtilRegisterCallback(0,sysutil_exit_callback,NULL);

	wait_label = gcmGetLabelAddress(GCM_APP_WAIT_LABEL_INDEX);
	*wait_label = sLabelValue;
	
	init_shader();
	init_texture();

	quad = createCube(5.0f);

	P = transpose(Matrix4::perspective(DEGTORAD(45.0f),aspect_ratio,1.0f,3000.0f));

	fnt_class alpha;
	init_fnt(gGcmContext, display_width, display_height, 2, &alpha);

	printf("Valore maxwidth: %d height: %d sXPos: %d -sLeftSafe: %d - sXRes: %d\n", alpha.fnt.maxwidth, alpha.fnt.height, alpha.sXPos, alpha.sLeftSafe, alpha.sXRes);

	running = 1;
	while(running) {
		sysUtilCheckCallback();

		ioPadGetInfo(&padinfo);
		for(int i=0; i < MAX_PADS; i++){
			if(padinfo.status[i]){
				ioPadGetData(i, &paddata);

				if(paddata.BTN_CROSS)
					goto done;
			}
		}

        drawFrame();
		alpha.setPosition(46, 35);
		alpha.setDimension(46, 35);
		alpha.setColor(0.4f, 1.0f, 0.8f, 1.0f);
		alpha.printf("Gerpijqy成成PA成 mangia pane a tradimento: %d", 10);

        flip();
    }

done:
    printf("rsxtest_dl done...\n");
	alpha.shutdown(&alpha);
	finish();
    return 0;
}
