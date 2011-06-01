#include "car.h"
#include "carwheelposition.h"
#include "dynamicsworld.h"
#include "tracksurface.h"
#include "carinput.h"
#include "camera.h"
#include "camera_chase.h"
#include "camera_fixed.h"
#include "camera_free.h"
#include "camera_mount.h"
#include "camera_orbit.h"
#include "texturemanager.h"
#include "textureinfo.h"
#include "modelmanager.h"
#include "model_joe03.h"
#include "mesh_gen.h"
#include "soundmanager.h"
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

struct LoadDrawable
{
	const std::string & path;
	const std::string & texsize;
	const int anisotropy;
	TEXTUREMANAGER & textures;
	MODELMANAGER & models;
	std::list<std::tr1::shared_ptr<MODEL> > & modellist;
	std::ostream & error;
	
	LoadDrawable(
		const std::string & path,
		const std::string & texsize,
		const int anisotropy,
		TEXTUREMANAGER & textures,
		MODELMANAGER & models,
		std::list<std::tr1::shared_ptr<MODEL> > & modellist,
		std::ostream & error) :
		path(path),
		texsize(texsize),
		anisotropy(anisotropy),
		textures(textures),
		models(models),
		modellist(modellist),
		error(error)
	{
		// ctor
	}
	
	bool operator()(
		const PTree & cfg,
		SCENENODE & topnode,
		keyed_container<SCENENODE>::handle * nodehandle = 0,
		keyed_container<DRAWABLE>::handle * drawhandle = 0)
	{
		std::vector<std::string> texname;
		if (!cfg.get("texture", texname)) return true;
		
		std::string meshname;
		if (!cfg.get("mesh", meshname, error)) return false;
		
		return operator()(meshname, texname, cfg, topnode, nodehandle, drawhandle);
	}
	
	bool operator()(
		const std::string & meshname,
		const std::vector<std::string> & texname,
		const PTree & cfg,
		SCENENODE & topnode,
		keyed_container<SCENENODE>::handle * nodeptr = 0,
		keyed_container<DRAWABLE>::handle * drawptr = 0)
	{
		DRAWABLE drawable;

		// set textures
		TEXTUREINFO info;
		info.mipmap = true;
		info.anisotropy = anisotropy;
		info.size = texsize;
		std::tr1::shared_ptr<TEXTURE> tex;
		if(texname.size() == 0)
		{
			error << "No texture defined" << std::endl;
			return false;
		}
		if(texname.size() > 0)
		{
			if (!textures.Load(path, texname[0], info, tex)) return false;
			drawable.SetDiffuseMap(tex);
		}
		if(texname.size() > 1)
		{
			if (!textures.Load(path, texname[1], info, tex)) return false;
			drawable.SetMiscMap1(tex);
		}
		if(texname.size() > 2)
		{
			if (!textures.Load(path, texname[2], info, tex)) return false;
			drawable.SetMiscMap2(tex);
		}

		// set mesh
		std::string scale;
		std::tr1::shared_ptr<MODEL> mesh;
		if (!cfg.get("scale", scale))
		{
			if (!models.Load(path, meshname, mesh)) return false;
		}
		else if (!models.Get(path, meshname+scale, mesh))
		{
			MODELMANAGER::const_iterator it;
			if (!models.Load(path, meshname, it)) return false;
			
			MATHVECTOR<float, 3> sc(1, 1, 1);
			cfg.get("scale", sc);
			
			std::tr1::shared_ptr<MODEL> temp(new MODEL());
			temp->SetVertexArray(it->second->GetVertexArray());
			temp->Scale(sc[0], sc[1], sc[2]);
			temp->GenerateMeshMetrics();
			temp->GenerateListID(error);
			
			models.Set(it->first+scale, temp);
			mesh = temp;
		}
		
		if (models.useDrawlists())
		{
			mesh->GenerateListID(error);
		}
		else
		{
			mesh->GenerateVertexArrayObject(error);
		}
		drawable.SetModel(*mesh);
		modellist.push_back(mesh);
		
		// set color
		MATHVECTOR<float, 4> col(1);
		if (cfg.get("color", col))
		{
			drawable.SetColor(col[0], col[1], col[2], col[3]);
		}
		
		// set node
		SCENENODE * node = &topnode;
		if (nodeptr != 0)
		{
			if (!nodeptr->valid())
			{
				*nodeptr = topnode.AddNode();
				assert(nodeptr->valid());
			}
			node = &topnode.GetNode(*nodeptr);
		}
		
		MATHVECTOR<float, 3> pos, rot;
		if (cfg.get("position", pos) | cfg.get("rotation", rot))
		{
			if (node == &topnode)
			{
				// position relative to parent, create child node
				keyed_container <SCENENODE>::handle nodehandle = topnode.AddNode();
				node = &topnode.GetNode(nodehandle);
			}
			node->GetTransform().SetTranslation(pos);
			node->GetTransform().SetRotation(QUATERNION<float>(rot[0]/180*M_PI, rot[1]/180*M_PI, rot[2]/180*M_PI));
		}
		
		// set drawable
		keyed_container<DRAWABLE>::handle drawtemp;
		keyed_container<DRAWABLE>::handle * draw = &drawtemp;
		if (drawptr != 0) draw = drawptr;
		
		std::string drawtype;
		if (cfg.get("draw", drawtype))
		{
			if (drawtype == "emissive")
			{
				drawable.SetDecal(true);
				*draw = node->GetDrawlist().lights_emissive.insert(drawable);
			}
			else if (drawtype == "transparent")
			{
				*draw = node->GetDrawlist().normal_blend.insert(drawable);
			}
		}
		else
		{
			*draw = node->GetDrawlist().car_noblend.insert(drawable);
		}
		
		return true;
	}
};

