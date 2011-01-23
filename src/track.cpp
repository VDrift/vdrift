#include "track.h"
#include "config.h"
#include "reseatable_reference.h"
#include "tracksurface.h"
#include "objectloader.h"
#include "texturemanager.h"
#include "k1999.h"

#include <functional>
#include <algorithm>
#include <list>
#include <map>
#include <string>
#include <iostream>
#include <fstream>
#include <sstream>

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

TRACK::TRACK(std::ostream & info, std::ostream & error) :
	info_output(info),
	error_output(error),
	vertical_tracking_skyboxes(false),
	racingline_visible(false),
	loaded(false),
	cull(false)
{
	// ctor
}

TRACK::~TRACK()
{
	Clear();
}

bool TRACK::Load(
	TEXTUREMANAGER & textures,
	MODELMANAGER & models,
	const std::string & trackpath,
	const std::string & trackdir,
	const std::string & effects_texturepath,
	const std::string & texsize,
	const int anisotropy,
	const bool reverse)
{
	Clear();

	info_output << "Loading track from path: " << trackpath << std::endl;

	//load parameters
	if (!LoadParameters(trackpath))
	{
		return false;
	}
	
	if (!LoadSurfaces(trackpath))
	{
		info_output << "Error loading: " << trackpath << std::endl;
		return false;
	}

	//load roads
	if (!LoadRoads(trackpath, reverse))
	{
		error_output << "Error during road loading; continuing with an unsmoothed track" << std::endl;
		ClearRoads();
	}

	//load the lap sequence
	if (!LoadLapSequence(trackpath, reverse))
	{
		return false;
	}

	if (!CreateRacingLines(effects_texturepath, texsize, textures))
	{
		return false;
	}

	//load objects
	if (!LoadObjects(tracknode, textures, models, trackpath, trackdir, texsize, anisotropy))
	{
		return false;
	}

	return true;
}

bool TRACK::LoadLapSequence(const std::string & trackpath, bool reverse)
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
	trackconfig.GetParam(section, "cull faces", cull);

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
			for (std::list <ROADSTRIP>::iterator i = roads.begin(); i != roads.end(); ++i)
			{
				if (curroad == roadid)
				{
					int curpatch = 0;
					for (std::vector<ROADPATCH>::const_iterator p = i->GetPatches().begin(); p != i->GetPatches().end(); ++p)
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
		info_output << "Track timing sectors: " << lapmarkers << std::endl;

	return true;
}

bool TRACK::DeferredLoad(
	TEXTUREMANAGER & textures,
	MODELMANAGER & models,
	const std::string & trackpath,
	const std::string & trackdir,
	const std::string & effects_texturedir,
	const std::string & texsize,
	const int anisotropy,
	const bool reverse,
	const bool dynamicshadowsenabled,
	const bool doagressivecombining)
{
	Clear();

	info_output << "Loading track from path: " << trackpath << std::endl;

	//load parameters
	if (!LoadParameters(trackpath))
		return false;

	if (!LoadSurfaces(trackpath))
	{
		info_output << "No Surfaces File. Continuing with standard surfaces" << std::endl;
	}
	
	//load roads
	if (!LoadRoads(trackpath, reverse))
	{
		error_output << "Error during road loading; continuing with an unsmoothed track" << std::endl;
		ClearRoads();
	}

	//load the lap sequence
	if (!LoadLapSequence(trackpath, reverse))
	{
		return false;
	}

	if (!CreateRacingLines(effects_texturedir, texsize, textures))
	{
		return false;
	}

	//load objects
	if (!BeginObjectLoad(tracknode, textures, models, trackpath, trackdir, texsize, anisotropy, dynamicshadowsenabled, doagressivecombining))
		return false;

	return true;
}

bool TRACK::ContinueDeferredLoad()
{
	if (Loaded())
		return true;

	std::pair <bool, bool> loadstatus = ContinueObjectLoad();
	if (loadstatus.first)
		return false;
	if (!loadstatus.second)
	{
		loaded = true;
	}
	return true;
}

int TRACK::DeferredLoadTotalObjects() const
{
	assert(objload.get());
	return objload->GetNumObjects();
}

void TRACK::Clear()
{
	objects.clear();
	models.clear();
	tracksurfaces.clear();
	ClearRoads();
	lapsequence.clear();
	start_positions.clear();
	racingline_node.Clear();
	tracknode.Clear();
	loaded = false;
}

bool TRACK::CreateRacingLines(
	const std::string & texturepath,
	const std::string & texsize,
	TEXTUREMANAGER & textures)
{
	TEXTUREINFO texinfo; 
	texinfo.size = texsize;
	if (!textures.Load(texturepath, "racingline.png", texinfo, racingline_texture)) return false;
	
	K1999 k1999data;
	int n = 0;
	for (std::list <ROADSTRIP>::iterator i = roads.begin(); i != roads.end(); ++i,++n)
	{
		if (k1999data.LoadData(&(*i)))
		{
			k1999data.CalcRaceLine();
			k1999data.UpdateRoadStrip(&(*i));
		}
		//else error_output << "Couldn't create racing line for roadstrip " << n << std::endl;
		
		i->CreateRacingLine(racingline_node, racingline_texture, error_output);
	}
	
	return true;
}

bool TRACK::LoadParameters(const std::string & trackpath)
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

	vertical_tracking_skyboxes = false; //default to false
	param.GetParam(section, "vertical tracking skyboxes", vertical_tracking_skyboxes);

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
		
		// wtf ?
		QUATERNION <float> orient(q[2], q[0], q[1], q[3]);

		//due to historical reasons the initial orientation places the car faces the wrong way
		QUATERNION <float> fixer; 
		fixer.Rotate(3.141593, 0, 0, 1);
		orient = fixer * orient;

		MATHVECTOR <float, 3> pos(f3[2], f3[0], f3[1]);

		start_positions.push_back(std::pair <MATHVECTOR <float, 3>, QUATERNION <float> >
				(pos, orient));

		sp_num++;
		sp_name.str("");
		sp_name << "start position " << sp_num;
	}

	return true;
}

