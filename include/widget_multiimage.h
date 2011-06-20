#ifndef _WIDGET_MULTIIMAGE_H
#define _WIDGET_MULTIIMAGE_H

#include "widget.h"
#include "sprite2d.h"
#include "mathvector.h"

#include <string>

class SCENENODE;

class WIDGET_MULTIIMAGE : public WIDGET
{
public:
	WIDGET_MULTIIMAGE();

	~WIDGET_MULTIIMAGE() {};

	virtual WIDGET * clone() const;

	virtual void SetAlpha(SCENENODE & scene, float newalpha);

	virtual void SetVisible(SCENENODE & scene, bool newvis);

	virtual void HookMessage(SCENENODE & scene, const std::string & message, const std::string & from);

	void SetupDrawable(
		SCENENODE & scene,
		TEXTUREMANAGER & textures,
		const std::string & texturesize,
		const std::string & newprefix,
		const std::string & newpostfix,
      	float x, float y, float w, float h,
      	std::ostream & error_output,
	    float z = 0);

private:
	std::string prefix;
	std::string postfix;
	std::string tsize;
	MATHVECTOR <float, 2> center;
	MATHVECTOR <float, 2> dim;
	SPRITE2D s1;
	float draworder;
	bool wasvisible;
	TEXTUREMANAGER * textures;
	std::ostream * errptr;
};

#endif
