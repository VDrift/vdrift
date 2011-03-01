#include "trackloader.h"
#include "collision_world.h"
#include "texturemanager.h"
#include "textureinfo.h"
#include "modelmanager.h"
#include "tobullet.h"
#include "config.h"
#include "k1999.h"

static void operator >> (std::istream & lhs, std::vector<std::string> & rhs)
{
	for (size_t i = 0; i < rhs.size() && !lhs.eof(); ++i)
	{
		std::string str;
		std::getline(lhs, str, ',');
		std::stringstream s(str);
		s >> str;
		rhs[i] = str;
	}
}

static btIndexedMesh GetIndexedMesh(const MODEL & model)
{
	const float * vertices;
	int vcount;
	const int * faces;
	int fcount;
	model.GetVertexArray().GetVertices(vertices, vcount);
	model.GetVertexArray().GetFaces(faces, fcount);

	assert(fcount % 3 == 0); //Face count is not a multiple of 3

	btIndexedMesh mesh;
	mesh.m_numTriangles = fcount / 3;
	mesh.m_triangleIndexBase = (const unsigned char *)faces;
	mesh.m_triangleIndexStride = sizeof(int) * 3;
	mesh.m_numVertices = vcount;
	mesh.m_vertexBase = (const unsigned char *)vertices;
	mesh.m_vertexStride = sizeof(float) * 3;
	mesh.m_vertexType = PHY_FLOAT;
	return mesh;
}

TRACK::LOADER::LOADER(
	TEXTUREMANAGER & textures,
	MODELMANAGER & models,
	COLLISION_WORLD & world,
	DATA & data,
	std::ostream & info_output,
	std::ostream & error_output,
	const std::string & trackpath,
	const std::string & trackdir,
	const std::string & texturedir,
	const std::string & texsize,
	const int anisotropy,
	const bool reverse,
	const bool dynamic_shadows,
	const bool agressive_combining) :
	texture_manager(textures),
	model_manager(models),
	world(world),
	data(data),
	info_output(info_output),
	error_output(error_output),
	trackpath(trackpath),
	trackdir(trackdir),
	texturedir(texturedir),
	texsize(texsize),
	anisotropy(anisotropy),
	reverse(reverse),
	dynamic_shadows(dynamic_shadows),
	agressive_combining(agressive_combining),
	packload(false),
	numobjects(0),
	params_per_object(17),
	expected_params(17),
	min_params(14),
	error(false),
	list(false),
	track_shape(0)
{
	objectpath = trackpath + "/objects";
	objectdir = trackdir + "/objects";
}

TRACK::LOADER::~LOADER()
{
	Clear();
}

void TRACK::LOADER::Clear()
{
	bodies.clear();
	track_config.clear();
}

bool TRACK::LOADER::BeginLoad()
{
	Clear();

	info_output << "Loading track from path: " << trackpath << std::endl;

	//load parameters
	if (!LoadParameters())
	{
		return false;
	}

	if (!LoadSurfaces())
	{
		info_output << "No Surfaces File. Continuing with standard surfaces" << std::endl;
	}
	
	//load roads
	if (!LoadRoads())
	{
		error_output << "Error during road loading; continuing with an unsmoothed track" << std::endl;
		data.roads.clear();
	}

	//load the lap sequence
	if (!LoadLapSequence())
	{
		return false;
	}

	if (!CreateRacingLines())
	{
		return false;
	}

	//load objects
	if (!BeginObjectLoad())
	{
		return false;
	}

	return true;
}

bool TRACK::LOADER::ContinueLoad()
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
		data.loaded = true;
	}
	
	return true;
}

bool TRACK::LOADER::BeginObjectLoad()
{
	list = true;
	packload = pack.LoadPack(objectpath + "/objects.jpk");
	
	std::string objectlist = objectpath + "/objects.txt";
	objectfile.open(objectlist.c_str());
	if (objectfile.good())
	{
		list = false;
		return Begin();
	}
	
	objectlist = objectpath + "/list.txt";
	objectfile.open(objectlist.c_str());
	if (!objectfile.good())
	{
		return false;
	}
	return BeginOld();
}

std::pair<bool, bool> TRACK::LOADER::ContinueObjectLoad()
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

