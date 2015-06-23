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

#include "trackloader.h"
#include "loadcollisionshape.h"
#include "physics/dynamicsworld.h"
#include "coordinatesystem.h"
#include "tobullet.h"
#include "k1999.h"
#include "content/contentmanager.h"
#include "graphics/texture.h"
#include "graphics/model.h"

#include "BulletCollision/CollisionShapes/btBoxShape.h"
#include "BulletCollision/CollisionShapes/btBvhTriangleMeshShape.h"
#include "BulletCollision/CollisionShapes/btCompoundShape.h"
#include "BulletCollision/CollisionShapes/btTriangleIndexVertexArray.h"
#include "BulletDynamics/Dynamics/btRigidBody.h"

#define EXTBULLET

static inline std::istream & operator >> (std::istream & lhs, btVector3 & rhs)
{
	std::string str;
	for (int i = 0; i < 3 && !lhs.eof(); ++i)
	{
		std::getline(lhs, str, ',');
		std::istringstream s(str);
		s >> rhs[i];
	}
	return lhs;
}

static inline std::istream & operator >> (std::istream & lhs, std::vector<std::string> & rhs)
{
	std::string str;
	for (size_t i = 0; i < rhs.size() && !lhs.eof(); ++i)
	{
		std::getline(lhs, str, ',');
		std::istringstream s(str);
		s >> rhs[i];
	}
	return lhs;
}

static btIndexedMesh GetIndexedMesh(const Model & model)
{
	const float * vertices;
	int vcount;
	const unsigned int * faces;
	int fcount;
	model.GetVertexArray().GetVertices(vertices, vcount);
	model.GetVertexArray().GetFaces(faces, fcount);

	assert(fcount % 3 == 0); //Face count is not a multiple of 3

	btIndexedMesh mesh;
	mesh.m_numTriangles = fcount / 3;
	mesh.m_triangleIndexBase = (const unsigned char *)faces;
	mesh.m_triangleIndexStride = sizeof(unsigned int) * 3;
	mesh.m_numVertices = vcount / 3;
	mesh.m_vertexBase = (const unsigned char *)vertices;
	mesh.m_vertexStride = sizeof(float) * 3;
	mesh.m_vertexType = PHY_FLOAT;
	return mesh;
}

struct Track::Loader::Object
{
	std::shared_ptr<Model> model;
	std::string texture;
	int transparent_blend;
	int clamptexture;
	int surface;
	bool mipmap;
	bool nolighting;
	bool skybox;
	bool collideable;
	bool cached;
};

Track::Loader::Loader(
	ContentManager & content,
	DynamicsWorld & world,
	Track::Data & data,
	std::ostream & info_output,
	std::ostream & error_output,
	const std::string & trackpath,
	const std::string & trackdir,
	const std::string & texturedir,
	const std::string & sharedobjectpath,
	const int anisotropy,
	const bool reverse,
	const bool dynamic_objects,
	const bool dynamic_shadows) :
	content(content),
	world(world),
	data(data),
	info_output(info_output),
	error_output(error_output),
	trackpath(trackpath),
	trackdir(trackdir),
	texturedir(texturedir),
	sharedobjectpath(sharedobjectpath),
	anisotropy(anisotropy),
	dynamic_objects(dynamic_objects),
	dynamic_shadows(dynamic_shadows),
	packload(false),
	numobjects(0),
	numloaded(0),
	params_per_object(17),
	expected_params(17),
	min_params(14),
	error(false),
	list(false),
	track_shape(0)
{
	objectpath = trackpath + "/objects";
	objectdir = trackdir + "/objects";
	data.reverse = reverse;
}

Track::Loader::~Loader()
{
	Clear();
}

void Track::Loader::Clear()
{
	bodies.clear();
	objectfile.close();
	pack.Close();
}