struct LoadBody
{
	SCENENODE & topnode;
	keyed_container<SCENENODE>::handle & bodynode;
	LoadDrawable & loadDrawable;
	bool damage;
	
	LoadBody(
		SCENENODE & topnode,
		keyed_container<SCENENODE>::handle & bodynode,
		LoadDrawable & loadDrawable,
		bool damage) :
		topnode(topnode),
		bodynode(bodynode),
		loadDrawable(loadDrawable),
		damage(damage)
	{
		// ctor
	}

	bool operator()(const PTree & cfg)
	{
		const PTree * link;
		if (damage && cfg.get("link", link))
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
	MODELMANAGER & models = loadDrawable.models;
	
	std::string tiredim;
	const PTree * cfg_tire;
	if (!cfg_wheel.get("tire", cfg_tire, error_output)) return false;
	if (!cfg_tire->get("size", tiredim, error_output)) return false;
	
	std::string brakename;
	const PTree * cfg_brake;
	if (!cfg_wheel.get("brake", cfg_brake, error_output)) return false;
	
	// load wheel
	std::string meshname;
	std::vector<std::string> texname;
	MODELMANAGER::const_iterator it;
	if (!cfg_wheel.get("mesh", meshname, error_output)) return false;
	if (!cfg_wheel.get("texture", texname, error_output)) return false;
	if (!models.Get(loadDrawable.path, meshname+tiredim, it))
	{
		if (!models.Load(loadDrawable.path, meshname, it)) return false;
		
		MATHVECTOR<float, 3> d(0);
		cfg_tire->get("size", d);
		float width = d[0] * 0.001;
		float diameter = d[2] * 0.0254;
		
		VERTEXARRAY varray;
		std::tr1::shared_ptr<MODEL> temp(new MODEL_JOE03());
		temp->SetVertexArray(it->second->GetVertexArray());
		temp->Translate(-0.75 * 0.5, 0, 0);
		temp->Scale(width, diameter, diameter);
		MESHGEN::mg_rim(varray, d[0], d[1], d[2], 10);
		temp->SetVertexArray(varray + temp->GetVertexArray());
		temp->GenerateMeshMetrics();
		temp->GenerateListID(error_output);
		
		models.Set(it->first+tiredim, temp);
	}
	if (!loadDrawable(meshname+tiredim, texname, cfg_wheel, topnode, &wheelnode)) return false;
	
	// load tire
	texname.clear();
	if (!cfg_tire->get("texture", texname, error_output)) return false;
	if (!models.Get("", "tire"+tiredim, it))
	{
		MATHVECTOR<float, 3> d(0);
		cfg_tire->get("size", d);
		
		VERTEXARRAY varray;
		std::tr1::shared_ptr<MODEL> temp(new MODEL_JOE03());
		MESHGEN::mg_tire(varray, d[0], d[1], d[2]);
		temp->SetVertexArray(varray);
		temp->GenerateMeshMetrics();
		temp->GenerateListID(error_output);
		
		models.Set("tire"+tiredim, temp);
	}
	if (!loadDrawable("tire"+tiredim, texname, *cfg_tire, topnode.GetNode(wheelnode))) return false;
	
	// load brake (optional)
	texname.clear();
	std::string radius;
	cfg_brake->get("radius", radius);
	if (!cfg_brake->get("texture", texname)) return true;
	if (!models.Get("", "brake"+radius, it))
	{
		float r;
		std::stringstream s(radius);
		s >> r;
		float diameter_mm = r * 2 * 1000;
		float thickness_mm = 0.025 * 1000;
		
		VERTEXARRAY varray;
		std::tr1::shared_ptr<MODEL> temp(new MODEL_JOE03());
		MESHGEN::mg_brake_rotor(varray, diameter_mm, thickness_mm);
		temp->SetVertexArray(varray);
		temp->GenerateMeshMetrics();
		temp->GenerateListID(error_output);
		
		models.Set("brake"+radius, temp);
	}
	if (!loadDrawable("brake"+radius, texname, *cfg_brake, topnode.GetNode(wheelnode))) return false;
	
	/* load fender (optional)
	keyed_container<SCENENODE>::handle floatingnode = topnode.AddNode();
	const PTree * cfg_fender;
	if (cfg_wheel.get("fender", cfg_fender))
	{
		floatingnode = topnode.AddNode();
		if (!loadDrawable(*cfg_fender, topnode.GetNode(floatingnode))) return false;
		
		MATHVECTOR<float, 3> pos = topnode.GetNode(wheelnode).GetTransform().GetTranslation();
		topnode.GetNode(floatingnode).GetTransform().SetTranslation(pos);
	}
	*/

	return true;
}

static bool LoadCameras(
	const PTree & cfg,
	const float camerabounce,
	CAMERA_SYSTEM & cameras,
	std::ostream & error_output)
{
	CAMERA_MOUNT * hood_cam = new CAMERA_MOUNT("hood");
	CAMERA_MOUNT * driver_cam = new CAMERA_MOUNT("incar");
	driver_cam->SetEffectStrength(camerabounce);
	hood_cam->SetEffectStrength(camerabounce);

	MATHVECTOR<float, 3> pos(0), hoodpos(0);
	if (!cfg.get("camera.view-position", pos, error_output)) return false;
	MATHVECTOR <float, 3> cam_offset(pos[0], pos[1], pos[2]);
	driver_cam->SetOffset(cam_offset);

	if (!cfg.get("camera.hood-mounted-view-position", hoodpos, error_output))
	{
		cam_offset.Set(pos[0] + 1, 0, pos[2]);
	}
	else
	{
		cam_offset.Set(hoodpos[0], hoodpos[1], hoodpos[2]);
	}
	hood_cam->SetOffset(cam_offset);

	float view_stiffness = 0.0;
	cfg.get("camera.view-stiffness", view_stiffness);
	driver_cam->SetStiffness(view_stiffness);
	hood_cam->SetStiffness(view_stiffness);
	cameras.Add(hood_cam);
	cameras.Add(driver_cam);

	CAMERA_FIXED * cam_chaserigid = new CAMERA_FIXED("chaserigid");
	cam_chaserigid->SetOffset(0, -6, 1.5);
	cameras.Add(cam_chaserigid);

	CAMERA_CHASE * cam_chase = new CAMERA_CHASE("chase");
	cam_chase->SetChaseHeight(2.0);
	cameras.Add(cam_chase);

	cameras.Add(new CAMERA_ORBIT("orbit"));
	cameras.Add(new CAMERA_FREE("free"));
/*
	// load additional views
	int i = 1;
	std::string istr = "1";
	std::string view_name;
	while(cfg.GetParam("view.name-" + istr, view_name))
	{
		float pos[3], angle[3];
		if (!cfg.GetParam("view.position-" + istr, pos)) continue;
		if (!cfg.GetParam("view.angle-" + istr, angle)) continue;

		CAMERA_MOUNT* next_view = new CAMERA_MOUNT(view_name);

		MATHVECTOR <float, 3> view_offset;
		view_offset.Set(pos);
		
		next_view->SetOffset(view_offset);
		next_view->SetRotation(angle[0] * 3.141593/180.0, angle[1] * 3.141593/180.0);
		cameras.Add(next_view);

		std::stringstream sstr;
		sstr << ++i;
		istr = sstr.str();
	}
*/
	return true;
}


// draw collision shape bounding boxes
static void DrawShape(
	SCENENODE & bodynoderef,
	const btCollisionShape * shape,
	const btTransform & transform,
	MODELMANAGER & models,
	std::list<std::tr1::shared_ptr<MODEL> > & modellist,
	std::ostream & error_output)
{
	if (shape->isCompound())
	{
		const btCompoundShape * compound = (const btCompoundShape *)shape;
		for (int i = 0; i < compound->getNumChildShapes(); ++i)
		{
			const btCollisionShape * childshape = compound->getChildShape(i);
			btTransform childtransform = transform * compound->getChildTransform(i);
			DrawShape(bodynoderef, childshape, childtransform, models, modellist, error_output);
		}
	}
	else
	{
		btVector3 min, max;
		shape->getAabb(btTransform::getIdentity(), min, max);
		
		MATHVECTOR<float,3> pos(ToMathVector<float>(transform.getOrigin()));
		MATHVECTOR<float,3> size(ToMathVector<float>(max - min));
		
		keyed_container<SCENENODE>::handle node = bodynoderef.AddNode();
		SCENENODE & noderef = bodynoderef.GetNode(node);
		noderef.GetTransform().SetTranslation(pos);
		
		VERTEXARRAY varray;
		varray.SetToUnitCube();
		varray.Scale(size[0], size[1], size[2]);
		
		MODEL * model = new MODEL();
		model->BuildFromVertexArray(varray, error_output);
		if (models.useDrawlists())
			model->GenerateListID(error_output);
		else
			model->GenerateVertexArrayObject(error_output);
		
		keyed_container<DRAWABLE> & dlist = GetDrawlist(noderef, NOBLEND);
		keyed_container<DRAWABLE>::handle draw = dlist.insert(DRAWABLE());
		DRAWABLE & drawref = dlist.get(draw);
		drawref.SetColor(1, 0, 0);
		drawref.SetModel(*model);
		
		modellist.push_back(std::tr1::shared_ptr<MODEL>(model));
	}
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

bool CAR::LoadLight(
	const PTree & cfg,
	MODELMANAGER & models,
	std::ostream & error_output)
{
	float radius;
	MATHVECTOR<float, 3> pos(0), col(0);
	if (!cfg.get("position", pos, error_output)) return false;
	if (!cfg.get("color", col, error_output)) return false;
	if (!cfg.get("radius", radius, error_output)) return false;
	
	lights.push_back(LIGHT());
	SCENENODE & bodynoderef = topnode.GetNode(bodynode);
	lights.back().node = bodynoderef.AddNode();
	SCENENODE & node = bodynoderef.GetNode(lights.back().node);
	MODEL & model = lights.back().model;
	VERTEXARRAY varray;
	varray.SetToUnitCube();
	varray.Scale(radius, radius, radius);
	node.GetTransform().SetTranslation(MATHVECTOR<float,3>(pos[0], pos[1], pos[2]));
	model.BuildFromVertexArray(varray, error_output);
	if (models.useDrawlists())
		model.GenerateListID(error_output);
	else
		model.GenerateVertexArrayObject(error_output);
	
	keyed_container <DRAWABLE> & dlist = GetDrawlist(node, OMNI);
	lights.back().draw = dlist.insert(DRAWABLE());
	DRAWABLE & draw = dlist.get(lights.back().draw);
	draw.SetColor(col[0], col[1], col[2]);
	draw.SetModel(model);
	draw.SetCull(true, true);
	draw.SetDrawEnable(false);
	
	return true;
}

bool CAR::LoadGraphics(
	const PTree & cfg,
	const std::string & carpath,
	const std::string & carname,
	const std::string & partspath,
	const MATHVECTOR <float, 3> & carcolor,
	const std::string & carpaint,
	const std::string & texsize,
	const int anisotropy,
	const float camerabounce,
	const bool damage,
	const bool debugmode,
	TEXTUREMANAGER & textures,
	MODELMANAGER & models,
	std::ostream & info_output,
	std::ostream & error_output)
{
	//write_inf(cfg, std::cerr);
	cartype = carname;
	LoadDrawable loadDrawable(carpath, texsize, anisotropy, textures, models, modellist, error_output);
	
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
	const PTree * cfg_wheel;
	if (!cfg.get("wheel", cfg_wheel, error_output)) return false;
	for (PTree::const_iterator i = cfg_wheel->begin(); i != cfg_wheel->end(); ++i)
	{
		if (!LoadWheel(i->second, loadDrawable, topnode, error_output))
		{
			return false;
		}
	}
	
	// load drawables
	LoadBody loadBody(topnode, bodynode, loadDrawable, damage);
	SCENENODE & bodynoderef = topnode.GetNode(bodynode);
	for(PTree::const_iterator i = cfg.begin(); i != cfg.end(); ++i)
	{
		if (i->first != "body" &&
			i->first != "wheel" &&
			i->first != "steering" &&
			i->first != "light-brake" &&
			i->first != "light-reverse")
		{
			i->second.forEachRecursive(loadBody);
		}
	}
	
	// load steering wheel
	const PTree * cfg_steer;
	if (cfg.get("steering", cfg_steer))
	{
		if (!loadDrawable(*cfg_steer, bodynoderef, &steernode, 0)) return false;
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
		if (!LoadLight(*cfg_light, models, error_output)) return false;
		
		std::stringstream sstr;
		sstr << ++i;
		istr = sstr.str();
	}
	i = 0;
	istr = "0";
	while (cfg.get("light-reverse-"+istr, cfg_light))
	{
		if (!LoadLight(*cfg_light, models, error_output)) return false;

		std::stringstream sstr;
		sstr << ++i;
		istr = sstr.str();
	}
	
	// load car brake/reverse graphics (optional)
	if (cfg.get("light-brake", cfg_light))
	{
		if (!loadDrawable(*cfg_light, bodynoderef, 0, &brakelights)) return false;
	}
	if (cfg.get("light-reverse", cfg_light))
	{
		if (!loadDrawable(*cfg_light, bodynoderef, 0, &reverselights)) return false;
	}
	
	if (!LoadCameras(cfg, camerabounce, cameras, error_output)) return false;
	
	SetColor(carcolor[0], carcolor[1], carcolor[2]);
	
	lookbehind = false;
	
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
	MODELMANAGER & models,
	DynamicsWorld & world,
	std::ostream & info_output,
	std::ostream & error_output)
{
	std::string carmodel;
	std::tr1::shared_ptr<MODEL> modelptr;
	if (!cfg.get("body.mesh", carmodel, error_output)) return false;
	if (!models.Load(carpath, carmodel, modelptr)) return false;
	
	btVector3 size = ToBulletVector(modelptr->GetAABB().GetSize());
	btVector3 center = ToBulletVector(modelptr->GetAABB().GetCenter());
	btVector3 position = ToBulletVector(initial_position);
	btQuaternion rotation = ToBulletQuaternion(initial_orientation);
	
	if (!dynamics.Load(cfg, size, center, position, rotation, damage, world, error_output)) return false;
	dynamics.SetABS(defaultabs);
	dynamics.SetTCS(defaulttcs);
	
	mz_nominalmax = (GetTireMaxMz(FRONT_LEFT) + GetTireMaxMz(FRONT_RIGHT)) * 0.5;
	
	// draw collision shape ounding boxes
	//btTransform offset(btTransform::getIdentity());
	//offset.setOrigin(btVector3(0, -0.206458, -0.283854));
	//DrawShape(topnode.GetNode(bodynode), dynamics.GetShape(), offset, models, modellist, error_output);
	
	return true;
}

bool CAR::LoadSounds(
	const std::string & carpath,
	const std::string & carname,
	const SOUNDINFO & soundinfo,
	SOUNDMANAGER & sounds,
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
			if (!sounds.Load(carpath, filename, soundinfo, soundptr)) return false;

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
		if (!sounds.Load(carpath, "engine", soundinfo, soundptr)) return false;
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
		if (!sounds.Load(carpath, "tire_squeal", soundinfo, soundptr)) return false;
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
		if (!sounds.Load(carpath, "gravel", soundinfo, soundptr)) return false;
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
		if (!sounds.Load(carpath, "grass", soundinfo, soundptr)) return false;
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
			if (!sounds.Load(carpath, "bump_rear", soundinfo, soundptr)) return false;
		}
		else
		{
			if (!sounds.Load(carpath, "bump_front", soundinfo, soundptr)) return false;
		}
		tirebump[i].SetBuffer(soundptr);
		tirebump[i].Enable3D(true);
		tirebump[i].Loop(false);
		tirebump[i].SetGain(1.0);
	}

