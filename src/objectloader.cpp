#include "objectloader.h"
#include "texturemanager.h"
#include "modelmanager.h"
#include "tracksurface.h"
#include "trackobject.h"

#include <string>
#include <fstream>

///read from the file stream and put it in "output".
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

bool OBJECTLOADER::BeginObjectLoad()
{
	CalculateNumObjects();
	
	models.reserve(numobjects);
	objects.reserve(numobjects);

	std::string objectlist = objectpath + "/list.txt";
	objectfile.open(objectlist.c_str());

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

	packload = pack.LoadPack(objectpath + "/objects.jpk");

	return true;
}

void OBJECTLOADER::CalculateNumObjects()
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

std::pair <bool,bool> OBJECTLOADER::ContinueObjectLoad()
{
	if (error)
	{
		return std::make_pair(true, false);
	}

	std::string model_name;
	if (!(GetParam(objectfile, model_name)))
	{
		sceneroot = unoptimized_scene;
		unoptimized_scene.Clear();
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
	int surface_id(0);
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
		GetParam(objectfile, surface_id);

	for (int i = 0; i < params_per_object - expected_params; i++)
		GetParam(objectfile, otherjunk);

	if (dynamicshadowsenabled && isashadow)
	{
		return std::make_pair(false, true);
	}

	std::tr1::shared_ptr<MODEL_JOE03> model;
	if (packload)
	{
		if (!model_manager.Load(model_name, model, &pack))
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
	models.push_back(model);

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
	keyed_container <DRAWABLE> * dlist = &unoptimized_scene.GetDrawlist().normal_noblend;
	if (transparent)
	{
		dlist = &unoptimized_scene.GetDrawlist().normal_blend;
	}
	else if (nolighting)
	{
		dlist = &unoptimized_scene.GetDrawlist().normal_noblend_nolighting;
	}
	if (skybox)
	{
		if (transparent)
		{
			dlist = &unoptimized_scene.GetDrawlist().skybox_blend;
		}
		else
		{
			dlist = &unoptimized_scene.GetDrawlist().skybox_noblend;
		}
	}
	keyed_container <DRAWABLE>::handle dref = dlist->insert(DRAWABLE());
	DRAWABLE & drawable = dlist->get(dref);
	if (model->HaveListID())
	{
		drawable.AddDrawList(model->GetListID());
	}
	else
	{
		assert(model->HaveVertexArrayObject());
		GLuint vao;
		unsigned int elementCount;
		model->GetVertexArrayObject(vao, elementCount);
		drawable.setVertexArrayObject(vao, elementCount);
	}
	drawable.SetDiffuseMap(diffuse_texture);
	drawable.SetMiscMap1(miscmap1_texture);
	drawable.SetMiscMap2(miscmap2_texture);
	drawable.SetDecal(transparent);
	drawable.SetCull(cull && (transparent_blend!=2), false);
	drawable.SetRadius(model->GetRadius());
	drawable.SetObjectCenter(model->GetCenter());
	drawable.SetSkybox(skybox);
	if (skybox && vertical_tracking_skyboxes) drawable.SetVerticalTrack(true);

	const TRACKSURFACE * surfacePtr = 0;
	if (collideable || driveable)
	{
		assert(surface_id >= 0 && surface_id < (int)surfaces.size());
		surfacePtr = &surfaces[surface_id];
		objects.push_back(TRACKOBJECT(model.get(), surfacePtr));
	}

	return std::make_pair(false, true);
}
