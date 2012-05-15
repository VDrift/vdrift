#ifndef _WIDGET_MULTIIMAGE_H
#define _WIDGET_MULTIIMAGE_H

#include "widget.h"
#include "sprite2d.h"
#include "mathvector.h"

class SCENENODE;

class WIDGET_MULTIIMAGE : public WIDGET
{
public:
	WIDGET_MULTIIMAGE();

	~WIDGET_MULTIIMAGE();

	virtual void SetAlpha(SCENENODE & scene, float newalpha);

	virtual void SetVisible(SCENENODE & scene, bool newvis);

	virtual void Update(SCENENODE & scene, float dt);

	void SetupDrawable(
		SCENENODE & scene,
		ContentManager & content,
		std::map<std::string, GUIOPTION> & optionmap,
		const std::string & option,
		const std::string & newprefix,
		const std::string & newpostfix,
      	float x, float y, float w, float h,
      	std::ostream & error_output,
	    float z = 0);

private:
	std::string image;
	std::string prefix;
	std::string postfix;
	std::string tsize;
	MATHVECTOR <float, 2> center;
	MATHVECTOR <float, 2> dim;
	SPRITE2D s1;
	float draworder;
	bool wasvisible;
	bool update;
	ContentManager * content;
	std::ostream * errptr;

	Slot1<const std::string &> set_image;
	void SetImage(const std::string & value);

	WIDGET_MULTIIMAGE(const WIDGET_MULTIIMAGE & other);
};

#endif
