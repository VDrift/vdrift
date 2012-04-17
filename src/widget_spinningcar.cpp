#include "widget_spinningcar.h"
#include "guioption.h"
#include "pathmanager.h"
#include "cfg/ptree.h"
#include "car.h"

WIDGET_SPINNINGCAR::WIDGET_SPINNINGCAR():
	pathptr(0),
	contentptr(0),
	errptr(0),
	rotation(0),
	wasvisible(false),
	updatecolor(false),
	updatecar(false),
	car(0)
{
	set_car.call.bind<WIDGET_SPINNINGCAR, &WIDGET_SPINNINGCAR::SetCar>(this);
	set_paint.call.bind<WIDGET_SPINNINGCAR, &WIDGET_SPINNINGCAR::SetPaint>(this);
	set_color.call.bind<WIDGET_SPINNINGCAR, &WIDGET_SPINNINGCAR::SetColor>(this);
}

WIDGET_SPINNINGCAR::~WIDGET_SPINNINGCAR()
{
	if (car) delete car;
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

	if (car)
	{
		GetCarNode(scene).SetChildVisibility(newvis);
	}

	if (newvis && !car)
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

	if (!newvis && car)
	{
		//std::cout << "Unloading spinning car due to visibility" << std::endl;
		Unload(scene);
	}
}

void WIDGET_SPINNINGCAR::Update(SCENENODE & scene, float dt)
{
	if (!car) return;

	if (updatecar)
	{
		Load(scene);
		updatecar = false;
		updatecolor = false;
	}

	if (updatecolor)
	{
		float r = float((carcolor >> 16) & 255) / 255;
		float g = float((carcolor >> 8) & 255) / 255;
		float b = float((carcolor >> 0) & 255) / 255;
		SetColor(scene, r, g, b);
		updatecolor = false;
	}

	Rotate(scene, dt);
}

void WIDGET_SPINNINGCAR::SetupDrawable(
	SCENENODE & scene,
	ContentManager & content,
	const PATHMANAGER & pathmanager,
	std::map<std::string, GUIOPTION> & optionmap,
	const float x, const float y,
	const MATHVECTOR <float, 3> & newcarpos,
	const std::string & option,
	std::ostream & error_output,
	int order)
{
	center.Set(x,y);
	carpos = newcarpos;
	pathptr = &pathmanager;
	contentptr = &content;
	errptr = &error_output;
	draworder = order;

	// connect slots
	std::map<std::string, GUIOPTION>::iterator i;
	i = optionmap.find(option);
	if (i != optionmap.end())
		set_car.connect(i->second.signal_val);
	i = optionmap.find(option + "_paint");
	if (i != optionmap.end())
		set_paint.connect(i->second.signal_val);
	i = optionmap.find(option + "_color");
	if (i != optionmap.end())
		set_color.connect(i->second.signal_val);
}

void WIDGET_SPINNINGCAR::SetCar(const std::string & name)
{
	if (carname != name)
	{
		carname = name;
		updatecar = true;
	}
}

void WIDGET_SPINNINGCAR::SetPaint(const std::string & paint)
{
	if (carpaint != paint)
	{
		carpaint = paint;
		updatecar = true;
	}
}

void WIDGET_SPINNINGCAR::SetColor(const std::string & colorstr)
{
	std::stringstream s;
	unsigned color;
	s << colorstr;
	s >> color;
	if (carcolor != color)
	{
		carcolor = color;
		updatecolor = true;
	}
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

	if (car)
	{
		delete car;
		car = 0;
	}
}

void WIDGET_SPINNINGCAR::Rotate(SCENENODE & scene, float delta)
{
	rotation += delta;
	QUATERNION <float> q;
	q.Rotate(M_PI * 1.5, 1, 0, 0);
	q.Rotate(rotation, 0, 1, 0);
	q.Rotate(M_PI * 0.1, 1, 0, 0);
	GetCarNode(scene).GetTransform().SetRotation(q);
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

	PTree carconf;
	file_open_basic fopen(pathptr->GetCarPath(carname), pathptr->GetCarPartsPath());
	if (!read_ini(carname + ".car", fopen, carconf))
	{
		*errptr << "Failed to load " << carname << std::endl;
		return;
	}

	if (!carnode.valid())
	{
		carnode = parent.AddNode();
	}

	SCENENODE & carnoderef = GetCarNode(parent);
	car = new CAR();

	std::string partspath = pathptr->GetCarPartsDir();
	std::string cardir = pathptr->GetCarsDir() + "/" + carname;
	if (!car->LoadGraphics(
			carconf, partspath, cardir, carname, carpaint, carcolor,
			anisotropy, camerabounce, damage, debugmode,
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

	// copy the car's scene to our scene
	carnoderef = car->GetNode();

	// move all of the drawables to the nocamtrans layer and disable camera transform
	carnoderef.ApplyDrawableContainerFunctor(CAMTRANS_FUNCTOR());
	carnoderef.ApplyDrawableFunctor(CAMTRANS_DRAWABLE_FUNCTOR());

	// set transform and visibility
	MATHVECTOR <float, 3> cartrans = carpos;
	cartrans[0] += center[0];
	cartrans[1] += center[1];
	carnoderef.GetTransform().SetTranslation(cartrans);

	Rotate(parent, 0);
	carnoderef.SetChildVisibility(wasvisible);
}

void WIDGET_SPINNINGCAR::SetColor(SCENENODE & scene, float r, float g, float b)
{
	if (!Valid())
		return;

	// save the current state of the transform
	SCENENODE & carnoderef = GetCarNode(scene);
	TRANSFORM oldtrans = carnoderef.GetTransform();

	// set the new car color
	car->SetColor(r, g, b);

	// re-copy nodes from the car to our GUI node
	carnoderef = car->GetNode();
	carnoderef.ApplyDrawableContainerFunctor(CAMTRANS_FUNCTOR());
	carnoderef.ApplyDrawableFunctor(CAMTRANS_DRAWABLE_FUNCTOR());

	// restore the original state
	carnoderef.SetTransform(oldtrans);
	Rotate(scene, 0);
	carnoderef.SetChildVisibility(wasvisible);
}
