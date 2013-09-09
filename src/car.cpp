/************************************************************************/
/*                                                                      */
/* This file is part of VDrift.                                         */
/*                                                                      */
/* VDrift is free software: you can redistribute it and/or modify       */
/* it under the terms of the GNU General Public License as published by */
/* the Free Software Foundation, either version 3 of the License, or    */
/* (at your option) any later version.                                  */
/*                                                                      */
/* VDrift is distributed in the hope that it will be useful,            */
/* but WITHOUT ANY WARRANTY; without even the implied warranty of       */
/* MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the        */
/* GNU General Public License for more details.                         */
/*                                                                      */
/* You should have received a copy of the GNU General Public License    */
/* along with VDrift.  If not, see <http://www.gnu.org/licenses/>.      */
/*                                                                      */
/************************************************************************/

#include "car.h"
#include "carinput.h"
#include "content/contentmanager.h"
#include "physics/dynamicsworld.h"
#include "physics/tracksurface.h"
#include "graphics/textureinfo.h"
#include "graphics/mesh_gen.h"
#include "graphics/model_obj.h"
#include "cfg/ptree.h"
#include "loaddrawable.h"
#include "loadcamera.h"
#include "camera.h"

#include <map>
#include <list>
#include <vector>
#include <sstream>
#include <string>

template <typename T>
static inline T clamp(T val, T min, T max)
{
	return (val < max) ? (val > min) ? val : min : max;
}

enum DrawlistEnum
{
	BLEND,
	NOBLEND,
	EMISSIVE,
	OMNI
};

static keyed_container <Drawable> & GetDrawlist(SceneNode & node, DrawlistEnum which)
{
	switch (which)
	{
		case BLEND:
		return node.GetDrawlist().normal_blend;

		case NOBLEND:
		return node.GetDrawlist().car_noblend;

		case EMISSIVE:
		return node.GetDrawlist().lights_emissive;

		case OMNI:
		return node.GetDrawlist().lights_omni;
	};
	assert(0);
	return node.GetDrawlist().car_noblend;
}

struct LoadBody
{
	SceneNode & topnode;
	keyed_container<SceneNode>::handle & bodynode;
	LoadDrawable & loadDrawable;

	LoadBody(
		SceneNode & topnode,
		keyed_container<SceneNode>::handle & bodynode,
		LoadDrawable & loadDrawable) :
		topnode(topnode),
		bodynode(bodynode),
		loadDrawable(loadDrawable)
	{
		// ctor
	}

	bool operator()(const PTree & cfg)
	{
		const PTree * link;
		if (cfg.get("link", link))
		{
			// load breakable body drawables
			if (!loadDrawable(cfg, topnode)) return false;
		}
		else
		{
			// load fixed body drawables
			if (!loadDrawable(cfg, topnode.GetNode(bodynode))) return false;
		}
		return true;
	}
};

