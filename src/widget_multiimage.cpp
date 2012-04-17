#include "widget_multiimage.h"
#include "guioption.h"
#include <cassert>

WIDGET_MULTIIMAGE::WIDGET_MULTIIMAGE() :
	wasvisible(false),
	update(false),
	content(0),
	errptr(0)
{
	set_image.call.bind<WIDGET_MULTIIMAGE, &WIDGET_MULTIIMAGE::SetImage>(this);
}

WIDGET_MULTIIMAGE::~WIDGET_MULTIIMAGE()
{
	// dtor
}

void WIDGET_MULTIIMAGE::SetupDrawable(
	SCENENODE & scene,
	ContentManager & content,
	std::map<std::string, GUIOPTION> & optionmap,
	const std::string & option,
	const std::string & newprefix,
	const std::string & newpostfix,
	float x, float y, float w, float h,
	std::ostream & error_output,
	float z)
{
	prefix = newprefix;
	postfix = newpostfix;
	errptr = &error_output;
	this->content = &content;
	center.Set(x, y);
	dim.Set(w, h);
	draworder = z;

	// connect slot
	std::map<std::string, GUIOPTION>::iterator i = optionmap.find(option);
	if (i != optionmap.end())
	{
		set_image.connect(i->second.signal_val);
	}
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

void WIDGET_MULTIIMAGE::Update(SCENENODE & scene, float dt)
{
	if (update)
	{
		assert(errptr);
		assert(content);
		s1.Load(scene, prefix, image + postfix, *content, draworder, *errptr);
		s1.SetToBillboard(center[0] - dim[0] * 0.5, center[1] - dim[1] * 0.5, dim[0], dim[1]);
		if (s1.Loaded()) s1.SetVisible(scene, wasvisible);
		update = false;
	}
}

void WIDGET_MULTIIMAGE::SetImage(const std::string & value)
{
	if (image != value)
	{
		image = value;
		update = true;
	}
}