bool Track::Loader::BeginLoad()
{
	Clear();

	info_output << "Loading track from path: " << trackpath << std::endl;

	if (!LoadSurfaces())
	{
		info_output << "No Surfaces File. Continuing with standard surfaces" << std::endl;
	}

	if (!LoadRoads())
	{
		error_output << "Error during road loading; continuing with an unsmoothed track" << std::endl;
		data.roads.clear();
	}

	if (!CreateRacingLines())
	{
		return false;
	}

	// load info
	std::string info_path = trackpath + "/track.txt";
	std::ifstream file(info_path.c_str());
	if (!file.good())
	{
		error_output << "Can't find track configfile: " << info_path << std::endl;
		return false;
	}

	// parse info
	PTree info;
	read_ini(file, info);
	info.get("cull faces", data.cull);
	info.get("vertical tracking skyboxes", data.vertical_tracking_skyboxes);

	if (!LoadStartPositions(info))
	{
		return false;
	}

	if (!LoadLapSections(info))
	{
		return false;
	}

	if (!BeginObjectLoad())
	{
		return false;
	}

	return true;
}

bool Track::Loader::ContinueLoad()
{
	if (data.loaded)
	{
		return true;
	}

	std::pair <bool, bool> loadstatus = ContinueObjectLoad();
	if (loadstatus.first)
	{
		return false;
	}

	if (!loadstatus.second)
	{
#ifndef EXTBULLET
		btCollisionObject * track_object = new btCollisionObject();
		//track_shape->createAabbTreeFromChildren();
		track_object->setCollisionShape(track_shape);
		world.addCollisionObject(track_object);
		data.objects.push_back(track_object);
		data.shapes.push_back(track_shape);
		track_shape = 0;
#endif
		data.loaded = true;
		Clear();
	}

	return true;
}

bool Track::Loader::BeginObjectLoad()
{
#ifndef EXTBULLET
	assert(track_shape == 0);
	track_shape = new btCompoundShape(true);
#endif

	list = true;
	packload = pack.Load(objectpath + "/objects.jpk");

	std::string objectlist = objectpath + "/list.txt";
	objectfile.open(objectlist.c_str());
	if (objectfile.good())
	{
		return BeginOld();
	}

	if (Begin())
	{
		list = false;
		return true;
	}

	return false;
}

std::pair<bool, bool> Track::Loader::ContinueObjectLoad()
{
	if (error)
	{
		return std::make_pair(true, false);
	}

	if (list)
	{
		return ContinueOld();
	}

	return Continue();
}

bool Track::Loader::Begin()
{
	content.load(track_config, objectdir, "objects.txt");
	if (track_config.get())
	{
		nodes = 0;
		if (track_config->get("object", nodes))
		{
			node_it = nodes->begin();
			numobjects = nodes->size();
			data.meshes.reserve(numobjects);
			return true;
		}
	}
	return false;
}

std::pair<bool, bool> Track::Loader::Continue()
{
	if (node_it == nodes->end())
	{
		return std::make_pair(false, false);
	}

	if (!LoadNode(node_it->second))
	{
		return std::make_pair(true, false);
	}

	node_it++;

	return std::make_pair(false, true);
}

bool Track::Loader::LoadShape(const PTree & cfg, const Model & model, Body & body)
{
	if (body.mass < 1E-3)
	{
		btTriangleIndexVertexArray * mesh = new btTriangleIndexVertexArray();
		mesh->addIndexedMesh(GetIndexedMesh(model));
		data.meshes.push_back(mesh);
		body.mesh = mesh;

		int surface = 0;
		cfg.get("surface", surface);
		if (surface >= (int)data.surfaces.size())
		{
			surface = 0;
		}

		btBvhTriangleMeshShape * shape = new btBvhTriangleMeshShape(mesh, true);
		shape->setUserPointer((void*)&data.surfaces[surface]);
		data.shapes.push_back(shape);
		body.shape = shape;
	}
	else
	{
		btVector3 center(0, 0, 0);
		cfg.get("mass-center", center);
		btTransform transform = btTransform::getIdentity();
		transform.getOrigin() -= center;

		btCompoundShape * compound = 0;
		btCollisionShape * shape = 0;
		LoadCollisionShape(cfg, transform, shape, compound);

		if (!shape)
		{
			// fall back to model bounding box
			btVector3 size = ToBulletVector(model.GetSize());
			shape = new btBoxShape(size * 0.5);
			center = center + ToBulletVector(model.GetCenter());
		}
		if (compound)
		{
			shape = compound;
		}
		data.shapes.push_back(shape);

		shape->calculateLocalInertia(body.mass, body.inertia);
		body.shape = shape;
		body.center = center;
	}

	return true;
}