bool TRACK::LOADER::Begin()
{
	std::string path = objectpath + "/objects.txt";
	std::ifstream file(path.c_str());
	if (file)
	{
		read_ini(file, track_config);
		//write_ini(track_config, std::cerr);
		nodes = 0;
		if (track_config.get("node", nodes))
		{
			node_it = nodes->begin();
			numobjects = nodes->size();
			data.models.reserve(numobjects);
			data.meshes.reserve(numobjects);
#ifndef EXTBULLET			
			assert(track_shape == 0);
			track_shape = new btCompoundShape(true);
#endif
			return true;
		}
	}
	return false;
}

std::pair<bool, bool> TRACK::LOADER::Continue()
{
	if (node_it == nodes->end())
	{
#ifndef EXTBULLET
		btCollisionObject * track_object = new btCollisionObject();
		track_shape->createAabbTreeFromChildren();
		track_object->setCollisionShape(track_shape);
		world.AddCollisionObject(track_object);
		data.objects.push_back(track_object);
		data.shapes.push_back(track_shape);
		track_shape = 0;
#endif
		Clear();
		return std::make_pair(false, false);
	}
	
	if (!LoadNode(node_it->second))
	{
		return std::make_pair(true, false);
	}
	
	node_it++;
	
	return std::make_pair(false, true);
}

TRACK::LOADER::body_iterator TRACK::LOADER::LoadBody(const std::string & name)
{
	body_iterator ib = bodies.find(name);
	if(ib != bodies.end())
	{
		return ib;
	}
	
	const PTree * sec = 0;
	if (!track_config.get("body." + name, sec))
	{
		std::string path = objectpath + "/" + name;
		std::ifstream file(path.c_str());
		if (!file)
		{
			std::cerr << "body." << name << " not found." << std::endl; 
			std::cerr << "External config: " << path << " not found." << std::endl; 
			return ib;
		}
		
		PTree & bodysec = track_config.set("body." + name);
		read_ini(file, bodysec);
		sec = &bodysec;
	}
	
	BODY body;
	std::string texture_name;
	std::string model_name;
	int clampuv = 0;
	bool mipmap = true;
	bool skybox = false;
	bool alphablend = false;
	bool doublesided = false;
	bool isashadow = false;
	
	sec->get("texture", texture_name, error_output);
	sec->get("model", model_name, error_output);
	sec->get("clampuv", clampuv);
	sec->get("mipmap", mipmap);
	sec->get("skybox", skybox);
	sec->get("alphablend", alphablend);
	sec->get("doublesided", doublesided);
	sec->get("isashadow", isashadow);
	sec->get("nolighting", body.nolighting);
	
	if (dynamic_shadows && isashadow)
	{
		return ib;
	}
	
	// load model
	std::tr1::shared_ptr<MODEL> model;
	if (packload)
	{
		if (!model_manager.Load(model_name, pack, model) &&
			!model_manager.Load(objectdir, model_name, model))
		{
			return ib;
		}
	}
	else
	{
		if (!model_manager.Load(objectdir, model_name, model))
		{
			return ib;
		}
	}
	data.models.push_back(model);
	
	// load shape
	int surface = -1;
	sec->get("surface", surface);
	body.collidable = sec->get("mass", body.mass);
	if (body.collidable)
	{
		if (body.mass < 1E-3)
		{
			btTriangleIndexVertexArray * mesh = new btTriangleIndexVertexArray();
			mesh->addIndexedMesh(GetIndexedMesh(*model));
			data.meshes.push_back(mesh);
			body.mesh = mesh;
			
			//assert(surface >= 0 && surface < (int)data.surfaces.size());
			if (surface < 0 || surface >= (int)data.surfaces.size())
			{
				std::cerr << surface << std::endl;
				surface = 0;
			}
			btBvhTriangleMeshShape * shape = new btBvhTriangleMeshShape(mesh, true);
			shape->setUserPointer((void*)&data.surfaces[surface]);
			data.shapes.push_back(shape);
			body.shape = shape;
		}
		else
		{
			btVector3 size = ToBulletVector(model->GetAABB().GetSize());
			btBoxShape * shape = new btBoxShape(size * 0.5);
			shape->calculateLocalInertia(body.mass, body.inertia);
			data.shapes.push_back(shape);
			body.shape = shape;
		}
	}
	
	// load textures
	std::vector<std::string> texture_names(3);
	std::stringstream s(texture_name);
	s >> texture_names;
	
	TEXTUREINFO texinfo;
	texinfo.mipmap = mipmap || anisotropy; //always mipmap if anisotropy is on
	texinfo.anisotropy = anisotropy;
	texinfo.repeatu = clampuv != 1 && clampuv != 2;
	texinfo.repeatv = clampuv != 1 && clampuv != 3;
	texinfo.size = texsize;
	
	std::tr1::shared_ptr<TEXTURE> diffuse;
	if (!texture_manager.Load(objectdir, texture_names[0], texinfo, diffuse))
	{
		return ib;
	}
	
	std::tr1::shared_ptr<TEXTURE> miscmap1;
	if (texture_names[1].length() > 0)
	{
		texture_manager.Load(objectdir, texture_names[1], texinfo, miscmap1);
	}
	
	std::tr1::shared_ptr<TEXTURE> miscmap2;
	if (texture_names[2].length() > 0)
	{
		texture_manager.Load(objectdir, texture_names[2], texinfo, miscmap2);
	}
	
	// setup drawable
	DRAWABLE & drawable = body.drawable;
	drawable.SetModel(*model);
	drawable.SetDiffuseMap(diffuse);
	drawable.SetMiscMap1(miscmap1);
	drawable.SetMiscMap2(miscmap2);
	drawable.SetDecal(alphablend);
	drawable.SetCull(data.cull && !doublesided, false);
	drawable.SetRadius(model->GetRadius());
	drawable.SetObjectCenter(model->GetCenter());
	drawable.SetSkybox(skybox);
	drawable.SetVerticalTrack(skybox && data.vertical_tracking_skyboxes);
	
	return bodies.insert(std::make_pair(name, body)).first;
}