static bool LoadWheel(
	const PTree & cfg_wheel,
	struct LoadDrawable & loadDrawable,
	SceneNode & topnode,
	std::ostream & error_output)
{
	keyed_container<SceneNode>::handle wheelnode = topnode.AddNode();
	ContentManager & content = loadDrawable.content;
	const std::string& path = loadDrawable.path;

	std::string meshname;
	std::vector<std::string> texname;
	std::tr1::shared_ptr<Model> mesh;
	const PTree * cfg_tire;
	Vec3 size(0);
	std::string sizestr;

	if (!cfg_wheel.get("mesh", meshname, error_output)) return false;
	if (!cfg_wheel.get("texture", texname, error_output)) return false;
	if (!cfg_wheel.get("tire", cfg_tire, error_output)) return false;
	if (!cfg_tire->get("size", sizestr, error_output)) return false;
	if (!cfg_tire->get("size", size, error_output)) return false;

	// load wheel
	bool genrim = true;
	cfg_wheel.get("genrim", genrim);
	if (genrim)
	{
		// get wheel disk mesh
		content.load(mesh, path, meshname);

		// gen wheel mesh
		meshname = meshname + sizestr;
		if (!content.get(mesh, path, meshname))
		{
			float width = size[0] * 0.001;
			float diameter = size[2] * 0.0254;

			VertexArray rimva, diskva;
			MeshGen::mg_rim(rimva, size[0], size[1], size[2], 10);
			diskva = mesh->GetVertexArray();
			diskva.Translate(-0.75 * 0.5, 0, 0);
			diskva.Scale(width, diameter, diameter);
			content.load(mesh, path, meshname, rimva + diskva);

			//MODEL_OBJ mo("wheel.obj", error_output);
			//mo.SetVertexArray(mesh->GetVertexArray());
			//mo.Save("wheel.obj", error_output);
		}
	}

	if (!loadDrawable(meshname, texname, cfg_wheel, topnode, &wheelnode))
	{
		return false;
	}

	// load tire (optional)
	texname.clear();
	if (cfg_tire->get("texture", texname))
	{
		meshname.clear();
		if (!cfg_tire->get("mesh", meshname))
		{
			// gen tire mesh
			meshname = "tire" + sizestr;
			if (!content.get(mesh, path, meshname))
			{
				VertexArray tireva;
				MeshGen::mg_tire(tireva, size[0], size[1], size[2]);
				content.load(mesh, path, meshname, tireva);

				//MODEL_OBJ mo("wheel.obj", error_output);
				//mo.SetVertexArray(mesh->GetVertexArray());
				//mo.Save("tire.obj", error_output);
			}
		}

		if (!loadDrawable(meshname, texname, *cfg_tire, topnode.GetNode(wheelnode)))
		{
			return false;
		}
	}

	// load brake (optional)
	texname.clear();
	const PTree * cfg_brake;
	if (cfg_wheel.get("brake", cfg_brake, error_output) &&
		cfg_brake->get("texture", texname))
	{
		float radius;
		std::string radiusstr;
		cfg_brake->get("radius", radius);
		cfg_brake->get("radius", radiusstr);

		meshname.clear();
		if (!cfg_brake->get("mesh", meshname))
		{
			// gen brake disk mesh
			meshname = "brake" + radiusstr;
			if (!content.get(mesh, path, meshname))
			{
				float diameter_mm = radius * 2 * 1000;
				float thickness_mm = 0.025 * 1000;
				VertexArray brakeva;
				MeshGen::mg_brake_rotor(brakeva, diameter_mm, thickness_mm);
				content.load(mesh, path, meshname, brakeva);
			}
		}

		if (!loadDrawable(meshname, texname, *cfg_brake, topnode.GetNode(wheelnode)))
		{
			return false;
		}
	}

	return true;
}

Car::Car() :
	steer_angle_max(0),
	last_steer(0),
	nos_active(false),
	driver_view(false),
	sector(-1),
	applied_brakes(0)
{
	// ctor
}

Car::~Car()
{
	for (size_t i = 0; i < cameras.size(); ++i)
		delete cameras[i];
}

bool Car::LoadLight(
	const PTree & cfg,
	ContentManager & content,
	std::ostream & error_output)
{
	float radius;
	std::string radiusstr;
	Vec3 pos(0), col(0);
	if (!cfg.get("position", pos, error_output)) return false;
	if (!cfg.get("color", col, error_output)) return false;
	if (!cfg.get("radius", radius, error_output)) return false;
	cfg.get("radius", radiusstr);

	lights.push_back(Light());

	SceneNode & bodynoderef = topnode.GetNode(bodynode);
	lights.back().node = bodynoderef.AddNode();

	SceneNode & node = bodynoderef.GetNode(lights.back().node);
	node.GetTransform().SetTranslation(Vec3(pos[0], pos[1], pos[2]));

	std::tr1::shared_ptr<Model> mesh;
	if (!content.get(mesh, "", "cube" + radiusstr))
	{
		VertexArray varray;
		varray.SetToUnitCube();
		varray.Scale(radius, radius, radius);
		content.load(mesh, "", "cube" + radiusstr, varray);
	}
	models.insert(mesh);

	keyed_container <Drawable> & dlist = GetDrawlist(node, OMNI);
	lights.back().draw = dlist.insert(Drawable());

	Drawable & draw = dlist.get(lights.back().draw);
	draw.SetColor(col[0], col[1], col[2]);
	draw.SetModel(*mesh);
	draw.SetCull(true, true);
	draw.SetDrawEnable(false);

	return true;
}