Track::Loader::body_iterator Track::Loader::LoadBody(const PTree & cfg)
{
	Body body;
	std::string texture_str;
	std::string model_name;
	int clampuv = 0;
	bool mipmap = true;
	bool alphablend = false;
	bool doublesided = false;
	bool isashadow = false;

	cfg.get("texture", texture_str, error_output);
	cfg.get("model", model_name, error_output);
	cfg.get("clampuv", clampuv);
	cfg.get("mipmap", mipmap);
	cfg.get("alphablend", alphablend);
	cfg.get("doublesided", doublesided);
	cfg.get("isashadow", isashadow);
	cfg.get("skybox", body.skybox);
	cfg.get("nolighting", body.nolighting);

	std::vector<std::string> texture_names(3);
	std::istringstream s(texture_str);
	s >> texture_names;

	// set relative path for models and textures, ugly hack
	// need to identify body references
	std::string name;
	if (cfg.value() == "body" && cfg.parent())
	{
		name = cfg.parent()->value();
	}
	else
	{
		name = cfg.value();
		size_t npos = name.rfind("/");
		if (npos < name.length())
		{
			std::string rel_path = name.substr(0, npos+1);
			model_name = rel_path + model_name;
			texture_names[0] = rel_path + texture_names[0];
			if (!texture_names[1].empty())
				texture_names[1] = rel_path + texture_names[1];
			if (!texture_names[2].empty())
				texture_names[2] = rel_path + texture_names[2];
		}
	}

	if (dynamic_shadows && isashadow)
	{
		return bodies.end();
	}

	std::shared_ptr<Model> model;
	if ((packload && content.load(model, objectdir, model_name, pack)) ||
		content.load(model, objectdir, model_name))
	{
		data.models.insert(model);
	}
	else
	{
		info_output << "Failed to load body " << cfg.value() << " model " << model_name << std::endl;
		return bodies.end();
	}

	body.collidable = cfg.get("mass", body.mass);
	if (body.collidable)
	{
		LoadShape(cfg, *model, body);
	}

	// load textures
	std::shared_ptr<Texture> tex[3];
	TextureInfo texinfo;
	texinfo.mipmap = mipmap || anisotropy; //always mipmap if anisotropy is on
	texinfo.anisotropy = anisotropy;
	texinfo.repeatu = clampuv != 1 && clampuv != 2;
	texinfo.repeatv = clampuv != 1 && clampuv != 3;
	content.load(tex[0], objectdir, texture_names[0], texinfo);
	if (!texture_names[1].empty())
	{
		content.load(tex[1], objectdir, texture_names[1], texinfo);
		data.textures.insert(tex[1]);
	}
	else
	{
		tex[1] = content.getFactory<Texture>().getZero();
	}
	if (!texture_names[2].empty())
	{
		texinfo.compress = false;
		content.load(tex[2], objectdir, texture_names[2], texinfo);
		data.textures.insert(tex[2]);
	}
	else
	{
		tex[2] = content.getFactory<Texture>().getZero();
	}

	// setup drawable
	Drawable & drawable = body.drawable;
	drawable.SetModel(*model);
	drawable.SetTextures(tex[0]->GetId(), tex[1]->GetId(), tex[2]->GetId());
	drawable.SetDecal(alphablend);
	drawable.SetCull(data.cull && !doublesided);

	return bodies.insert(std::make_pair(name, body)).first;
}

void Track::Loader::AddBody(SceneNode & scene, const Body & body)
{
	bool nolighting = body.nolighting;
	bool alphablend = body.drawable.GetDecal();
	keyed_container<Drawable> * dlist = &scene.GetDrawList().normal_noblend;
	if (body.skybox)
	{
		if (alphablend)
		{
			dlist = &scene.GetDrawList().skybox_blend;
		}
		else
		{
			dlist = &scene.GetDrawList().skybox_noblend;
		}
	}
	else
	{
		if (alphablend)
		{
			dlist = &scene.GetDrawList().normal_blend;
		}
		else if (nolighting)
		{
			dlist = &scene.GetDrawList().normal_noblend_nolighting;
		}
	}
	dlist->insert(body.drawable);
}

