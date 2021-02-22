#include "bitmapfnt.h"
#include "bitmapfntrenderer.h"

f32 BitmapFNTRenderer::sR;
f32 BitmapFNTRenderer::sG;
f32 BitmapFNTRenderer::sB;
f32 BitmapFNTRenderer::sA;

BitmapFNTRenderer::BitmapFNTRenderer()
{
	BitmapFNT::spRenderer = this;
}

BitmapFNTRenderer::~BitmapFNTRenderer()
{

}
