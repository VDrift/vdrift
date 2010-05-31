#include "objectloader.h"

#include <string>
#include <fstream>

#include "texturemanager.h"

OBJECTLOADER::OBJECTLOADER(
	const std::string & ntrackpath,
	SCENENODE & nsceneroot,
	int nanisotropy,
	bool newdynamicshadowsenabled,
	std::ostream & ninfo_output,
	std::ostream & nerror_output,
	bool newcull,
	bool doagressivecombining)
: trackpath(ntrackpath),
	sceneroot(nsceneroot),
	info_output(ninfo_output),
	error_output(nerror_output),
	error(false),
	numobjects(0),
	packload(false),
	anisotropy(nanisotropy),
	cull(newcull),
	params_per_object(17),
	expected_params(17),
	min_params(14),
	dynamicshadowsenabled(newdynamicshadowsenabled),
	agressivecombine(doagressivecombining)
{
}

bool OBJECTLOADER::BeginObjectLoad()
{
	CalculateNumObjects();

	std::string objectpath = trackpath + "/objects";
	std::string objectlist = objectpath + "/list.txt";
	objectfile.open(objectlist.c_str());

	if (!GetParam(objectfile, params_per_object))
		return false;

	if (params_per_object != expected_params)
		info_output << "Track object list has " << params_per_object << " params per object, expected " << expected_params << ", this is fine, continuing" << std::endl;
	
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
	objectpath = trackpath + "/objects";
	std::string objectlist = objectpath + "/list.txt";
	std::ifstream f(objectlist.c_str());

	int params_per_object;
	if (!GetParam(f, params_per_object))
	{
		numobjects = 0;
		return;
	}

	numobjects = 0;

	std::string junk;
	while (GetParam(f, junk))
	{
		for (int i = 0; i < params_per_object-1; i++)
			GetParam(f, junk);

		numobjects++;
	}

	//info_output << "!!!!!!! " << numobjects << endl;
}

