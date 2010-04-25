#ifndef _WIDGET_MULTIIMAGE_H
#define _WIDGET_MULTIIMAGE_H

#include "widget.h"
#include "mathvector.h"
#include "sprite2d.h"
#include "scenegraph.h"

#include <string>
#include <cassert>

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
	/*SPRITE2D s2;
	SPRITE2D * lastsprite;
	SPRITE2D * activesprite;
	float transtime;*/
	
public:
	WIDGET_MULTIIMAGE() : errptr(NULL),wasvisible(false) {}
	//~WIDGET_MULTIIMAGE() {s1.Unload(parentnode);}
	virtual WIDGET * clone() const {return new WIDGET_MULTIIMAGE(*this);};
	
	void SetupDrawable(SCENENODE & scene, const std::string & texturesize, const std::string & datapath,
			   const std::string & newprefix, const std::string & newpostfix, 
      			   float x, float y, float w, float h, std::ostream & error_output,
	    		   int order=0)
	{
		data = datapath;
		prefix = newprefix;
		postfix = newpostfix;
		tsize = texturesize;
		
		errptr = &error_output;
		
		center.Set(x,y);
		dim.Set(w,h);
		
		draworder = order;
	}
	
	virtual void SetAlpha(SCENENODE & scene, float newalpha)
	{
		/*if (lastsprite)
			lastsprite->SetAlpha(newalpha);
		if (activesprite)
			activesprite->SetAlpha(newalpha);*/
		if (s1.Loaded())
			s1.SetAlpha(scene, newalpha);
	}
	
	virtual void SetVisible(SCENENODE & scene, bool newvis)
	{
		wasvisible = newvis;
		/*if (lastsprite)
			lastsprite->SetVisible(newvis);
		if (activesprite)
			activesprite->SetVisible(newvis);*/
		if (s1.Loaded())
			s1.SetVisible(scene, newvis);
	}
	
	virtual void HookMessage(SCENENODE & scene, const std::string & message)
	{
		assert(errptr);
		
		std::string filename = data+"/"+prefix+message+postfix;
		//std::cout << "Will load: " << filename << std::endl;
		/*lastsprite = activesprite;
		
		//find a free sprite to use
		if (lastsprite == &s1)
			activesprite = &s2;
		else
			activesprite = &s1;
		
		activesprite->Load(parentnode, filename, tsize, draworder, *errptr);
		activesprite->SetToBillboard(center[0],center[1],dim[0],dim[1]);*/
		s1.Load(scene, filename, tsize, draworder, *errptr);
		s1.SetToBillboard(center[0]-dim[0]*0.5,center[1]-dim[1]*0.5,dim[0],dim[1]);
		
		if (s1.Loaded())
			s1.SetVisible(scene, wasvisible);
	}
	
	/*virtual bool ProcessInput(float cursorx, float cursory, bool cursordown, bool cursorjustup)
	{
		float trans = transtime/0.5;
		if (transtime < 0)
			trans = 0.0;
		else
			transtime -= 0.004;
		
		if (lastsprite)
			lastsprite->SetAlpha(trans);
		if (activesprite)
			activesprite->SetAlpha(1.0-trans);
		
		return false;
	}*/
};

#endif
