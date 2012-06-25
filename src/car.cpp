#include "car.h"
#include "carwheelposition.h"
#include "dynamicsworld.h"
#include "tracksurface.h"
#include "carinput.h"
#include "contentmanager.h"
#include "textureinfo.h"
#include "model_joe03.h"
#include "mesh_gen.h"
#include "loaddrawable.h"
#include "loadcamera.h"
#include "camera.h"
#include "cfg/ptree.h"

#include <map>
#include <list>
#include <vector>
#include <sstream>
#include <string>

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
	std::ostream & error_output)
{
	keyed_container<SCENENODE>::handle wheelnode = topnode.AddNode();
	ContentManager & content = loadDrawable.content;
	const std::string& path = loadDrawable.path;

	std::string meshname;
	std::vector<std::string> texname;
	if (!cfg_wheel.get("mesh", meshname, error_output)) return false;
	if (!cfg_wheel.get("texture", texname, error_output)) return false;

	std::string tiredim;
	const PTree * cfg_tire;
	if (!cfg_wheel.get("tire", cfg_tire, error_output)) return false;
	if (!cfg_tire->get("size", tiredim, error_output)) return false;

	MATHVECTOR<float, 3> size(0);
	cfg_tire->get("size", size);
	float width = size[0] * 0.001;
	float diameter = size[2] * 0.0254;

	// get wheel disk mesh
	std::tr1::shared_ptr<MODEL> mesh;
	if (!content.load(path, meshname, mesh)) return false;

	// gen wheel mesh
	if (!content.get(path, meshname+tiredim, mesh))
	{
		VERTEXARRAY rimva, diskva;
		MESHGEN::mg_rim(rimva, size[0], size[1], size[2], 10);
		diskva = mesh->GetVertexArray();
		diskva.Translate(-0.75 * 0.5, 0, 0);
		diskva.Scale(width, diameter, diameter);
		content.load(path, meshname+tiredim, rimva + diskva, mesh);
	}

	// load wheel
	if (!loadDrawable(meshname+tiredim, texname, cfg_wheel, topnode, &wheelnode))
	{
		return false;
	}

	// tire (optional)
	texname.clear();
	if (!cfg_tire->get("texture", texname, error_output)) return true;

	// gen tire mesh
	if (!content.get(path, "tire"+tiredim, mesh))
	{
		VERTEXARRAY tireva;
		MESHGEN::mg_tire(tireva, size[0], size[1], size[2]);
		content.load(path, "tire"+tiredim, tireva, mesh);
	}

	// load tire
	if (!loadDrawable("tire"+tiredim, texname, *cfg_tire, topnode.GetNode(wheelnode)))
	{
		return false;
	}

	// brake (optional)
	texname.clear();
	std::string brakename;
	const PTree * cfg_brake;
	if (!cfg_wheel.get("brake", cfg_brake, error_output)) return true;
	if (!cfg_brake->get("texture", texname)) return true;

	float radius;
	std::string radiusstr;
	cfg_brake->get("radius", radius);
	cfg_brake->get("radius", radiusstr);

	// gen brake disk mesh
	if (!content.get(path, "brake"+radiusstr, mesh))
	{
		float diameter_mm = radius * 2 * 1000;
		float thickness_mm = 0.025 * 1000;
		VERTEXARRAY brakeva;
		MESHGEN::mg_brake_rotor(brakeva, diameter_mm, thickness_mm);
		content.load(path, "brake"+radiusstr, brakeva, mesh);
	}

	// load brake disk
	if (!loadDrawable("brake"+radiusstr, texname, *cfg_brake, topnode.GetNode(wheelnode)))
	{
		return false;
	}

	return true;
}

CAR::CAR() :
	gearsound_check(0),
	brakesound_check(false),
	handbrakesound_check(false),
	steer_angle_max(0),
	last_steer(0),
	sector(-1),
	applied_brakes(0)
{
	// ctor
}

CAR::~CAR()
{
	for (unsigned i = 0; i < cameras.size(); ++i)
	{
		delete cameras[i];
	}
}