bool TRACK::LoadSurfaces(const std::string & trackpath)
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
		
		tracksurfaces.push_back(TRACKSURFACE());
		TRACKSURFACE & surface = tracksurfaces.back();

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
	info_output << "Loaded surfaces file, " << tracksurfaces.size() << " surfaces." << std::endl;
	
	return true;
}

bool TRACK::BeginObjectLoad(
	SCENENODE & sceneroot,
	TEXTUREMANAGER & texture_manager,
	MODELMANAGER & model_manager,
	const std::string & trackpath,
	const std::string & trackdir,
	const std::string & texsize,
	int anisotropy,
	bool dynamicshadowsenabled,
	bool doagressivecombining)
{
	objload.reset(
		new OBJECTLOADER(
			sceneroot,
			texture_manager,
			model_manager,
			models,
			objects,
			info_output,
			error_output,
			tracksurfaces,
			trackpath,
			trackdir,
			texsize,
			anisotropy,
			vertical_tracking_skyboxes,
			dynamicshadowsenabled,
			doagressivecombining,
			cull));

	if (!objload->BeginObjectLoad()) return false;

	return true;
}

std::pair <bool,bool> TRACK::ContinueObjectLoad()
{
	assert(objload.get());
	return objload->ContinueObjectLoad();
}

bool TRACK::LoadObjects(
	SCENENODE & sceneroot,
	TEXTUREMANAGER & textures,
	MODELMANAGER & models,
	const std::string & trackpath,
	const std::string & trackdir,
	const std::string & texsize,
	const int anisotropy)
{
	BeginObjectLoad(sceneroot, textures, models, trackpath, trackdir, texsize, anisotropy, false, false);
	
	std::pair <bool, bool> loadstatus = ContinueObjectLoad();
	while (!loadstatus.first && loadstatus.second)
	{
		loadstatus = ContinueObjectLoad();
	}
	return !loadstatus.first;
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

bool TRACK::LoadRoads(const std::string & trackpath, bool reverse)
{
	ClearRoads();

	std::ifstream trackfile;
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

bool TRACK::CastRay(
	const MATHVECTOR <float, 3> & origin,
	const MATHVECTOR <float, 3> & direction,
	const float seglen,
	int & patch_id,
	MATHVECTOR <float, 3> & outtri,
	const BEZIER * & colpatch,
	MATHVECTOR <float, 3> & normal) const
{
	bool col = false;
	for (std::list <ROADSTRIP>::const_iterator i = roads.begin(); i != roads.end(); ++i)
	{
		MATHVECTOR <float, 3> coltri, colnorm;
		const BEZIER * colbez = NULL;
		if (i->Collide(origin, direction, seglen, patch_id, coltri, colbez, colnorm))
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

optional <const BEZIER *> ROADSTRIP::FindBezierAtOffset(const BEZIER * bezier, int offset) const
{
	std::vector<ROADPATCH>::const_iterator it = patches.end(); //this iterator will hold the found ROADPATCH

	//search for the roadpatch containing the bezier and store an iterator to it in "it"
	for (std::vector<ROADPATCH>::const_iterator i = patches.begin(); i != patches.end(); ++i)
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
				std::vector<ROADPATCH>::const_reverse_iterator rit(it);
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

std::pair <MATHVECTOR <float, 3>, QUATERNION <float> > TRACK::GetStart(unsigned int index) const
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