bool Car::LoadGraphics(
	const PTree & cfg,
	const std::string & carpath,
	const std::string & carname,
	const std::string & carwheel,
	const std::string & carpaint,
	const Vec3 & carcolor,
	const int anisotropy,
	const float camerabounce,
	ContentManager & content,
	std::ostream & error_output)
{
	//write_inf(cfg, std::cerr);
	cartype = carname;

	// init drawable load functor
	LoadDrawable loadDrawable(carpath, anisotropy, content, models, textures, error_output);

	// load body first
	const PTree * cfg_body;
	std::string meshname;
	std::vector<std::string> texname;
	if (!cfg.get("body", cfg_body, error_output)) return false;
	if (!cfg_body->get("mesh", meshname, error_output)) return false;
	if (!cfg_body->get("texture", texname, error_output)) return false;
	if (carpaint != "default") texname[0] = carpaint;
	if (!loadDrawable(meshname, texname, *cfg_body, topnode, &bodynode)) return false;

	// load wheels
	const PTree * cfg_wheels;
	if (!cfg.get("wheel", cfg_wheels, error_output)) return false;

	std::tr1::shared_ptr<PTree> sel_wheel;
	if (carwheel != "default" && !content.load(sel_wheel, carpath, carwheel)) return false;

	for (PTree::const_iterator i = cfg_wheels->begin(); i != cfg_wheels->end(); ++i)
	{
		const PTree * cfg_wheel = &i->second;

		// override default wheel with selected, not very efficient, fixme
		PTree opt_wheel;
		if (sel_wheel.get())
		{
			opt_wheel.set(*sel_wheel);
			opt_wheel.merge(*cfg_wheel);
			cfg_wheel = &opt_wheel;
		}

		if (!LoadWheel(*cfg_wheel, loadDrawable, topnode, error_output))
		{
			error_output << "Failed to load wheels." << std::endl;
			return false;
		}
	}

	// load drawables
	LoadBody loadBody(topnode, bodynode, loadDrawable);
	for (PTree::const_iterator i = cfg.begin(); i != cfg.end(); ++i)
	{
		if (i->first != "body" &&
			i->first != "steering" &&
			i->first != "light-brake" &&
			i->first != "light-reverse")
		{
			loadBody(i->second);
		}
	}

	// load steering wheel
	const PTree * cfg_steer;
	if (cfg.get("steering", cfg_steer))
	{
		SceneNode & bodynoderef = topnode.GetNode(bodynode);
		if (!loadDrawable(*cfg_steer, bodynoderef, &steernode, 0))
		{
			error_output << "Failed to load steering wheel." << std::endl;
			return false;
		}
		cfg_steer->get("max-angle", steer_angle_max);
		steer_angle_max = steer_angle_max / 180.0 * M_PI;
		SceneNode & steernoderef = bodynoderef.GetNode(steernode);
		steer_orientation = steernoderef.GetTransform().GetRotation();
	}

	// load brake/reverse light point light sources (optional)
	int i = 0;
	std::string istr = "0";
	const PTree * cfg_light;
	while (cfg.get("light-brake-"+istr, cfg_light))
	{
		if (!LoadLight(*cfg_light, content, error_output))
		{
			error_output << "Failed to load lights." << std::endl;
			return false;
		}

		std::stringstream sstr;
		sstr << ++i;
		istr = sstr.str();
	}
	i = 0;
	istr = "0";
	while (cfg.get("light-reverse-"+istr, cfg_light))
	{
		if (!LoadLight(*cfg_light, content, error_output))
		{
			error_output << "Failed to load lights." << std::endl;
			return false;
		}

		std::stringstream sstr;
		sstr << ++i;
		istr = sstr.str();
	}

	// load car brake/reverse graphics (optional)
	if (cfg.get("light-brake", cfg_light))
	{
		SceneNode & bodynoderef = topnode.GetNode(bodynode);
		if (!loadDrawable(*cfg_light, bodynoderef, 0, &brakelights))
		{
			error_output << "Failed to load lights." << std::endl;
			return false;
		}
	}
	if (cfg.get("light-reverse", cfg_light))
	{
		SceneNode & bodynoderef = topnode.GetNode(bodynode);
		if (!loadDrawable(*cfg_light, bodynoderef, 0, &reverselights))
		{
			error_output << "Failed to load lights." << std::endl;
			return false;
		}
	}

	const PTree * cfg_cams;
	if (!cfg.get("camera", cfg_cams))
	{
		return false;
	}
	if (!cfg_cams->size())
	{
		error_output << "No cameras defined." << std::endl;
		return false;
	}
	cameras.reserve(cfg_cams->size());
	for (PTree::const_iterator i = cfg_cams->begin(); i != cfg_cams->end(); ++i)
	{
		Camera * cam = LoadCamera(i->second, camerabounce, error_output);
		if (!cam) return false;
		cameras.push_back(cam);
	}

	SetColor(carcolor[0], carcolor[1], carcolor[2]);

	return true;
}

