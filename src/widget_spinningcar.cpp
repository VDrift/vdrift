#include "widget_spinningcar.h"
#include "car.h"
#include "cfg/ptree.h"
#include "pathmanager.h"

WIDGET_SPINNINGCAR::WIDGET_SPINNINGCAR():
	pathptr(0),
	contentptr(0),
	errptr(0),
	rotation(0),
	wasvisible(false),
	r(1), g(1), b(1)
{
	// ctor
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
	if (from.find("Car") != std::string::npos)
	{
		if (carname == message) return;
		carpaint.clear();	// car changed reset paint
		carname = message;
		reload = true;
	}
	else if (from.find("Paint") != std::string::npos)
	{
		if (carpaint == message) return;
		carpaint = message;
		reload = true;
	}
	else if (from.find("Color") != std::string::npos)
	{
		MATHVECTOR<float, 3> rgb;
		s << message;
		s >> rgb;

		if (fabs(rgb[0] - r) < 1.0/128.0 &&
			fabs(rgb[1] - g) < 1.0/128.0 &&
			fabs(rgb[2] - b) < 1.0/128.0)
		{
			return;
		}

		r = rgb[0];
		g = rgb[1];
		b = rgb[2];
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
	ContentManager & content,
	const PATHMANAGER & pathmanager,
	const float x,
	const float y,
	const MATHVECTOR <float, 3> & newcarpos,
	std::ostream & error_output,
	int order)
{
	center.Set(x,y);
	carpos = newcarpos;
	pathptr = &pathmanager;
	contentptr = &content;
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
	assert(pathptr);
	assert(errptr);
	assert(contentptr);

	Unload(parent);

	std::stringstream loadlog;
	int anisotropy = 0;
	float camerabounce = 0;
	bool damage = false;
	bool debugmode = false;

	std::string partspath = pathptr->GetCarPartsDir();
	std::string carnamebase = carname.substr(0, carname.find("/"));
	std::string cardir = pathptr->GetCarsDir()+"/"+carnamebase;
	std::string carpath = pathptr->GetCarPath(carnamebase);

	PTree carconf;
	file_open_basic fopen(carpath, pathptr->GetCarPartsPath());
	if (!read_ini(carname.substr(carname.find("/")+1), fopen, carconf))
	{
		*errptr << "Error loading car's configfile: " << carpath << std::endl;
		return;
	}

	if (!carnode.valid())
	{
		carnode = parent.AddNode();
	}

	SCENENODE & carnoderef = GetCarNode(parent);
	car.push_back(CAR());

	if (!car.back().LoadGraphics(
			carconf, cardir, carname, partspath,
			MATHVECTOR<float, 3>(r, g, b),
			carpaint, anisotropy,
			camerabounce, damage, debugmode,
			*contentptr, loadlog, loadlog))
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

	MATHVECTOR <float, 3> cartrans = carpos;
	cartrans[0] += center[0];
	cartrans[1] += center[1];
	carnoderef.GetTransform().SetTranslation(cartrans);

	//set initial rotation
	Update(parent, 0);

	carnoderef.SetChildVisibility(wasvisible);

	//if (!loadlog.str().empty()) *errptr << "Loading log: " << loadlog.str() << std::endl;
}

void WIDGET_SPINNINGCAR::SetColor(SCENENODE & scene, float r, float g, float b)
{
	if (!Valid())
		return;

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