bool Track::Loader::LoadNode(const PTree & sec)
{
	const PTree * sec_body;
	if (!sec.get("body", sec_body, error_output))
	{
		return false;
	}

	body_iterator ib = LoadBody(*sec_body);
	if (ib == bodies.end())
	{
		//info_output << "Object " << sec.value() << " failed to load body" << std::endl;
		return true;
	}

	Vec3 position, angle;
	bool has_transform = sec.get("position",  position) | sec.get("rotation", angle);
	Quat rotation(angle[0]/180*M_PI, angle[1]/180*M_PI, angle[2]/180*M_PI);

	const Body & body = ib->second;
	if (body.mass < 1E-3)
	{
		// static geometry
		if (has_transform)
		{
			// static geometry instanced
			SceneNode::Handle h = data.static_node.AddNode();
			SceneNode & node = data.static_node.GetNode(h);
			node.GetTransform().SetTranslation(position);
			node.GetTransform().SetRotation(rotation);
			AddBody(node, body);
		}
		else
		{
			// static geometry pretransformed(non instanced)
			AddBody(data.static_node, body);
		}

		if (body.collidable)
		{
			// static geometry collidable
			btTransform transform;
			transform.setOrigin(ToBulletVector(position));
			transform.setRotation(ToBulletQuaternion(rotation));
#ifndef EXTBULLET
			track_shape->addChildShape(transform, body.shape);
#else
			btCollisionObject * object = new btCollisionObject();
			object->setActivationState(DISABLE_SIMULATION);
			object->setWorldTransform(transform);
			object->setCollisionShape(body.shape);
			object->setUserPointer(body.shape->getUserPointer());
			data.objects.push_back(object);
			world.addCollisionObject(object);
#endif
		}
	}
	else
	{
		// fix postion due to rotation around mass center
		Vec3 center_local = ToMathVector<float>(body.center);
		Vec3 center_world = center_local;
		rotation.RotateVector(center_world);
		position = position - center_local + center_world;

		if (dynamic_objects)
		{
			// dynamic geometry
			data.body_transforms.push_back(MotionState());
			data.body_transforms.back().rotation = ToBulletQuaternion(rotation);
			data.body_transforms.back().position = ToBulletVector(position);
			data.body_transforms.back().massCenterOffset = -body.center;

			btRigidBody::btRigidBodyConstructionInfo info(body.mass, &data.body_transforms.back(), body.shape, body.inertia);
			info.m_friction = 0.9;

			btRigidBody * object = new btRigidBody(info);
			object->setContactProcessingThreshold(0.0);
			data.objects.push_back(object);
			world.addRigidBody(object);

			SceneNode::Handle node_handle = data.dynamic_node.AddNode();
			SceneNode & node = data.dynamic_node.GetNode(node_handle);
			node.GetTransform().SetTranslation(position);
			node.GetTransform().SetRotation(rotation);
			data.body_nodes.push_back(node_handle);
			AddBody(node, body);
		}
		else
		{
			// dynamic geometry as static geometry collidable
			btTransform transform;
			transform.setOrigin(ToBulletVector(position));
			transform.setRotation(ToBulletQuaternion(rotation));

			btCollisionObject * object = new btCollisionObject();
			object->setActivationState(DISABLE_SIMULATION);
			object->setWorldTransform(transform);
			object->setCollisionShape(body.shape);
			object->setUserPointer(body.shape->getUserPointer());
			data.objects.push_back(object);
			world.addCollisionObject(object);

			SceneNode::Handle h = data.static_node.AddNode();
			SceneNode & node = data.static_node.GetNode(h);
			node.GetTransform().SetTranslation(position);
			node.GetTransform().SetRotation(rotation);
			AddBody(node, body);
		}
	}

	return true;
}

/// read from the file stream and put it in "output".
/// return true if the get was successful, else false
template <typename T>
static bool get(std::ifstream & f, T & output)
{
	if (!f.good()) return false;

	std::string instr;
	f >> instr;
	if (instr.empty()) return false;

	while (!instr.empty() && instr[0] == '#' && f.good())
	{
		f.ignore(1024, '\n');
		f >> instr;
	}

	if (!f.good() && !instr.empty() && instr[0] == '#') return false;

	std::istringstream s(instr);
	s >> output;
	return true;
}