void TRACK::LOADER::AddBody(SCENENODE & scene, const BODY & body)
{
	bool nolighting = body.nolighting;
	bool alphablend = body.drawable.GetDecal();
	bool skybox = body.drawable.GetSkybox();
	keyed_container<DRAWABLE> * dlist = &scene.GetDrawlist().normal_noblend;
	if (alphablend)
	{
		dlist = &scene.GetDrawlist().normal_blend;
	}
	else if (nolighting)
	{
		dlist = &scene.GetDrawlist().normal_noblend_nolighting;
	}
	if (skybox)
	{
		if (alphablend)
		{
			dlist = &scene.GetDrawlist().skybox_blend;
		}
		else
		{
			dlist = &scene.GetDrawlist().skybox_noblend;
		}
	}
	dlist->insert(body.drawable);
}

bool TRACK::LOADER::LoadNode(const PTree & sec)
{
	std::string bodyname;
	if (!sec.get("body", bodyname, error_output))
	{
		return false;
	}
	
	body_iterator ib = LoadBody(bodyname);
	if (ib == bodies.end())
	{
		return true;
	}
	
	MATHVECTOR<float, 3> position, angle;
	bool has_transform = sec.get("position",  position) | sec.get("rotation", angle);
	QUATERNION<float> rotation(angle[0]/180*M_PI, angle[1]/180*M_PI, angle[2]/180*M_PI);
	
	btTransform transform;
	transform.setOrigin(ToBulletVector(position));
	transform.setRotation(ToBulletQuaternion(rotation));
	
	const BODY & body = ib->second;
	if (body.mass < 1E-3)
	{
		if (has_transform)
		{
			keyed_container <SCENENODE>::handle sh = data.static_node.AddNode();
			SCENENODE & node = data.static_node.GetNode(sh);
			node.GetTransform().SetTranslation(position);
			node.GetTransform().SetRotation(rotation);
			AddBody(node, body);
		}
		else
		{
			AddBody(data.static_node, body);
		}
		
		if (body.collidable)
		{
#ifndef EXTBULLET
			track_shape->addChildShape(transform, body.shape);
#else
			btCollisionObject * object = new btCollisionObject();
			object->setWorldTransform(transform);
			object->setCollisionShape(body.shape);
			object->setUserPointer(body.shape->getUserPointer());
			data.objects.push_back(object);
			world.AddCollisionObject(object);
#endif
		}
	}
	else
	{
		data.body_transforms.push_back(btDefaultMotionState(transform));
		
		btRigidBody::btRigidBodyConstructionInfo info(body.mass, &data.body_transforms.back(), body.shape, body.inertia);
		info.m_friction = 0.9;
		
		btRigidBody * object = new btRigidBody(info);
		object->setContactProcessingThreshold(0.0);
		data.objects.push_back(object);
		world.AddRigidBody(object);
		
		keyed_container <SCENENODE>::handle nh = data.dynamic_node.AddNode();
		SCENENODE & node = data.dynamic_node.GetNode(nh);
		node.GetTransform().SetTranslation(position);
		node.GetTransform().SetRotation(rotation);
		data.body_nodes.push_back(nh);
		AddBody(node, body);
	}
	
	return true;
}

