#include "car.h"
#include "carwheelposition.h"
#include "configfile.h"
#include "coordinatesystems.h"

#include <iostream>
using std::cout;
using std::endl;

#include <algorithm>

#include <fstream>
using std::ifstream;

#include <map>
using std::pair;

#include <list>
using std::list;

#include <vector>
using std::vector;

#include <sstream>
using std::stringstream;

#include <string>
using std::string;

#include <iomanip>


#ifdef _WIN32
bool isnan(float number) {return (number != number);}
bool isnan(double number) {return (number != number);}
#endif

CAR::CAR() : topnode(NULL),bodydraw(NULL),glassdraw(NULL),bodynode(NULL),drivernode(NULL),auto_clutch(true),last_auto_clutch(1.0),remaining_shift_time(0.0),shift_gear(0),shifted(true),last_steer(0),autoshift_enabled(false),shift_time(0.2),debug_wheel_draw(false),sector(-1)
{
	for (int i = 0; i < 4; i++)
	{
		curpatch[i] = NULL;
		wheelnode[i] = NULL;
		floatingnode[i] = NULL;
	}
		
	feedbackbuffer.resize(20,0.0);
	feedbackbufferpos = 0;
}

bool CAR::Load (CONFIGFILE & carconf, const std::string & carpath, const std::string & driverpath, const std::string & carname, const std::string & carpaint,
		const MATHVECTOR <float, 3> & initial_position, const QUATERNION <float> & initial_orientation,
		SCENENODE & sceneroot, bool soundenabled, const SOUNDINFO & sound_device_info, const SOUNDBUFFERLIBRARY & soundbufferlibrary,
		int anisotropy, bool defaultabs, bool defaulttcs, const std::string & texsize, float camerabounce,
  		bool debugmode, std::ostream & info_output, std::ostream & error_output )
{
	topnode = &sceneroot.AddNode();
	assert(topnode);
	debug_wheel_draw = debugmode;

	cartype = carname;

	driver_cam.SetEffectStrength(camerabounce);
	hood_cam.SetEffectStrength(camerabounce);
	
	std::stringstream nullout;

	//load car body graphics
	if ( !LoadInto ( topnode, bodynode, bodydraw, carpath+"/"+carname+"/body.joe", bodymodel, carpath+"/"+carname+
		"/textures/body"+carpaint+".png", bodytexture,
       		carpath+"/"+carname+"/textures/body-misc1.png", bodytexture_misc1,
       		anisotropy, texsize, error_output ) )
		return false;

	//load driver graphics
	if (!LoadInto(bodynode, drivernode, driverdraw, driverpath+"/body.joe", drivermodel,
		driverpath+"/textures/body.png", drivertexture,
  		driverpath+"/textures/body-misc1.png", drivertexture_misc1,
      		anisotropy, texsize, error_output))
	{
		drivernode = NULL;
		error_output << "Error loading driver graphics: " << driverpath << endl;
	}

	//load car brake light texture
	{
		TEXTUREINFO texinfo;
		texinfo.SetName(carpath+"/"+carname+"/textures/brake.png");
		texinfo.SetMipMap(true);
		texinfo.SetAnisotropy(anisotropy);
		if (!braketexture.Loaded())
		{
			if (!braketexture.Load(texinfo, nullout, texsize))
			{
				info_output << "No car brake texture exists, continuing without one" << endl;
			}
			else
			{
				assert(bodydraw);
			}
		}
	}

	//load car reverse light texture
	{
		TEXTUREINFO texinfo;
		texinfo.SetName(carpath+"/"+carname+"/textures/reverse.png");
		texinfo.SetMipMap(true);
		texinfo.SetAnisotropy(anisotropy);
		if (!reversetexture.Loaded())
		{
			if (!reversetexture.Load(texinfo, nullout, texsize))
			{
				info_output << "No car reverse texture exists, continuing without one" << endl;
			}
			else
			{
				assert(bodydraw);
			}
		}
	}

	bodydraw->SetSelfIllumination(false);

	//load car interior graphics
	if ( !LoadInto ( bodynode, bodynode, interiordraw, carpath+"/"+carname+"/interior.joe", interiormodel,
	      		carpath+"/"+carname+"/textures/interior.png", interiortexture,
	 		carpath+"/"+carname+"/textures/interior-misc1.png", interiortexture_misc1,
	 		anisotropy, texsize, nullout ) )
	{
		info_output << "No car interior model exists, continuing without one" << endl;
		//return false;
	}

	//load car glass graphics
	if ( !LoadInto ( bodynode, bodynode, glassdraw, carpath+"/"+carname+"/glass.joe", glassmodel,
	     		carpath+"/"+carname+"/textures/glass.png", glasstexture,
			carpath+"/"+carname+"/textures/glass-misc1.png", glasstexture_misc1,
			anisotropy, texsize, nullout ) )
	{
		info_output << "No car glass model exists, continuing without one" << endl;
		//return false;
	}
	else
		glassdraw->SetPartialTransparency(true);

	//load wheel graphics
	for (int i = 0; i < 2; i++) //front pair
	{
		if ( !LoadInto ( topnode, wheelnode[i], wheeldraw[i],
			carpath+"/"+carname+"/wheel_front.joe", wheelmodelfront,
   			carpath+"/"+carname+"/textures/wheel_front.png", wheeltexturefront,
      			carpath+"/"+carname+"/textures/wheel_front-misc1.png", wheeltexturefront_misc1,
      			anisotropy, texsize, error_output ) )
			return false;
		
		//load floating elements
		std::stringstream nullout;
		LoadInto ( topnode, floatingnode[i], floatingdraw[i],
			carpath+"/"+carname+"/floating_front.joe", floatingmodelfront,
   			"", bodytexture,
      		"", bodytexture_misc1,
      		anisotropy, texsize, nullout );
	}
	for (int i = 2; i < 4; i++) //rear pair
	{
		if ( !LoadInto ( topnode, wheelnode[i], wheeldraw[i],
		     	carpath+"/"+carname+"/wheel_rear.joe", wheelmodelrear,
			carpath+"/"+carname+"/textures/wheel_rear.png", wheeltexturerear,
			carpath+"/"+carname+"/textures/wheel_rear-misc1.png", wheeltexturerear_misc1,
			anisotropy, texsize, error_output ) )
			return false;
		
		//load floating elements
		std::stringstream nullout;
		LoadInto ( topnode, floatingnode[i], floatingdraw[i],
			carpath+"/"+carname+"/floating_rear.joe", floatingmodelrear,
   			"", bodytexture,
      		"", bodytexture_misc1,
      		anisotropy, texsize, nullout );
	}

	//load debug wheel graphics
	if (debug_wheel_draw)
	{
		for (int w = 0; w < 4; w++)
		{
			for (int i = 0; i < 10; i++)
			{
				debugwheelnode[w*10+i] = &topnode->AddNode();
				debugwheeldraw[w*10+i] = &debugwheelnode[w*10+i]->AddDrawable();
				if (w < 2)
				{
					//debugwheeldraw[w*10+i]->SetModel(&wheelmodelfront);
					debugwheeldraw[w*10+i]->AddDrawList(wheelmodelfront.GetListID());
					debugwheeldraw[w*10+i]->SetDiffuseMap(&wheeltexturefront);
					debugwheeldraw[w*10+i]->SetObjectCenter(wheelmodelfront.GetCenter());
				}
				else
				{
					//debugwheeldraw[w*10+i]->SetModel(&wheelmodelrear);
					debugwheeldraw[w*10+i]->AddDrawList(wheelmodelrear.GetListID());
					debugwheeldraw[w*10+i]->SetDiffuseMap(&wheeltexturerear);
					debugwheeldraw[w*10+i]->SetObjectCenter(wheelmodelrear.GetCenter());
				}

				debugwheeldraw[w*10+i]->SetColor(1,1,1,0.25);
				debugwheeldraw[w*10+i]->SetPartialTransparency(true);
			}
		}
	}

	//load up the car mass and other properties here
	if (!LoadDynamics(carconf, error_output))
		return false;
	
	//generate collision box
	GenerateCollisionData();

	//set ABS and TCS
	dynamics.SetABS(defaultabs);
	dynamics.SetTCS(defaulttcs);

	QUATERNION <float> fixer; //due to historical reasons the initial orientation places the car faces the wrong way
	fixer.Rotate(3.141593,0,0,1);
	dynamics.SetInitialConditions(initial_position, fixer*initial_orientation);

	//setup the cameras
	/*driver_cam.SetPositionBlending(false);
	driver_cam.SetChaseHeight(0.0f);
	driver_cam.SetChaseDistance(0.0f);*/

	//load sounds
	if (soundenabled)
		if (!LoadSounds(carpath, carname, sound_device_info, soundbufferlibrary, info_output, error_output))
			return false;
	
	lookbehind = false;

	return true;
}