void Track::Loader::CalculateNumOld()
{
	numobjects = 0;
	std::string objectlist = objectpath + "/list.txt";
	std::ifstream f(objectlist.c_str());
	int params_per_object;
	if (get(f, params_per_object))
	{
		std::string junk;
		while (get(f, junk))
		{
			for (int i = 0; i < params_per_object-1; ++i)
			{
				get(f, junk);
			}
			numobjects++;
		}
	}
}

bool Track::Loader::BeginOld()
{
	CalculateNumOld();

	if (!get(objectfile, params_per_object))
	{
		return false;
	}

	if (params_per_object != expected_params)
	{
		error_output << "Track object list has " << params_per_object << " params per object, expected " << expected_params << std::endl;
		return false;
	}

	return true;
}

bool Track::Loader::AddObject(const Object & object)
{
	data.models.insert(object.model);

	TextureInfo texinfo;
	texinfo.mipmap = object.mipmap || anisotropy; //always mipmap if anisotropy is on
	texinfo.anisotropy = anisotropy;
	texinfo.repeatu = object.clamptexture != 1 && object.clamptexture != 2;
	texinfo.repeatv = object.clamptexture != 1 && object.clamptexture != 3;

	std::shared_ptr<Texture> texture0, texture1, texture2;
	{
		content.load(texture0, objectdir, object.texture, texinfo);
		data.textures.insert(texture0);
	}
	{
		std::string texname = object.texture.substr(0, std::max<int>(0, object.texture.length()-4)) + "-misc1.png";
		std::string filepath = objectpath + "/" + texname;
		if (std::ifstream(filepath.c_str()))
		{
			content.load(texture1, objectdir, texname, texinfo);
			data.textures.insert(texture1);
		}
		else
		{
			texture1 = content.getFactory<Texture>().getZero();
		}
	}
	{
		texinfo.compress = false;
		std::string texname = object.texture.substr(0, std::max<int>(0, object.texture.length()-4)) + "-misc2.png";
		std::string filepath = objectpath + "/" + texname;
		if (std::ifstream(filepath.c_str()))
		{
			content.load(texture2, objectdir, texname, texinfo);
			data.textures.insert(texture2);
		}
		else
		{
			texture2 = content.getFactory<Texture>().getZero();
		}
	}

	//use a different drawlist layer where necessary
	bool transparent = (object.transparent_blend==1);
	keyed_container <Drawable> * dlist = &data.static_node.GetDrawList().normal_noblend;
	if (transparent)
	{
		dlist = &data.static_node.GetDrawList().normal_blend;
	}
	else if (object.nolighting)
	{
		dlist = &data.static_node.GetDrawList().normal_noblend_nolighting;
	}
	if (object.skybox)
	{
		if (transparent)
		{
			dlist = &data.static_node.GetDrawList().skybox_blend;
		}
		else
		{
			dlist = &data.static_node.GetDrawList().skybox_noblend;
		}
	}
	SceneNode::DrawableHandle dref = dlist->insert(Drawable());
	Drawable & drawable = dlist->get(dref);
	drawable.SetModel(*object.model);
	drawable.SetTextures(texture0->GetId(), texture1->GetId(), texture2->GetId());
	drawable.SetDecal(transparent);
	drawable.SetCull(data.cull && (object.transparent_blend != 2));

	if (object.collideable)
	{
		btTriangleIndexVertexArray * mesh = new btTriangleIndexVertexArray();
		mesh->addIndexedMesh(GetIndexedMesh(*object.model));
		data.meshes.push_back(mesh);

		assert(object.surface >= 0 && object.surface < (int)data.surfaces.size());
		btBvhTriangleMeshShape * shape = new btBvhTriangleMeshShape(mesh, true);
		shape->setUserPointer((void*)&data.surfaces[object.surface]);
		data.shapes.push_back(shape);

#ifndef EXTBULLET
		btTransform transform = btTransform::getIdentity();
		track_shape->addChildShape(transform, shape);
#else
		btCollisionObject * co = new btCollisionObject();
		co->setActivationState(DISABLE_SIMULATION);
		co->setCollisionShape(shape);
		co->setUserPointer(shape->getUserPointer());
		data.objects.push_back(co);
		world.addCollisionObject(co);
#endif
	}
	return true;
}