/// read from the file stream and put it in "output".
/// return true if the get was successful, else false
template <typename T>
static bool GetParam(std::ifstream & f, T & output)
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

	std::stringstream sstr(instr);
	sstr >> output;
	return true;
}

void TRACK::LOADER::CalculateNumOld()
{
	numobjects = 0;
	std::string objectlist = objectpath + "/list.txt";
	std::ifstream f(objectlist.c_str());
	int params_per_object;
	if (GetParam(f, params_per_object))
	{
		std::string junk;
		while (GetParam(f, junk))
		{
			for (int i = 0; i < params_per_object-1; ++i)
			{
				GetParam(f, junk);
			}
			numobjects++;
		}
	}
}

bool TRACK::LOADER::BeginOld()
{
	CalculateNumOld();
	
	data.models.reserve(numobjects);
	
	if (!GetParam(objectfile, params_per_object)) return false;
	
	if (params_per_object != expected_params)
	{
		info_output << "Track object list has " << params_per_object << " params per object, expected " << expected_params << ", this is fine, continuing" << std::endl;
	}
	
	if (params_per_object < min_params)
	{
		error_output << "Track object list has " << params_per_object << " params per object, expected " << expected_params << std::endl;
		return false;
	}
	
	return true;
}

std::pair<bool, bool> TRACK::LOADER::ContinueOld()
{
	std::string model_name;
	if (!(GetParam(objectfile, model_name)))
	{
		Clear();
		return std::make_pair(false, false);
	}
	
	assert(objectfile.good());

	std::string diffuse_texture_name;
	bool mipmap;
	bool nolighting;
	bool skybox;
	int transparent_blend;
	float bump_wavelength;
	float bump_amplitude;
	bool driveable;
	bool collideable;
	float friction_notread;
	float friction_tread;
	float rolling_resistance;
	float rolling_drag;
	bool isashadow(false);
	int clamptexture(0);
	int surface(0);
	std::string otherjunk;

	GetParam(objectfile, diffuse_texture_name);
	GetParam(objectfile, mipmap);
	GetParam(objectfile, nolighting);
	GetParam(objectfile, skybox);
	GetParam(objectfile, transparent_blend);
	GetParam(objectfile, bump_wavelength);
	GetParam(objectfile, bump_amplitude);
	GetParam(objectfile, driveable);
	GetParam(objectfile, collideable);
	GetParam(objectfile, friction_notread);
	GetParam(objectfile, friction_tread);
	GetParam(objectfile, rolling_resistance);
	GetParam(objectfile, rolling_drag);

	if (params_per_object >= 15)
		GetParam(objectfile, isashadow);

	if (params_per_object >= 16)
		GetParam(objectfile, clamptexture);

	if (params_per_object >= 17)
		GetParam(objectfile, surface);

	for (int i = 0; i < params_per_object - expected_params; i++)
		GetParam(objectfile, otherjunk);

	if (dynamic_shadows && isashadow)
	{
		return std::make_pair(false, true);
	}

	std::tr1::shared_ptr<MODEL> model;
	if (packload)
	{
		if (!model_manager.Load(model_name, pack, model))
		{
			return std::make_pair(true, false);
		}
	}
	else
	{
		if (!model_manager.Load(objectdir, model_name, model))
		{
			return std::make_pair(true, false);
		}
	}
	data.models.push_back(model);

	TEXTUREINFO texinfo;
	texinfo.mipmap = mipmap || anisotropy; //always mipmap if anisotropy is on
	texinfo.anisotropy = anisotropy;
	texinfo.repeatu = clamptexture != 1 && clamptexture != 2;
	texinfo.repeatv = clamptexture != 1 && clamptexture != 3;
	texinfo.size = texsize;

	std::tr1::shared_ptr<TEXTURE> diffuse_texture;
	if (!texture_manager.Load(objectdir, diffuse_texture_name, texinfo, diffuse_texture))
	{
		error_output << "Skipping object " << model_name << " and continuing" << std::endl;
		return std::make_pair(false, true);
	}

	std::tr1::shared_ptr<TEXTURE> miscmap1_texture;		
	{
		std::string texture_name = diffuse_texture_name.substr(0, std::max(0, (int)diffuse_texture_name.length()-4)) + "-misc1.png";
		std::string filepath = objectpath + "/" + texture_name;
		if (std::ifstream(filepath.c_str()))
		{
			if (!texture_manager.Load(objectdir, texture_name, texinfo, miscmap1_texture))
			{
				error_output << "Error loading texture: " << filepath << " for object " << model_name << ", continuing" << std::endl;
			}
		}
	}

	std::tr1::shared_ptr<TEXTURE> miscmap2_texture;
	{
		std::string texture_name = diffuse_texture_name.substr(0, std::max(0, (int)diffuse_texture_name.length()-4)) + "-misc2.png";
		std::string filepath = objectpath + "/" + texture_name;
		if (std::ifstream(filepath.c_str()))
		{
			if (!texture_manager.Load(objectdir, texture_name, texinfo, miscmap2_texture))
			{
				error_output << "Error loading texture: " << filepath << " for object " << model_name << ", continuing" << std::endl;
			}
		}
	}

	//use a different drawlist layer where necessary
	bool transparent = (transparent_blend==1);
	keyed_container <DRAWABLE> * dlist = &data.static_node.GetDrawlist().normal_noblend;
	if (transparent)
	{
		dlist = &data.static_node.GetDrawlist().normal_blend;
	}
	else if (nolighting)
	{
		dlist = &data.static_node.GetDrawlist().normal_noblend_nolighting;
	}
	if (skybox)
	{
		if (transparent)
		{
			dlist = &data.static_node.GetDrawlist().skybox_blend;
		}
		else
		{
			dlist = &data.static_node.GetDrawlist().skybox_noblend;
		}
	}
	keyed_container <DRAWABLE>::handle dref = dlist->insert(DRAWABLE());
	DRAWABLE & drawable = dlist->get(dref);
	drawable.SetModel(*model);
	drawable.SetDiffuseMap(diffuse_texture);
	drawable.SetMiscMap1(miscmap1_texture);
	drawable.SetMiscMap2(miscmap2_texture);
	drawable.SetDecal(transparent);
	drawable.SetCull(data.cull && (transparent_blend!=2), false);
	drawable.SetRadius(model->GetRadius());
	drawable.SetObjectCenter(model->GetCenter());
	drawable.SetSkybox(skybox);
	drawable.SetVerticalTrack(skybox && data.vertical_tracking_skyboxes);

	if (collideable || driveable)
	{
		btTriangleIndexVertexArray * mesh = new btTriangleIndexVertexArray();
		mesh->addIndexedMesh(GetIndexedMesh(*model));
		data.meshes.push_back(mesh);
		
		assert(surface >= 0 && surface < (int)data.surfaces.size());
		btBvhTriangleMeshShape * shape = new btBvhTriangleMeshShape(mesh, true);
		shape->setUserPointer((void*)&data.surfaces[surface]);
		data.shapes.push_back(shape);
		
		btCollisionObject * object = new btCollisionObject();
		object->setCollisionShape(shape);
		data.objects.push_back(object);
		
		world.AddCollisionObject(object);
	}

	return std::make_pair(false, true);
}