///unload any loaded assets
CAR::~CAR()
{
	if (topnode)
		topnode->Clear();
}

bool CAR::LoadSounds(const std::string & carpath, const std::string & carname, const SOUNDINFO & sound_device_info,
		     const SOUNDBUFFERLIBRARY & soundbufferlibrary, std::ostream & info_output, std::ostream & error_output)
{
	//check for sound specification file
	CONFIGFILE aud;
	if (aud.Load(carpath+"/"+carname+"/"+carname+".aud"))
	{
		list <string> sections;
		aud.GetSectionList(sections);
		for (list <string>::iterator i = sections.begin(); i != sections.end(); ++i)
		{
			//load the buffer
			string filename;
			if (!GetConfigfileParam(error_output, aud, *i+".filename", filename)) return false;
			if (!soundbuffers[filename].GetLoaded())
				if (!soundbuffers[filename].Load(carpath+"/"+carname+"/"+filename, sound_device_info, error_output))
				{
					error_output << "Error loading sound: " << carpath+"/"+carname+"/"+filename << endl;
					return false;
				}

			enginesounds.push_back(std::pair <ENGINESOUNDINFO, SOUNDSOURCE> ());
			ENGINESOUNDINFO & info = enginesounds.back().first;
			SOUNDSOURCE & sound = enginesounds.back().second;

			if (!GetConfigfileParam(error_output, aud, *i+".MinimumRPM", info.minrpm)) return false;
			if (!GetConfigfileParam(error_output, aud, *i+".MaximumRPM", info.maxrpm)) return false;
			if (!GetConfigfileParam(error_output, aud, *i+".NaturalRPM", info.naturalrpm)) return false;

			string powersetting;
			if (!GetConfigfileParam(error_output, aud, *i+".power", powersetting)) return false;
			if (powersetting == "on")
				info.power = ENGINESOUNDINFO::POWERON;
			else if (powersetting == "off")
				info.power = ENGINESOUNDINFO::POWEROFF;
			else //assume it's used in both ways
				info.power = ENGINESOUNDINFO::BOTH;

			sound.SetBuffer(soundbuffers[filename]);
			sound.Set3DEffects(true);
			sound.SetLoop(true);
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

			for (list <ENGINESOUNDINFO>::iterator i = (*cursounds).begin(); i != (*cursounds).end(); ++i)
			{
				//set start blend
				if (i == (*cursounds).begin())
					i->fullgainrpmstart = i->minrpm;
				//else, the blend start has been set already by the previous iteration

				//set end blend
				list <ENGINESOUNDINFO>::iterator inext = i;
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
			for (list <ENGINESOUNDINFO>::iterator i = (*cursounds).begin(); i != (*cursounds).end(); ++i)
			{
				assert(temporary_to_actual_map.find(&(*i)) != temporary_to_actual_map.end());
				*temporary_to_actual_map[&(*i)] = *i;
			}
		}

		/*for (std::list <std::pair <ENGINESOUNDINFO, SOUNDSOURCE> >::iterator i = enginesounds.begin(); i != enginesounds.end(); i++)
		{
			ENGINESOUNDINFO & info = i->first;
			SOUNDSOURCE & sound = i->second;
			std::cout << sound.GetSoundBuffer().GetName() << ": " << info.power << "," << info.minrpm << "," << info.fullgainrpmstart << "," << info.naturalrpm << "," << info.fullgainrpmend << "," << info.maxrpm << endl;
		}*/
	}
	else
	{
		if (!soundbuffers["engine.wav"].Load(carpath+"/"+carname+"/engine.wav", sound_device_info, error_output))
		{
			error_output << "Unable to load engine sound: "+carpath+"/"+carname+"/engine.wav" << endl;
			return false;
		}
		enginesounds.push_back(std::pair <ENGINESOUNDINFO, SOUNDSOURCE> ());
		SOUNDSOURCE & enginesound = enginesounds.back().second;
		enginesound.SetBuffer(soundbuffers["engine.wav"]);
		enginesound.Set3DEffects(true);
		enginesound.SetLoop(true);
		enginesound.SetGain(0);
		enginesound.Play();
	}

	//set up tire squeal sounds
	for (int i = 0; i < 4; i++)
	{
		const SOUNDBUFFER * buf = soundbufferlibrary.GetBuffer("tire_squeal");
		if (!buf)
		{
			error_output << "Can't load tire_squeal sound" << endl;
			return false;
		}
		tiresqueal[i].SetBuffer(*buf);
		tiresqueal[i].Set3DEffects(true);
		tiresqueal[i].SetLoop(true);
		tiresqueal[i].SetGain(0);
		int samples = tiresqueal[i].GetSoundBuffer().GetSoundInfo().GetSamples();
		tiresqueal[i].SeekToSample((samples/4)*i);
		tiresqueal[i].Play();
	}
	
	//set up tire gravel sounds
	for (int i = 0; i < 4; i++)
	{
		const SOUNDBUFFER * buf = soundbufferlibrary.GetBuffer("gravel");
		if (!buf)
		{
			error_output << "Can't load gravel sound" << endl;
			return false;
		}
		gravelsound[i].SetBuffer(*buf);
		gravelsound[i].Set3DEffects(true);
		gravelsound[i].SetLoop(true);
		gravelsound[i].SetGain(0);
		int samples = gravelsound[i].GetSoundBuffer().GetSoundInfo().GetSamples();
		gravelsound[i].SeekToSample((samples/4)*i);
		gravelsound[i].Play();
	}

	//set up tire grass sounds
	for (int i = 0; i < 4; i++)
	{
		const SOUNDBUFFER * buf = soundbufferlibrary.GetBuffer("grass");
		if (!buf)
		{
			error_output << "Can't load grass sound" << endl;
			return false;
		}
		grasssound[i].SetBuffer(*buf);
		grasssound[i].Set3DEffects(true);
		grasssound[i].SetLoop(true);
		grasssound[i].SetGain(0);
		int samples = grasssound[i].GetSoundBuffer().GetSoundInfo().GetSamples();
		grasssound[i].SeekToSample((samples/4)*i);
		grasssound[i].Play();
	}
	
	//set up bump sounds
	for (int i = 0; i < 4; i++)
	{
		const SOUNDBUFFER * buf = soundbufferlibrary.GetBuffer("bump_front");
		if (i >= 2)
			buf = soundbufferlibrary.GetBuffer("bump_rear");
		if (!buf)
		{
			error_output << "Can't load bump sound: " << i << endl;
			return false;
		}
		tirebump[i].SetBuffer(*buf);
		tirebump[i].Set3DEffects(true);
		tirebump[i].SetLoop(false);
		tirebump[i].SetGain(1.0);
	}
	
	//set up crash sound
	{
		const SOUNDBUFFER * buf = soundbufferlibrary.GetBuffer("crash");
		if (!buf)
		{
			error_output << "Can't load crash sound" << endl;
			return false;
		}
		crashsound.SetBuffer(*buf);
		crashsound.Set3DEffects(true);
		crashsound.SetLoop(false);
		crashsound.SetGain(1.0);
	}

	{
		const SOUNDBUFFER * buf = soundbufferlibrary.GetBuffer("wind");
		if (!buf)
		{
			error_output << "Can't load wind sound" << endl;
			return false;
		}
		roadnoise.SetBuffer(*buf);
		roadnoise.Set3DEffects(true);
		roadnoise.SetLoop(true);
		roadnoise.SetGain(0);
		roadnoise.SetPitch(1.0);
		roadnoise.Play();
	}

	return true;
}

bool CAR::LoadInto ( SCENENODE * parentnode, SCENENODE * & output_scenenodeptr, DRAWABLE * & output_drawableptr, const std::string & joefile,
		     MODEL_JOE03 & output_model,
       	const std::string & texfile, TEXTURE_GL & output_texture_diffuse,
	const std::string & misc1texfile, TEXTURE_GL & output_texture_misc1,
		     int anisotropy, const std::string & texsize, std::ostream & error_output )
{
	assert ( parentnode );

	if (!output_model.Loaded())
	{
		std::stringstream nullout;
		if (!output_model.ReadFromFile(joefile.substr(0,std::max((long unsigned int)0,(long unsigned int) joefile.size()-3))+"ova", nullout))
		{
			if (!output_model.Load(joefile, error_output))
			{
				error_output << "Error loading model: " << joefile << endl;
				return false;
			}
		}
	}

	{
		TEXTUREINFO texinfo;
		texinfo.SetName(texfile);
		texinfo.SetMipMap(true);
		texinfo.SetAnisotropy(anisotropy);
		const string texture_size(texsize);
		if (!output_texture_diffuse.Loaded())
			if (!output_texture_diffuse.Load(texinfo, error_output, texture_size))
			{
				error_output << "Error loading texture: " << texfile << endl;
				return false;
			}
	}

	if (!misc1texfile.empty())
	{
		ifstream filecheck(misc1texfile.c_str());
		if (filecheck)
		{
			if (!output_texture_misc1.Loaded())
			{
				TEXTUREINFO texinfo;
				texinfo.SetName(misc1texfile);
				texinfo.SetMipMap(true);
				texinfo.SetAnisotropy(anisotropy);
				const string texture_size(texsize);

				if (!output_texture_misc1.Load(texinfo, error_output, texture_size))
				{
					error_output << "Error loading texture: " << texfile << endl;
					return false;
				}
			}
		}
	}

	SCENENODE * node = parentnode;
	if (!output_scenenodeptr)
	{
		output_scenenodeptr = &parentnode->AddNode();
		node = output_scenenodeptr;
	}

	output_drawableptr = &node->AddDrawable();
	//output_drawableptr->SetModel(&output_model);
	output_drawableptr->AddDrawList(output_model.GetListID());
	output_drawableptr->SetDiffuseMap(&output_texture_diffuse);
	output_drawableptr->SetObjectCenter(output_model.GetCenter());
	if (output_texture_misc1.Loaded()) output_drawableptr->SetMiscMap1(&output_texture_misc1);

	return true;
}

void CAR::CopyPhysicsResultsIntoDisplay()
{
	if (!bodynode)
		return;

	MATHVECTOR <float, 3> vec;
	vec = dynamics.GetOriginPosition();
	QUATERNION <float> quat;
	quat = dynamics.GetOrientation();

	bodynode->GetTransform().SetTranslation(vec);

	vec = dynamics.GetCenterOfMassPosition();
	roadnoise.SetPosition(vec[0],vec[1],vec[2]);
	crashsound.SetPosition(vec[0],vec[1],vec[2]);

	QUATERNION <float> fixer;
	fixer.Rotate(-3.141593*0.5, 0, 0, 1);
	bodynode->GetTransform().SetRotation(quat*fixer);

	for (int i = 0; i < 4; i++)
	{
		vec = dynamics.GetWheelPosition(WHEEL_POSITION(i));
		wheelnode[WHEEL_POSITION(i)]->GetTransform().SetTranslation(vec);
		tirebump[i].SetPosition(vec[0],vec[1],vec[2]);
		QUATERNION <float> wheelquat;
		QUATERNION <double> dblfixer;
		dblfixer.Rotate(-3.141593*0.5, 0, 0, 1);
		wheelquat = dynamics.GetWheelOrientation(WHEEL_POSITION(i),dblfixer);
		wheelnode[WHEEL_POSITION(i)]->GetTransform().SetRotation(wheelquat);
		
		if (floatingnode[i])
		{
			floatingnode[i]->GetTransform().SetTranslation(vec);
			QUATERNION <float> floatquat;
			floatquat = dynamics.GetOrientation()*dynamics.GetWheelSteeringAndSuspensionOrientation(WHEEL_POSITION(i))*dblfixer;
			floatingnode[i]->GetTransform().SetRotation(floatquat);
		}

		//update debug wheel drawing
		if (debug_wheel_draw)
		{
			for (int n = 0; n < 10; n++)
			{
				vec = dynamics.GetWheelPositionAtDisplacement(WHEEL_POSITION(i), n/9.0);
				//vec = dynamics.GetWheelPositionAtDisplacement(WHEEL_POSITION(i), 0/9.0);
				debugwheelnode[i*10+n]->GetTransform().SetTranslation(vec);
				debugwheelnode[i*10+n]->GetTransform().SetRotation(wheelquat);
			}
		}
	}
	
	//copy to collision object as well
	collisionobject.SetPosition(CarLocalToWorld(collisiondimensions.GetCenter()));
	collisionobject.SetQuaternion(GetOrientation());
}

void GetConfigfilePoints(CONFIGFILE & c, const std::string & sectionname, const std::string & paramprefix, std::vector <std::pair <double, double> > & output_points)
{
	std::list <std::string> params;
	c.GetParamList(params, sectionname);
	//std::cout << "Section name: " << sectionname << std::endl;
	//std::cout << "Params: " << params.size() << std::endl;
	for (std::list <std::string>::iterator i = params.begin(); i != params.end(); i++)
	{
		if (i->find(paramprefix) == 0)
		{
			float point[3];
			point[0] = 0;
			point[1] = 0;
			point[2] = 0;
			if (c.GetParam(sectionname+"."+*i, point))
			{
				output_points.push_back(std::make_pair(point[0], point[1]));
			}
		}
	}
}

bool CAR::LoadDynamics(CONFIGFILE & c, std::ostream & error_output)
{
	string drive = "RWD";
	int version(1);
	c.GetParam("version", version);
	//std::cout << "Version: " << version << std::endl;
	if (version > 2)
	{
		error_output << "Unsupported car version: " << version << endl;
		return false;
	}
	float temp_vec3[3];

	//load the engine
	{
		float engine_mass, engine_redline, engine_rpm_limit, engine_inertia,
			engine_start_rpm, engine_stall_rpm, engine_fuel_consumption;
		MATHVECTOR <double, 3> engine_position;

		//no longer used
		//if (!GetConfigfileParam(error_output, c, "engine.max-power", engine_max_power)) return false;

		if (!GetConfigfileParam(error_output, c, "engine.peak-engine-rpm", engine_redline)) return false; //used only for the redline graphics
		dynamics.GetEngine().SetRedline(engine_redline);

		if (!GetConfigfileParam(error_output, c, "engine.rpm-limit", engine_rpm_limit)) return false;
		dynamics.GetEngine().SetRPMLimit(engine_rpm_limit);

		if (!GetConfigfileParam(error_output, c, "engine.inertia", engine_inertia)) return false;
		dynamics.GetEngine().SetInertia(engine_inertia);

		//if (!GetConfigfileParam(error_output, c, "engine.idle", engine_idle)) return false;
		//dynamics.GetEngine().SetIdle(engine_idle);

		if (!GetConfigfileParam(error_output, c, "engine.start-rpm", engine_start_rpm)) return false;
		dynamics.GetEngine().SetStartRPM(engine_start_rpm);

		if (!GetConfigfileParam(error_output, c, "engine.stall-rpm", engine_stall_rpm)) return false;
		dynamics.GetEngine().SetStallRPM(engine_stall_rpm);

		if (!GetConfigfileParam(error_output, c, "engine.fuel-consumption", engine_fuel_consumption)) return false;
		dynamics.GetEngine().SetFuelConsumption(engine_fuel_consumption);

		if (!GetConfigfileParam(error_output, c, "engine.mass", engine_mass)) return false;
		if (!GetConfigfileParam(error_output, c, "engine.position", temp_vec3)) return false;
		if (version == 2)
			COORDINATESYSTEMS::ConvertCarCoordinateSystemV2toV1(temp_vec3[0],temp_vec3[1],temp_vec3[2]);
		engine_position.Set(temp_vec3[0],temp_vec3[1],temp_vec3[2]);
		dynamics.GetEngine().SetMass(engine_mass);
		dynamics.GetEngine().SetPosition(engine_position);
		dynamics.AddMassParticle(engine_mass, engine_position);

		float torque_point[3];
		string torque_str("engine.torque-curve-00");
		vector <pair <double, double> > torques;
		int curve_num = 0;
		while (c.GetParam(torque_str, torque_point))
		{
			torques.push_back(pair <float, float> (torque_point[0], torque_point[1]));

			curve_num++;
			stringstream str;
			str << "engine.torque-curve-";
			str.width(2);
			str.fill('0');
			str << curve_num;
			torque_str = str.str();
		}
		if (torques.size() <= 1)
		{
			error_output << "You must define at least 2 torque curve points." << endl;
			return false;
		}
		dynamics.GetEngine().SetTorqueCurve(engine_redline, torques);
	}

	//load the clutch
	{
		float sliding, radius, area, max_pressure;

		if (!GetConfigfileParam(error_output, c, "clutch.sliding", sliding)) return false;
		dynamics.GetClutch().SetSlidingFriction(sliding);

		if (!GetConfigfileParam(error_output, c, "clutch.radius", radius)) return false;
		dynamics.GetClutch().SetRadius(radius);

		if (!GetConfigfileParam(error_output, c, "clutch.area", area)) return false;
		dynamics.GetClutch().SetArea(area);

		if (!GetConfigfileParam(error_output, c, "clutch.max-pressure", max_pressure)) return false;
		dynamics.GetClutch().SetMaxPressure(max_pressure);
	}

	//load the transmission
	{
		int gears;
		float ratio;

		if (!GetConfigfileParam(error_output, c, "transmission.gear-ratio-r", ratio)) return false;
		dynamics.GetTransmission().SetGearRatio(-1, ratio);

		if (!GetConfigfileParam(error_output, c, "transmission.gears", gears)) return false;

		for (int i = 0; i < gears; i++)
		{
			stringstream s;
			s << "transmission.gear-ratio-" << i+1;
			if (!GetConfigfileParam(error_output, c, s.str(), ratio)) return false;
			dynamics.GetTransmission().SetGearRatio(i+1, ratio);
		}

		c.GetParam("transmission.shift-time", shift_time);
	}

	//load the differential(s)
	{
		float final_drive, anti_slip, anti_slip_torque(0), anti_slip_torque_deceleration_factor(0);

		if (!GetConfigfileParam(error_output, c, "differential.final-drive", final_drive)) return false;
		if (!GetConfigfileParam(error_output, c, "differential.anti-slip", anti_slip)) return false;
		c.GetParam("differential.anti-slip-torque", anti_slip_torque);
		c.GetParam("differential.anti-slip-torque-deceleration-factor", anti_slip_torque_deceleration_factor);

		string drive;
		if (!GetConfigfileParam(error_output, c, "drive", drive)) return false;
		dynamics.SetDrive(drive);

		//TODO: add support for center diff parameters

		if (drive == "RWD")
		{
			dynamics.GetRearDifferential().SetFinalDrive(final_drive);
			dynamics.GetRearDifferential().SetAntiSlip(anti_slip, anti_slip_torque, anti_slip_torque_deceleration_factor);
		}
		else if (drive == "FWD")
		{
			dynamics.GetFrontDifferential().SetFinalDrive(final_drive);
			dynamics.GetFrontDifferential().SetAntiSlip(anti_slip, anti_slip_torque, anti_slip_torque_deceleration_factor);
		}
		else if (drive == "AWD")
		{
			dynamics.GetRearDifferential().SetFinalDrive(1.0);
			dynamics.GetRearDifferential().SetAntiSlip(anti_slip, anti_slip_torque, anti_slip_torque_deceleration_factor);
			dynamics.GetFrontDifferential().SetFinalDrive(1.0);
			dynamics.GetFrontDifferential().SetAntiSlip(anti_slip, anti_slip_torque, anti_slip_torque_deceleration_factor);
			dynamics.GetCenterDifferential().SetFinalDrive(final_drive);
			dynamics.GetCenterDifferential().SetAntiSlip(anti_slip, anti_slip_torque, anti_slip_torque_deceleration_factor);
		}
		else
		{
			error_output << "Unknown drive type: " << drive << endl;
			return false;
		}
	}

	//load the brake
	{
		for (int i = 0; i < 2; i++)
		{
			string pos = "front";
			WHEEL_POSITION left = FRONT_LEFT;
			WHEEL_POSITION right = FRONT_RIGHT;
			if (i == 1)
			{
				left = REAR_LEFT;
				right = REAR_RIGHT;
				pos = "rear";
			}

			float friction, max_pressure, area, bias, radius, handbrake(0);

			if (!GetConfigfileParam(error_output, c, "brakes-"+pos+".friction", friction)) return false;
			dynamics.GetBrake(left).SetFriction(friction);
			dynamics.GetBrake(right).SetFriction(friction);

			if (!GetConfigfileParam(error_output, c, "brakes-"+pos+".area", area)) return false;
			dynamics.GetBrake(left).SetArea(area);
			dynamics.GetBrake(right).SetArea(area);

			if (!GetConfigfileParam(error_output, c, "brakes-"+pos+".radius", radius)) return false;
			dynamics.GetBrake(left).SetRadius(radius);
			dynamics.GetBrake(right).SetRadius(radius);

			c.GetParam("brakes-"+pos+".handbrake", handbrake);
			dynamics.GetBrake(left).SetHandbrake(handbrake);
			dynamics.GetBrake(right).SetHandbrake(handbrake);

			if (!GetConfigfileParam(error_output, c, "brakes-"+pos+".bias", bias)) return false;
			//if (i == 1) bias = 1.0 - bias;
			dynamics.GetBrake(left).SetBias(bias);
			dynamics.GetBrake(right).SetBias(bias);

			if (!GetConfigfileParam(error_output, c, "brakes-"+pos+".max-pressure", max_pressure)) return false;
			dynamics.GetBrake(left).SetMaxPressure(max_pressure*bias);
			dynamics.GetBrake(right).SetMaxPressure(max_pressure*bias);
		}
	}

	//load the fuel tank
	{
		float pos[3];
		MATHVECTOR <double, 3> position;
		float capacity;
		float volume;
		float fuel_density;

		if (!GetConfigfileParam(error_output, c, "fuel-tank.capacity", capacity)) return false;
		dynamics.GetFuelTank().SetCapacity(capacity);

		if (!GetConfigfileParam(error_output, c, "fuel-tank.volume", volume)) return false;
		dynamics.GetFuelTank().SetVolume(volume);

		if (!GetConfigfileParam(error_output, c, "fuel-tank.fuel-density", fuel_density)) return false;
		dynamics.GetFuelTank().SetDensity(fuel_density);

		if (!GetConfigfileParam(error_output, c, "fuel-tank.position", pos)) return false;
		if (version == 2)
			COORDINATESYSTEMS::ConvertCarCoordinateSystemV2toV1(pos[0],pos[1],pos[2]);
		position.Set(pos[0],pos[1],pos[2]);
		dynamics.GetFuelTank().SetPosition(position);
		//dynamics.AddMassParticle(fuel_density*volume, position);
	}

	//load the suspension
	{
		for (int i = 0; i < 2; i++)
		{
			string posstr = "front";
			string posshortstr = "F";
			WHEEL_POSITION posl = FRONT_LEFT;
			WHEEL_POSITION posr = FRONT_RIGHT;
			if (i == 1)
			{
				posstr = "rear";
				posshortstr = "R";
				posl = REAR_LEFT;
				posr = REAR_RIGHT;
			}

			float spring_constant, bounce, rebound, travel, camber, caster, toe, anti_roll, maxcompvel;
			float hinge[3];
			MATHVECTOR <double, 3> tempvec;

			if (!GetConfigfileParam(error_output, c, "suspension-"+posstr+".spring-constant", spring_constant)) return false;
			dynamics.GetSuspension(posl).SetSpringConstant(spring_constant);
			dynamics.GetSuspension(posr).SetSpringConstant(spring_constant);

			if (!GetConfigfileParam(error_output, c, "suspension-"+posstr+".bounce", bounce)) return false;
			dynamics.GetSuspension(posl).SetBounce(bounce);
			dynamics.GetSuspension(posr).SetBounce(bounce);

			if (!GetConfigfileParam(error_output, c, "suspension-"+posstr+".rebound", rebound)) return false;
			dynamics.GetSuspension(posl).SetRebound(rebound);
			dynamics.GetSuspension(posr).SetRebound(rebound);
			
			std::vector <std::pair <double, double> > damper_factor_points;
			GetConfigfilePoints(c, "suspension-"+posstr, "damper-factor", damper_factor_points);
			dynamics.GetSuspension(posl).SetDamperFactorPoints(damper_factor_points);
			dynamics.GetSuspension(posr).SetDamperFactorPoints(damper_factor_points);
			
			std::vector <std::pair <double, double> > spring_factor_points;
			GetConfigfilePoints(c, "suspension-"+posstr, "spring-factor", spring_factor_points);
			dynamics.GetSuspension(posl).SetSpringFactorPoints(spring_factor_points);
			dynamics.GetSuspension(posr).SetSpringFactorPoints(spring_factor_points);

			if (!GetConfigfileParam(error_output, c, "suspension-"+posstr+".max-compression-velocity", maxcompvel)) return false;
			dynamics.GetSuspension(posl).SetMaxCompressionVelocity(maxcompvel);
			dynamics.GetSuspension(posr).SetMaxCompressionVelocity(maxcompvel);

			if (!GetConfigfileParam(error_output, c, "suspension-"+posstr+".travel", travel)) return false;
			dynamics.GetSuspension(posl).SetTravel(travel);
			dynamics.GetSuspension(posr).SetTravel(travel);

			if (!GetConfigfileParam(error_output, c, "suspension-"+posstr+".camber", camber)) return false;
			dynamics.GetSuspension(posl).SetCamber(camber);
			dynamics.GetSuspension(posr).SetCamber(camber);

			if (!GetConfigfileParam(error_output, c, "suspension-"+posstr+".caster", caster)) return false;
			dynamics.GetSuspension(posl).SetCaster(caster);
			dynamics.GetSuspension(posr).SetCaster(caster);

			if (!GetConfigfileParam(error_output, c, "suspension-"+posstr+".toe", toe)) return false;
			dynamics.GetSuspension(posl).SetToe(toe);
			dynamics.GetSuspension(posr).SetToe(toe);

			if (!GetConfigfileParam(error_output, c, "suspension-"+posstr+".anti-roll", anti_roll)) return false;
			dynamics.GetSuspension(posl).SetAntiRollK(anti_roll);
			dynamics.GetSuspension(posr).SetAntiRollK(anti_roll);

			/*if (!GetConfigfileParam(error_output, c, "suspension-"+posshortstr+"L.position", position)) return false;
			if (version == 2)
				COORDINATESYSTEMS::ConvertCarCoordinateSystemV2toV1(position[0],position[1],position[2]);
			tempvec.Set(position[0],position[1], position[2]);
			dynamics.GetSuspension(posl).SetPosition(tempvec);

			if (!GetConfigfileParam(error_output, c, "suspension-"+posshortstr+"R.position", position)) return false;
			if (version == 2)
				COORDINATESYSTEMS::ConvertCarCoordinateSystemV2toV1(position[0],position[1],position[2]);
			tempvec.Set(position[0],position[1], position[2]);
			dynamics.GetSuspension(posr).SetPosition(tempvec);*/

			if (!GetConfigfileParam(error_output, c, "suspension-"+posshortstr+"L.hinge", hinge)) return false;
			//cap hinge to reasonable values
			for (int i = 0; i < 3; i++)
			{
				if (hinge[i] < -100)
					hinge[i] = -100;
				if (hinge[i] > 100)
					hinge[i] = 100;
			}
			if (version == 2)
				COORDINATESYSTEMS::ConvertCarCoordinateSystemV2toV1(hinge[0],hinge[1],hinge[2]);
			tempvec.Set(hinge[0],hinge[1], hinge[2]);
			dynamics.GetSuspension(posl).SetHinge(tempvec);

			if (!GetConfigfileParam(error_output, c, "suspension-"+posshortstr+"R.hinge", hinge)) return false;
			for (int i = 0; i < 3; i++)
			{
				if (hinge[i] < -100)
					hinge[i] = -100;
				if (hinge[i] > 100)
					hinge[i] = 100;
			}
			if (version == 2)
				COORDINATESYSTEMS::ConvertCarCoordinateSystemV2toV1(hinge[0],hinge[1],hinge[2]);
			tempvec.Set(hinge[0],hinge[1], hinge[2]);
			dynamics.GetSuspension(posr).SetHinge(tempvec);
		}
	}

	//load the wheels
	{
		for (int i = 0; i < 4; i++)
		{
			string posstr;
			WHEEL_POSITION pos;
			if (i == 0)
			{
				posstr = "FL";
				pos = FRONT_LEFT;
			}
			else if (i == 1)
			{
				posstr = "FR";
				pos = FRONT_RIGHT;
			}
			else if (i == 2)
			{
				posstr = "RL";
				pos = REAR_LEFT;
			}
			else
			{
				posstr = "RR";
				pos = REAR_RIGHT;
			}

			float roll_height, mass;
			float position[3];
			MATHVECTOR <double, 3> tempvec;

			if (!GetConfigfileParam(error_output, c, "wheel-"+posstr+".mass", mass)) return false;
			dynamics.GetWheel(pos).SetMass(mass);

			if (!GetConfigfileParam(error_output, c, "wheel-"+posstr+".roll-height", roll_height)) return false;
			dynamics.GetWheel(pos).SetRollHeight(roll_height);

			if (!GetConfigfileParam(error_output, c, "wheel-"+posstr+".position", position)) return false;
			if (version == 2)
				COORDINATESYSTEMS::ConvertCarCoordinateSystemV2toV1(position[0],position[1],position[2]);
			tempvec.Set(position[0],position[1], position[2]);
			dynamics.GetWheel(pos).SetExtendedPosition(tempvec);

			dynamics.AddMassParticle(mass, tempvec);
		}

		//load the rotational inertia parameter from the tire section
		float front_inertia;
		float rear_inertia;
		if (!GetConfigfileParam(error_output, c, "tire-front.rotational-inertia", front_inertia)) return false;
		dynamics.GetWheel(FRONT_LEFT).SetInertia(front_inertia);
		dynamics.GetWheel(FRONT_RIGHT).SetInertia(front_inertia);

		if (!GetConfigfileParam(error_output, c, "tire-rear.rotational-inertia", rear_inertia)) return false;
		dynamics.GetWheel(REAR_LEFT).SetInertia(rear_inertia);
		dynamics.GetWheel(REAR_RIGHT).SetInertia(rear_inertia);
	}

	//load the tire parameters
	{
		WHEEL_POSITION leftside = FRONT_LEFT;
		WHEEL_POSITION rightside = FRONT_RIGHT;
		string posstr = "front";

		for (int p = 0; p < 2; p++)
		{
			if (p == 1)
			{
				leftside = REAR_LEFT;
				rightside = REAR_RIGHT;
				posstr = "rear";
			}

			vector <double> longitudinal;
			vector <double> lateral;
			vector <double> aligning;
			longitudinal.resize(11);
			lateral.resize(15);
			aligning.resize(18);

			//read lateral
			int numinfile;
			for (int i = 0; i < 15; i++)
			{
				numinfile = i;
				if (i == 11)
					numinfile = 111;
				else if (i == 12)
					numinfile = 112;
				else if (i > 12)
					numinfile -= 1;
				stringstream str;
				str << "tire-"+posstr+".a" << numinfile;
				float value;
				if (!GetConfigfileParam(error_output, c, str.str(), value)) return false;
				lateral[i] = value;
			}

			//read longitudinal
			for (int i = 0; i < 11; i++)
			{
				stringstream str;
				str << "tire-"+posstr+".b" << i;
				float value;
				if (!GetConfigfileParam(error_output, c, str.str(), value)) return false;
				longitudinal[i] = value;
			}

			//read aligning
			for (int i = 0; i < 18; i++)
			{
				stringstream str;
				str << "tire-"+posstr+".c" << i;
				float value;
				if (!GetConfigfileParam(error_output, c, str.str(), value)) return false;
				aligning[i] = value;
			}

			dynamics.GetTire(leftside).SetPacejkaParameters(longitudinal, lateral, aligning);
			dynamics.GetTire(rightside).SetPacejkaParameters(longitudinal, lateral, aligning);

			float rolling_resistance[3];
			if (!GetConfigfileParam(error_output, c, "tire-"+posstr+".rolling-resistance", rolling_resistance)) return false;
			dynamics.GetTire(leftside).SetRollingResistance(rolling_resistance[0], rolling_resistance[1]);
			dynamics.GetTire(rightside).SetRollingResistance(rolling_resistance[0], rolling_resistance[1]);

			float tread;
			float radius;
			if (!GetConfigfileParam(error_output, c, "tire-"+posstr+".radius", radius)) return false;
			dynamics.GetTire(leftside).SetRadius(radius);
			dynamics.GetTire(rightside).SetRadius(radius);
			if (!GetConfigfileParam(error_output, c, "tire-"+posstr+".tread", tread)) return false;
			dynamics.GetTire(leftside).SetTread(tread);
			dynamics.GetTire(rightside).SetTread(tread);
		}

		for (int i = 0; i < 4; i++)
			dynamics.GetTire(WHEEL_POSITION(i)).CalculateSigmaHatAlphaHat();
	}

	//load the mass-only particles
	{
		MATHVECTOR <double, 3> position;
		float pos[3];
		float mass;

		if (c.GetParam("contact-points.mass", mass))
		{
			int paramnum(0);
			string paramname("contact-points.position-00");
			stringstream output_supression;
			while (GetConfigfileParam(output_supression, c, paramname, pos))
			{
				if (version == 2)
					COORDINATESYSTEMS::ConvertCarCoordinateSystemV2toV1(pos[0],pos[1],pos[2]);
				position.Set(pos[0],pos[1],pos[2]);
				dynamics.AddMassParticle(mass, position);
				paramnum++;
				stringstream str;
				str << "contact-points.position-";
				str.width(2);
				str.fill('0');
				str << paramnum;
				paramname = str.str();
			}
		}

		string paramname = "particle-00";
		int paramnum = 0;
		stringstream output_supression;
		while (GetConfigfileParam(output_supression, c, paramname+".mass", mass))
		{
			if (!GetConfigfileParam(error_output, c, paramname+".position", pos)) return false;
			if (version == 2)
				COORDINATESYSTEMS::ConvertCarCoordinateSystemV2toV1(pos[0],pos[1],pos[2]);
			position.Set(pos[0],pos[1],pos[2]);
			dynamics.AddMassParticle(mass, position);
			paramnum++;
			stringstream str;
			str << "particle-";
			str.width(2);
			str.fill('0');
			str << paramnum;
			paramname = str.str();
		}
	}
	
	//load the max steering angle
	{
		float maxangle = 45.0;
		if ( !GetConfigfileParam ( error_output, c, "steering.max-angle", maxangle ) )
			return false;
	
		dynamics.SetMaxSteeringAngle ( maxangle );
	}

	//load the driver
	{
		float mass;
		float pos[3], hoodpos[3];
		MATHVECTOR <double, 3> position;

		if (!GetConfigfileParam(error_output, c, "driver.mass", mass)) return false;
		if (!GetConfigfileParam(error_output, c, "driver.position", pos)) return false;
		if (drivernode) //move the driver model to the coordinates given
		{
			MATHVECTOR <float, 3> floatpos;
			floatpos.Set(pos[0], pos[1], pos[2]);
			drivernode->GetTransform().SetTranslation(floatpos);
		}
		if (version == 2)
			COORDINATESYSTEMS::ConvertCarCoordinateSystemV2toV1(pos[0],pos[1],pos[2]);
		position.Set(pos[0], pos[1], pos[2]);
		dynamics.AddMassParticle(mass, position);

		if (!GetConfigfileParam(error_output, c, "driver.view-position", pos)) return false;
		if (version == 2)
			COORDINATESYSTEMS::ConvertCarCoordinateSystemV2toV1(pos[0],pos[1],pos[2]);
		view_position.Set(pos);

		if (!GetConfigfileParam(error_output, c, "driver.hood-mounted-view-position", hoodpos))
		{
			pos[1] = 0;
			pos[0] += 1.0;
			hood_position.Set(pos);
		}
		else
		{
			if (version == 2)
				COORDINATESYSTEMS::ConvertCarCoordinateSystemV2toV1(hoodpos[0],hoodpos[1],hoodpos[2]);
			hood_position.Set(hoodpos);
		}

		float view_stiffness = 0.0;
		if (!c.GetParam("driver.view-stiffness", view_stiffness))
			view_stiffness = 0.0;
		driver_cam.SetStiffness(view_stiffness);
		hood_cam.SetStiffness(view_stiffness);
	}

	//load the aerodynamics
	{
		float drag_area, drag_c, lift_area, lift_c, lift_eff;
		float pos[3];
		MATHVECTOR <double, 3> position;

		if (!GetConfigfileParam(error_output, c, "drag.frontal-area", drag_area)) return false;
		if (!GetConfigfileParam(error_output, c, "drag.drag-coefficient", drag_c)) return false;
		if (!GetConfigfileParam(error_output, c, "drag.position", pos)) return false;
		if (version == 2)
			COORDINATESYSTEMS::ConvertCarCoordinateSystemV2toV1(pos[0],pos[1],pos[2]);
		position.Set(pos[0], pos[1], pos[2]);
		dynamics.AddAerodynamicDevice(position, drag_area, drag_c, 0,0,0);

		for (int i = 0; i < 2; i++)
		{
			string wingpos = "front";
			if (i == 1)
				wingpos = "rear";
			if (!GetConfigfileParam(error_output, c, "wing-"+wingpos+".frontal-area", drag_area)) return false;
			if (!GetConfigfileParam(error_output, c, "wing-"+wingpos+".drag-coefficient", drag_c)) return false;
			if (!GetConfigfileParam(error_output, c, "wing-"+wingpos+".surface-area", lift_area)) return false;
			if (!GetConfigfileParam(error_output, c, "wing-"+wingpos+".lift-coefficient", lift_c)) return false;
			if (!GetConfigfileParam(error_output, c, "wing-"+wingpos+".efficiency", lift_eff)) return false;
			if (!GetConfigfileParam(error_output, c, "wing-"+wingpos+".position", pos)) return false;
			if (version == 2)
				COORDINATESYSTEMS::ConvertCarCoordinateSystemV2toV1(pos[0],pos[1],pos[2]);
			position.Set(pos[0], pos[1], pos[2]);
			dynamics.AddAerodynamicDevice(position, drag_area, drag_c, lift_area, lift_c, lift_eff);
		}
	}

	dynamics.UpdateMass();
	
	mz_nominalmax = (GetTireMaxMz(FRONT_LEFT) + GetTireMaxMz(FRONT_RIGHT))*0.5;

	return true;
}

void CAR::HandleInputs(const std::vector <float> & inputs, float dt)
{
	assert(inputs.size() == CARINPUT::INVALID); //this looks weird, but it ensures that our inputs vector contains exactly one item per input

	//std::cout << "Throttle: " << inputs[CARINPUT::THROTTLE] << std::endl;
	//std::cout << "Shift up: " << inputs[CARINPUT::SHIFT_UP] << std::endl;

	//set the brakes
	for (int i = 0; i < 4; i++)
		dynamics.GetBrake(WHEEL_POSITION(i)).SetBrakeFactor(inputs[CARINPUT::BRAKE]);

	//set the handbrakes
	for (int i = 0; i < 4; i++)
		dynamics.GetBrake(WHEEL_POSITION(i)).SetHandbrakeFactor(inputs[CARINPUT::HANDBRAKE]);

	//do steering
	float steer_value = inputs[CARINPUT::STEER_RIGHT];
	if (std::abs(inputs[CARINPUT::STEER_LEFT]) > std::abs(inputs[CARINPUT::STEER_RIGHT])) //use whichever control is larger
		steer_value = -inputs[CARINPUT::STEER_LEFT];
	dynamics.SetSteering(steer_value);
	last_steer = steer_value;

    //start the engine if requested
	if (inputs[CARINPUT::START_ENGINE])
		dynamics.GetEngine().StartEngine();

	//do shifting
	int gear_change = 0;
	if (autoshift_enabled)
		gear_change = AutoShift();
	if (inputs[CARINPUT::SHIFT_UP] == 1.0)
		gear_change = 1;
	if (inputs[CARINPUT::SHIFT_DOWN] == 1.0)
		gear_change = -1;
	int cur_gear = dynamics.GetTransmission().GetGear();
	int new_gear = cur_gear + gear_change;

	//if (gear_change != 0) std::cout << "Gear change to: " << new_gear << std::endl;

	//check for dedicated gear controls
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

	if (cur_gear != new_gear && shifted)
	{
		if (new_gear <= dynamics.GetTransmission().GetForwardGears() && new_gear >= -dynamics.GetTransmission().GetReverseGears())
		{
			ShiftGears(new_gear, (new_gear == 0)); //immediately shift into neutral
		}
	}

	if (remaining_shift_time <= shift_time * 0.5 && !shifted)
	{
		shifted = true;
		dynamics.GetTransmission().Shift(shift_gear);
	}

	float throttle = inputs[CARINPUT::THROTTLE];
	float clutch = inputs[CARINPUT::CLUTCH];
	if (auto_clutch)
	{
		if (!dynamics.GetEngine().GetCombustion())
		{
		    dynamics.GetEngine().StartEngine();
		    cout << "clutch: " << clutch << "gear: " << cur_gear << endl;
		}
		throttle = ShiftAutoClutchThrottle(throttle, dt);
		clutch = AutoClutch(last_auto_clutch, dt);
		last_auto_clutch = clutch;
	}
	dynamics.GetEngine().SetThrottle(throttle);
	dynamics.GetClutch().SetClutch(clutch);

	//do camera inputs
	orbit_cam.RotateDown(-inputs[CARINPUT::PAN_UP]*dt);
	orbit_cam.RotateDown(inputs[CARINPUT::PAN_DOWN]*dt);
	orbit_cam.RotateRight(-inputs[CARINPUT::PAN_LEFT]*dt);
	orbit_cam.RotateRight(inputs[CARINPUT::PAN_RIGHT]*dt);
	orbit_cam.ZoomIn(inputs[CARINPUT::ZOOM_IN]*dt*4.0);
	orbit_cam.ZoomIn(-inputs[CARINPUT::ZOOM_OUT]*dt*4.0);

	// update brake/reverse lights
	if (bodydraw)
	{
		if(inputs[CARINPUT::BRAKE] > 0 && braketexture.Loaded())
			bodydraw->SetAdditiveMap1(&braketexture);
		else
			bodydraw->SetAdditiveMap1(NULL);
		if (cur_gear < 0 && reversetexture.Loaded())
			bodydraw->SetAdditiveMap2(&reversetexture);
		else
			bodydraw->SetAdditiveMap2(NULL);
		bodydraw->SetSelfIllumination(true);
	}

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
}

float CAR::AutoClutch(float last_clutch, float dt) const
{
	const float threshold = 1000.0;
	const float margin = 100.0;//100.0;
	const float geareffect = 1.0; //zero to 1, defines special consideration of first/reverse gear

	float rpm = dynamics.GetEngine().GetRPM();
	float driveshaft_rpm = dynamics.CalculateDriveshaftRPM();
	driveshaft_rpm = rpm;

	//take into account locked brakes
	bool protect_against_brake_lockup = true;
	bool braking(true);
	if (protect_against_brake_lockup)
	{
		for (int i = 0; i < 4; i++)
		{
			if (dynamics.WheelDriven(WHEEL_POSITION(i)))
			{
                braking = braking && dynamics.GetBrake(WHEEL_POSITION(i)).WillLock();
			}
		}
		if (braking)
            return 0; 
	}

	//use driveshaft_rpm if it's lower
	if (driveshaft_rpm < rpm)
		rpm = driveshaft_rpm;

	const float maxrpm = dynamics.GetEngine().GetRPMLimit();
	const float stallrpm = dynamics.GetEngine().GetStallRPM()+margin*(maxrpm/2000.0);
	const int gear = dynamics.GetTransmission().GetGear();

	float gearfactor = 1.0;
	if (gear <= 1)
		gearfactor = 2.0;
	float thresh = threshold * (maxrpm/7000.0) * ((1.0-geareffect)+gearfactor*geareffect) + stallrpm;
	if (dynamics.GetClutch().IsLocked())
		thresh *= 0.5;
	float clutch = (rpm-stallrpm) / (thresh-stallrpm);

	//std::cout << rpm << ", " << stallrpm << ", " << threshold << ", " << clutch << std::endl;

	if (clutch < 0)
		clutch = 0;
	if (clutch > 1.0)
		clutch = 1.0;

	float newauto = clutch * ShiftAutoClutch();

	//rate limit the autoclutch
	const float min_engage_time = 0.05; //the fastest time in seconds for auto-clutch engagement
	const float engage_rate_limit = 1.0/min_engage_time;
	const float rate = (last_clutch - newauto)/dt; //engagement rate in clutch units per second
	if (rate > engage_rate_limit)
		newauto = last_clutch - engage_rate_limit*dt;
    
    return newauto;
}

float CAR::ShiftAutoClutch() const
{
	float shift_clutch = 1.0;
	if (remaining_shift_time > shift_time * 0.5)
	    shift_clutch = 0.0;
	else if (remaining_shift_time > 0.0)
	    shift_clutch = 1.0 - remaining_shift_time / (shift_time * 0.5);
	return shift_clutch;
}

float CAR::ShiftAutoClutchThrottle(float throttle, float dt)
{
	if(remaining_shift_time > 0.0)
	{
	    const float erpm = dynamics.GetEngine().GetRPM();
	    const float drpm = dynamics.CalculateDriveshaftRPM();
	    const float redl =  dynamics.GetEngine().GetRedline();
	    if(erpm < drpm && erpm < redl)
	    {
	        remaining_shift_time += dt;
            return 1.0;
	    }
	    else
	    {
	        return 0.5 * throttle;
	    }
	}
	return throttle;
}

///return the gear change (0 for no change, -1 for shift down, 1 for shift up)
int CAR::AutoShift() const
{
	// only autoshift if a shift is not in progress
	if (shifted)
	{
        if (GetClutch() == 1.0)
        {
            int current_gear = dynamics.GetTransmission().GetGear();
            float rpm = dynamics.CalculateDriveshaftRPM();
            float redline = GetEngineRedline();

            // shift up when driveshaft speed exceeds engine redline
            // we do not shift up from neutral/reverse
            if (rpm > redline && current_gear > 0)
            {
                return 1;
            }
            // shift down when driveshaft speed below shift_down_point
            // we do not auto shift down from 1st gear to neutral
            float drpm = DownshiftPoint(current_gear);
            if(rpm < drpm)
            {
                return -1; 
            }
        }
    }
	return 0;
}

float CAR::DownshiftPoint(int gear) const
{
	float shift_down_point = 0.0;
	if (gear > 1)
	{
        float current_gear_ratio = dynamics.GetTransmission().GetGearRatio(gear);
        float lower_gear_ratio = dynamics.GetTransmission().GetGearRatio(gear - 1);
		float peak_engine_speed = GetEngineRedline();
		shift_down_point = 0.75 * peak_engine_speed / lower_gear_ratio * current_gear_ratio;
	}
	return shift_down_point;
}

void CAR::UpdateSounds(float dt)
{
	//update engine sounds
	float rpm = GetEngineRPM();
	float throttle = dynamics.GetEngine().GetThrottle();

	const MATHVECTOR <double, 3> & engine_pos = dynamics.GetEnginePosition();

	float total_gain = 0.0;
	std::list <std::pair <SOUNDSOURCE *, float> > gainlist;

	float loudest = 0.0; //for debugging

	for (std::list <std::pair <ENGINESOUNDINFO, SOUNDSOURCE> >::iterator i = enginesounds.begin(); i != enginesounds.end(); ++i)
	{
		ENGINESOUNDINFO & info = i->first;
		SOUNDSOURCE & sound = i->second;

		float gain = 1.0;

		if (rpm < info.minrpm)
			gain = 0;
		else if (rpm < info.fullgainrpmstart && info.fullgainrpmstart > info.minrpm)
			gain *= (rpm - info.minrpm)/(info.fullgainrpmstart-info.minrpm);

		if (rpm > info.maxrpm)
			gain = 0;
		else if (rpm > info.fullgainrpmend && info.fullgainrpmend < info.maxrpm)
			gain *= 1.0-(rpm - info.fullgainrpmend)/(info.maxrpm-info.fullgainrpmend);

		if (info.power == ENGINESOUNDINFO::BOTH)
			gain *= throttle * 0.5 + 0.5;
		else if (info.power == ENGINESOUNDINFO::POWERON)
			gain *= throttle;
		else if (info.power == ENGINESOUNDINFO::POWEROFF)
			gain *= (1.0-throttle);

		total_gain += gain;
		if (gain > loudest)
			loudest = gain;
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
			i->first->SetGain(0.0);
		else if (enginesounds.size() == 1 && enginesounds.back().first.power == ENGINESOUNDINFO::BOTH)
			i->first->SetGain(i->second);
		else
			i->first->SetGain(i->second/total_gain);

		//if (i->second == loudest) std::cout << i->first->GetSoundBuffer().GetName() << ": " << i->second << std::endl;
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
		if (dynamics.GetWheelContactProperties(WHEEL_POSITION(i)).GetSurface() == SURFACE::ASPHALT)
		{
			thesound = tiresqueal;
		}
		else if (dynamics.GetWheelContactProperties(WHEEL_POSITION(i)).GetSurface() == SURFACE::GRASS)
		{
			thesound = grasssound;
			maxgain = 0.4; // up the grass sound volume a little
		}
		else if (dynamics.GetWheelContactProperties(WHEEL_POSITION(i)).GetSurface() == SURFACE::GRAVEL)
		{
			thesound = gravelsound;
			maxgain = 0.4;
		}
		else if (dynamics.GetWheelContactProperties(WHEEL_POSITION(i)).GetSurface() == SURFACE::CONCRETE)
		{
			thesound = tiresqueal;
			maxgain = 0.3;
			pitchvariation = 0.25;
		}
		else if (dynamics.GetWheelContactProperties(WHEEL_POSITION(i)).GetSurface() == SURFACE::SAND)
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
		MATHVECTOR <float, 3> vec;
		vec = dynamics.GetWheelPosition(WHEEL_POSITION(i));
		thesound[i].SetPosition(vec[0], vec[1], vec[2]);

		MATHVECTOR <float, 3> groundvel;
		groundvel = dynamics.GetWheelVelocity(WHEEL_POSITION(i));
		thesound[i].SetGain(squeal*maxgain);
		float pitch = (groundvel.Magnitude()-5.0)*0.1;
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
		MATHVECTOR <float, 3> vel;
		vel = dynamics.GetBody().GetVelocity();
		float gain = vel.Magnitude();
		if (gain < 0)
			gain = -gain;
		gain *= 0.02;
		gain *= gain;
		if (gain > 1.0)	gain = 1.0;
		roadnoise.SetGain(gain);
		//std::cout << gain << endl;
	}
	
	//update bump noise sound
	{
		//std::cout << GetSpeed() << std::endl;
		for (int i = 0; i < 4; i++)
		{
			suspensionbumpdetection[i].Update(dynamics.GetSuspension(WHEEL_POSITION(i)).GetVelocity(),
											  dynamics.GetSuspension(WHEEL_POSITION(i)).GetDisplacementPercent(), dt);
			if (suspensionbumpdetection[i].JustSettled())
			{
				float bumpsize = suspensionbumpdetection[i].GetTotalBumpSize();
				
				const float breakevenms = 5.0;
				float gain = bumpsize * GetSpeed() / breakevenms;
				if (gain > 1)
					gain = 1;
				if (gain < 0)
					gain = 0;
				
				//std::cout << i << ". bump: " << suspensionbumpdetection[i].GetTotalBumpSize() << ", " << gain << std::endl;
				
				if (gain > 0 && !tirebump[i].Audible())
				{
					tirebump[i].SetGain(gain);
					tirebump[i].Stop();
					tirebump[i].Play();
				}
			}
			/*else if (i == 1)
			{
				std::cout << i << ". state: " << suspensionbumpdetection[i].state << ", vel: " << dynamics.GetSuspension(WHEEL_POSITION(i)).GetVelocity() << std::endl;
			}*/
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
}

void CAR::UpdateCameras(float dt)
{
	MATHVECTOR <double, 3> doublevec;
	doublevec = view_position;
	MATHVECTOR <float, 3> floatvec;
	floatvec = dynamics.CarLocalToWorld(doublevec);
	QUATERNION <float> floatq;
	floatq = dynamics.GetBody().GetOrientation();
	MATHVECTOR <float, 3> accel;
	accel = dynamics.GetLastBodyForce()/dynamics.GetBody().GetMass();
	
	// reverse the camera direction
	if (lookbehind)
	{
		floatq.Rotate(3.141593,0,0,1);
		
		MATHVECTOR<float, 3> driver_reverse_offset(0.2,0,0);
		floatq.RotateVector(driver_reverse_offset);
		driver_cam.Set(floatvec+driver_reverse_offset, floatq, accel, dt);
		doublevec = hood_position;
		floatvec = dynamics.CarLocalToWorld(doublevec);
		hood_cam.Set(floatvec+MATHVECTOR<float, 3>(0,0,0.15), floatq, accel, dt);
		rigidchase_cam.Reset(GetCenterOfMassPosition(), floatq);
		chase_cam.Set(GetCenterOfMassPosition()+MATHVECTOR<float, 3>(0,0,0.5), floatq, dt, lookbehind);
	}
	else
	{
		//driver_cam.Reset(floatvec, floatq);
		driver_cam.Set(floatvec, floatq, accel, dt);
		doublevec = hood_position;
		floatvec = dynamics.CarLocalToWorld(doublevec);
		//hood_cam.Set(floatvec, floatq);
		hood_cam.Set(floatvec, floatq, accel, dt);
		rigidchase_cam.Reset(GetCenterOfMassPosition(), floatq);
		chase_cam.Set(GetCenterOfMassPosition()+MATHVECTOR<float, 3>(0,0,0.5), floatq, dt, lookbehind);
	}
	orbit_cam.SetFocus(GetCenterOfMassPosition());
}

CAR::SUSPENSIONBUMPDETECTION::SUSPENSIONBUMPDETECTION() : state(SETTLED), laststate(SETTLED),
						displacetime(0.01), displacevelocitythreshold(0.5),
						settletime(0.01), settlevelocitythreshold(0.0),
						displacetimer(0), settletimer(0),
						dpstart(0), dpend(0)
{
}

CAR::CRASHDETECTION::CRASHDETECTION() : lastvel(0), curmaxdecel(0), maxdecel(0), deceltrigger(200)
{
}

float CAR::GetFeedback()
{
	//return 0.5*(dynamics.GetTire(FRONT_LEFT).GetFeedback()+dynamics.GetTire(FRONT_RIGHT).GetFeedback());
	assert(!feedbackbuffer.empty());
	float feedback = 0.0;
	for (std::vector <float>::iterator i = feedbackbuffer.begin(); i != feedbackbuffer.end(); ++i)
		feedback += *i;
	feedback /= feedbackbuffer.size();
	//std::cout << feedback << ", " << mz_nominalmax << ", " << feedback/(mz_nominalmax*0.1) << std::endl;
	return feedback/(mz_nominalmax*0.025);
}

void CAR::GenerateCollisionData()
{
	collisiondimensions = bodymodel.GetAABB();
	MATHVECTOR <float, 3> minbody = collisiondimensions.GetPos();
	float bodyminheight = minbody[2];
	bodyminheight += collisiondimensions.GetSize()[2]*0.2;
	
	//collisiondimensions.DebugPrint(std::cout);
	//collisiondimensions.DebugPrint2(std::cout);
	
	for (int i = 0; i < 4; i++)
	{
		MATHVECTOR <float, 3> wheelpos;
		wheelpos = dynamics.GetWheelPositionAtDisplacement(WHEEL_POSITION(i),0);
		QUATERNION <float> fixer;
		fixer.Rotate(3.141593*0.5, 0, 0, 1);
		fixer.RotateVector(wheelpos);
		
		MODEL_JOE03 * curwheelmodel = &wheelmodelfront;
		if (i > 1)
			curwheelmodel = &wheelmodelrear;
		AABB <float> wheelaabb;
		float sidefactor = 1.0;
		if (i == 1 || i == 3)
			sidefactor = -1.0;
		wheelaabb.SetFromCorners(wheelpos-curwheelmodel->GetAABB().GetSize()*0.5*sidefactor,
								  wheelpos+curwheelmodel->GetAABB().GetSize()*0.5*sidefactor);
		collisiondimensions.CombineWith(wheelaabb);
		//std::cout << i << ". ";wheelaabb.DebugPrint2(std::cout);
	}
	
	//collisiondimensions.DebugPrint(std::cout);
	
	//correct for minimum body height
	MATHVECTOR <float, 3> mincorner = collisiondimensions.GetPos();
	MATHVECTOR <float, 3> maxcorner = collisiondimensions.GetPos() + collisiondimensions.GetSize();
	if (mincorner[2] < bodyminheight)
		mincorner[2] = bodyminheight;
	
	//std::cout << "minbody2: " << minbody[2] << ", bodyminheight: " << bodyminheight << std::endl;
	//std::cout << mincorner[2] << std::endl;
	
	//fix coordinate system
	QUATERNION <float> fixer;
	fixer.Rotate(-3.141593*0.5, 0, 0, 1);
	fixer.RotateVector(mincorner);
	fixer.RotateVector(maxcorner);
	
	collisiondimensions.SetFromCorners(mincorner, maxcorner);
	//collisiondimensions.DebugPrint(std::cout);
	
	COLLISION_OBJECT_SETTINGS colsettings;
	colsettings.SetDynamicObject();
	colsettings.SetObjectID(this);
	collisionobject.InitBox(collisiondimensions.GetSize()*0.5, colsettings);
}

float CAR::GetTireSquealAmount(WHEEL_POSITION i) const
{
	if (dynamics.GetWheelContactProperties(WHEEL_POSITION(i)).GetSurface() == SURFACE::NONE)
		return 0;
		
	MATHVECTOR <float, 3> groundvel;
	groundvel = dynamics.GetWheelVelocity(WHEEL_POSITION(i));
	QUATERNION <float> wheelspace;
	wheelspace = dynamics.GetBody().GetOrientation() * dynamics.GetWheelSteeringAndSuspensionOrientation(WHEEL_POSITION(i));
	(-wheelspace).RotateVector(groundvel);
	float wheelspeed = dynamics.GetWheel(WHEEL_POSITION(i)).GetAngularVelocity()*dynamics.GetTire(WHEEL_POSITION(i)).GetRadius();
	groundvel[0] -= wheelspeed;
	groundvel[1] *= 2.0;
	groundvel[2] = 0;
	float squeal = (groundvel.Magnitude()-3.0)*0.2;
	
	std::pair <double, double> slideslip = dynamics.GetTire(i).GetSlideSlipRatios();
	double maxratio = std::max(std::abs(slideslip.first), std::abs(slideslip.second));
	float squealfactor = std::max(0.0,maxratio - 1.0);
	squeal *= squealfactor;
	if (squeal < 0)
		squeal = 0;
	if (squeal > 1)
		squeal = 1;
		
	//std::cout << "origsqueal: " << (groundvel.Magnitude()-5.0)*0.2 << ", squealfactor: " << maxratio << ", squeal: " << squeal << std::endl;

	return squeal;
}
