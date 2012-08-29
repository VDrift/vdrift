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
#include "physics/world.h"
#include "physics/vehicleinfo.h"
#include "graphics/textureinfo.h"
#include "graphics/mesh_gen.h"
#include "sound/sound.h"
#include "cfg/ptree.h"
#include "tracksurface.h"
#include "loadvehicle.h"
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

enum WHICHDRAWLIST
{
	BLEND,
	NOBLEND,
	EMISSIVE,
	OMNI
};

static keyed_container <DRAWABLE> & GetDrawlist(SCENENODE & node, WHICHDRAWLIST which)
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
	SCENENODE & topnode;
	keyed_container<SCENENODE>::handle & bodynode;
	LoadDrawable & loadDrawable;

	LoadBody(
		SCENENODE & topnode,
		keyed_container<SCENENODE>::handle & bodynode,
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
	SCENENODE & topnode,
	std::ostream & error)
{
	ContentManager & content = loadDrawable.content;
	const std::string & path = loadDrawable.path;

	std::string meshname;
	std::vector<std::string> texname;
	std::tr1::shared_ptr<MODEL> mesh;

	// wheel size
	std::string sizestr;
	if (!cfg_wheel.get("size", sizestr, error))
	{
		return false;
	}
	MATHVECTOR<float, 3> size(0);
	cfg_wheel.get("size", size);
	float width = size[0] * 0.001f;
	float ratio = size[1] * 0.01f;
	float diameter = size[2] * 0.0254f;

	// root node for wheel components
	keyed_container<SCENENODE>::handle wheelnode = topnode.AddNode();
	MATHVECTOR<float, 3> pos, rot;
	if (cfg_wheel.get("position", pos) | cfg_wheel.get("rotation", rot))
	{
		QUATERNION<float> q(rot[0]/180*M_PI, rot[1]/180*M_PI, rot[2]/180*M_PI);
		SCENENODE & node = topnode.GetNode(wheelnode);
		node.GetTransform().SetTranslation(pos);
		node.GetTransform().SetRotation(q);
	}

	// rim
	const PTree * cfg_rim;
	if (!cfg_wheel.get("rim", cfg_rim) ||
		!cfg_rim->get("texture", texname, error))
	{
		return false;
	}

	// gen rim mesh (optional)
	if (!cfg_rim->get("mesh", meshname))
	{
		if (!cfg_rim->get("hubmesh", meshname, error) ||
			!content.load(mesh, path, meshname))
		{
			return false;
		}

		meshname = meshname + sizestr;
		if (!content.get(mesh, path, meshname))
		{
			VERTEXARRAY va;
			MESHGEN::RimSpec spec;
			spec.set(width, ratio, diameter);
			spec.hub = &mesh->GetVertexArray();
			MESHGEN::CreateRim(va, spec);
			content.load(mesh, path, meshname, va);
		}
	}

	// load rim
	if (!loadDrawable(meshname, texname, *cfg_rim, topnode.GetNode(wheelnode)))
	{
		return false;
	}

	// tire (optional)
	const PTree * cfg_tire;
	if (!cfg_wheel.get("tire", cfg_tire) ||
		!cfg_tire->get("texture", texname))
	{
		return true;
	}

	// gen tire mesh (optional)
	if (!cfg_tire->get("mesh", meshname))
	{
		meshname = "tire" + sizestr;
		if (!content.get(mesh, path, meshname))
		{
			VERTEXARRAY va;
			MESHGEN::TireSpec spec;
			spec.set(width, ratio, diameter);
			MESHGEN::CreateTire(va, spec);
			content.load(mesh, path, meshname, va);
			//MODEL_OBJ obj;
			//obj.SetVertexArray(mesh->GetVertexArray());
			//obj.Save("tire"+sizestr+".joe", error_output);
		}
	}

	// load tire
	if (!loadDrawable(meshname, texname, *cfg_tire, topnode.GetNode(wheelnode)))
	{
		return false;
	}

	// brake disk (optional)
	const PTree * cfg_brake;
	if (!cfg_wheel.get("brake", cfg_brake) ||
		!cfg_brake->get("texture", texname))
	{
		return true;
	}

	// gen brake disk mesh (optional)
	if (!cfg_brake->get("mesh", meshname))
	{
		float radius = 0.15f;
		float thickness = 0.025f;
		std::string radiusstr;
		cfg_brake->get("radius", radius);
		cfg_brake->get("radius", radiusstr);

		meshname = "brake" + radiusstr;
		if (!content.get(mesh, path, meshname))
		{
			VERTEXARRAY va;
			MESHGEN::CreateRotor(va, radius, thickness);
			content.load(mesh, path, meshname, va);
		}
	}

	// load brake disk
	if (!loadDrawable(meshname, texname, *cfg_brake, topnode.GetNode(wheelnode)))
	{
		return false;
	}

	return true;
}

