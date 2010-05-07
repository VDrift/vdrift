#ifndef _WIDGET_MULTIIMAGE_H
#define _WIDGET_MULTIIMAGE_H

#include "widget.h"
#include "mathvector.h"
#include "sprite2d.h"

#include <string>
#include <cassert>

class SCENENODE;

class WIDGET_MULTIIMAGE : public WIDGET
{
private:
	std::string data;
	std::string prefix;
	std::string postfix;
	std::string tsize;
	MATHVECTOR <float, 2> center;
	MATHVECTOR <float, 2> dim;
	int draworder;
	std::ostream * errptr;
	SPRITE2D s1;
	bool wasvisible;
	TEXTUREMANAGER * textures;
	
public:
	WIDGET_MULTIIMAGE() : errptr(NULL), wasvisible(false), textures(NULL) {}

	virtual WIDGET * clone() const {return new WIDGET_MULTIIMAGE(*this);};
	
	void SetupDrawable(
		SCENENODE & scene,
		const std::string & texturesize,
		TEXTUREMANAGER & textures,
		const std::string & datapath,
		const std::string & newprefix,
		const std::string & newpostfix, 
      	float x, float y, float w, float h,
      	std::ostream & error_output,
	    int order=0)
	{
		data = datapath;
		prefix = newprefix;
		postfix = newpostfix;
		tsize = texturesize;
		
		errptr = &error_output;
		this->textures = &textures;
		
		center.Set(x,y);
		dim.Set(w,h);
		
		draworder = order;
	}
	
	virtual void SetAlpha(SCENENODE & scene, float newalpha)
	{
		if (s1.Loaded())
			s1.SetAlpha(scene, newalpha);
	}
	
	virtual void SetVisible(SCENENODE & scene, bool newvis)
	{
		wasvisible = newvis;
		if (s1.Loaded())
			s1.SetVisible(scene, newvis);
	}
	
	virtual void HookMessage(SCENENODE & scene, const std::string & message)
	{
		assert(errptr);
		assert(textures);
		
		std::string filename = data + "/" + prefix + message + postfix;
		s1.Load(scene, filename, tsize, *textures, draworder, *errptr);
		s1.SetToBillboard(center[0]-dim[0]*0.5,center[1]-dim[1]*0.5,dim[0],dim[1]);
		
		if (s1.Loaded())
			s1.SetVisible(scene, wasvisible);
	}
};

#endif