bool TRACK::LOADER::LoadParameters()
{
	std::string parampath = trackpath + "/track.txt";
	CONFIG param;
	if (!param.Load(parampath))
	{
		error_output << "Can't find track configfile: " << parampath << std::endl;
		return false;
	}
	
	CONFIG::const_iterator section;
	param.GetSection("", section);
	
	param.GetParam(section, "vertical tracking skyboxes", data.vertical_tracking_skyboxes);
	
	int sp_num = 0;
	std::stringstream sp_name;
	sp_name << "start position " << sp_num;
	std::vector<float> f3(3);
	while(param.GetParam(section, sp_name.str(), f3))
	{
		std::stringstream so_name;
		so_name << "start orientation " << sp_num;
		QUATERNION <float> q;
		std::vector <float> angle(3, 0.0);
		if(param.GetParam(section, so_name.str(), angle, error_output))
		{
			q.SetEulerZYX(angle[0] * M_PI/180, angle[1] * M_PI/180, angle[2] * M_PI/180);
		}

		QUATERNION <float> orient(q[2], q[0], q[1], q[3]);

		//due to historical reasons the initial orientation places the car faces the wrong way
		QUATERNION <float> fixer; 
		fixer.Rotate(3.141593, 0, 0, 1);
		orient = fixer * orient;

		MATHVECTOR <float, 3> pos(f3[2], f3[0], f3[1]);

		data.start_positions.push_back(
			std::pair <MATHVECTOR <float, 3>, QUATERNION <float> >(pos, orient));

		sp_num++;
		sp_name.str("");
		sp_name << "start position " << sp_num;
	}

	return true;
}

