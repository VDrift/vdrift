#include "track.h"
#include "configfile.h"
#include "reseatable_reference.h"
#include "tracksurfacetype.h"

#include <functional>

#include <algorithm>

#include <list>
using std::list;

#include <map>
using std::pair;

#include <string>
using std::string;

#include <iostream>
using std::cout;
using std::endl;

#include <fstream>
using std::ifstream;

#include <sstream>
using std::stringstream;

bool TRACK::Load(const std::string & trackpath, const std::string & effects_texturepath, SCENENODE & rootnode, bool reverse, int anisotropy, const std::string & texsize, std::ostream & info_output, std::ostream & error_output)
{
	Clear();

	info_output << "Loading track from path: " << trackpath << endl;

	//load parameters
	if (!LoadParameters(trackpath, info_output, error_output))
	{
		return false;
	}
	
	if (!LoadSurfaces(trackpath, info_output, error_output))
	{
		info_output << "No surfaces file. Continuing with standard surfaces" << endl;
	}

	//load roads
	if (!LoadRoads(trackpath, reverse, error_output))
	{
		error_output << "Error during road loading; continuing with an unsmoothed track" << std::endl;
		ClearRoads();
	}

	//load the lap sequence
	if (!LoadLapSequence(trackpath, reverse, info_output, error_output))
	{
		return false;
	}

	if (!CreateRacingLines(&rootnode, effects_texturepath, texsize, error_output))
	{
		return false;
	}

	//load objects
	if (!LoadObjects(trackpath, rootnode, anisotropy, info_output, error_output))
	{
		return false;
	}

	info_output << "Track loading was successful: " << model_library.size() << " unique models, " << texture_library.size() << " unique textures" << endl;

	return true;
}