bool CAR::LoadLight(
	const PTree & cfg,
	ContentManager & content,
	std::ostream & error_output)
{
	float radius;
	std::string radiusstr;
	MATHVECTOR<float, 3> pos(0), col(0);
	if (!cfg.get("position", pos, error_output)) return false;
	if (!cfg.get("color", col, error_output)) return false;
	if (!cfg.get("radius", radius, error_output)) return false;
	cfg.get("radius", radiusstr);

	lights.push_back(LIGHT());

	SCENENODE & bodynoderef = topnode.GetNode(bodynode);
	lights.back().node = bodynoderef.AddNode();

	SCENENODE & node = bodynoderef.GetNode(lights.back().node);
	node.GetTransform().SetTranslation(MATHVECTOR<float,3>(pos[0], pos[1], pos[2]));

	std::tr1::shared_ptr<MODEL> mesh;
	if (!content.get("", "cube"+radiusstr, mesh))
	{
		VERTEXARRAY varray;
		varray.SetToUnitCube();
		varray.Scale(radius, radius, radius);
		content.load("", "cube"+radiusstr, varray, mesh);
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
	const MATHVECTOR <float, 3> & carcolor,
	const std::string & carpaint,
	const int anisotropy,
	const float camerabounce,
	ContentManager & content,
	std::ostream & error_output)
{
	//write_inf(cfg, std::cerr);
	cartype = carname;
	LoadDrawable loadDrawable(carpath, anisotropy, content, models, error_output);

	// load body first
	const PTree * cfg_body;
	std::string meshname;
	std::vector<std::string> texname;
	if (!cfg.get("body", cfg_body, error_output))
	{
		error_output << "there is a problem with the .car file" << std::endl;
		return false;
	}
	if (!cfg_body->get("mesh", meshname, error_output)) return false;
	if (!cfg_body->get("texture", texname, error_output)) return false;
	if (carpaint != "default") texname[0] = carpaint;
	if (!loadDrawable(meshname, texname, *cfg_body, topnode, &bodynode)) return false;

	// load wheels
	const PTree * cfg_wheel;
	if (!cfg.get("wheel", cfg_wheel, error_output)) return false;
	for (PTree::const_iterator i = cfg_wheel->begin(); i != cfg_wheel->end(); ++i)
	{
		if (!LoadWheel(i->second, loadDrawable, topnode, error_output))
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
		SCENENODE & bodynoderef = topnode.GetNode(bodynode);
		if (!loadDrawable(*cfg_steer, bodynoderef, &steernode, 0))
		{
			error_output << "Failed to load steering wheel." << std::endl;
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
		SCENENODE & bodynoderef = topnode.GetNode(bodynode);
		if (!loadDrawable(*cfg_light, bodynoderef, 0, &brakelights))
		{
			error_output << "Failed to load lights." << std::endl;
			return false;
		}
	}
	if (cfg.get("light-reverse", cfg_light))
	{
		SCENENODE & bodynoderef = topnode.GetNode(bodynode);
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
		CAMERA * cam = LoadCamera(i->second, camerabounce, error_output);
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
	ContentManager & content,
	DynamicsWorld & world,
	std::ostream & info_output,
	std::ostream & error_output)
{
	std::string carmodel;
	std::tr1::shared_ptr<MODEL> modelptr;
	if (!cfg.get("body.mesh", carmodel, error_output)) return false;
	if (!content.load(carpath, carmodel, modelptr)) return false;

	btVector3 size = ToBulletVector(modelptr->GetAABB().GetSize());
	btVector3 center = ToBulletVector(modelptr->GetAABB().GetCenter());
	btVector3 position = ToBulletVector(initial_position);
	btQuaternion rotation = ToBulletQuaternion(initial_orientation);

	if (!dynamics.Load(cfg, size, center, position, rotation, damage, world, error_output)) return false;
	dynamics.SetABS(defaultabs);
	dynamics.SetTCS(defaulttcs);

	mz_nominalmax = (GetTireMaxMz(FRONT_LEFT) + GetTireMaxMz(FRONT_RIGHT)) * 0.5;

	return true;
}

bool CAR::LoadSounds(
	const std::string & carpath,
	const std::string & carname,
	ContentManager & content,
	std::ostream & info_output,
	std::ostream & error_output)
{
	//check for sound specification file
	std::string path_aud = carpath+"/"+carname+".aud";
	std::ifstream file_aud(path_aud.c_str());
	if (file_aud.good())
	{
		PTree aud;
		read_ini(file_aud, aud);
		for (PTree::const_iterator i = aud.begin(); i != aud.end(); ++i)
		{
			const PTree & audi = i->second;

			std::string filename;
			std::tr1::shared_ptr<SOUNDBUFFER> soundptr;
			if (!audi.get("filename", filename, error_output)) return false;
			if (!content.load(carpath, filename, soundptr)) return false;

			enginesounds.push_back(std::pair <ENGINESOUNDINFO, SOUNDSOURCE> ());
			ENGINESOUNDINFO & info = enginesounds.back().first;
			SOUNDSOURCE & sound = enginesounds.back().second;

			if (!audi.get("MinimumRPM", info.minrpm, error_output)) return false;
			if (!audi.get("MaximumRPM", info.maxrpm, error_output)) return false;
			if (!audi.get("NaturalRPM", info.naturalrpm, error_output)) return false;

			bool powersetting;
			if (!audi.get("power", powersetting, error_output)) return false;
			if (powersetting)
				info.power = ENGINESOUNDINFO::POWERON;
			else if (!powersetting)
				info.power = ENGINESOUNDINFO::POWEROFF;
			else //assume it's used in both ways
				info.power = ENGINESOUNDINFO::BOTH;

			sound.SetBuffer(soundptr);
			sound.Enable3D(true);
			sound.Loop(true);
			sound.SetGain(0);
			sound.Play();
		}

		//set blend start and end locations -- requires multiple passes
		std::map <ENGINESOUNDINFO *, ENGINESOUNDINFO *> temporary_to_actual_map;
		std::list <ENGINESOUNDINFO> poweron_sounds;
		std::list <ENGINESOUNDINFO> poweroff_sounds;
		for (std::list <std::pair <ENGINESOUNDINFO, SOUNDSOURCE> >::iterator i = enginesounds.begin(); i != enginesounds.end(); ++i)
		{
			ENGINESOUNDINFO & info = i->first;
			if (info.power == ENGINESOUNDINFO::POWERON)
			{
				poweron_sounds.push_back(info);
				temporary_to_actual_map[&poweron_sounds.back()] = &info;
			}
			else if (info.power == ENGINESOUNDINFO::POWEROFF)
			{
				poweroff_sounds.push_back(info);
				temporary_to_actual_map[&poweroff_sounds.back()] = &info;
			}
		}

		poweron_sounds.sort();
		poweroff_sounds.sort();

		//we only support 2 overlapping sounds at once each for poweron and poweroff; this
		// algorithm fails for other cases (undefined behavior)
		std::list <ENGINESOUNDINFO> * cursounds = &poweron_sounds;
		for (int n = 0; n < 2; n++)
		{
			if (n == 1)
				cursounds = &poweroff_sounds;

			for (std::list <ENGINESOUNDINFO>::iterator i = (*cursounds).begin(); i != (*cursounds).end(); ++i)
			{
				//set start blend
				if (i == (*cursounds).begin())
					i->fullgainrpmstart = i->minrpm;
				//else, the blend start has been set already by the previous iteration

				//set end blend
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

			//now assign back to the actual infos
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
		if (!content.load(carpath, "engine", soundptr)) return false;
		enginesounds.push_back(std::pair <ENGINESOUNDINFO, SOUNDSOURCE> ());
		SOUNDSOURCE & enginesound = enginesounds.back().second;
		enginesound.SetBuffer(soundptr);
		enginesound.Enable3D(true);
		enginesound.Loop(true);
		enginesound.SetGain(0);
		enginesound.Play();
	}

	//set up tire squeal sounds
	for (int i = 0; i < 4; ++i)
	{
		std::tr1::shared_ptr<SOUNDBUFFER> soundptr;
		if (!content.load(carpath, "tire_squeal", soundptr)) return false;
		tiresqueal[i].SetBuffer(soundptr);
		tiresqueal[i].Enable3D(true);
		tiresqueal[i].Loop(true);
		tiresqueal[i].SetGain(0);
		int samples = soundptr->GetSoundInfo().samples;
		tiresqueal[i].SeekToSample((samples/4)*i);
		tiresqueal[i].Play();
	}

	//set up tire gravel sounds
	for (int i = 0; i < 4; ++i)
	{
		std::tr1::shared_ptr<SOUNDBUFFER> soundptr;
		if (!content.load(carpath, "gravel", soundptr)) return false;
		gravelsound[i].SetBuffer(soundptr);
		gravelsound[i].Enable3D(true);
		gravelsound[i].Loop(true);
		gravelsound[i].SetGain(0);
		int samples = soundptr->GetSoundInfo().samples;
		gravelsound[i].SeekToSample((samples/4)*i);
		gravelsound[i].Play();
	}

	//set up tire grass sounds
	for (int i = 0; i < 4; ++i)
	{
		std::tr1::shared_ptr<SOUNDBUFFER> soundptr;
		if (!content.load(carpath, "grass", soundptr)) return false;
		grasssound[i].SetBuffer(soundptr);
		grasssound[i].Enable3D(true);
		grasssound[i].Loop(true);
		grasssound[i].SetGain(0);
		int samples = soundptr->GetSoundInfo().samples;
		grasssound[i].SeekToSample((samples/4)*i);
		grasssound[i].Play();
	}

	//set up bump sounds
	for (int i = 0; i < 4; ++i)
	{
		std::tr1::shared_ptr<SOUNDBUFFER> soundptr;
		if (i >= 2)
		{
			if (!content.load(carpath, "bump_rear", soundptr)) return false;
		}
		else
		{
			if (!content.load(carpath, "bump_front", soundptr)) return false;
		}
		tirebump[i].SetBuffer(soundptr);
		tirebump[i].Enable3D(true);
		tirebump[i].Loop(false);
		tirebump[i].SetGain(1.0);
	}

	//set up crash sound
	{
		std::tr1::shared_ptr<SOUNDBUFFER> soundptr;
		if (!content.load(carpath, "crash", soundptr)) return false;
		crashsound.SetBuffer(soundptr);
		crashsound.Enable3D(true);
		crashsound.Loop(false);
		crashsound.SetGain(1.0);
	}

	//set up gear sound
	{
		std::tr1::shared_ptr<SOUNDBUFFER> soundptr;
		if (!content.load(carpath, "gear", soundptr)) return false;
		gearsound.SetBuffer(soundptr);
		gearsound.Enable3D(true);
		gearsound.Loop(false);
		gearsound.SetGain(1.0);
	}

	//set up brake sound
	{
		std::tr1::shared_ptr<SOUNDBUFFER> soundptr;
		if (!content.load(carpath, "brake", soundptr)) return false;
		brakesound.SetBuffer(soundptr);
		brakesound.Enable3D(true);
		brakesound.Loop(false);
		brakesound.SetGain(1.0);
	}

	//set up handbrake sound
	{
		std::tr1::shared_ptr<SOUNDBUFFER> soundptr;
		if (!content.load(carpath, "handbrake", soundptr)) return false;
		handbrakesound.SetBuffer(soundptr);
		handbrakesound.Enable3D(true);
		handbrakesound.Loop(false);
		handbrakesound.SetGain(1.0);
	}

	{
		std::tr1::shared_ptr<SOUNDBUFFER> soundptr;
		if (!content.load(carpath, "wind", soundptr)) return false;
		roadnoise.SetBuffer(soundptr);
		roadnoise.Enable3D(true);
		roadnoise.Loop(true);
		roadnoise.SetGain(0);
		roadnoise.SetPitch(1.0);
		roadnoise.Play();
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

void CAR::SetPosition(const MATHVECTOR <float, 3> & new_position)
{
	btVector3 newpos = ToBulletVector(new_position);
	dynamics.SetPosition(newpos);
	dynamics.AlignWithGround();
}

void CAR::UpdateGraphics()
{
	if (!bodynode.valid()) return;
	assert(dynamics.GetNumBodies() == topnode.Nodes());

	unsigned int i = 0;
	keyed_container<SCENENODE> & childlist = topnode.GetNodelist();
	for (keyed_container<SCENENODE>::iterator ni = childlist.begin(); ni != childlist.end(); ++ni, ++i)
	{
		MATHVECTOR<float, 3> pos = ToMathVector<float>(dynamics.GetPosition(i));
		QUATERNION<float> rot = ToMathQuaternion<float>(dynamics.GetOrientation(i));
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

void CAR::UpdateSounds(float dt)
{
	MATHVECTOR <float, 3> vec = GetPosition();
	roadnoise.SetPosition(vec[0],vec[1],vec[2]);
	crashsound.SetPosition(vec[0],vec[1],vec[2]);
	gearsound.SetPosition(vec[0],vec[1],vec[2]);
	brakesound.SetPosition(vec[0],vec[1],vec[2]);
	handbrakesound.SetPosition(vec[0],vec[1],vec[2]);

	//update engine sounds
	float rpm = GetEngineRPM();
	float throttle = dynamics.GetEngine().GetThrottle();

	btVector3 engine_pos = dynamics.GetEnginePosition();

	float total_gain = 0.0;
	std::list <std::pair <SOUNDSOURCE *, float> > gainlist;

	float loudest = 0.0; //for debugging

	for (std::list <std::pair <ENGINESOUNDINFO, SOUNDSOURCE> >::iterator i = enginesounds.begin(); i != enginesounds.end(); ++i)
	{
		ENGINESOUNDINFO & info = i->first;
		SOUNDSOURCE & sound = i->second;

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
		if (gain > loudest)
		{
			loudest = gain;
		}
		gainlist.push_back(std::pair <SOUNDSOURCE *, float> (&sound, gain));

		float pitch = rpm / info.naturalrpm;
		sound.SetPitch(pitch);

		sound.SetPosition(engine_pos[0], engine_pos[1], engine_pos[2]);
	}

	//normalize gains
	assert(total_gain >= 0.0);
	for (std::list <std::pair <SOUNDSOURCE *, float> >::iterator i = gainlist.begin(); i != gainlist.end(); ++i)
	{
		if (total_gain == 0.0)
		{
			i->first->SetGain(0.0);
		}
		else if (enginesounds.size() == 1 && enginesounds.back().first.power == ENGINESOUNDINFO::BOTH)
		{
			i->first->SetGain(i->second);
		}
		else
		{
			i->first->SetGain(i->second/total_gain);
		}
		//if (i->second == loudest) std::cout << i->first->GetSoundTrack().GetName() << ": " << i->second << std::endl;
	}

	//update tire squeal sounds
	for (int i = 0; i < 4; i++)
	{
		// make sure we don't get overlap
		gravelsound[i].SetGain(0.0);
		grasssound[i].SetGain(0.0);
		tiresqueal[i].SetGain(0.0);

		float squeal = GetTireSquealAmount(WHEEL_POSITION(i));
		float maxgain = 0.3;
		float pitchvariation = 0.4;

		SOUNDSOURCE * thesound;
		const TRACKSURFACE & surface = dynamics.GetWheelContact(WHEEL_POSITION(i)).GetSurface();
		if (surface.type == TRACKSURFACE::ASPHALT)
		{
			thesound = tiresqueal;
		}
		else if (surface.type == TRACKSURFACE::GRASS)
		{
			thesound = grasssound;
			maxgain = 0.4; // up the grass sound volume a little
		}
		else if (surface.type == TRACKSURFACE::GRAVEL)
		{
			thesound = gravelsound;
			maxgain = 0.4;
		}
		else if (surface.type == TRACKSURFACE::CONCRETE)
		{
			thesound = tiresqueal;
			maxgain = 0.3;
			pitchvariation = 0.25;
		}
		else if (surface.type == TRACKSURFACE::SAND)
		{
			thesound = grasssound;
			maxgain = 0.25; // quieter for sand
			pitchvariation = 0.25;
		}
		else
		{
			thesound = tiresqueal;
			maxgain = 0.0;
		}

		// set the sound position
		btVector3 vec = dynamics.GetWheelPosition(WHEEL_POSITION(i));
		thesound[i].SetPosition(vec[0], vec[1], vec[2]);

		const btVector3 & groundvel = dynamics.GetWheelVelocity(WHEEL_POSITION(i));
		thesound[i].SetGain(squeal * maxgain);
		float pitch = (groundvel.length() - 5.0) * 0.1;
		if (pitch < 0)
			pitch = 0;
		if (pitch > 1)
			pitch = 1;
		pitch = 1.0 - pitch;
		pitch *= pitchvariation;
		pitch = pitch + (1.0-pitchvariation);
		if (pitch < 0.1)
			pitch = 0.1;
		if (pitch > 4.0)
			pitch = 4.0;
		thesound[i].SetPitch(pitch);
	}

	//update road noise sound
	{
		btScalar gain = dynamics.GetVelocity().length();
		if (gain < 0) gain = -gain;
		gain *= 0.02;
		gain *= gain;
		if (gain > 1.0)	gain = 1.0;
		roadnoise.SetGain(gain);
		//std::cout << gain << std::endl;
	}
/*
	//update bump noise sound
	{
		for (int i = 0; i < 4; i++)
		{
			suspensionbumpdetection[i].Update(
				dynamics.GetSuspension(WHEEL_POSITION(i)).GetVelocity(),
				dynamics.GetSuspension(WHEEL_POSITION(i)).GetDisplacementFraction(),
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
	{
		crashdetection.Update(GetSpeed(), dt);
		float crashdecel = crashdetection.GetMaxDecel();
		if (crashdecel > 0)
		{
			const float mingainat = 500;
			const float maxgainat = 3000;
			const float mingain = 0.1;
			float gain = (crashdecel-mingainat)/(maxgainat-mingainat);
			if (gain > 1)
				gain = 1;
			if (gain < mingain)
				gain = mingain;

			//std::cout << crashdecel << ", gain: " << gain << std::endl;

			if (!crashsound.Audible())
			{
				crashsound.SetGain(gain);
				crashsound.Stop();
				crashsound.Play();
			}
		}
	}

	//update gear sound
	{
		if (gearsound_check != GetGear())
		{
			float gain = 0.0;
			if (GetEngineRPM() != 0.0)
				gain = GetEngineRPMLimit() / GetEngineRPM();
			if (gain > 0.05)
				gain = 0.05;
			if (gain < 0.025)
				gain = 0.025;

			if (!gearsound.Audible())
			{
				gearsound.SetGain(gain);
				gearsound.Stop();
				gearsound.Play();
			}
			gearsound_check = GetGear();
		}
	}
}

void CAR::Update(double dt)
{
	UpdateGraphics();
	UpdateSounds(dt);
}

void CAR::GetSoundList(std::list <SOUNDSOURCE *> & outputlist)
{
	for (std::list <std::pair <ENGINESOUNDINFO, SOUNDSOURCE> >::iterator i =
		enginesounds.begin(); i != enginesounds.end(); ++i)
	{
		outputlist.push_back(&i->second);
	}

	for (int i = 0; i < 4; i++)
		outputlist.push_back(&tiresqueal[i]);

	for (int i = 0; i < 4; i++)
		outputlist.push_back(&grasssound[i]);

	for (int i = 0; i < 4; i++)
		outputlist.push_back(&gravelsound[i]);

	for (int i = 0; i < 4; i++)
		outputlist.push_back(&tirebump[i]);

	outputlist.push_back(&crashsound);

	outputlist.push_back(&gearsound);

	outputlist.push_back(&brakesound);

	outputlist.push_back(&handbrakesound);

	outputlist.push_back(&roadnoise);
}

void CAR::GetEngineSoundList(std::list <SOUNDSOURCE *> & outputlist)
{
	for (std::list <std::pair <ENGINESOUNDINFO, SOUNDSOURCE> >::iterator i =
		enginesounds.begin(); i != enginesounds.end(); ++i)
	{
		outputlist.push_back(&i->second);
	}
}

void CAR::HandleInputs(const std::vector <float> & inputs, float dt)
{
	assert(inputs.size() == CARINPUT::INVALID); //this looks weird, but it ensures that our inputs vector contains exactly one item per input

	//std::cout << "Throttle: " << inputs[CARINPUT::THROTTLE] << std::endl;
	//std::cout << "Shift up: " << inputs[CARINPUT::SHIFT_UP] << std::endl;

	// recover from a rollover
	if (inputs[CARINPUT::ROLLOVER_RECOVER])
		dynamics.RolloverRecover();

	//set brakes
	dynamics.SetBrake(inputs[CARINPUT::BRAKE]);
	dynamics.SetHandBrake(inputs[CARINPUT::HANDBRAKE]);

	//do steering
	float steer_value = inputs[CARINPUT::STEER_RIGHT];
	if (std::abs(inputs[CARINPUT::STEER_LEFT]) > std::abs(inputs[CARINPUT::STEER_RIGHT])) //use whichever control is larger
		steer_value = -inputs[CARINPUT::STEER_LEFT];
	dynamics.SetSteering(steer_value);
	last_steer = steer_value;
	QUATERNION<float> steer;
	steer.Rotate(-steer_value * steer_angle_max, 0, 0, 1);
	steer_rotation = steer_orientation * steer;

    //start the engine if requested
	if (inputs[CARINPUT::START_ENGINE])
		dynamics.StartEngine();

	//do shifting
	int gear_change = 0;
	if (inputs[CARINPUT::SHIFT_UP] == 1.0)
		gear_change = 1;
	if (inputs[CARINPUT::SHIFT_DOWN] == 1.0)
		gear_change = -1;
	int cur_gear = dynamics.GetTransmission().GetGear();
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

	nosactive = nos > 0;

	dynamics.ShiftGear(new_gear);
	dynamics.SetThrottle(throttle);
	dynamics.SetClutch(clutch);
	dynamics.SetNOS(nos);

	//do driver aid toggles
	if (inputs[CARINPUT::ABS_TOGGLE])
		dynamics.SetABS(!dynamics.GetABSEnabled());
	if (inputs[CARINPUT::TCS_TOGGLE])
		dynamics.SetTCS(!dynamics.GetTCSEnabled());

	//update brake sound
	{
		if (inputs[CARINPUT::BRAKE] > 0 && !brakesound_check)
		{
			if (!brakesound.Audible())
			{
				float gain = 0.1;
				brakesound.SetGain(gain);
				brakesound.Stop();
				brakesound.Play();
			}
			brakesound_check = true;
		}
		if (inputs[CARINPUT::BRAKE] <= 0)
			brakesound_check = false;
	}

	//update handbrake sound
	{
		if (inputs[CARINPUT::HANDBRAKE] > 0 && !handbrakesound_check)
		{
			if (!handbrakesound.Audible())
			{
				float gain = 0.1;
				handbrakesound.SetGain(gain);
				handbrakesound.Stop();
				handbrakesound.Play();
			}
			handbrakesound_check = true;
		}
		if (inputs[CARINPUT::HANDBRAKE] <= 0)
			handbrakesound_check = false;
	}
}

float CAR::GetFeedback()
{
	return dynamics.GetFeedback() / (mz_nominalmax * 0.025);
}

float CAR::GetTireSquealAmount(WHEEL_POSITION i) const
{
	const TRACKSURFACE & surface = dynamics.GetWheelContact(WHEEL_POSITION(i)).GetSurface();
	if (surface.type == TRACKSURFACE::NONE) return 0;

	btQuaternion wheelspace = dynamics.GetUprightOrientation(WHEEL_POSITION(i));
	btVector3 groundvel = quatRotate(wheelspace.inverse(), dynamics.GetWheelVelocity(WHEEL_POSITION(i)));
	float wheelspeed = dynamics.GetWheel(WHEEL_POSITION(i)).GetAngularVelocity() * dynamics.GetTire(WHEEL_POSITION(i)).GetRadius();
	groundvel[0] -= wheelspeed;
	groundvel[1] *= 2.0;
	groundvel[2] = 0;
	float squeal = (groundvel.length() - 3.0) * 0.2;

	double slide = dynamics.GetTire(i).GetSlide() / dynamics.GetTire(i).GetIdealSlide();
	double slip = dynamics.GetTire(i).GetSlip() / dynamics.GetTire(i).GetIdealSlip();
	double maxratio = std::max(std::abs(slide), std::abs(slip));
	float squealfactor = std::max(0.0, maxratio - 1.0);
	squeal *= squealfactor;
	if (squeal < 0) squeal = 0;
	if (squeal > 1) squeal = 1;

	return squeal;
}

void CAR::EnableGlass(bool enable)
{
	SCENENODE & bodynoderef = topnode.GetNode(bodynode);
	keyed_container<DRAWABLE> & normal_blend = bodynoderef.GetDrawlist().normal_blend;
	for (keyed_container<DRAWABLE>::iterator i = normal_blend.begin(); i != normal_blend.end(); ++i)
	{
		i->SetDrawEnable(enable);
	}
}

bool CAR::Serialize(joeserialize::Serializer & s)
{
	_SERIALIZE_(s,dynamics);
	_SERIALIZE_(s,last_steer);
	return true;
}