CAR::CAR() :
	psound(0),
	gearsound_check(0),
	brakesound_check(false),
	handbrakesound_check(false),
	steer_angle_max(0),
	last_steer(0),
	nos_active(false),
	driver_view(false),
	sector(-1),
	applied_brakes(0)
{
	// ctor
}

CAR::~CAR()
{
	RemoveSounds();

	for (size_t i = 0; i < cameras.size(); ++i)
		delete cameras[i];
}

bool CAR::LoadLight(
	const PTree & cfg,
	ContentManager & content,
	std::ostream & error)
{
	float radius;
	std::string radiusstr;
	MATHVECTOR<float, 3> pos(0), col(0);
	if (!cfg.get("position", pos, error)) return false;
	if (!cfg.get("color", col, error)) return false;
	if (!cfg.get("radius", radius, error)) return false;
	cfg.get("radius", radiusstr);

	lights.push_back(LIGHT());

	SCENENODE & bodynoderef = topnode.GetNode(bodynode);
	lights.back().node = bodynoderef.AddNode();

	SCENENODE & node = bodynoderef.GetNode(lights.back().node);
	node.GetTransform().SetTranslation(MATHVECTOR<float,3>(pos[0], pos[1], pos[2]));

	std::tr1::shared_ptr<MODEL> mesh;
	if (!content.get(mesh, "", "cube" + radiusstr))
	{
		VERTEXARRAY varray;
		varray.SetToUnitCube();
		varray.Scale(radius, radius, radius);
		content.load(mesh, "", "cube" + radiusstr, varray);
	}
	models.push_back(mesh);

	keyed_container <DRAWABLE> & dlist = GetDrawlist(node, OMNI);
	lights.back().draw = dlist.insert(DRAWABLE());

	DRAWABLE & draw = dlist.get(lights.back().draw);
	draw.SetColor(col[0], col[1], col[2]);
	draw.SetModel(*mesh);
	draw.SetCull(true, true);
	draw.SetDrawEnable(false);

	return true;
}

