#include "easyttfont.h"
#include "easyttfontrenderer.h"

f32 EasyTTFontRenderer::sR;
f32 EasyTTFontRenderer::sG;
f32 EasyTTFontRenderer::sB;
f32 EasyTTFontRenderer::sA;

EasyTTFontRenderer::EasyTTFontRenderer()
{
	EasyTTFont::spRenderer = this;
}

EasyTTFontRenderer::~EasyTTFontRenderer()
{

}
