#include "widget_spinningcar.h"

#include "car.h"
#include "configfile.h"

WIDGET_SPINNINGCAR::WIDGET_SPINNINGCAR():
	errptr(NULL), rotation(0), wasvisible(false), r(1), g(1), b(1), textures(NULL)
{
	
}

WIDGET * WIDGET_SPINNINGCAR::clone() const
{
	return new WIDGET_SPINNINGCAR(*this);
}

void WIDGET_SPINNINGCAR::SetAlpha(SCENENODE & scene, float newalpha)
{
	// TODO:
	//if (!car.empty()) car.back().SetAlpha(newalpha);
}

void WIDGET_SPINNINGCAR::SetVisible(SCENENODE & scene, bool newvis)
{
	wasvisible = newvis;
	
	//std::cout << "newvis: " << newvis << std::endl;
	
	if (!car.empty())
	{
		GetCarNode(scene).SetChildVisibility(newvis);
	}
	
	if (newvis && car.empty())
	{
		if (!carpaint.empty() && !carname.empty())
		{
			Load(scene);
			//std::cout << "Loading car on visibility change:" << std::endl;
		}
		else
		{
			//std::cout << "Not loading car on visibility change since no carname or carpaint are present:" << std::endl;
		}
	}
	
	if (!newvis && !car.empty())
	{
		//std::cout << "Unloading spinning car due to visibility" << std::endl;
		Unload(scene);
	}
	
	//std::cout << carname << std::endl;
	//std::cout << carpaint << std::endl;
}

void WIDGET_SPINNINGCAR::HookMessage(SCENENODE & scene, const std::string & message, const std::string & from)
{
	assert(errptr);
	
	bool reload(false);
	std::stringstream s;
	if (from.find("CarWheel") != std::string::npos)
	{
		if (carname == message) return;
		carpaint.clear();	// car changed reset paint
		carname = message;
		reload = true;
	}
	else if (from.find("PaintWheel") != std::string::npos)
	{
		if (carpaint == message) return;
		carpaint = message;
		reload = true;
	}
	else if (from.find("Red") != std::string::npos)
	{
		float value;
		s << message;
		s >> value;
		if (fabs(value - r) < 1.0/128.0) return;
		r = value;	
	}
	else if (from.find("Green") != std::string::npos)
	{
		float value;
		s << message;
		s >> value;
		if (fabs(value - g) < 1.0/128.0) return;
		g = value;
			
	}
	else if (from.find("Blue") != std::string::npos)
	{
		float value;
		s << message;
		s >> value;
		if (fabs(value - b) < 1.0/128.0) return;
		b = value;	
	}

	if (!wasvisible)
	{
		return;
	}

	if (!carpaint.empty() && !carname.empty() && reload)
	{
		Load(scene);
	}
	
	if (!car.empty())
	{
		SetColor(scene, r, g, b);
	}
}

void WIDGET_SPINNINGCAR::Update(SCENENODE & scene, float dt)
{
	if (car.empty()) return;
	
	rotation += dt;
	QUATERNION <float> q;
	q.Rotate(3.141593*1.5, 1, 0, 0);
	q.Rotate(rotation, 0, 1, 0);
	q.Rotate(3.141593*0.1, 1, 0, 0);
	GetCarNode(scene).GetTransform().SetRotation(q);
}

void WIDGET_SPINNINGCAR::SetupDrawable(
	SCENENODE & scene,
	const std::string & texturesize,
	const std::string & datapath,
	float x,
	float y,
	const MATHVECTOR <float, 3> & newcarpos,
	MANAGER<TEXTURE, TEXTUREINFO> & textures,
	std::ostream & error_output,
	int order)
{
	tsize = texturesize;
	data = datapath;
	center.Set(x,y);
	carpos = newcarpos;
	this->textures = &textures;
	errptr = &error_output;
	draworder = order;
}

SCENENODE & WIDGET_SPINNINGCAR::GetCarNode(SCENENODE & parent)
{
	return parent.GetNode(carnode);
}

void WIDGET_SPINNINGCAR::Unload(SCENENODE & parent)
{
	if (carnode.valid())
	{
		GetCarNode(parent).Clear();
	}
	car.clear();
}

/// this functor copies all normal layers to nocamtrans layers so
/// the car doesn't get the camera transform
struct CAMTRANS_FUNCTOR
{
	void operator()(DRAWABLE_CONTAINER <keyed_container> & drawlist)
	{
		drawlist.nocamtrans_noblend = drawlist.car_noblend;
		drawlist.car_noblend.clear();
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

void WIDGET_SPINNINGCAR::Load(SCENENODE & parent)
{
	assert(errptr);
	assert(textures);
	
	Unload(parent);
	
	std::string carpath = data + "/cars/" + carname;
	std::stringstream loadlog;

	if (!carnode.valid())
	{
		carnode = parent.AddNode();
	}
	
	SCENENODE & carnoderef = GetCarNode(parent);
	car.push_back(CAR());
	
	CONFIGFILE carconf;
	if (!carconf.Load(carpath + "/" + carname + ".car"))
	{
		*errptr << "Error loading car's configfile: " << carpath + "/" + carname + ".car" << std::endl;
		return;
	}
	
	MATHVECTOR <float, 3> carcolor(r, g, b);
	
	MATHVECTOR <float, 3> cartrans = carpos;
	cartrans[0] += center[0];
	cartrans[1] += center[1];
	
	QUATERNION <float> carrot;
	
	if (!car.back().Load(
		carconf, carpath, "", carname, 
		*textures, carpaint, carcolor,
		cartrans, carrot, NULL,
		false, SOUNDINFO(),
		SOUNDBUFFERLIBRARY(),
		0, true, true, "large", 0,
		false, data + "/carparts", loadlog, loadlog))
	{
		*errptr << "Couldn't load spinning car: " << carname << std::endl;
		if (!loadlog.str().empty())
		{
			*errptr << "Loading log: " << loadlog.str() << std::endl;
		}
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

	//if (!loadlog.str().empty()) *errptr << "Loading log: " << loadlog.str() << std::endl;
}

void WIDGET_SPINNINGCAR::SetColor(SCENENODE & scene, float r, float g, float b)
{
	// save the current state of the transform
	SCENENODE & carnoderef = GetCarNode(scene);
	TRANSFORM oldtrans = carnoderef.GetTransform();
	
	// set the new car color
	car.back().SetColor(r,g,b);
	
	// re-copy nodes from the car to our GUI node
	carnoderef = car.back().GetNode();
	carnoderef.ApplyDrawableContainerFunctor(CAMTRANS_FUNCTOR());
	carnoderef.ApplyDrawableFunctor(CAMTRANS_DRAWABLE_FUNCTOR());
	
	// restore the original state
	carnoderef.SetTransform(oldtrans);
	Update(scene, 0);
	carnoderef.SetChildVisibility(wasvisible);
}
