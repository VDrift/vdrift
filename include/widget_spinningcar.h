#ifndef _WIDGET_SPINNINGCAR_H
#define _WIDGET_SPINNINGCAR_H

#include "widget.h"
#include "model_joe03.h"
#include "texture.h"
#include "scenegraph.h"
#include "mathvector.h"
#include "sprite2d.h"
#include "configfile.h"
#include "coordinatesystems.h"
#include "car.h"

#include <string>
#include <cassert>
#include <sstream>

class WIDGET_SPINNINGCAR : public WIDGET
{
public:
	/// this functor copies all normal layers to nocamtrans layers so
	/// the car doesn't get the camera transform
	struct CAMTRANS_FUNCTOR
	{
		void operator()(DRAWABLE_CONTAINER <keyed_container> & drawlist)
		{
			drawlist.nocamtrans_noblend = drawlist.normal_noblend;
			drawlist.normal_noblend.clear();
			drawlist.nocamtrans_blend = drawlist.normal_blend;
			drawlist.normal_blend.clear();
		}
	};
	
	struct CAMTRANS_DRAWABLE_FUNCTOR
	{
		void operator()(DRAWABLE & draw)
		{
			draw.SetCameraTransformEnable(false);
		}
	};
	
private:
	std::string data;
	std::string tsize;
	MATHVECTOR <float, 2> center;
	MATHVECTOR <float, 3> carpos;
	int draworder;
	std::ostream * errptr;
	float rotation;
	std::string lastcarname;
	std::string carname;
	std::string lastcarpaint;
	bool wasvisible;
	
	keyed_container <SCENENODE>::handle carnode;
	
	std::list <CAR> car; ///< only ever one element, please
	
	SCENENODE & GetCarNode(SCENENODE & parent)
	{
		return parent.GetNode(carnode);
	}
	
	void Unload(SCENENODE & parent)
	{
		if (carnode.valid())
			GetCarNode(parent).Clear();
		car.clear();
	}
	
	void Load(SCENENODE & parent, const std::string & carname, const std::string & paintstr)
	{
		Unload(parent);
		
		//std::cout << "Loading car " << carname << ", " << paintstr << std::endl;
		
		std::string carpath = data+"/cars/";
		
		std::stringstream loadlog;
		
		if (!carnode.valid())
		{
			carnode = parent.AddNode();
		}
		
		SCENENODE & carnoderef = GetCarNode(parent);
		
		car.push_back(CAR());
		
		assert(errptr);
		
		CONFIGFILE carconf;
		if (!carconf.Load(carpath+carname+"/"+carname+".car"))
		{
			*errptr << "Error loading car's configfile: " << carpath+carname+"/"+carname+".car" << std::endl;
			return;
		}
		
		MATHVECTOR <float, 3> cartrans = carpos;
		cartrans[0] += center[0];
		cartrans[1] += center[1];
		
		QUATERNION <float> carrot;
		
		if (!car.back().Load(carconf, carpath, "",
			carname, paintstr,
			cartrans, carrot,
			NULL,
			false, SOUNDINFO(0,0,0,0),
			SOUNDBUFFERLIBRARY(),
			0, true, true, "large", 0,
			false, loadlog, loadlog))
		{
			*errptr << "Couldn't load spinning car: " << carname << std::endl;
			if (!loadlog.str().empty())
				*errptr << "Loading log: " << loadlog.str() << std::endl;
			Unload(parent);
			return;
		}
		
		//copy the car's scene to our scene
		carnoderef = car.back().GetNode();
		
		//move all of the drawables to the nocamtrans layer and disable camera transform
		carnoderef.ApplyDrawableContainerFunctor(CAMTRANS_FUNCTOR());
		carnoderef.ApplyDrawableFunctor(CAMTRANS_DRAWABLE_FUNCTOR());
		
		carnoderef.GetTransform().SetTranslation(cartrans);
		
		//set initial rotation
		Update(parent, 0);
		
		carnoderef.SetChildVisibility(wasvisible);
		
		if (!loadlog.str().empty())
			*errptr << "Loading log: " << loadlog.str() << std::endl;
	}
	
public:
	WIDGET_SPINNINGCAR() : errptr(NULL),rotation(0),wasvisible(false)
	{
	}
	virtual WIDGET * clone() const {return new WIDGET_SPINNINGCAR(*this);};
	
	void SetupDrawable(SCENENODE & scene, const std::string & texturesize, const std::string & datapath,
      			   float x, float y, const MATHVECTOR <float, 3> & newcarpos, std::ostream & error_output, int order=0)
	{
		data = datapath;
		tsize = texturesize;
		
		errptr = &error_output;
		
		center.Set(x,y);
		
		carpos = newcarpos;
		
		draworder = order;
	}
	
	virtual void SetAlpha(SCENENODE & scene, float newalpha)
	{
		// TODO:
		//if (!car.empty()) car.back().SetAlpha(newalpha);
	}
	
	virtual void SetVisible(SCENENODE & scene, bool newvis)
	{
		wasvisible = newvis;
		
		//std::cout << "newvis: " << newvis << std::endl;
		
		if (!car.empty())
		{
			GetCarNode(scene).SetChildVisibility(newvis);
		}
		
		if (newvis && car.empty())
		{
			if (!lastcarpaint.empty() && !carname.empty())
			{
				Load(scene, carname, lastcarpaint);
			}
			else
			{
				//std::cout << "Not loading car on visibility change since no carname or carpaint are present:" << std::endl;
				//std::cout << carname << std::endl;
				//std::cout << lastcarpaint << std::endl;
			}
		}
		
		if (!newvis)
		{
			//std::cout << "Unloading spinning car due to visibility" << std::endl;
			Unload(scene);
		}
	}
	
	virtual void HookMessage(SCENENODE & scene, const std::string & message)
	{
		assert(errptr);
		
		//std::cout << "Message: " << message << std::endl;
		
		//if the message is all numbers and two digits, assume it's the car paint
		std::string paintstr;
		if (message.find_first_not_of("0123456789") == std::string::npos && message.length() == 2)
		{
			paintstr = message;
		}
		else
		{
			lastcarname = carname;
			carname = message;
			//the paint should come in as the second message; that's when we do the actual load
			return;
		}
		
		assert(!carname.empty());
		assert(!paintstr.empty());
		
		//if everything is exactly the same don't re-load.  if we're not visible, don't load.
		if (carname == lastcarname && paintstr == lastcarpaint)
		{
			//std::cout << "Not loading car because it's already loaded:" << std::endl;
			//std::cout << carname << "/" << lastcarname << ", " << paintstr << "/" << lastcarpaint << std::endl;
			return;
		}
		else
		{
			//std::cout << "Car/paint change:" << std::endl;
			//std::cout << carname << "/" << lastcarname << ", " << paintstr << "/" << lastcarpaint << std::endl;
		}
		
		lastcarpaint = paintstr;
		
		if (!wasvisible)
		{
			//std::cout << "Not visible, returning" << std::endl;
			return;
		}
		
		Load(scene, carname, paintstr);
	}
	
	virtual void Update(SCENENODE & scene, float dt)
	{
		if (car.empty())
			return;
		
		rotation += dt;
		QUATERNION <float> q;
		q.Rotate(3.141593*1.5,1,0,0);
		q.Rotate(rotation,0,1,0);
		q.Rotate(3.141593*0.1,1,0,0);
		GetCarNode(scene).GetTransform().SetRotation(q);
	}
};

#endif