bool Car::LoadPhysics(
	std::ostream & error_output,
	ContentManager & content,
	DynamicsWorld & world,
	const PTree & cfg,
	const std::string & carpath,
	const std::string & cartire,
	const Vec3 & initial_position,
	const Quat & initial_orientation,
	const bool defaultabs,
	const bool defaulttcs,
	const bool damage)
{
	std::string carmodel;
	if (!cfg.get("body.mesh", carmodel, error_output))
		return false;

	std::tr1::shared_ptr<Model> model;
	content.load(model, carpath, carmodel);

	btVector3 size = ToBulletVector(model->GetSize());
	btVector3 center = ToBulletVector(model->GetCenter());
	btVector3 position = ToBulletVector(initial_position);
	btQuaternion rotation = ToBulletQuaternion(initial_orientation);

	if (!dynamics.Load(
		error_output, content, world,
		cfg, carpath, cartire,
		size, center, position, rotation,
		damage))
	{
		return false;
	}

	dynamics.SetABS(defaultabs);
	dynamics.SetTCS(defaulttcs);

	mz_nominalmax = 0.05f * 9.81f / dynamics.GetInvMass(); // fixme: make this a steering feedback parameter

	return true;
}

bool Car::LoadSounds(
	const std::string & carpath,
	const std::string & carname,
	Sound & soundsystem,
	ContentManager & content,
	std::ostream & error_output)
{
	return sound.Load(carpath, carname, soundsystem, content, error_output);
}

void Car::SetColor(float r, float g, float b)
{
	SceneNode & bodynoderef = topnode.GetNode(bodynode);
	keyed_container<Drawable> & car_noblend = bodynoderef.GetDrawlist().car_noblend;
	for (keyed_container<Drawable>::iterator i = car_noblend.begin(); i != car_noblend.end(); ++i)
	{
		i->SetColor(r, g, b, 1);
	}
}

void Car::SetPosition(const Vec3 & new_position)
{
	btVector3 newpos = ToBulletVector(new_position);
	dynamics.SetPosition(newpos);
	dynamics.AlignWithGround();
}

void Car::UpdateGraphics()
{
	if (!bodynode.valid()) return;
	assert(dynamics.GetNumBodies() == topnode.Nodes());

	unsigned int i = 0;
	keyed_container<SceneNode> & childlist = topnode.GetNodelist();
	for (keyed_container<SceneNode>::iterator ni = childlist.begin(); ni != childlist.end(); ++ni, ++i)
	{
		Vec3 pos = ToMathVector<float>(dynamics.GetPosition(i));
		Quat rot = ToQuaternion<float>(dynamics.GetOrientation(i));
		ni->GetTransform().SetTranslation(pos);
		ni->GetTransform().SetRotation(rot);
	}

	// brake/reverse lights
	SceneNode & bodynoderef = topnode.GetNode(bodynode);
	for (std::list<Light>::iterator i = lights.begin(); i != lights.end(); i++)
	{
		SceneNode & node = bodynoderef.GetNode(i->node);
		Drawable & draw = GetDrawlist(node, OMNI).get(i->draw);
		draw.SetDrawEnable(applied_brakes > 0);
	}
	if (brakelights.valid())
	{
		GetDrawlist(bodynoderef, EMISSIVE).get(brakelights).SetDrawEnable(applied_brakes > 0);
	}
	if (reverselights.valid())
	{
		GetDrawlist(bodynoderef, EMISSIVE).get(reverselights).SetDrawEnable(GetGear() < 0);
	}

	// steering
	if (steernode.valid())
	{
		SceneNode & steernoderef = bodynoderef.GetNode(steernode);
		steernoderef.GetTransform().SetRotation(steer_rotation);
	}
}