std::pair <bool,bool> OBJECTLOADER::ContinueObjectLoad(
	std::map <std::string, MODEL_JOE03> & model_library,
	std::list <TRACK_OBJECT> & objects,
	std::list <TRACKSURFACE> & surfaces,
	bool usesurfaces,
 	bool vertical_tracking_skyboxes,
 	const std::string & texture_size,
 	TEXTUREMANAGER & textures)
{
	std::string model_name;

	if (error)
		return std::make_pair(true, false);

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
	int surface_type(TRACKSURFACE::ASPHALT);
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
		GetParam(objectfile, surface_type);
		
		
	for (int i = 0; i < params_per_object - expected_params; i++)
		GetParam(objectfile, otherjunk);

	MODEL * model(NULL);

	if (model_library.find(model_name) == model_library.end())
	{
		if (packload)
		{
			if (!model_library[model_name].Load(model_name, &pack, error_output))
			{
				error_output << "Error loading model: " << objectpath + "/" + model_name << " from pack " << objectpath + "/objects.jpk" << std::endl;
				return std::make_pair(true, false); //fail the entire track loading
			}
		}
		else
		{
			if (!model_library[model_name].Load(objectpath + "/" + model_name, NULL, error_output))
			{
				error_output << "Error loading model: " << objectpath + "/" + model_name << std::endl;
				return std::make_pair(true, false); //fail the entire track loading
			}
		}

		model = &model_library[model_name];
	}

	bool skip = false;
	
	if (dynamicshadowsenabled && isashadow)
		skip = true;

	TEXTUREINFO texinfo;
	texinfo.SetName(objectpath + "/" + diffuse_texture_name);
	texinfo.SetMipMap(mipmap || anisotropy); //always mipmap if anisotropy is on
	texinfo.SetAnisotropy(anisotropy);
	bool clampu = clamptexture == 1 || clamptexture == 2;
	bool clampv = clamptexture == 1 || clamptexture == 3;
	texinfo.SetRepeat(!clampu, !clampv);
	texinfo.SetSize(texture_size);
	TEXTUREPTR diffuse_texture = textures.Get(texinfo);
	if (!diffuse_texture->Loaded())
	{
		error_output << "Error loading texture: " << objectpath + "/" + diffuse_texture_name << ", skipping object " << model_name << " and continuing" << std::endl;
		skip = true; //fail the loading of this model only
	}

	if (!skip)
	{
		TEXTUREPTR miscmap1_texture;
		std::string miscmap1_texture_name = diffuse_texture_name.substr(0, std::max(0, (int)diffuse_texture_name.length()-4)) + "-misc1.png";
		std::string filepath = objectpath + "/" + miscmap1_texture_name;
		std::ifstream filecheck(filepath.c_str());
		if (filecheck)
		{
			TEXTUREINFO texinfo;
			texinfo.SetName(filepath);
			texinfo.SetMipMap(mipmap);
			texinfo.SetAnisotropy(anisotropy);
			texinfo.SetSize(texture_size);
			miscmap1_texture = textures.Get(texinfo);
			if (!miscmap1_texture->Loaded())
			{
				error_output << "Error loading texture: " << objectpath + "/" + miscmap1_texture_name << " for object " << model_name << ", continuing" << std::endl;
			}
		}

		//use a different drawlist layer where necessary
		bool transparent = (transparent_blend==1);
		keyed_container <DRAWABLE> * dlist = &unoptimized_scene.GetDrawlist().normal_noblend;
		if (transparent)
			dlist = &unoptimized_scene.GetDrawlist().normal_blend;
		else if (nolighting)
			dlist = &unoptimized_scene.GetDrawlist().normal_noblend_nolighting;
		if (skybox)
		{
			if (transparent)
				dlist = &unoptimized_scene.GetDrawlist().skybox_blend;
			else
				dlist = &unoptimized_scene.GetDrawlist().skybox_noblend;
		}
		keyed_container <DRAWABLE>::handle dref = dlist->insert(DRAWABLE());
		DRAWABLE & drawable = dlist->get(dref);
		
		drawable.AddDrawList(model->GetListID());
		drawable.SetDiffuseMap(diffuse_texture);
		//if (miscmap1_texture) drawable.SetMiscMap1(miscmap1_texture);
		drawable.SetMiscMap1(miscmap1_texture);
		drawable.SetDecal(transparent);
		drawable.SetCull(cull && (transparent_blend!=2), false);
		drawable.SetRadius(model->GetRadius());
		drawable.SetObjectCenter(model->GetCenter());
		drawable.SetSkybox(skybox);
		if (skybox && vertical_tracking_skyboxes)
		{
			drawable.SetVerticalTrack(true);
		}

		const TRACKSURFACE * surfacePtr = NULL;
		if (collideable || driveable)
		{
			if(usesurfaces)
			{
				assert(surface_type >= 0 && surface_type < (int)surfaces.size());
				std::list<TRACKSURFACE>::iterator it = surfaces.begin();
				while(surface_type-- > 0) it++;
				surfacePtr = &*it;
			}
			else
			{
				TRACKSURFACE surface;
				surface.setType(surface_type);
				surface.bumpWaveLength = bump_wavelength;
				surface.bumpAmplitude = bump_amplitude;
				surface.frictionNonTread = friction_notread;
				surface.frictionTread = friction_tread;
				surface.rollResistanceCoefficient = rolling_resistance;
				surface.rollingDrag = rolling_drag;

				// could use hash here(assume we dont have many surfaces)
				std::list<TRACKSURFACE>::reverse_iterator ri;
				for(ri = surfaces.rbegin() ; ri != surfaces.rend(); ++ri)
				{
					if (*ri == surface) break;
				}
				if (ri == surfaces.rend())
				{
					surfaces.push_back(surface);
					ri = surfaces.rbegin();
				}
				surfacePtr = &*ri;
			}
		}

		TRACK_OBJECT object(model, surfacePtr);
		objects.push_back(object);
	}

	return std::make_pair(false, true);
}