	//set up crash sound
	{
		std::tr1::shared_ptr<SOUNDBUFFER> soundptr;
		if (!sounds.Load(carpath, "crash", soundinfo, soundptr)) return false;
		crashsound.SetBuffer(soundptr);
		crashsound.Enable3D(true);
		crashsound.Loop(false);
		crashsound.SetGain(1.0);
	}

	//set up gear sound
	{
		std::tr1::shared_ptr<SOUNDBUFFER> soundptr;
		if (!sounds.Load(carpath, "gear", soundinfo, soundptr)) return false;
		gearsound.SetBuffer(soundptr);
		gearsound.Enable3D(true);
		gearsound.Loop(false);
		gearsound.SetGain(1.0);
	}

	//set up brake sound
	{
		std::tr1::shared_ptr<SOUNDBUFFER> soundptr;
		if (!sounds.Load(carpath, "brake", soundinfo, soundptr)) return false;
		brakesound.SetBuffer(soundptr);
		brakesound.Enable3D(true);
		brakesound.Loop(false);
		brakesound.SetGain(1.0);
	}

	//set up handbrake sound
	{
		std::tr1::shared_ptr<SOUNDBUFFER> soundptr;
		if (!sounds.Load(carpath, "handbrake", soundinfo, soundptr)) return false;
		handbrakesound.SetBuffer(soundptr);
		handbrakesound.Enable3D(true);
		handbrakesound.Loop(false);
		handbrakesound.SetGain(1.0);
	}