bool TRACK::LOADER::LoadSurfaces()
{
	std::string path = trackpath + "/surfaces.txt";
	
	CONFIG param;
	if (!param.Load(path))
	{
		info_output << "Can't find surfaces configfile: " << path << std::endl;
		return false;
	}
	
	for (CONFIG::const_iterator section = param.begin(); section != param.end(); ++section)
	{
		if (section->first.find("surface") != 0) continue;
		
		data.surfaces.push_back(TRACKSURFACE());
		TRACKSURFACE & surface = data.surfaces.back();

		std::string type;
		param.GetParam(section, "Type", type);
		surface.setType(type);
		
		float temp = 0.0;
		param.GetParam(section, "BumpWaveLength", temp, error_output);
		if (temp <= 0.0)
		{
			error_output << "Surface Type = " << type << " has BumpWaveLength = 0.0 in " << path << std::endl;
			temp = 1.0;
		}
		surface.bumpWaveLength = temp;
		
		param.GetParam(section, "BumpAmplitude", temp, error_output);
		surface.bumpAmplitude = temp;
		
		param.GetParam(section, "FrictionNonTread", temp, error_output);
		surface.frictionNonTread = temp;
		
		param.GetParam(section, "FrictionTread", temp, error_output);
		surface.frictionTread = temp;
		
		param.GetParam(section, "RollResistanceCoefficient", temp, error_output);
		surface.rollResistanceCoefficient = temp;
		
		param.GetParam(section, "RollingDrag", temp, error_output);
		surface.rollingDrag = temp;
	}
	info_output << "Loaded surfaces file, " << data.surfaces.size() << " surfaces." << std::endl;
	
	return true;
}

bool TRACK::LOADER::LoadRoads()
{
	data.roads.clear();
	
	std::string roadpath = trackpath + "/roads.trk";
	std::ifstream trackfile;
	trackfile.open(roadpath.c_str());
	if (!trackfile)
	{
		error_output << "Error opening roads file: " << trackpath + "/roads.trk" << std::endl;
		return false;
	}

	int numroads;

	trackfile >> numroads;

	for (int i = 0; i < numroads && trackfile; i++)
	{
		data.roads.push_back(ROADSTRIP());
		data.roads.back().ReadFrom(trackfile, error_output);
	}

	if (reverse)
	{
		ReverseRoads();
		data.direction = DATA::DIRECTION_REVERSE;
	}
	else
		data.direction = DATA::DIRECTION_FORWARD;

	return true;
}