bool CAR::LoadGraphics(
	const PTree & cfg,
	const std::string & carpath,
	const std::string & carname,
	const std::string & carpaint,
	const MATHVECTOR <float, 3> & carcolor,
	const int anisotropy,
	const float camerabounce,
	ContentManager & content,
	std::ostream & error)
{
	//write_inf(cfg, std::cerr);
	cartype = carname;
	LoadDrawable loadDrawable(carpath, anisotropy, content, models, error);

	// load body first
	const PTree * cfg_body;
	std::string meshname;
	std::vector<std::string> texname;
	if (!cfg.get("body", cfg_body, error))
	{
		error << "there is a problem with the .car file" << std::endl;
		return false;
	}
	if (!cfg_body->get("mesh", meshname, error)) return false;
	if (!cfg_body->get("texture", texname, error)) return false;
	if (carpaint != "default") texname[0] = carpaint;
	if (!loadDrawable(meshname, texname, *cfg_body, topnode, &bodynode)) return false;

	// load wheels
	const PTree * cfg_wheel;
	if (!cfg.get("wheel", cfg_wheel, error)) return false;
	for (PTree::const_iterator i = cfg_wheel->begin(); i != cfg_wheel->end(); ++i)
	{
		if (!LoadWheel(i->second, loadDrawable, topnode, error))
		{
			error << "Failed to load wheels." << std::endl;
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
		SCENENODE & bodynoderef = topnode.GetNode(bodynode);
		if (!loadDrawable(*cfg_steer, bodynoderef, &steernode, 0))
		{
			error << "Failed to load steering wheel." << std::endl;
			return false;
		}
		cfg_steer->get("max-angle", steer_angle_max);
		steer_angle_max = steer_angle_max / 180.0 * M_PI;
		SCENENODE & steernoderef = bodynoderef.GetNode(steernode);
		steer_orientation = steernoderef.GetTransform().GetRotation();
	}

	// load brake/reverse light point light sources (optional)
	int i = 0;
	std::string istr = "0";
	const PTree * cfg_light;
	while (cfg.get("light-brake-"+istr, cfg_light))
	{
		if (!LoadLight(*cfg_light, content, error))
		{
			error << "Failed to load lights." << std::endl;
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
		if (!LoadLight(*cfg_light, content, error))
		{
			error << "Failed to load lights." << std::endl;
			return false;
		}

		std::stringstream sstr;
		sstr << ++i;
		istr = sstr.str();
	}

	// load car brake/reverse graphics (optional)
	if (cfg.get("light-brake", cfg_light))
	{
		SCENENODE & bodynoderef = topnode.GetNode(bodynode);
		if (!loadDrawable(*cfg_light, bodynoderef, 0, &brakelights))
		{
			error << "Failed to load lights." << std::endl;
			return false;
		}
	}
	if (cfg.get("light-reverse", cfg_light))
	{
		SCENENODE & bodynoderef = topnode.GetNode(bodynode);
		if (!loadDrawable(*cfg_light, bodynoderef, 0, &reverselights))
		{
			error << "Failed to load lights." << std::endl;
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
		error << "No cameras defined." << std::endl;
		return false;
	}
	cameras.reserve(cfg_cams->size());
	for (PTree::const_iterator i = cfg_cams->begin(); i != cfg_cams->end(); ++i)
	{
		CAMERA * cam = LoadCamera(i->second, camerabounce, error);
		if (!cam) return false;
		cameras.push_back(cam);
	}

	SetColor(carcolor[0], carcolor[1], carcolor[2]);

	return true;
}

bool CAR::LoadPhysics(
	const PTree & cfg,
	const std::string & carpath,
	const MATHVECTOR <float, 3> & initial_position,
	const QUATERNION <float> & initial_orientation,
	const bool defaultabs,
	const bool defaulttcs,
	const bool damage,
	sim::World & world,
	ContentManager & content,
	std::ostream & error)
{
	std::string carmodel;
	if (!cfg.get("body.mesh", carmodel, error))
		return false;

	std::tr1::shared_ptr<MODEL> model;
	content.load(model, carpath, carmodel);

	btVector3 size = cast(model->GetSize());
	btVector3 center = cast(model->GetCenter());
	btVector3 position = cast(initial_position);
	btQuaternion rotation = cast(initial_orientation);

	// init motion states
	motion_state.resize(topnode.Nodes());

	// register motion states
	sim::VehicleInfo vinfo;
	vinfo.motionstate.resize(motion_state.size());
	for (int i = 0; i < motion_state.size(); ++i)
	{
		vinfo.motionstate[i] = &motion_state[i];
	}

	// get vehicle info
	if (!LoadVehicle(cfg, damage, center, size, vinfo, error))
	{
		return false;
	}

	// init vehicle
	vehicle.init(vinfo, position, rotation, world);
	vehicle.setABS(defaultabs);
	vehicle.setTCS(defaulttcs);

	// init vehicle state
	vehicle.getState(vstate);

	// steering torque scale factor
	mz_nominalmax = 1;//vehicle.getWheel(0).tire.getMaxMz() + vehicle.getWheel(1).tire.getMaxMz();

	return true;
}

bool CAR::LoadSounds(
	const PTree & cfg,
	const std::string & carpath,
	const std::string & carname,
	SOUND & sound,
	ContentManager & content,
	std::ostream & error)
{
	psound = &sound;

	// check for sound specification file
	std::string path_aud = carpath + "/" + carname + ".aud";
	std::ifstream file_aud(path_aud.c_str());
	if (file_aud.good())
	{
		PTree aud;
		read_ini(file_aud, aud);
		enginesounds.reserve(aud.size());
		for (PTree::const_iterator i = aud.begin(); i != aud.end(); ++i)
		{
			const PTree & audi = i->second;

			std::string filename;
			std::tr1::shared_ptr<SOUNDBUFFER> soundptr;
			if (!audi.get("filename", filename, error)) return false;

			enginesounds.push_back(ENGINESOUNDINFO());
			ENGINESOUNDINFO & info = enginesounds.back();

			if (!audi.get("MinimumRPM", info.minrpm, error)) return false;
			if (!audi.get("MaximumRPM", info.maxrpm, error)) return false;
			if (!audi.get("NaturalRPM", info.naturalrpm, error)) return false;

			bool powersetting;
			if (!audi.get("power", powersetting, error)) return false;
			if (powersetting)
				info.power = ENGINESOUNDINFO::POWERON;
			else if (!powersetting)
				info.power = ENGINESOUNDINFO::POWEROFF;
			else
				info.power = ENGINESOUNDINFO::BOTH;

			info.sound_source = sound.AddSource(soundptr, 0, true, true);
			sound.SetSourceGain(info.sound_source, 0);
		}

		// set blend start and end locations -- requires multiple passes
		std::map <ENGINESOUNDINFO *, ENGINESOUNDINFO *> temporary_to_actual_map;
		std::list <ENGINESOUNDINFO> poweron_sounds, poweroff_sounds;
		for (std::vector <ENGINESOUNDINFO>::iterator i = enginesounds.begin(); i != enginesounds.end(); ++i)
		{
			if (i->power == ENGINESOUNDINFO::POWERON)
			{
				poweron_sounds.push_back(*i);
				temporary_to_actual_map[&poweron_sounds.back()] = &*i;
			}
			else if (i->power == ENGINESOUNDINFO::POWEROFF)
			{
				poweroff_sounds.push_back(*i);
				temporary_to_actual_map[&poweroff_sounds.back()] = &*i;
			}
		}

		poweron_sounds.sort();
		poweroff_sounds.sort();

		// we only support 2 overlapping sounds at once each for poweron and poweroff; this
		// algorithm fails for other cases (undefined behavior)
		std::list <ENGINESOUNDINFO> * cursounds = &poweron_sounds;
		for (int n = 0; n < 2; n++)
		{
			if (n == 1)
				cursounds = &poweroff_sounds;

			for (std::list <ENGINESOUNDINFO>::iterator i = (*cursounds).begin(); i != (*cursounds).end(); ++i)
			{
				// set start blend
				if (i == (*cursounds).begin())
					i->fullgainrpmstart = i->minrpm;

				// set end blend
				std::list <ENGINESOUNDINFO>::iterator inext = i;
				inext++;
				if (inext == (*cursounds).end())
					i->fullgainrpmend = i->maxrpm;
				else
				{
					i->fullgainrpmend = inext->minrpm;
					inext->fullgainrpmstart = i->maxrpm;
				}
			}

			// now assign back to the actual infos
			for (std::list <ENGINESOUNDINFO>::iterator i = (*cursounds).begin(); i != (*cursounds).end(); ++i)
			{
				assert(temporary_to_actual_map.find(&(*i)) != temporary_to_actual_map.end());
				*temporary_to_actual_map[&(*i)] = *i;
			}
		}
	}
	else
	{
		std::tr1::shared_ptr<SOUNDBUFFER> soundptr;
		content.load(soundptr, carpath, "engine");
		enginesounds.push_back(ENGINESOUNDINFO());
		enginesounds.back().sound_source = sound.AddSource(soundptr, 0, true, true);
	}

	// init tire sounds
	const PTree *wheel_cfg;
	if (!cfg.get("wheel", wheel_cfg, error))
	{
		return false;
	}

	int wheel_count = wheel_cfg->size();
	if (wheel_count < 2)
	{
		error << "Weel count: " << wheel_count << ". At least two wheels expected." << std::endl;
		return false;
	}

	roadsound.resize(wheel_count);
	gravelsound.resize(wheel_count);
	grasssound.resize(wheel_count);
	bumpsound.resize(wheel_count);

	//set up tire squeal sounds
	for (int i = 0; i < wheel_count; ++i)
	{
		std::tr1::shared_ptr<SOUNDBUFFER> soundptr;
		content.load(soundptr, carpath, "tire_squeal");
		roadsound[i] = sound.AddSource(soundptr, i * 0.25, true, true);
	}

	//set up tire gravel sounds
	for (int i = 0; i < wheel_count; ++i)
	{
		std::tr1::shared_ptr<SOUNDBUFFER> soundptr;
		content.load(soundptr, carpath, "gravel");
		gravelsound[i] = sound.AddSource(soundptr, i * 0.25, true, true);
	}

	//set up tire grass sounds
	for (int i = 0; i < wheel_count; ++i)
	{
		std::tr1::shared_ptr<SOUNDBUFFER> soundptr;
		content.load(soundptr, carpath, "grass");
		grasssound[i] = sound.AddSource(soundptr, i * 0.25, true, true);
	}

	//set up bump sounds
	for (int i = 0; i < wheel_count; ++i)
	{
		std::tr1::shared_ptr<SOUNDBUFFER> soundptr;
		if (i >= 2)
		{
			content.load(soundptr, carpath, "bump_rear");
		}
		else
		{
			content.load(soundptr, carpath, "bump_front");
		}
		bumpsound[i] = sound.AddSource(soundptr, 0, true, false);
	}

	//set up crash sound
	{
		std::tr1::shared_ptr<SOUNDBUFFER> soundptr;
		content.load(soundptr, carpath, "crash");
		crashsound = sound.AddSource(soundptr, 0, true, false);
	}

	//set up gear sound
	{
		std::tr1::shared_ptr<SOUNDBUFFER> soundptr;
		content.load(soundptr, carpath, "gear");
		gearsound = sound.AddSource(soundptr, 0, true, false);
	}

	//set up brake sound
	{
		std::tr1::shared_ptr<SOUNDBUFFER> soundptr;
		content.load(soundptr, carpath, "brake");
		brakesound = sound.AddSource(soundptr, 0, true, false);
	}

	//set up handbrake sound
	{
		std::tr1::shared_ptr<SOUNDBUFFER> soundptr;
		content.load(soundptr, carpath, "handbrake");
		handbrakesound = sound.AddSource(soundptr, 0, true, false);
	}

	{
		std::tr1::shared_ptr<SOUNDBUFFER> soundptr;
		content.load(soundptr, carpath, "wind");
		roadnoise = sound.AddSource(soundptr, 0, true, true);
	}

	return true;
}

void CAR::SetColor(float r, float g, float b)
{
	SCENENODE & bodynoderef = topnode.GetNode(bodynode);
	keyed_container<DRAWABLE> & car_noblend = bodynoderef.GetDrawlist().car_noblend;
	for (keyed_container<DRAWABLE>::iterator i = car_noblend.begin(); i != car_noblend.end(); ++i)
	{
		i->SetColor(r, g, b, 1);
	}
}

void CAR::UpdateGraphics()
{
	if (!bodynode.valid()) return;

	assert(unsigned(motion_state.size()) == topnode.Nodes());

	int i = 0;
	keyed_container<SCENENODE> & childlist = topnode.GetNodelist();
	for (keyed_container<SCENENODE>::iterator ni = childlist.begin(); ni != childlist.end(); ++ni, ++i)
	{
		MATHVECTOR<float, 3> pos = cast(motion_state[i].position);
		QUATERNION<float> rot = cast(motion_state[i].rotation);
		ni->GetTransform().SetTranslation(pos);
		ni->GetTransform().SetRotation(rot);
	}

	// brake/reverse lights
	SCENENODE & bodynoderef = topnode.GetNode(bodynode);
	for (std::list<LIGHT>::iterator i = lights.begin(); i != lights.end(); i++)
	{
		SCENENODE & node = bodynoderef.GetNode(i->node);
		DRAWABLE & draw = GetDrawlist(node, OMNI).get(i->draw);
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
		SCENENODE & steernoderef = bodynoderef.GetNode(steernode);
		steernoderef.GetTransform().SetRotation(steer_rotation);
	}
}

void CAR::RemoveSounds()
{
	if (!psound) return;

	// reverse order (really worth it?)
	psound->RemoveSource(roadnoise);
	psound->RemoveSource(handbrakesound);
	psound->RemoveSource(brakesound);
	psound->RemoveSource(gearsound);
	psound->RemoveSource(crashsound);

	size_t i = bumpsound.size();
	while (i) psound->RemoveSource(bumpsound[--i]);

	i = grasssound.size();
	while (i) psound->RemoveSource(grasssound[--i]);

	i = gravelsound.size();
	while (i) psound->RemoveSource(gravelsound[--i]);

	i = roadsound.size();
	while (i) psound->RemoveSource(roadsound[--i]);

	i = enginesounds.size();
	while (i) psound->RemoveSource(enginesounds[--i].sound_source);
}

void CAR::UpdateSounds(float dt)
{
	if (!psound) return;

	MATHVECTOR <float, 3> pos_car = GetPosition();
	MATHVECTOR <float, 3> pos_eng = cast(vehicle.getEngine().getPosition());

	psound->SetSourcePosition(roadnoise, pos_car[0], pos_car[1], pos_car[2]);
	psound->SetSourcePosition(crashsound, pos_car[0], pos_car[1], pos_car[2]);
	psound->SetSourcePosition(gearsound, pos_car[0], pos_car[1], pos_car[2]);
	psound->SetSourcePosition(brakesound, pos_car[0], pos_car[1], pos_car[2]);
	psound->SetSourcePosition(handbrakesound, pos_car[0], pos_car[1], pos_car[2]);

	GetOrientation().RotateVector(pos_eng);
	pos_eng = pos_eng + pos_car;

	// update engine sounds
	float rpm = GetEngineRPM();
	float throttle = vehicle.getEngine().getThrottle();
	float total_gain = 0.0;

	std::vector<std::pair<size_t, float> > gainlist;
	gainlist.reserve(enginesounds.size());
	for (std::vector<ENGINESOUNDINFO>::iterator i = enginesounds.begin(); i != enginesounds.end(); ++i)
	{
		ENGINESOUNDINFO & info = *i;
		float gain = 1.0;

		if (rpm < info.minrpm)
		{
			gain = 0;
		}
		else if (rpm < info.fullgainrpmstart && info.fullgainrpmstart > info.minrpm)
		{
			gain *= (rpm - info.minrpm) / (info.fullgainrpmstart - info.minrpm);
		}

		if (rpm > info.maxrpm)
		{
			gain = 0;
		}
		else if (rpm > info.fullgainrpmend && info.fullgainrpmend < info.maxrpm)
		{
			gain *= 1.0 - (rpm - info.fullgainrpmend) / (info.maxrpm - info.fullgainrpmend);
		}

		if (info.power == ENGINESOUNDINFO::BOTH)
		{
			gain *= throttle * 0.5 + 0.5;
		}
		else if (info.power == ENGINESOUNDINFO::POWERON)
		{
			gain *= throttle;
		}
		else if (info.power == ENGINESOUNDINFO::POWEROFF)
		{
			gain *= (1.0-throttle);
		}

		total_gain += gain;
		gainlist.push_back(std::make_pair(info.sound_source, gain));

		float pitch = rpm / info.naturalrpm;

		psound->SetSourcePosition(info.sound_source, pos_eng[0], pos_eng[1], pos_eng[2]);
		psound->SetSourcePitch(info.sound_source, pitch);
	}

	// normalize gains
	assert(total_gain >= 0.0);
	for (std::vector<std::pair<size_t, float> >::iterator i = gainlist.begin(); i != gainlist.end(); ++i)
	{
		float gain;
		if (total_gain == 0.0)
		{
			gain = 0.0;
		}
		else if (enginesounds.size() == 1 && enginesounds.back().power == ENGINESOUNDINFO::BOTH)
		{
			gain = i->second;
		}
		else
		{
			gain = i->second / total_gain;
		}
		psound->SetSourceGain(i->first, gain);
	}

	// update tire squeal sounds
	for (size_t i = 0; i < roadsound.size(); ++i)
	{
		// make sure we don't get overlap
		psound->SetSourceGain(gravelsound[i], 0.0);
		psound->SetSourceGain(grasssound[i], 0.0);
		psound->SetSourceGain(roadsound[i], 0.0);

		float squeal = GetTireSquealAmount(i);
		size_t * sound_active = &roadsound[i];
		float pitchvariation = 0.0;
		float maxgain = 0.0;

		const sim::Surface * surface = vehicle.getWheel(i).ray.getSurface();
		if (surface)
		{
			const TRACKSURFACE * ts = static_cast<const TRACKSURFACE *>(surface);
			pitchvariation = ts->pitch_variation;
			maxgain = ts->max_gain;
			if (ts->sound_id == 0)
			{
				sound_active = &roadsound[i];
			}
			else if (ts->sound_id == 1)
			{
				sound_active = &gravelsound[i];
			}
			else if (ts->sound_id == 2)
			{
				sound_active = &grasssound[i];
			}

			const sim::WheelContact & c = vehicle.getWheelContact(i);
			btVector3 cp = vehicle.getTransform() * c.rA;
			float cv = btSqrt(c.v1 * c.v1 + c.v2 * c.v2);
			float pitch = 0.1f * cv - 0.5f;
			pitch = clamp(pitch, 0.0f, 1.0f);
			pitch = 1.0 - pitch * pitchvariation;
			pitch = clamp(pitch, 0.1f, 4.0f);

			psound->SetSourcePosition(*sound_active, cp[0], cp[1], cp[2]);
			psound->SetSourcePitch(*sound_active, pitch);
			psound->SetSourceGain(*sound_active, squeal * maxgain);
		}
	}

	//update road noise sound
	{
		float gain = 4E-4 * vehicle.getVelocity().length2();
		if (gain > 1) gain = 1;
		psound->SetSourceGain(roadnoise, gain);
	}
/*
	//update bump noise sound
	{
		for (int i = 0; i < 4; i++)
		{
			suspensionbumpdetection[i].Update(
				vehicle.GetSuspension(i).GetVelocity(),
				vehicle.GetSuspension(i).GetDisplacementFraction(),
				dt);
			if (suspensionbumpdetection[i].JustSettled())
			{
				float bumpsize = suspensionbumpdetection[i].GetTotalBumpSize();

				const float breakevenms = 5.0;
				float gain = bumpsize * GetSpeed() / breakevenms;
				if (gain > 1)
					gain = 1;
				if (gain < 0)
					gain = 0;

				if (gain > 0 && !tirebump[i].Audible())
				{
					tirebump[i].SetGain(gain);
					tirebump[i].Stop();
					tirebump[i].Play();
				}
			}
		}
	}
*/
	//update crash sound
	crashdetection.Update(GetSpeed(), dt);
	float crashdecel = crashdetection.GetMaxDecel();
	if (crashdecel > 0)
	{
		const float mingainat = 200;
		const float maxgainat = 2000;
		float gain = (crashdecel - mingainat) / (maxgainat - mingainat);
		gain = clamp(gain, 0.1f, 1.0f);

		if (!psound->GetSourcePlaying(crashsound))
		{
			psound->ResetSource(crashsound);
			psound->SetSourceGain(crashsound, gain);
		}
	}

	// update gear sound
	if (driver_view && gearsound_check != GetGear())
	{
		float gain = 0.0;
		if (GetEngineRPM() != 0.0)
			gain = GetEngineRPMLimit() / GetEngineRPM();
		gain = clamp(gain, 0.25f, 0.50f);

		if (!psound->GetSourcePlaying(gearsound))
		{
			psound->ResetSource(gearsound);
			psound->SetSourceGain(gearsound, gain);
		}
		gearsound_check = GetGear();
	}
}

void CAR::Update(double dt)
{
	UpdateGraphics();
	UpdateSounds(dt);
}

void CAR::ProcessInputs(const std::vector <float> & inputs)
{
	 // ensure that our inputs vector contains exactly one item per input
	assert(inputs.size() == CARINPUT::INVALID);

	// recover from a rollover
	if (inputs[CARINPUT::ROLLOVER_RECOVER])
		vehicle.rolloverRecover();

	//set brakes
	vehicle.setBrake(inputs[CARINPUT::BRAKE]);
	vehicle.setHandBrake(inputs[CARINPUT::HANDBRAKE]);

	// do steering
	float steer_value = inputs[CARINPUT::STEER_RIGHT];
	if (std::abs(inputs[CARINPUT::STEER_LEFT]) > std::abs(inputs[CARINPUT::STEER_RIGHT])) //use whichever control is larger
		steer_value = -inputs[CARINPUT::STEER_LEFT];
	vehicle.setSteering(steer_value);
	last_steer = steer_value;
	QUATERNION<float> steer;
	steer.Rotate(-steer_value * steer_angle_max, 0, 0, 1);
	steer_rotation = steer_orientation * steer;

    // start the engine if requested
	if (inputs[CARINPUT::START_ENGINE])
		vehicle.startEngine();

	// do shifting
	int gear_change = 0;
	if (inputs[CARINPUT::SHIFT_UP] == 1.0)
		gear_change = 1;
	if (inputs[CARINPUT::SHIFT_DOWN] == 1.0)
		gear_change = -1;
	int cur_gear = vehicle.getTransmission().getGear();
	int new_gear = cur_gear + gear_change;

	if (inputs[CARINPUT::REVERSE])
		new_gear = -1;
	if (inputs[CARINPUT::NEUTRAL])
		new_gear = 0;
	if (inputs[CARINPUT::FIRST_GEAR])
		new_gear = 1;
	if (inputs[CARINPUT::SECOND_GEAR])
		new_gear = 2;
	if (inputs[CARINPUT::THIRD_GEAR])
		new_gear = 3;
	if (inputs[CARINPUT::FOURTH_GEAR])
		new_gear = 4;
	if (inputs[CARINPUT::FIFTH_GEAR])
		new_gear = 5;
	if (inputs[CARINPUT::SIXTH_GEAR])
		new_gear = 6;

	applied_brakes = inputs[CARINPUT::BRAKE];

	float throttle = inputs[CARINPUT::THROTTLE];
	float clutch = 1 - inputs[CARINPUT::CLUTCH];
	float nos = inputs[CARINPUT::NOS];

	nos_active = nos > 0;

	vehicle.setGear(new_gear);
	vehicle.setThrottle(throttle);
	vehicle.setClutch(clutch);
	vehicle.setNOS(nos);

	// do driver aid toggles
	if (inputs[CARINPUT::ABS_TOGGLE])
		vehicle.setABS(!vehicle.getABSEnabled());
	if (inputs[CARINPUT::TCS_TOGGLE])
		vehicle.setTCS(!vehicle.getTCSEnabled());

	// update interior sounds
	if (!psound || !driver_view) return;

/*	// disable brake sound, sounds weird
	if (inputs[CARINPUT::BRAKE] > 0 && !brakesound_check)
	{
		if (!psound->GetSourcePlaying(brakesound))
		{
			psound->ResetSource(brakesound);
			psound->SetSourceGain(brakesound, 0.5);
		}
		brakesound_check = true;
	}
	if (inputs[CARINPUT::BRAKE] <= 0)
		brakesound_check = false;
*/
	// handbrake sound
	if (inputs[CARINPUT::HANDBRAKE] > 0 && !handbrakesound_check)
	{
		if (!psound->GetSourcePlaying(handbrakesound))
		{
			psound->ResetSource(handbrakesound);
			psound->SetSourceGain(handbrakesound, 0.5);
		}
		handbrakesound_check = true;
	}
	if (inputs[CARINPUT::HANDBRAKE] <= 0)
		handbrakesound_check = false;
}

float CAR::GetFeedback()
{
	return vehicle.getFeedback() / mz_nominalmax;
}

float CAR::GetTireSquealAmount(int i) const
{
	const sim::Surface * surface = vehicle.getWheel(i).ray.getSurface();
	if (!surface || surface == sim::Surface::None())
		return 0.0f;

	// tire thermal load (dissipated power * time step)
	const sim::WheelContact & c = vehicle.getWheelContact(i);
	float w1 = c.friction1.accumImpulse * c.v1;
	float w2 = c.friction2.accumImpulse * c.v2;
	float thermal_load = sqrtf(w1 * w1 + w2 * w2);

	// scale squeal with thermal load
	float squeal = thermal_load * 2E-3f - 0.1f;
	squeal = clamp(squeal, 0.0f, 1.0f);

	// abuse squeal to indicate ideal slip, slide
	const sim::Tire & tire = vehicle.getWheel(i).tire;
	float slip = tire.getSlip() / tire.getIdealSlip();
	float slide = tire.getSlide() / tire.getIdealSlide();
	float squeal_factor = std::max(std::abs(slip), std::abs(slide)) - 1.0f;
	squeal_factor = clamp(squeal_factor, 0.0f, 1.0f);

	squeal = squeal * squeal_factor;
	return squeal;
}

void CAR::SetInteriorView(bool value)
{
	if (driver_view == value) return;

	driver_view = value;

	// disable/enable glass
	SCENENODE & bodynoderef = topnode.GetNode(bodynode);
	keyed_container<DRAWABLE> & normal_blend = bodynoderef.GetDrawlist().normal_blend;
	for (keyed_container<DRAWABLE>::iterator i = normal_blend.begin(); i != normal_blend.end(); ++i)
	{
		i->SetDrawEnable(!driver_view);
	}
}

void CAR::DebugPrint(std::ostream & out, bool p1, bool p2, bool p3, bool p4) const
{
	vehicle.print(out, p1, p2, p3, p4);
}

static bool serialize(joeserialize::Serializer & s, btVector3 & v)
{
	_SERIALIZE_(s, v[0]);
	_SERIALIZE_(s, v[1]);
	_SERIALIZE_(s, v[2]);
	return true;
}

static bool serialize(joeserialize::Serializer & s, btMatrix3x3 & m)
{
	if (!serialize(s, m[0])) return false;
	if (!serialize(s, m[1])) return false;
	if (!serialize(s, m[2])) return false;
	return true;
}

static bool serialize(joeserialize::Serializer & s, btTransform & t)
{
	if (!serialize(s, t.getBasis())) return false;
	if (!serialize(s, t.getOrigin())) return false;
	return true;
}

bool CAR::Serialize(joeserialize::Serializer & s)
{
	bool write = (s.GetIODirection() == joeserialize::Serializer::DIRECTION_INPUT);
	if (!write) vehicle.getState(vstate);
	for (int i = 0; i < vstate.shaft_angvel.size(); ++i)
	{
		_SERIALIZE_(s, vstate.shaft_angvel[i]);
	}
	if (!serialize(s, vstate.transform)) return false;
	if (!serialize(s, vstate.lin_velocity)) return false;
	if (!serialize(s, vstate.ang_velocity)) return false;
	_SERIALIZE_(s, vstate.brake);
	_SERIALIZE_(s, vstate.clutch);
	_SERIALIZE_(s, vstate.shift_time);
	_SERIALIZE_(s, vstate.tacho_rpm);
	_SERIALIZE_(s, vstate.gear);
	_SERIALIZE_(s, vstate.shifted);
	_SERIALIZE_(s, vstate.auto_shift);
	_SERIALIZE_(s, vstate.auto_clutch);
	_SERIALIZE_(s, vstate.abs_enabled);
	_SERIALIZE_(s, vstate.tcs_enabled);
	if (write) vehicle.setState(vstate);

	_SERIALIZE_(s, last_steer);
	_SERIALIZE_(s, nos_active);
	_SERIALIZE_(s, driver_view);
	_SERIALIZE_(s, sector);
	return true;
}
