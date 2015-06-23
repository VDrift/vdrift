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

#include "cargraphics.h"
#include "physics/carinput.h"
#include "loaddrawable.h"
#include "loadcamera.h"
#include "camera.h"
#include "tobullet.h"
#include "physics/cardynamics.h"
#include "graphics/textureinfo.h"
#include "graphics/mesh_gen.h"
#include "graphics/model_obj.h"
#include "content/contentmanager.h"
#include "cfg/ptree.h"

template <typename T>
static inline T clamp(T val, T min, T max)
{
	return (val < max) ? (val > min) ? val : min : max;
}

struct LoadBody
{
	SceneNode & topnode;
	SceneNode::Handle & bodynode;
	LoadDrawable & loadDrawable;

	LoadBody(
		SceneNode & topnode,
		SceneNode::Handle & bodynode,
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
	SceneNode::Handle wheelnode = topnode.AddNode();
	ContentManager & content = loadDrawable.content;
	const std::string& path = loadDrawable.path;

	std::string meshname;
	std::vector<std::string> texname;
	std::shared_ptr<Model> mesh;
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

CarGraphics::CarGraphics() :
	steer_angle_max(0),
	applied_brakes(0),
	interior_view(false),
	loaded(false)
{
	// ctor
}

CarGraphics::CarGraphics(const CarGraphics & other) :
	steer_angle_max(0),
	applied_brakes(0),
	interior_view(false),
	loaded(false)
{
	// we don't really support copying of these suckers
	assert(!other.loaded);
}

CarGraphics & CarGraphics::operator= (const CarGraphics & other)
{
	// we don't really support copying of these suckers
	assert(!other.loaded && !loaded);
	return *this;
}

CarGraphics::~CarGraphics()
{
	ClearCameras();
}

bool CarGraphics::Load(
	const PTree & cfg,
	const std::string & carpath,
	const std::string & /*carname*/,
	const std::string & carwheel,
	const std::string & carpaint,
	const Vec3 & carcolor,
	const int anisotropy,
	const float camerabounce,
	ContentManager & content,
	std::ostream & error_output)
{
	assert(!loaded);

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

	std::shared_ptr<PTree> sel_wheel;
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
		steer_rotation = steer_orientation;
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

		std::ostringstream sstr;
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

		std::ostringstream sstr;
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

	if (!LoadCameras(cfg, camerabounce, error_output))
	{
		return false;
	}

	SetColor(carcolor[0], carcolor[1], carcolor[2]);
	loaded = true;
	return true;
}

void CarGraphics::Update(const std::vector<float> & inputs)
{
	assert(inputs.size() >= CarInput::INVALID);

	applied_brakes = inputs[CarInput::BRAKE];

	float steer_value = inputs[CarInput::STEER_RIGHT] - inputs[CarInput::STEER_LEFT];

	Quat steer;
	steer.Rotate(-steer_value * steer_angle_max, 0, 0, 1);
	steer_rotation = steer_orientation * steer;
}

void CarGraphics::Update(const CarDynamics & dynamics)
{
	if (!bodynode.valid()) return;
	assert(dynamics.GetNumBodies() == topnode.GetNodeList().size());

	unsigned i = 0;
	SceneNode::List & childlist = topnode.GetNodeList();
	for (SceneNode::List::iterator ni = childlist.begin(); ni != childlist.end(); ++ni, ++i)
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
		Drawable & draw = node.GetDrawList().lights_omni.get(i->draw);
		draw.SetDrawEnable(applied_brakes > 0);
	}
	if (brakelights.valid())
	{
		Drawable & draw = bodynoderef.GetDrawList().lights_emissive.get(brakelights);
		draw.SetDrawEnable(applied_brakes > 0);
	}
	if (reverselights.valid())
	{
		Drawable & draw = bodynoderef.GetDrawList().lights_emissive.get(reverselights);
		draw.SetDrawEnable(dynamics.GetTransmission().GetGear() < 0);
	}

	// steering
	if (steernode.valid())
	{
		SceneNode & steernoderef = bodynoderef.GetNode(steernode);
		steernoderef.GetTransform().SetRotation(steer_rotation);
	}
}

void CarGraphics::SetColor(float r, float g, float b)
{
	SceneNode & bodynoderef = topnode.GetNode(bodynode);
	keyed_container<Drawable> & car_noblend = bodynoderef.GetDrawList().car_noblend;
	for (keyed_container<Drawable>::iterator i = car_noblend.begin(); i != car_noblend.end(); ++i)
	{
		i->SetColor(r, g, b, 1);
	}
}

void CarGraphics::EnableInteriorView(bool value)
{
	if (interior_view == value) return;

	// disable glass drawing
	interior_view = value;
	SceneNode & bodynoderef = topnode.GetNode(bodynode);
	keyed_container<Drawable> & normal_blend = bodynoderef.GetDrawList().normal_blend;
	for (keyed_container<Drawable>::iterator i = normal_blend.begin(); i != normal_blend.end(); ++i)
	{
		i->SetDrawEnable(!interior_view);
	}
}

SceneNode & CarGraphics::GetNode()
{
	return topnode;
}

const std::vector<Camera*> & CarGraphics::GetCameras() const
{
	return cameras;
}

bool CarGraphics::LoadLight(
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

	std::shared_ptr<Model> mesh;
	if (!content.get(mesh, "", "cube" + radiusstr))
	{
		VertexArray varray;
		varray.SetToUnitCube();
		varray.Scale(radius, radius, radius);
		content.load(mesh, "", "cube" + radiusstr, varray);
	}
	models.insert(mesh);

	keyed_container <Drawable> & dlist = node.GetDrawList().lights_omni;
	lights.back().draw = dlist.insert(Drawable());

	Drawable & draw = dlist.get(lights.back().draw);
	draw.SetColor(col[0], col[1], col[2]);
	draw.SetModel(*mesh);
	draw.SetCull(true);
	draw.SetDrawEnable(false);

	return true;
}

bool CarGraphics::LoadCameras(
	const PTree & cfg,
	const float cambounce,
	std::ostream & error_output)
{
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
		Camera * cam = LoadCamera(i->second, cambounce, error_output);
		if (!cam)
		{
			ClearCameras();
			return false;
		}
		cameras.push_back(cam);
	}

	return true;
}

void CarGraphics::ClearCameras()
{
	for (size_t i = 0; i < cameras.size(); ++i)
	{
		delete cameras[i];
	}
	cameras.clear();
}