std::pair<bool, bool> Track::Loader::ContinueOld()
{
	std::string model_name;
	if (!get(objectfile, model_name))
	{
		return std::make_pair(false, false);
	}

	Object object;
	bool isashadow;
	std::string junk;

	get(objectfile, object.texture);
	get(objectfile, object.mipmap);
	get(objectfile, object.nolighting);
	get(objectfile, object.skybox);
	get(objectfile, object.transparent_blend);
	get(objectfile, junk);//bump_wavelength);
	get(objectfile, junk);//bump_amplitude);
	get(objectfile, junk);//driveable);
	get(objectfile, object.collideable);
	get(objectfile, junk);//friction_notread);
	get(objectfile, junk);//friction_tread);
	get(objectfile, junk);//rolling_resistance);
	get(objectfile, junk);//rolling_drag);
	get(objectfile, isashadow);
	get(objectfile, object.clamptexture);
	get(objectfile, object.surface);
	for (int i = 0; i < params_per_object - expected_params; i++)
	{
		get(objectfile, junk);
	}

	if (dynamic_shadows && isashadow)
	{
		return std::make_pair(false, true);
	}

	if (packload)
	{
		content.load(object.model, objectdir, model_name, pack);
	}
	else
	{
		content.load(object.model, objectdir, model_name);
	}

	// fixme: ugly hack to make vertical tracking work
	// should be fixed in the model data instead
	if (object.skybox && data.vertical_tracking_skyboxes)
	{
		VertexArray va = object.model->GetVertexArray();
		va.Translate(0, 0, -object.model->GetCenter()[2]);
		object.model->Load(va, error_output);
	}

	if (!AddObject(object))
	{
		return std::make_pair(true, false);
	}

	return std::make_pair(false, true);
}

bool Track::Loader::LoadSurfaces()
{
	std::string path = trackpath + "/surfaces.txt";
	std::ifstream file(path.c_str());
	if (!file.good())
	{
		info_output << "Can't find surfaces configfile: " << path << std::endl;
		return false;
	}

	PTree param;
	read_ini(file, param);
	for (PTree::const_iterator is = param.begin(); is != param.end(); ++is)
	{
		if (is->first.find("surface") != 0)
		{
			continue;
		}

		const PTree & surf_cfg = is->second;
		data.surfaces.push_back(TrackSurface());
		TrackSurface & surface = data.surfaces.back();

		std::string type;
		surf_cfg.get("Type", type);
		surface.setType(type);

		float temp = 0.0;
		surf_cfg.get("BumpWaveLength", temp, error_output);
		if (temp <= 0.0)
		{
			error_output << "Surface Type = " << type << " has BumpWaveLength = 0.0 in " << path << std::endl;
			temp = 1.0;
		}
		surface.bumpWaveLength = temp;

		surf_cfg.get("BumpAmplitude", temp, error_output);
		surface.bumpAmplitude = temp;

		surf_cfg.get("FrictionNonTread", temp, error_output);
		surface.frictionNonTread = temp;

		surf_cfg.get("FrictionTread", temp, error_output);
		surface.frictionTread = temp;

		surf_cfg.get("RollResistanceCoefficient", temp, error_output);
		surface.rollResistanceCoefficient = temp;

		surf_cfg.get("RollingDrag", temp, error_output);
		surface.rollingDrag = temp;
	}
	info_output << "Loaded surfaces file, " << data.surfaces.size() << " surfaces." << std::endl;

	return true;
}

bool Track::Loader::LoadRoads()
{
	data.roads.clear();

	std::string roadpath = trackpath + "/roads.trk";
	std::ifstream trackfile(roadpath.c_str());
	if (!trackfile.good())
	{
		error_output << "Error opening roads file: " << trackpath + "/roads.trk" << std::endl;
		return false;
	}

	int numroads = 0;
	trackfile >> numroads;
	for (int i = 0; i < numroads && trackfile; ++i)
	{
		data.roads.push_back(RoadStrip());
		data.roads.back().ReadFrom(trackfile, data.reverse, error_output);
	}

	return true;
}

bool Track::Loader::CreateRacingLines()
{
	K1999 k1999data;
	for (std::list <RoadStrip>::iterator i = data.roads.begin(); i != data.roads.end(); ++i)
	{
		if (k1999data.LoadData(*i))
		{
			k1999data.CalcRaceLine();
			k1999data.UpdateRoadStrip(*i);
			CreateRacingLine(*i);
		}
	}
	return true;
}