bool TRACK::LoadLapSequence(const std::string & trackpath, bool reverse, std::ostream & info_output, std::ostream & error_output)
{
	string parampath = trackpath + "/track.txt";
	CONFIGFILE trackconfig;
	if (!trackconfig.Load(parampath))
	{
		error_output << "Can't find track configfile: " << parampath << endl;
		return false;
	}
	
	trackconfig.GetParam("cull faces", cull);

	int lapmarkers = 0;
	if (trackconfig.GetParam("lap sequences", lapmarkers))
	{
		for (int l = 0; l < lapmarkers; l++)
		{
			float lapraw[3];
			stringstream lapname;
			lapname << "lap sequence " << l;
			trackconfig.GetParam(lapname.str(), lapraw);
			int roadid = lapraw[0];
			int patchid = lapraw[1];

			//info_output << "Looking for lap sequence: " << roadid << ", " << patchid << endl;

			int curroad = 0;
			for (list <ROADSTRIP>::iterator i = roads.begin(); i != roads.end(); ++i)
			{
				if (curroad == roadid)
				{
					int curpatch = 0;
					for (list <ROADPATCH>::const_iterator p = i->GetPatchList().begin(); p != i->GetPatchList().end(); ++p)
					{
						if (curpatch == patchid)
						{
							lapsequence.push_back(&p->GetPatch());
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
	if (!lapsequence.empty())
	{
		BEZIER* start_patch = const_cast <BEZIER *> (lapsequence[0]);
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
		info_output << "Track timing sectors: " << lapmarkers << endl;

	return true;
}

bool TRACK::DeferredLoad(const std::string & trackpath, const std::string & effects_texturepath, SCENENODE & rootnode, bool reverse, int anisotropy, const std::string & texsize, bool dynamicshadowsenabled, bool doagressivecombining, std::ostream & info_output, std::ostream & error_output)
{
	Clear();

	texture_size = texsize;

	info_output << "Loading track from path: " << trackpath << endl;

	//load parameters
	if (!LoadParameters(trackpath, info_output, error_output))
		return false;

	if (!LoadSurfaces(trackpath, info_output, error_output))
	{
		info_output << "No Surfaces File. Continuing with standard surfaces" << endl;
	}
	
	//load roads
	if (!LoadRoads(trackpath, reverse, error_output))
	{
		error_output << "Error during road loading; continuing with an unsmoothed track" << std::endl;
		ClearRoads();
	}

	//load the lap sequence
	if (!LoadLapSequence(trackpath, reverse, info_output, error_output))
	{
		return false;
	}

	if (!CreateRacingLines(&rootnode, effects_texturepath, texsize, error_output))
	{
		return false;
	}

	//load objects
	if (!BeginObjectLoad(trackpath, rootnode, anisotropy, dynamicshadowsenabled, doagressivecombining, info_output, error_output))
		return false;

	return true;
}

bool TRACK::ContinueDeferredLoad()
{
	if (Loaded())
		return true;

	pair <bool,bool> loadstatus = ContinueObjectLoad();
	if (loadstatus.first)
		return false;
	if (!loadstatus.second)
	{
		loaded = true;
	}
	return true;
}

void TRACK::Clear()
{
	objects.clear();
	model_library.clear();
	texture_library.clear();
	ClearRoads();
	lapsequence.clear();
	start_positions.clear();
	racingline_node = NULL;
	loaded = false;
	usesurfaces = false;
}

bool TRACK::LoadParameters(const std::string & trackpath, std::ostream & info_output, std::ostream & error_output)
{
	string parampath = trackpath + "/track.txt";
	CONFIGFILE param;
	if (!param.Load(parampath))
	{
		error_output << "Can't find track configfile: " << parampath << endl;
		return false;
	}

	vertical_tracking_skyboxes = false; //default to false
	param.GetParam("vertical tracking skyboxes", vertical_tracking_skyboxes);
	//cout << vertical_tracking_skyboxes << endl;

	int sp_num = 0;
	stringstream sp_name;
	sp_name << "start position " << sp_num;
	float f3[3];
	float f1;
	while (param.GetParam(sp_name.str(), f3))
	{
		MATHVECTOR <float, 3> pos;
		pos.Set(f3[2], f3[0], f3[1]);

		sp_name.str("");
		sp_name << "start orientation-xyz " << sp_num;
		if (!param.GetParam(sp_name.str(), f3))
		{
			error_output << "No matching orientation xyz for start position " << sp_num << endl;
			return false;
		}
		sp_name.str("");
		sp_name << "start orientation-w " << sp_num;
		if (!param.GetParam(sp_name.str(), f1))
		{
			error_output << "No matching orientation w for start position " << sp_num << endl;
			return false;
		}

		QUATERNION <float> orient(f3[2], f3[0], f3[1], f1);
		//QUATERNION <float> orient(f3[0], f3[1], f3[2], f1);

		start_positions.push_back(std::pair <MATHVECTOR <float, 3>, QUATERNION <float> >
				(pos, orient));

		sp_num++;
		sp_name.str("");
		sp_name << "start position " << sp_num;
	}

	return true;
}

bool TRACK::LoadSurfaces(const std::string & trackpath, std::ostream & info_output, std::ostream & error_output)
{
	string parampath = trackpath + "/surfaces.txt";
	CONFIGFILE param;
	if (!param.Load(parampath))
	{
		info_output << "Can't find surfaces configfile: " << parampath << endl;
		return false;
	}
	
	usesurfaces = true;
	
	list <string> sectionlist;
	param.GetSectionList(sectionlist);
		
	TRACKSURFACE tempsurface;
	
	// set the size of track surfaces to hold new elements
	tracksurfaces.resize(sectionlist.size());
	
	for (list<string>::const_iterator section = sectionlist.begin(); section != sectionlist.end(); ++section)
	{
		
		int indexnum;
		param.GetParam(*section + ".ID", indexnum);
		
		tempsurface.surfaceName = *section;
		
		float temp = 0.0;
		param.GetParamOrPrintError(*section + ".BumpWaveLength", temp, error_output);
		tempsurface.bumpWaveLength = temp;
		
		param.GetParamOrPrintError(*section + ".BumpAmplitude", temp, error_output);
		tempsurface.bumpAmplitude = temp;
		
		param.GetParamOrPrintError(*section + ".FrictionNonTread", temp, error_output);
		tempsurface.frictionNonTread = temp;
		
		param.GetParamOrPrintError(*section + ".FrictionTread", temp, error_output);
		tempsurface.frictionTread = temp;
		
		param.GetParamOrPrintError(*section + ".RollResistanceCoefficient", temp, error_output);
		tempsurface.rollResistanceCoefficient = temp;
		
		param.GetParamOrPrintError(*section + ".RollingDrag", temp, error_output);
		tempsurface.rollingDrag = temp;
		
		tracksurfaces[indexnum] = tempsurface;
	}
	info_output << "Found and loaded surfaces file" << endl;
	
	return true;
}

bool TRACK::BeginObjectLoad(const std::string & trackpath, SCENENODE & sceneroot, int anisotropy, bool dynamicshadowsenabled, bool doagressivecombining, std::ostream & info_output, std::ostream & error_output)
{
	objload.reset(new OBJECTLOADER(trackpath, sceneroot, anisotropy, dynamicshadowsenabled, info_output, error_output, cull, doagressivecombining));

	if (!objload->BeginObjectLoad())
		return false;

	return true;
}

bool TRACK::OBJECTLOADER::BeginObjectLoad()
{
	CalculateNumObjects();

	string objectpath = trackpath + "/objects";
	string objectlist = objectpath + "/list.txt";
	objectfile.open(objectlist.c_str());

	if (!GetParam(objectfile, params_per_object))
		return false;

	if (params_per_object != expected_params)
		info_output << "Track object list has " << params_per_object << " params per object, expected " << expected_params << ", this is fine, continuing" << endl;
	
	if (params_per_object < min_params)
	{
		error_output << "Track object list has " << params_per_object << " params per object, expected " << expected_params << endl;
		return false;
	}

	packload = pack.LoadPack(objectpath + "/objects.jpk");

	return true;
}

void TRACK::OBJECTLOADER::CalculateNumObjects()
{
	objectpath = trackpath + "/objects";
	string objectlist = objectpath + "/list.txt";
	ifstream f(objectlist.c_str());

	int params_per_object;
	if (!GetParam(f, params_per_object))
	{
		numobjects = 0;
		return;
	}

	numobjects = 0;

	string junk;
	while (GetParam(f, junk))
	{
		for (int i = 0; i < params_per_object-1; i++)
			GetParam(f, junk);

		numobjects++;
	}

	//info_output << "!!!!!!! " << numobjects << endl;
}

bool TRACK::OBJECTLOADER::GetSurfacesBool()
{
	info_output << "calling Get Surfaces Bool when we shouldn't!!! " << endl;
	if (params_per_object >= 17)
		return true;
	else
		return false;
}

std::pair <bool,bool> TRACK::OBJECTLOADER::ContinueObjectLoad(std::map <std::string, MODEL_JOE03> & model_library,
	std::map <std::string, TEXTURE> & texture_library,
	std::list <TRACK_OBJECT> & objects,
 	bool vertical_tracking_skyboxes, const std::string & texture_size)
{
	string model_name;

	if (error)
		return pair <bool,bool> (true, false);

	if (!(GetParam(objectfile, model_name)))
	{
		info_output << "Track loading was successful: " << model_library.size() << " unique models, " << texture_library.size() << " unique textures" << endl;
		Optimize();
		info_output << "Objects before optimization: " << unoptimized_scene.GetDrawableList().size() << ", objects after optimization: " << sceneroot.GetDrawableList().size() << endl;
		unoptimized_scene.Clear();
		return pair <bool,bool> (false, false);
	}

	assert(objectfile.good());

	string diffuse_texture_name;
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
	int surface_type(2);

	string otherjunk;

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
				error_output << "Error loading model: " << objectpath + "/" + model_name << " from pack " << objectpath + "/objects.jpk" << endl;
				return pair <bool,bool> (true, false); //fail the entire track loading
			}
		}
		else
		{
			if (!model_library[model_name].Load(objectpath + "/" + model_name, NULL, error_output))
			{
				error_output << "Error loading model: " << objectpath + "/" + model_name << endl;
				return pair <bool,bool> (true, false); //fail the entire track loading
			}
		}

		model = &model_library[model_name];

		//prob need to ensure physics system doesn't need this data before deleting it
		//model->ClearMeshData();
	}

	bool skip = false;
	
	if (dynamicshadowsenabled && isashadow)
		skip = true;

	if (texture_library.find(diffuse_texture_name) == texture_library.end())
	{
		TEXTUREINFO texinfo;
		texinfo.SetName(objectpath + "/" + diffuse_texture_name);
		texinfo.SetMipMap(mipmap || anisotropy); //always mipmap if anisotropy is on
		texinfo.SetAnisotropy(anisotropy);
		bool clampu = clamptexture == 1 || clamptexture == 2;
		bool clampv = clamptexture == 1 || clamptexture == 3;
		texinfo.SetRepeat(!clampu, !clampv);
		if (!texture_library[diffuse_texture_name].Load(texinfo, error_output, texture_size))
		{
			error_output << "Error loading texture: " << objectpath + "/" + diffuse_texture_name << ", skipping object " << model_name << " and continuing" << endl;
			skip = true; //fail the loading of this model only
		}
	}

	if (!skip)
	{
		reseatable_reference <TEXTURE> miscmap1;
		string miscmap1_texture_name = diffuse_texture_name.substr(0,std::max(0,(int)diffuse_texture_name.length()-4));
		miscmap1_texture_name += "-misc1.png";
		if (texture_library.find(miscmap1_texture_name) == texture_library.end())
		{
			TEXTUREINFO texinfo;
			string filepath = objectpath + "/" + miscmap1_texture_name;
			texinfo.SetName(filepath);
			texinfo.SetMipMap(mipmap);
			texinfo.SetAnisotropy(anisotropy);

			ifstream filecheck(filepath.c_str());
			if (filecheck)
			{
				if (!texture_library[miscmap1_texture_name].Load(texinfo, error_output, texture_size))
				{
					error_output << "Error loading texture: " << objectpath + "/" + miscmap1_texture_name << " for object " << model_name << ", continuing" << endl;
					texture_library.erase(miscmap1_texture_name);
					//don't fail, this isn't a critical error
				}
				else
					miscmap1 = texture_library[miscmap1_texture_name];
			}
		}
		else
			miscmap1 = texture_library.find(miscmap1_texture_name)->second;

		TEXTURE * diffuse = &texture_library[diffuse_texture_name];

		//info_output << "Loading " << model_name << endl;

		//DRAWABLE & d = sceneroot.AddDrawable();
		DRAWABLE & d = unoptimized_scene.AddDrawable();
		//d.SetModel(model);
		d.AddDrawList(model->GetListID());
		d.SetDiffuseMap(diffuse);
		if (miscmap1)
			d.SetMiscMap1(&miscmap1.get());
		d.SetLit(!nolighting);
		d.SetPartialTransparency((transparent_blend==1));
		d.SetCull(cull && (transparent_blend!=2), false);
		d.SetRadius(model->GetRadius());
		d.SetObjectCenter(model->GetCenter());
		d.SetSkybox(skybox);
		if (skybox && vertical_tracking_skyboxes)
		{
			d.SetVerticalTrack(true);
			//cout << "Vertical track" << endl;
		}

		objects.push_back(TRACK_OBJECT(model, diffuse,
				bump_wavelength,
				bump_amplitude,
				driveable,
				collideable,
				friction_notread,
				friction_tread,
				rolling_resistance,
				rolling_drag,
				surface_type));

		objects.back().InitCollisionObject();
	}

	return pair <bool,bool> (false, true);
}

std::pair <bool,bool> TRACK::ContinueObjectLoad()
{
	assert(objload.get());
	// usesurfaces = objload->GetSurfacesBool();
	return objload->ContinueObjectLoad(model_library, texture_library, objects, vertical_tracking_skyboxes, texture_size);
}

bool TRACK::LoadObjects(const std::string & trackpath, SCENENODE & sceneroot, int anisotropy, std::ostream & info_output, std::ostream & error_output)
{
	BeginObjectLoad(trackpath, sceneroot, anisotropy, false, false, info_output, error_output);
	pair <bool,bool> loadstatus = ContinueObjectLoad();
	while (!loadstatus.first && loadstatus.second)
	{
		loadstatus = ContinueObjectLoad();
	}
	return !loadstatus.first;
}

bool ROADPATCH::Collide(const MATHVECTOR <float, 3> & origin, const MATHVECTOR <float, 3> & direction, float seglen, MATHVECTOR <float, 3> & outtri, MATHVECTOR <float, 3> & normal) const
{
	bool col = patch.CollideSubDivQuadSimpleNorm(origin, direction, outtri, normal);
	if (col && (outtri-origin).Magnitude() <= seglen)
		return true;
	else
		return false;
}

bool ROADSTRIP::ReadFrom(std::istream & openfile, std::ostream & error_output)
{
	patches.clear();

	assert(openfile);

	//number of patches in this road strip
	int num;
	openfile >> num;

	//add all road patches to this strip
	int badcount = 0;
	for (int i = 0; i < num; i++)
	{
		BEZIER * prevbezier = NULL;
		if (!patches.empty())
			prevbezier = &patches.back().GetPatch();

		patches.push_back(ROADPATCH());
		patches.back().GetPatch().ReadFrom(openfile);

		if (prevbezier)
			prevbezier->Attach(patches.back().GetPatch());

		if (patches.back().GetPatch().CheckForProblems())
		{
			badcount++;
			patches.pop_back();
		}
	}

	if (badcount > 0)
		error_output << "Rejected " << badcount << " bezier patch(es) from roadstrip due to errors" << std::endl;

	//close the roadstrip
	if (patches.size() > 2)
	{
		//only close it if it ends near where it starts
		if (((patches.back().GetPatch().GetFL() - patches.front().GetPatch().GetBL()).Magnitude() < 0.1) &&
		    ((patches.back().GetPatch().GetFR() - patches.front().GetPatch().GetBR()).Magnitude() < 0.1))
		{
			patches.back().GetPatch().Attach(patches.front().GetPatch());
			closed = true;
		}
	}

	GenerateSpacePartitioning();

	return true;
}

void ROADSTRIP::GenerateSpacePartitioning()
{
	aabb_part.Clear();

	for (std::list <ROADPATCH>::iterator i = patches.begin(); i != patches.end(); ++i)
	{
		ROADPATCH * rp = &(*i);
		aabb_part.Add(rp, i->GetPatch().GetAABB());
	}

	aabb_part.Optimize();
}

bool ROADSTRIP::Collide(const MATHVECTOR <float, 3> & origin, const MATHVECTOR <float, 3> & direction, float seglen, MATHVECTOR <float, 3> & outtri, const BEZIER * & colpatch, MATHVECTOR <float, 3> & normal) const
{
	std::list <ROADPATCH *> candidates;
	aabb_part.Query(AABB<float>::RAY(origin, direction, seglen), candidates);
	bool col = false;
	for (std::list <ROADPATCH *>::iterator i = candidates.begin(); i != candidates.end(); ++i)
	{
		MATHVECTOR <float, 3> coltri, colnorm;
		if ((*i)->Collide(origin, direction, seglen, coltri, colnorm))
		{
			if (!col || (coltri-origin).Magnitude() < (outtri-origin).Magnitude())
			{
				outtri = coltri;
				normal = colnorm;
				colpatch = &(*i)->GetPatch();
			}

			col = true;
		}
	}

	return col;
}

void ROADSTRIP::Reverse()
{
	patches.reverse();

	for (std::list <ROADPATCH>::iterator i = patches.begin(); i != patches.end(); ++i)
	{
		i->GetPatch().Reverse();
		i->GetPatch().ResetDistFromStart();
	}

	//fix pointers to next patches for race placement
	for (std::list <ROADPATCH>::iterator i = patches.begin(); i != patches.end(); ++i)
	{
		std::list <ROADPATCH>::iterator n = i;
		n++;
		BEZIER * nextpatchptr = NULL;
		if (n != patches.end())
		{
			nextpatchptr = &(n->GetPatch());
			//i->GetPatch().Attach(*nextpatchptr, true);
			i->GetPatch().Attach(*nextpatchptr);
		}
		else
		{
			i->GetPatch().ResetNextPatch();
			i->GetPatch().Attach(patches.front().GetPatch());
		}
	}
}

void TRACK::Reverse()
{
	//move timing sector 0 back 1 patch so we'll still drive over it when going in reverse around the track
	if (!lapsequence.empty())
	{
		int counts = 0;

		for (std::list <ROADSTRIP>::iterator i = roads.begin(); i != roads.end(); ++i)
		{
			optional <const BEZIER *> newstartline = i->FindBezierAtOffset(lapsequence[0],-1);
			if (newstartline)
			{
				lapsequence[0] = newstartline.get();
				counts++;
			}
		}

		assert(counts == 1); //do a sanity check, because I don't trust the FindBezierAtOffset function
	}

	//reverse the timing sectors
	if (lapsequence.size() > 1)
	{
		//reverse the lap sequence, but keep the first bezier where it is (remember, the track is a loop)
		//so, for example, now instead of 1 2 3 4 we should have 1 4 3 2
		std::vector <const BEZIER *>::iterator secondbezier = lapsequence.begin();
		++secondbezier;
		assert(secondbezier != lapsequence.end());
		std::reverse(secondbezier, lapsequence.end());
	}


	//flip start positions
	for (std::vector <std::pair <MATHVECTOR <float, 3>, QUATERNION <float> > >::iterator i = start_positions.begin();
		i != start_positions.end(); ++i)
	{
		i->second.Rotate(3.141593, 0,0,1);
	}

	//reverse start positions
	std::reverse(start_positions.begin(), start_positions.end());

	//reverse roads
	std::for_each(roads.begin(), roads.end(), std::mem_fun_ref(&ROADSTRIP::Reverse));
}

bool TRACK::LoadRoads(const std::string & trackpath, bool reverse, std::ostream & error_output)
{
	ClearRoads();

	ifstream trackfile;
	trackfile.open((trackpath + "/roads.trk").c_str());
	if (!trackfile)
	{
		error_output << "Error opening roads file: " << trackpath + "/roads.trk" << std::endl;
		return false;
	}

	int numroads;

	trackfile >> numroads;

	for (int i = 0; i < numroads && trackfile; i++)
	{
		roads.push_back(ROADSTRIP());
		roads.back().ReadFrom(trackfile, error_output);
	}

	if (reverse)
	{
		Reverse();
		direction = DIRECTION_REVERSE;
	}
	else
		direction = DIRECTION_FORWARD;

	return true;
}

bool TRACK::CollideRoads(const MATHVECTOR <float, 3> & origin, const MATHVECTOR <float, 3> & direction, float seglen, MATHVECTOR <float, 3> & outtri, const BEZIER * & colpatch, MATHVECTOR <float, 3> & normal) const
{
	bool col = false;
	for (std::list <ROADSTRIP>::const_iterator i = roads.begin(); i != roads.end(); ++i)
	{
		MATHVECTOR <float, 3> coltri, colnorm;
		const BEZIER * colbez = NULL;
		if (i->Collide(origin, direction, seglen, coltri, colbez, colnorm))
		{
			if (!col || (coltri-origin).Magnitude() < (outtri-origin).Magnitude())
			{
				outtri = coltri;
				normal = colnorm;
				colpatch = colbez;
			}

			col = true;
		}
	}

	return col;
}

void ROADPATCH::AddRacinglineScenenode(SCENENODE * node, ROADPATCH * nextpatch,
				       TEXTURE_GL & racingline_texture, std::ostream & error_output)
{
	assert(node);

	if (!nextpatch)
		return;

	//Create racing line scenenode
	racingline_draw = &node->AddDrawable();

	racingline_draw->SetDiffuseMap(&racingline_texture);
	racingline_draw->SetLit(false);
	racingline_draw->SetPartialTransparency(true);
	racingline_draw->SetDecal(true);
	racingline_draw->SetVertArray(&racingline_vertexarray);

	MATHVECTOR <float, 3> v0 = racing_line + (patch.GetPoint(0,0) - racing_line).Normalize()*0.1;
	MATHVECTOR <float, 3> v1 = racing_line + (patch.GetPoint(0,3) - racing_line).Normalize()*0.1;
	MATHVECTOR <float, 3> v2 = nextpatch->racing_line + (nextpatch->GetPatch().GetPoint(0,3) - nextpatch->racing_line).Normalize()*0.1;
	MATHVECTOR <float, 3> v3 = nextpatch->racing_line + (nextpatch->GetPatch().GetPoint(0,0) - nextpatch->racing_line).Normalize()*0.1;

	//transform from bezier space into world space
	v0.Set(v0[2],v0[0],v0[1]);
	v1.Set(v1[2],v1[0],v1[1]);
	v2.Set(v2[2],v2[0],v2[1]);
	v3.Set(v3[2],v3[0],v3[1]);

	float trackoffset = 0.1;
	v0[2] += trackoffset;
	v1[2] += trackoffset;
	v2[2] += trackoffset;
	v3[2] += trackoffset;

	float vcorners[12];
	float uvs[8];
	int bfaces[6];

	//std::cout << v0 << std::endl;

	vcorners[0] = v0[0]; vcorners[1] = v0[1]; vcorners[2] = v0[2];
	vcorners[3] = v1[0]; vcorners[4] = v1[1]; vcorners[5] = v1[2];
	vcorners[6] = v2[0]; vcorners[7] = v2[1]; vcorners[8] = v2[2];
	vcorners[9] = v3[0]; vcorners[10] = v3[1]; vcorners[11] = v3[2];

	//std::cout << v0 << endl;
	//std::cout << racing_line << endl;

	uvs[0] = 0;
	uvs[1] = 0;
	uvs[2] = 1;
	uvs[3] = 0;
	uvs[4] = 1;
	uvs[5] = (v2-v1).Magnitude();
	uvs[6] = 0;
	uvs[7] = (v2-v1).Magnitude();

	bfaces[0] = 0;
	bfaces[1] = 2;
	bfaces[2] = 1;
	bfaces[3] = 0;
	bfaces[4] = 3;
	bfaces[5] = 2;

	racingline_vertexarray.SetFaces(bfaces, 6);
	racingline_vertexarray.SetVertices(vcorners, 12);
	racingline_vertexarray.SetTexCoordSets(1);
	racingline_vertexarray.SetTexCoords(0, uvs, 8);
}

optional <const BEZIER *> ROADSTRIP::FindBezierAtOffset(const BEZIER * bezier, int offset) const
{
	std::list <ROADPATCH>::const_iterator it = patches.end(); //this iterator will hold the found ROADPATCH

	//search for the roadpatch containing the bezier and store an iterator to it in "it"
	for (std::list <ROADPATCH>::const_iterator i = patches.begin(); i != patches.end(); ++i)
	{
		if (&i->GetPatch() == bezier)
		{
			it = i;
			break;
		}
	}

	if (it == patches.end())
		return optional <const BEZIER *>(); //return nothing
	else
	{
		//now do the offset
		int curoffset = offset;
		while (curoffset != 0)
		{
			if (curoffset < 0)
			{
				//why is this so difficult?  all i'm trying to do is make the iterator loop around
				std::list <ROADPATCH>::const_reverse_iterator rit(it);
				if (rit == patches.rend())
					rit = patches.rbegin();
				rit++;
				if (rit == patches.rend())
					rit = patches.rbegin();
				it = rit.base();
				if (it == patches.end())
					it = patches.begin();

				curoffset++;
			}
			else if (curoffset > 0)
			{
				it++;
				if (it == patches.end())
					it = patches.begin();

				curoffset--;
			}
		}

		assert(it != patches.end());
		return optional <const BEZIER *>(&it->GetPatch());
	}
}

std::string booltostr(bool val)
{
	if (val)
		return "Y";
	else return "N";
}

std::string GetDrawableSortString(const DRAWABLE & d)
{
	std::string s = d.GetDiffuseMap()->GetTextureInfo().GetName();
	s.append(booltostr(d.GetLit()));
	s.append(booltostr(d.GetSkybox()));
	s.append(booltostr(d.GetPartialTransparency()));
	s.append(booltostr(d.GetCull()));
	return s;
}

bool DrawableOptimizeLessThan(const DRAWABLE & d1, const DRAWABLE & d2)
{
	/*return (d1.GetDiffuseMap() < d2.GetDiffuseMap()) ||
			(d1.GetLit() < d2.GetLit()) ||
			(d1.GetSkybox() < d2.GetSkybox()) ||
			(d1.GetPartialTransparency() < d2.GetPartialTransparency()) ||
			(d1.GetCull() < d2.GetCull());*/
	
	//return (d1.GetDiffuseMap() < d2.GetDiffuseMap());
	
	return GetDrawableSortString(d1) < GetDrawableSortString(d2);
	
	/* ||
			(d1.GetObjectCenter()[0] < d2.GetObjectCenter()[0]) ||
			(d1.GetObjectCenter()[1] < d2.GetObjectCenter()[1]);*/
}

bool DrawableOptimizeEqual(const DRAWABLE & d1, const DRAWABLE & d2)
{
	return (d1.GetDiffuseMap() == d2.GetDiffuseMap()) &&
			(d1.GetLit() == d2.GetLit()) &&
			(d1.GetSkybox() == d2.GetSkybox()) &&
			(d1.GetPartialTransparency() == d2.GetPartialTransparency()) &&
			(d1.GetCull() == d2.GetCull());
}

void TRACK::OBJECTLOADER::Optimize()
{
	unoptimized_scene.GetDrawableList().sort(DrawableOptimizeLessThan);

	DRAWABLE lastmatch;
	DRAWABLE * lastdrawable = NULL;

	bool optimize = false;
	const float optimizemetric = 10.0; //this seems like a ridiculous metric, but *doubles* framerates on my ATI 4850
	//if (unoptimized_scene.GetDrawableList().size() > 2500) optimize = true;
	if (agressivecombine) optimize = true; //framerate only gained here for ATI cards, so we leave agressivecombine false for NVIDIA

	for (std::list <DRAWABLE>::const_iterator i = unoptimized_scene.GetDrawableList().begin();
		i != unoptimized_scene.GetDrawableList().end(); ++i)
	{
		if (optimize)
		{
			if (DrawableOptimizeEqual(lastmatch, *i))
			{
				assert(i->IsDrawList());
				assert(lastdrawable);

				//combine culling spheres
				MATHVECTOR <float, 3> center1(lastdrawable->GetObjectCenter()), center2(i->GetObjectCenter());
				float radius1(lastdrawable->GetRadius()), radius2(i->GetRadius());

				//find the new center point by taking the halfway point of the two centers
				MATHVECTOR <float, 3> newcenter((center2+center1)*0.5);

				float maxradius = std::max(radius1, radius2);

				//find the new radius by taking half the distance between the centers plus the max radius
				//float newradius = (center2-center1).Magnitude()*0.5+maxradius;
				float newradius = (center2-center1).Magnitude()*0.5+maxradius;
				
				if (newradius > (radius1+radius2)*optimizemetric) //don't combine if it's not worth it
				//if (0)
				{
					lastmatch = *i;

					DRAWABLE & d = sceneroot.AddDrawable();
					d = *i;
					lastdrawable = &d;
					
					//std::cout << "Not optimizing: " << i->GetRadius() << " and " << lastdrawable->GetRadius() << " to " << newradius << std::endl;
				}
				else
				{
					lastdrawable->AddDrawList(i->GetDrawLists()[0]);
					lastdrawable->SetObjectCenter(newcenter);
					lastdrawable->SetRadius(newradius);
					
					//std::cout << "Optimizing: " << i->GetRadius() << " and " << lastdrawable->GetRadius() << " to " << newradius << std::endl;
				}

				/*std::cout << "center1: " << center1 << std::endl;
				std::cout << "radius1: " << radius1 << std::endl;
				std::cout << "center2: " << center2 << std::endl;
				std::cout << "radius2: " << radius2 << std::endl;
				std::cout << "newcenter: " << newcenter << std::endl;
				std::cout << "newradius: " << newradius << std::endl;*/
			}
			else if (i->IsDrawList())
			{
				lastmatch = *i;

				DRAWABLE & d = sceneroot.AddDrawable();
				d = *i;
				lastdrawable = &d;
			}
		}
		else
		{
			DRAWABLE & d = sceneroot.AddDrawable();
			d = *i;
		}
	}
}

std::pair <MATHVECTOR <float, 3>, QUATERNION <float> > TRACK::GetStart(unsigned int index)
{
	assert(!start_positions.empty());
	unsigned int laststart = start_positions.size()-1;
	if (index > laststart)
	{
		std::pair <MATHVECTOR <float, 3>, QUATERNION <float> > sp = start_positions[laststart];
		MATHVECTOR <float, 3> backward(6,0,0);
		backward = backward * (index-laststart);
		sp.second.RotateVector(backward);
		sp.first = sp.first + backward;
		return sp;
	}
	else
		return start_positions[index];
}