void TRACK::LOADER::ReverseRoads()
{
	//move timing sector 0 back 1 patch so we'll still drive over it when going in reverse around the track
	if (!data.lap.empty())
	{
		int counts = 0;

		for (std::list <ROADSTRIP>::iterator i = data.roads.begin(); i != data.roads.end(); ++i)
		{
			optional <const BEZIER *> newstartline = i->FindBezierAtOffset(data.lap[0], -1);
			if (newstartline)
			{
				data.lap[0] = newstartline.get();
				counts++;
			}
		}

		assert(counts == 1); //do a sanity check, because I don't trust the FindBezierAtOffset function
	}

	//reverse the timing sectors
	if (data.lap.size() > 1)
	{
		//reverse the lap sequence, but keep the first bezier where it is (remember, the track is a loop)
		//so, for example, now instead of 1 2 3 4 we should have 1 4 3 2
		std::vector <const BEZIER *>::iterator secondbezier = data.lap.begin();
		++secondbezier;
		assert(secondbezier != data.lap.end());
		std::reverse(secondbezier, data.lap.end());
	}


	//flip start positions
	for (std::vector <std::pair <MATHVECTOR <float, 3>, QUATERNION <float> > >::iterator i = data.start_positions.begin();
		i != data.start_positions.end(); ++i)
	{
		i->second.Rotate(3.141593, 0,0,1);
	}

	//reverse start positions
	std::reverse(data.start_positions.begin(), data.start_positions.end());

	//reverse roads
	std::for_each(data.roads.begin(), data.roads.end(), std::mem_fun_ref(&ROADSTRIP::Reverse));
}

bool TRACK::LOADER::LoadLapSequence()
{
	std::string parampath = trackpath + "/track.txt";
	CONFIG trackconfig;
	if (!trackconfig.Load(parampath))
	{
		error_output << "Can't find track configfile: " << parampath << std::endl;
		return false;
	}
	
	CONFIG::const_iterator section;
	trackconfig.GetSection("", section);
	trackconfig.GetParam(section, "cull faces", data.cull);

	int lapmarkers = 0;
	if (trackconfig.GetParam(section, "lap sequences", lapmarkers))
	{
		for (int l = 0; l < lapmarkers; l++)
		{
			std::vector<float> lapraw(3);
			std::stringstream lapname;
			lapname << "lap sequence " << l;
			trackconfig.GetParam(section, lapname.str(), lapraw);
			int roadid = lapraw[0];
			int patchid = lapraw[1];

			//info_output << "Looking for lap sequence: " << roadid << ", " << patchid << endl;

			int curroad = 0;
			for (std::list <ROADSTRIP>::iterator i = data.roads.begin(); i != data.roads.end(); ++i)
			{
				if (curroad == roadid)
				{
					int curpatch = 0;
					for (std::vector<ROADPATCH>::const_iterator p = i->GetPatches().begin(); p != i->GetPatches().end(); ++p)
					{
						if (curpatch == patchid)
						{
							data.lap.push_back(&p->GetPatch());
							//info_output << "Lap sequence found: " << roadid << ", " << patchid << "= " << &p->GetPatch() << endl;
						}
						curpatch++;
					}
				}
				curroad++;
			}
		}
	}

	// calculate distance from starting line for each patch to account for those tracks
	// where starting line is not on the 1st patch of the road
	// note this only updates the road with lap sequence 0 on it
	if (!data.lap.empty())
	{
		BEZIER* start_patch = const_cast <BEZIER *> (data.lap[0]);
		start_patch->dist_from_start = 0.0;
		BEZIER* curr_patch = start_patch->next_patch;
		float total_dist = start_patch->length;
		int count = 0;
		while ( curr_patch && curr_patch != start_patch)
		{
			count++;
			curr_patch->dist_from_start = total_dist;
			total_dist += curr_patch->length;
			curr_patch = curr_patch->next_patch;
		}
	}

	if (lapmarkers == 0)
		info_output << "No lap sequence found; lap timing will not be possible" << std::endl;
	else
		info_output << "Track timing sectors: " << lapmarkers << std::endl;

	return true;
}

bool TRACK::LOADER::CreateRacingLines()
{
	TEXTUREINFO texinfo; 
	texinfo.size = texsize;
	if (!texture_manager.Load(texturedir, "racingline.png", texinfo, data.racingline_texture))
	{
		return false;
	}
	
	K1999 k1999data;
	int n = 0;
	for (std::list <ROADSTRIP>::iterator i = data.roads.begin(); i != data.roads.end(); ++i,++n)
	{
		if (k1999data.LoadData(&(*i)))
		{
			k1999data.CalcRaceLine();
			k1999data.UpdateRoadStrip(&(*i));
		}
		//else error_output << "Couldn't create racing line for roadstrip " << n << std::endl;
		
		i->CreateRacingLine(data.racingline_node, data.racingline_texture, error_output);
	}
	
	return true;
}