	{
		std::tr1::shared_ptr<SOUNDBUFFER> soundptr;
		if (!sounds.Load(carpath, "wind", soundinfo, soundptr)) return false;
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
	
	QUATERNION <float> rot = GetOrientation();
	cameras.Active()->Reset(new_position, rot);
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

void CAR::UpdateCameras(float dt)
{
	MATHVECTOR <float, 3> pos = ToMathVector<float>(dynamics.GetPosition());
	QUATERNION <float> rot = ToMathQuaternion<float>(dynamics.GetOrientation());
	
	// reverse the camera direction
	if (lookbehind)
	{
		rot.Rotate(M_PI, 0, 0, 1);
	}
	
	cameras.Active()->Update(pos, rot, dt);
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

	//update bump noise sound
	{
		for (int i = 0; i < 4; i++)
		{
//			suspensionbumpdetection[i].Update(
//				dynamics.GetSuspension(WHEEL_POSITION(i)).GetVelocity(),
//				dynamics.GetSuspension(WHEEL_POSITION(i)).GetDisplacementFraction(),
//				dt);
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
	dynamics.Update();
	UpdateGraphics();
	UpdateCameras(dt);
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
	if(inputs[CARINPUT::ROLLOVER_RECOVER])
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

	dynamics.ShiftGear(new_gear);
	dynamics.SetThrottle(throttle);
	dynamics.SetClutch(clutch);

	//do driver aid toggles
	if (inputs[CARINPUT::ABS_TOGGLE])
		dynamics.SetABS(!dynamics.GetABSEnabled());
	if (inputs[CARINPUT::TCS_TOGGLE])
		dynamics.SetTCS(!dynamics.GetTCSEnabled());

	// check for rear view button
	if (inputs[CARINPUT::REAR_VIEW])
	{
		lookbehind = true;
	}
	else
	{
		lookbehind = false;
	}

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
		if(inputs[CARINPUT::BRAKE] <= 0)
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
		if(inputs[CARINPUT::HANDBRAKE] <= 0)
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
