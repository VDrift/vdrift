#include "widget_multiimage.h"

#include <cassert>

WIDGET_MULTIIMAGE::WIDGET_MULTIIMAGE() :
	wasvisible(false),
	textures(0),
	errptr(0)
{
	// ctor
}

WIDGET * WIDGET_MULTIIMAGE::clone() const
{
	return new WIDGET_MULTIIMAGE(*this);
}

void WIDGET_MULTIIMAGE::SetupDrawable(
	SCENENODE & scene,
	TEXTUREMANAGER & textures,
	const std::string & texturesize,
	const std::string & newprefix,
	const std::string & newpostfix, 
	float x, float y, float w, float h,
	std::ostream & error_output,
	float z)
{
	prefix = newprefix;
	postfix = newpostfix;
	tsize = texturesize;
	errptr = &error_output;
	this->textures = &textures;
	center.Set(x, y);
	dim.Set(w, h);
	draworder = z;
}

void WIDGET_MULTIIMAGE::SetAlpha(SCENENODE & scene, float newalpha)
{
	if (s1.Loaded()) s1.SetAlpha(scene, newalpha);
}

void WIDGET_MULTIIMAGE::SetVisible(SCENENODE & scene, bool newvis)
{
	wasvisible = newvis;
	if (s1.Loaded()) s1.SetVisible(scene, newvis);
}

void WIDGET_MULTIIMAGE::HookMessage(SCENENODE & scene, const std::string & message, const std::string & from)
{
	assert(errptr);
	assert(textures);

	s1.Load(scene, prefix, message + postfix, tsize, *textures, draworder, *errptr);
	s1.SetToBillboard(center[0] - dim[0] * 0.5, center[1] - dim[1] * 0.5, dim[0], dim[1]);

	if (s1.Loaded()) s1.SetVisible(scene, wasvisible);
}