template <bool set_faces>
static void AddRacingLineSegment(
	const RoadPatch & patch,
	std::vector<float> & vertices,
	std::vector<float> & texcoords,
	std::vector<unsigned int> & faces,
	Vec3 & prev_segment,
	float & distance)
{
	if (set_faces)
	{
		const unsigned int n = texcoords.size() / 2;
		const unsigned int fs[6] = {n, n + 1, n + 3, n + 3, n + 2, n};
		faces.insert(faces.end(), fs, fs + 6);
	}

	const Bezier & p = patch.GetPatch();
	distance += (p.GetRacingLine() - prev_segment).Magnitude();
	prev_segment = p.GetRacingLine();

	const float tc[4] = {0, distance, 1, distance};
	texcoords.insert(texcoords.end(), tc, tc + 4);

	const float hwidth = 0.2;
	const Vec3 zoffset(0.0, 0.0, 0.1);
	const Vec3 r = p.GetRacingLine() + zoffset;
	const Vec3 t = (p.GetPoint(0, 0) - p.GetPoint(0, 3)).Normalize();
	const Vec3 v0 = r - t * hwidth;
	const Vec3 v1 = r + t * hwidth;

	const float vc[6] = {v0[0], v0[1], v0[2], v1[0], v1[1], v1[2]};
	vertices.insert(vertices.end(), vc, vc + 6);
}

void Track::Loader::CreateRacingLine(const RoadStrip & strip)
{
	if (strip.GetPatches().empty())
		return;

	// get texture
	std::shared_ptr<Texture> texture;
	content.load(texture, texturedir, "racingline.png", TextureInfo());
	data.textures.insert(texture);

	// calculate batch size per drawable
	const size_t batch_max_size = 256;
	const size_t strip_size = strip.GetPatches().size();
	const size_t batch_count = (strip_size + batch_max_size - 1) / batch_max_size;
	const size_t batch_size = strip_size - (strip_size / batch_count) * (batch_count - 1);

	// allocate batch vertex data
	VertexArray vertex_array;
	std::vector<float> vertices;
	std::vector<float> texcoords;
	std::vector<unsigned int> faces;
	vertices.reserve((batch_size + 1) * 6);
	texcoords.reserve((batch_size + 1) * 4);
	faces.reserve(batch_size * 6);

	// generate racing line
	float line_length = 0;
	Vec3 line_center = strip.GetPatches()[0].GetPatch().GetRacingLine();
	for (size_t n = 0, m = 0; n < batch_count; ++n)
	{
		// fill batch
		for (size_t me = std::min(m + batch_size, strip_size); m < me; ++m)
		{
			AddRacingLineSegment<true>(strip.GetPatches()[m], vertices, texcoords, faces, line_center, line_length);
		}
		// cap batch
		const size_t mc = (m < strip_size) ? m : 0;
		AddRacingLineSegment<false>(strip.GetPatches()[mc], vertices, texcoords, faces, line_center, line_length);

		// set vertex array
		vertex_array.Clear();
		vertex_array.Add(
			&faces[0], faces.size(),
			&vertices[0], vertices.size(),
			&texcoords[0], texcoords.size());

		// create model
		std::shared_ptr<Model> model(new Model());
		model->Load(vertex_array, error_output);
		data.models.insert(model);

		// register drawable
		SceneNode::DrawableHandle dh = data.racingline_node.GetDrawList().normal_blend.insert(Drawable());
		Drawable & d = data.racingline_node.GetDrawList().normal_blend.get(dh);
		d.SetTextures(texture->GetId());
		d.SetModel(*model);
		//d.SetDecal(true);

		vertices.clear();
		texcoords.clear();
		faces.clear();
	}
}