void Car::Update(double dt)
{
	UpdateGraphics();
	sound.Update(dynamics, dt);
}

void Car::HandleInputs(const std::vector <float> & inputs)
{
	 // ensure that our inputs vector contains exactly one item per input
	assert(inputs.size() == CarInput::INVALID);

	// recover from a rollover
	if (inputs[CarInput::ROLLOVER_RECOVER])
		dynamics.RolloverRecover();

	// set brakes
	dynamics.SetBrake(inputs[CarInput::BRAKE]);
	dynamics.SetHandBrake(inputs[CarInput::HANDBRAKE]);

	// do steering
	float steer_value = inputs[CarInput::STEER_RIGHT];
	if (std::abs(inputs[CarInput::STEER_LEFT]) > std::abs(inputs[CarInput::STEER_RIGHT])) //use whichever control is larger
		steer_value = -inputs[CarInput::STEER_LEFT];
	dynamics.SetSteering(steer_value);
	last_steer = steer_value;
	Quat steer;
	steer.Rotate(-steer_value * steer_angle_max, 0, 0, 1);
	steer_rotation = steer_orientation * steer;

    // start the engine if requested
	if (inputs[CarInput::START_ENGINE])
		dynamics.StartEngine();

	// do shifting
	int gear_change = 0;
	if (inputs[CarInput::SHIFT_UP] == 1.0)
		gear_change = 1;
	if (inputs[CarInput::SHIFT_DOWN] == 1.0)
		gear_change = -1;
	int cur_gear = dynamics.GetTransmission().GetGear();
	int new_gear = cur_gear + gear_change;

	if (inputs[CarInput::REVERSE])
		new_gear = -1;
	if (inputs[CarInput::NEUTRAL])
		new_gear = 0;
	if (inputs[CarInput::FIRST_GEAR])
		new_gear = 1;
	if (inputs[CarInput::SECOND_GEAR])
		new_gear = 2;
	if (inputs[CarInput::THIRD_GEAR])
		new_gear = 3;
	if (inputs[CarInput::FOURTH_GEAR])
		new_gear = 4;
	if (inputs[CarInput::FIFTH_GEAR])
		new_gear = 5;
	if (inputs[CarInput::SIXTH_GEAR])
		new_gear = 6;

	applied_brakes = inputs[CarInput::BRAKE];

	float throttle = inputs[CarInput::THROTTLE];
	float clutch = 1 - inputs[CarInput::CLUTCH];
	float nos = inputs[CarInput::NOS];

	nos_active = nos > 0;

	dynamics.ShiftGear(new_gear);
	dynamics.SetThrottle(throttle);
	dynamics.SetClutch(clutch);
	dynamics.SetNOS(nos);

	// do driver aid toggles
	if (inputs[CarInput::ABS_TOGGLE])
		dynamics.SetABS(!dynamics.GetABSEnabled());

	if (inputs[CarInput::TCS_TOGGLE])
		dynamics.SetTCS(!dynamics.GetTCSEnabled());


}

float Car::GetFeedback()
{
	return dynamics.GetFeedback() / mz_nominalmax;
}

float Car::GetTireSquealAmount(WheelPosition i) const
{
	return dynamics.GetTireSquealAmount(i);
}

void Car::SetInteriorView(bool value)
{
	if (driver_view == value) return;

	driver_view = value;

	// disable/enable glass
	SceneNode & bodynoderef = topnode.GetNode(bodynode);
	keyed_container<Drawable> & normal_blend = bodynoderef.GetDrawlist().normal_blend;
	for (keyed_container<Drawable>::iterator i = normal_blend.begin(); i != normal_blend.end(); ++i)
	{
		i->SetDrawEnable(!driver_view);
	}

	sound.EnableInteriorSounds(value);
}

bool Car::Serialize(joeserialize::Serializer & s)
{
	_SERIALIZE_(s,dynamics);
	_SERIALIZE_(s,last_steer);
	return true;
}
