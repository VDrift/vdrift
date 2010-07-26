#include "trackloader.h"

#include "contentmanager.h"
#include "textureloader.h"
#include "modeljoeloader.h"

#include <string>
#include <fstream>

TrackLoader::TrackLoader(
	const std::string & ntrackpath,
	SCENENODE & nsceneroot,
	int nanisotropy,
	bool newdynamicshadowsenabled,
	std::ostream & ninfo_output,
	std::ostream & nerror_output,
	bool newcull,
	bool doagressivecombining) :
	trackpath(ntrackpath),
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

bool TrackLoader::BeginObjectLoad()
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

void TrackLoader::CalculateNumObjects()
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
}

std::pair <bool, bool> TrackLoader::ContinueObjectLoad(
	std::list <TrackObject> & objects,
	const std::vector <TRACKSURFACE> & surfaces,
 	const bool vertical_tracking_skyboxes,
 	const std::string & texture_size,
 	ContentManager & content)
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

	ModelJoeLoader joeload;
	if (packload)
	{
		joeload.pack = &pack;
	}
		
	joeload.name = model_name;
	ModelPtr model = content.get<MODEL>(joeload);
	if (!model.get())
	{
		return std::make_pair(true, false); //fail the entire track loading
	}

	bool skip = (dynamicshadowsenabled && isashadow);
	
	TextureLoader texload;
	texload.name = objectpath + "/" + diffuse_texture_name;
	texload.mipmap = mipmap || anisotropy; //always mipmap if anisotropy is on
	texload.anisotropy = anisotropy;
	bool clampu = clamptexture == 1 || clamptexture == 2;
	bool clampv = clamptexture == 1 || clamptexture == 3;
	texload.repeatu = !clampu;
	texload.repeatv = !clampv;
	texload.setSize(texture_size);
	TexturePtr diffuse_texture = content.get<TEXTURE>(texload);
	if (!diffuse_texture.get())
	{
		error_output << "Error loading texture: " << texload.name << ", skipping object " << model_name << " and continuing" << std::endl;
		skip = true; //fail the loading of this model only
	}

	if (!skip)
	{
		TexturePtr miscmap1_texture;
		{
			std::string miscmap1_texture_name = diffuse_texture_name.substr(0, std::max(0, (int)diffuse_texture_name.length()-4)) + "-misc1.png";
			std::string filepath = objectpath + "/" + miscmap1_texture_name;
			std::ifstream filecheck(filepath.c_str());
			if (filecheck)
			{
				TextureLoader texload;
				texload.name = filepath;
				texload.mipmap = mipmap;
				texload.anisotropy = anisotropy;
				texload.setSize(texture_size);
				miscmap1_texture = content.get<TEXTURE>(texload);
				if (!miscmap1_texture.get())
				{
					error_output << "Error loading texture: " << objectpath + "/" + miscmap1_texture_name << " for object " << model_name << ", continuing" << std::endl;
				}
			}
		}
		
		TexturePtr miscmap2_texture;
		{
			std::string miscmap2_texture_name = diffuse_texture_name.substr(0, std::max(0, (int)diffuse_texture_name.length()-4)) + "-misc2.png";
			std::string filepath = objectpath + "/" + miscmap2_texture_name;
			std::ifstream filecheck(filepath.c_str());
			if (filecheck)
			{
				TextureLoader texload;
				texload.name = filepath;
				texload.mipmap = mipmap;
				texload.anisotropy = anisotropy;
				texload.setSize(texture_size);
				miscmap2_texture = content.get<TEXTURE>(texload);
				if (!miscmap2_texture.get())
				{
					error_output << "Error loading texture: " << objectpath + "/" + miscmap2_texture_name << " for object " << model_name << ", continuing" << std::endl;
				}
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
		drawable.SetMiscMap1(miscmap1_texture);
		drawable.SetMiscMap2(miscmap2_texture);
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
			assert(surface_id >= 0 && surface_id < (int)surfaces.size());
			surfacePtr = &surfaces[surface_id];
		}

		objects.push_back(TrackObject(model, surfacePtr));
	}

	return std::make_pair(false, true);
}