bool Track::Loader::LoadStartPositions(const PTree & info)
{
	int sp_num = 0;
	std::ostringstream sp_name;
	sp_name << "start position " << sp_num;
	std::vector<float> f3(3);
	while (info.get(sp_name.str(), f3))
	{
		std::ostringstream so_name;
		so_name << "start orientation " << sp_num;
		Quat q;
		std::vector <float> angle(3, 0.0);
		if (info.get(so_name.str(), angle, error_output))
		{
			q.SetEulerZYX(angle[0] * M_PI/180, angle[1] * M_PI/180, angle[2] * M_PI/180);
		}

		Quat orient(q[2], q[0], q[1], q[3]);

		//due to historical reasons the initial orientation places the car faces the wrong way
		Quat fixer;
		fixer.Rotate(M_PI_2, 0, 0, 1);
		orient = fixer * orient;

		Vec3 pos(f3[2], f3[0], f3[1]);

		data.start_positions.push_back(
			std::pair <Vec3, Quat >(pos, orient));

		sp_num++;
		sp_name.str("");
		sp_name << "start position " << sp_num;
	}

	if (data.reverse)
	{
		// flip start positions
		for (std::vector <std::pair <Vec3, Quat > >::iterator i = data.start_positions.begin();
			i != data.start_positions.end(); ++i)
		{
			i->second.Rotate(M_PI, 0, 0, 1);
		}

		// reverse start positions
		std::reverse(data.start_positions.begin(), data.start_positions.end());
	}

	return true;
}

bool Track::Loader::LoadLapSections(const PTree & info)
{
	// get timing sectors
	int lapmarkers = 0;
	if (info.get("lap sequences", lapmarkers))
	{
		for (int l = 0; l < lapmarkers; l++)
		{
			std::vector<float> lapraw(3);
			std::ostringstream lapname;
			lapname << "lap sequence " << l;
			info.get(lapname.str(), lapraw);
			int roadid = lapraw[0];
			int patchid = lapraw[1];

			int curroad = 0;
			for (std::list<RoadStrip>::iterator i = data.roads.begin(); i != data.roads.end(); ++i, ++curroad)
			{
				if (curroad == roadid)
				{
					int num_patches = i->GetPatches().size();
					assert(patchid < num_patches);

					// adjust id for reverse case
					if (data.reverse)
						patchid = num_patches - patchid;

					data.lap.push_back(&i->GetPatches()[patchid].GetPatch());
					break;
				}
			}
		}
	}

	if (data.lap.empty())
	{
		info_output << "No lap sequence found. Lap timing will not be possible." << std::endl;
		return true;
	}

	// adjust timing sectors if reverse
	if (data.reverse)
	{
		if (data.lap.size() > 1)
		{
			// reverse the lap sequence, but keep the first bezier where it is (remember, the track is a loop)
			// so, for example, now instead of 1 2 3 4 we should have 1 4 3 2
			std::vector<const Bezier *>::iterator secondbezier = data.lap.begin() + 1;
			assert(secondbezier != data.lap.end());
			std::reverse(secondbezier, data.lap.end());
		}

		// move timing sector 0 back so we'll still drive over it when going in reverse around the track
		// find patch in front of first start position
		const Bezier * lap0 = 0;
		float minlen2 = 10E6;
		Vec3 pos = data.start_positions[0].first;
		Vec3 dir = Direction::Forward;
		data.start_positions[0].second.RotateVector(dir);
		Vec3 bpos(pos[1], pos[2], pos[0]);
		Vec3 bdir(dir[1], dir[2], dir[0]);
		for (std::list<RoadStrip>::iterator r = data.roads.begin(); r != data.roads.end(); ++r)
		{
			for (std::vector<RoadPatch>::iterator p = r->GetPatches().begin(); p != r->GetPatches().end(); ++p)
			{
				Vec3 vec = p->GetPatch().GetBL() - bpos;
				float len2 = vec.MagnitudeSquared();
				bool fwd = vec.dot(bdir) > 0;
				if (fwd && len2 < minlen2)
				{
					minlen2 = len2;
					lap0 = &p->GetPatch();
				}
			}
		}
		if (lap0)
			data.lap[0] = lap0;
	}

	// calculate distance from starting line for each patch to account for those tracks
	// where starting line is not on the 1st patch of the road
	// note this only updates the road with lap sequence 0 on it
	Bezier* start_patch = const_cast <Bezier *> (data.lap[0]);
	start_patch->dist_from_start = 0.0;
	Bezier* curr_patch = start_patch->next_patch;
	float total_dist = start_patch->length;
	int count = 0;
	while ( curr_patch && curr_patch != start_patch)
	{
		count++;
		curr_patch->dist_from_start = total_dist;
		total_dist += curr_patch->length;
		curr_patch = curr_patch->next_patch;
	}

	info_output << "Track timing sectors: " << lapmarkers << std::endl;
	return true;
}
