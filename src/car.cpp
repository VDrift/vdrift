#include "car.h"

#include <fstream>
#include <map>
#include <list>
#include <vector>
#include <sstream>
#include <string>

#include "carwheelposition.h"
#include "configfile.h"
#include "coordinatesystems.h"
#include "collision_world.h"
#include "tracksurface.h"
#include "configfile.h"
#include "carinput.h"
#include "mesh_gen.h"

#include "camera_fixed.h"
#include "camera_free.h"
#include "camera_chase.h"
#include "camera_orbit.h"
#include "camera_mount.h"

#ifdef _WIN32
bool isnan(float number) {return (number != number);}
bool isnan(double number) {return (number != number);}
#endif

CAR::CAR()
:	gearsound_check(0),
	brakesound_check(false),
	handbrakesound_check(false),
	last_steer(0),
	sector(-1),
	applied_brakes(0)
{
}

bool CAR::GenerateWheelMesh(CONFIGFILE & carconf,
							const CARTIRE<double> & tire,
							SCENENODE & topnode,
							keyed_container <SCENENODE>::handle & output_scenenode,
							keyed_container <DRAWABLE>::handle & output_drawable,
							VERTEXARRAY & output_varray,
							TEXTUREMANAGER & textures,
							const std::string & confsection,
							const std::string & sharedpartspath,
							int anisotropy,
							const std::string & texsize,
							MODEL_JOE03 & output_rim_model,
							std::ostream & error_output)
{
	output_scenenode = topnode.AddNode();
	SCENENODE & node = topnode.GetNode(output_scenenode);

	// create the drawable in the correct layer depending on blend status
	output_drawable = GetDrawlistNoBlend(node).insert(DRAWABLE());
	DRAWABLE & draw = GetDrawlistNoBlend(node).get(output_drawable);
	
	// load model
	//float radius = section_width*0.001f * aspect_ratio*0.01f + rim_diameter*0.0254f*0.5;
	float rimDiameter_in = (tire.GetRadius() - tire.GetSidewallWidth()*tire.GetAspectRatio())/(0.0254f*0.5);
	float rim_diameter = (tire.GetRadius() - tire.GetSidewallWidth()*tire.GetAspectRatio())*2.f;
	float rim_width = 1;
	MESHGEN::mesh_gen_tire(&output_varray, tire.GetSidewallWidth()*1000.f, tire.GetAspectRatio()*100.f, rimDiameter_in);
	output_varray.Rotate(-M_PI_2, 0, 0, 1);
	draw.SetVertArray(&output_varray);
	
	//draw.SetObjectCenter(wheelmeshgen[i].GetCenter());

	// load textures
	std::string modelpath = sharedpartspath+"/wheel/";
	std::string texpath = sharedpartspath+"/wheel/textures/";
	
	std::string tiretex;
	if (!carconf.GetParam(confsection + ".tiretexture", tiretex, error_output))
		return false;
	
	{
		TEXTUREINFO texinfo;
		texinfo.SetName(texpath + tiretex + ".png");
		texinfo.SetMipMap(true);
		texinfo.SetAnisotropy(anisotropy);
		texinfo.SetSize(texsize);
		TEXTUREPTR texture = textures.Get(texinfo);
		if (!texture->Loaded())
		{
			error_output << "Tire texture couldn't be loaded: " << texpath + tiretex + ".png" << std::endl;
			return false;
		}
		draw.SetDiffuseMap(texture);
	}
	
	{
		std::string misc1texfile = texpath + tiretex + "-misc1.png";
		
		std::ifstream filecheck(misc1texfile.c_str());
		if (filecheck)
		{
			TEXTUREINFO texinfo;
			texinfo.SetName(misc1texfile);
			texinfo.SetMipMap(true);
			texinfo.SetAnisotropy(anisotropy);
			texinfo.SetSize(texsize);
			TEXTUREPTR misc1 = textures.Get(texinfo);
			if (!misc1->Loaded())
			{
				error_output << "Error loading tire misc texture: " << misc1texfile << std::endl;
				return false;
			}
			draw.SetMiscMap1(misc1);
		}
	}
	
	draw.SetBlur(false);
	draw.SetPartialTransparency(false);
	
	// now load the rim
	std::string wheelname;
	if (!carconf.GetParam(confsection + ".wheel", wheelname, error_output))
		return false;
	keyed_container <DRAWABLE>::handle rim_draw;
	if (!LoadInto(node,
			 output_scenenode,
			 rim_draw, 
			 modelpath+wheelname+".joe",
			 output_rim_model,
			 textures,
			 texpath+wheelname+".png",
			 texpath+wheelname+"-misc1.png",
			 texsize,
			 anisotropy,
			 false,
			 MATHVECTOR <float, 3>(rim_width,rim_diameter,rim_diameter),
			 error_output))
		return false;
	
	return true;
}

bool CAR::Load (
	CONFIGFILE & carconf,
	const std::string & carpath,
	const std::string & driverpath,
	const std::string & carname,
	TEXTUREMANAGER & textures,
	const std::string & carpaint,
	const MATHVECTOR <float, 3> & carcolor,
	const MATHVECTOR <float, 3> & initial_position,
	const QUATERNION <float> & initial_orientation,
	COLLISION_WORLD * world,
	bool soundenabled,
	const SOUNDINFO & sound_device_info,
	const SOUNDBUFFERLIBRARY & soundbufferlibrary,
	int anisotropy,
	bool defaultabs,
	bool defaulttcs,
	const std::string & texsize,
	float camerabounce,
	bool debugmode,
	const std::string & sharedpartspath,
	std::ostream & info_output,
	std::ostream & error_output)
{
	cartype = carname;
	std::stringstream nullout;

	//load car body graphics
	if ( !LoadInto ( topnode, bodynode, bodydraw, carpath + "/body.joe", bodymodel,
			textures, carpath + "/textures/body" + carpaint + ".png", carpath + "/textures/body-misc1.png",
			texsize, anisotropy, false, MATHVECTOR <float, 3>(1,1,1), error_output ) )
	{
		return false;
	}
	SCENENODE & bodynoderef = topnode.GetNode(bodynode);
	DRAWABLE & bodydrawref = GetDrawlistNoBlend(bodynoderef).get(bodydraw);
	bodydrawref.SetSelfIllumination(false);
	bodydrawref.SetColor(carcolor[0], carcolor[1], carcolor[2], 1); // set alpha to 1, body opaque
	
	//load driver graphics
	if (!driverpath.empty())
	{
		if (!LoadInto(topnode.GetNode(bodynode), drivernode, driverdraw, driverpath + "/body.joe", drivermodel,
				textures, driverpath + "/textures/body.png", driverpath + "/textures/body-misc1.png",
				texsize, anisotropy, false, MATHVECTOR <float, 3>(1,1,1), error_output))
		{
			drivernode.invalidate();
			error_output << "Error loading driver graphics: " << driverpath << std::endl;
		}
	}

	//load car brake light texture
	{
		TEXTUREINFO texinfo;
		texinfo.SetName(carpath + "/textures/brake.png");
		texinfo.SetMipMap(true);
		texinfo.SetAnisotropy(anisotropy);
		texinfo.SetSize(texsize);
		TEXTUREPTR braketexture = textures.Get(texinfo);
		if (!braketexture->Loaded())
		{
			info_output << "No car brake texture exists, continuing without one" << std::endl;
		}
		else
		{
			assert(bodydraw.valid());
			bodydrawref.SetAdditiveMap1(braketexture);
		}
	}

	//load car reverse light texture
	{
		TEXTUREINFO texinfo;
		texinfo.SetName(carpath + "/textures/reverse.png");
		texinfo.SetMipMap(true);
		texinfo.SetAnisotropy(anisotropy);
		texinfo.SetSize(texsize);
		TEXTUREPTR reversetexture = textures.Get(texinfo);
		if (!reversetexture->Loaded())
		{
			info_output << "No car reverse texture exists, continuing without one" << std::endl;
		}
		else
		{
			assert(bodydraw.valid());
			bodydrawref.SetAdditiveMap2(reversetexture);
		}
	}

	//load car interior graphics
	if ( !LoadInto ( topnode.GetNode(bodynode), bodynode, interiordraw, carpath + "/interior.joe", interiormodel,
			textures, carpath + "/textures/interior.png", carpath + "/textures/interior-misc1.png",
			texsize, anisotropy, false, MATHVECTOR <float, 3>(1,1,1), nullout ) )
	{
		info_output << "No car interior model exists, continuing without one" << std::endl;
	}

	//load car glass graphics
	if ( !LoadInto ( topnode.GetNode(bodynode), bodynode, glassdraw, carpath + "/glass.joe", glassmodel,
			textures, carpath + "/textures/glass.png", carpath + "/textures/glass-misc1.png",
			texsize, anisotropy, true, MATHVECTOR <float, 3>(1,1,1), nullout ) )
	{
		info_output << "No car glass model exists, continuing without one" << std::endl;
	}

	// get coordinate system version
	int version = 1;
	carconf.GetParam("version", version);
	
	//load dynamics
	if (!dynamics.Load(carconf, error_output)) return false;
	
	//load wheel graphics
	for (int i = 0; i < 2; i++) //front pair
	{
		if (dynamics.GetTire(WHEEL_POSITION(i)).GetSidewallWidth() <= 0)
		{
			// fallback to premodeled wheels
			if ( !LoadInto ( topnode, wheelnode[i], wheeldraw[i], carpath + "/wheel_front.joe", wheelmodelfront,
				textures, carpath + "/textures/wheel_front.png", carpath + "/textures/wheel_front-misc1.png",
				texsize, anisotropy, false, MATHVECTOR <float, 3>(1,1,1), error_output ) )
				return false;
		}
		else
		{
			if (!GenerateWheelMesh(carconf, dynamics.GetTire(WHEEL_POSITION(i)), topnode,
					wheelnode[i], wheeldraw[i], wheelmeshgen[i], textures, "tire-front", sharedpartspath,
					anisotropy, texsize, wheelrim[i], error_output))
			{
				error_output << "Error generating wheel mesh for wheel " << i << std::endl;
				return false;
			}
		}
		
		//load floating elements
		std::stringstream nullout;
		LoadInto ( topnode, floatingnode[i], floatingdraw[i], carpath + "/floating_front.joe", floatingmodelfront,
			textures, carpath + "/textures/body" + carpaint + ".png", carpath + "/textures/body-misc1.png",
      		texsize, anisotropy, false, MATHVECTOR <float, 3>(1,1,1), nullout );
	}
	for (int i = 2; i < 4; i++) //rear pair
	{
		if (dynamics.GetTire(WHEEL_POSITION(i)).GetSidewallWidth() <= 0)
		{
			if (dynamics.GetTire(WHEEL_POSITION(REAR_LEFT)).GetSidewallWidth() <= 0 ||
				dynamics.GetTire(WHEEL_POSITION(REAR_RIGHT)).GetSidewallWidth() <= 0)
			{
				if ( !LoadInto ( topnode, wheelnode[i], wheeldraw[i], carpath + "/wheel_rear.joe", wheelmodelrear,
					textures, carpath + "/textures/wheel_rear.png", carpath + "/textures/wheel_rear-misc1.png",
					texsize, anisotropy, false, MATHVECTOR <float, 3>(1,1,1), error_output ) )
					return false;
			}
		}
		else
		{
			if (!GenerateWheelMesh(carconf, dynamics.GetTire(WHEEL_POSITION(i)), topnode,
					wheelnode[i], wheeldraw[i], wheelmeshgen[i], textures, "tire-rear", sharedpartspath,
					anisotropy, texsize, wheelrim[i], error_output))
			{
				error_output << "Error generating wheel mesh for wheel " << i << std::endl;
				return false;
			}
		}

		//load floating elements
		std::stringstream nullout;
		LoadInto ( topnode, floatingnode[i], floatingdraw[i],
			carpath + "/floating_rear.joe", floatingmodelrear,
			textures, carpath + "/textures/body" + carpaint + ".png", carpath + "/textures/body-misc1.png",
      		texsize, anisotropy, false, MATHVECTOR <float, 3>(1,1,1), nullout );
	}
	
	//init dynamics
	if (world)
	{
		MATHVECTOR<double, 3> position;
		QUATERNION<double> orientation;
		position = initial_position;
		orientation = initial_orientation;

		// temporary hack for cardynamics
		wheelmodelfront.SetVertexArray(wheelmeshgen[0]);
		wheelmodelrear.SetVertexArray(wheelmeshgen[2]);
		wheelmodelfront.GenerateMeshMetrics();
		wheelmodelrear.GenerateMeshMetrics();

		dynamics.Init(*world, bodymodel, wheelmodelfront, wheelmodelrear, position, orientation);
		dynamics.SetABS(defaultabs);
		dynamics.SetTCS(defaulttcs);
	}

	// load driver
	{
		float pos[3];
		if (!carconf.GetParam("driver.position", pos, error_output)) return false;
		if (version == 2) COORDINATESYSTEMS::ConvertCarCoordinateSystemV2toV1(pos[0], pos[1], pos[2]);
		if (drivernode.valid()) //move the driver model to the coordinates given
		{
			SCENENODE & drivernoderef = topnode.GetNode(bodynode).GetNode(drivernode);
			MATHVECTOR <float, 3> floatpos;
			floatpos.Set(pos[0], pos[1], pos[2]);
			drivernoderef.GetTransform().SetTranslation(floatpos);
		}
	}

	// load views
	{
		CONFIGFILE & c = carconf;
		CAMERA_MOUNT * hood_cam = new CAMERA_MOUNT("hood");
		CAMERA_MOUNT * driver_cam = new CAMERA_MOUNT("incar");
		driver_cam->SetEffectStrength(camerabounce);
		hood_cam->SetEffectStrength(camerabounce);

		float pos[3], hoodpos[3];
		if (!c.GetParam("driver.view-position", pos, error_output)) return false;
		if (version == 2) COORDINATESYSTEMS::ConvertCarCoordinateSystemV2toV1(pos[0], pos[1], pos[2]);
		MATHVECTOR <float, 3> cam_offset;
		cam_offset.Set(pos);
		driver_cam->SetOffset(cam_offset);

		if (!c.GetParam("driver.hood-mounted-view-position", hoodpos, error_output))
		{
			pos[1] = 0;
			pos[0] += 1.0;
			cam_offset.Set(pos);
		}
		else
		{
			if (version == 2)
				COORDINATESYSTEMS::ConvertCarCoordinateSystemV2toV1(hoodpos[0],hoodpos[1],hoodpos[2]);
			cam_offset.Set(hoodpos);
		}
		hood_cam->SetOffset(cam_offset);

		float view_stiffness = 0.0;
		c.GetParam("driver.view-stiffness", view_stiffness);
		driver_cam->SetStiffness(view_stiffness);
		hood_cam->SetStiffness(view_stiffness);
		cameras.Add(hood_cam);
		cameras.Add(driver_cam);

		CAMERA_FIXED * cam_chaserigid = new CAMERA_FIXED("chaserigid");
		cam_chaserigid->SetOffset(-6, 0, 1.5);
		cameras.Add(cam_chaserigid);

		CAMERA_CHASE * cam_chase = new CAMERA_CHASE("chase");
		cam_chase->SetChaseHeight(2.0);
		cameras.Add(cam_chase);

		cameras.Add(new CAMERA_ORBIT("orbit"));
		cameras.Add(new CAMERA_FREE("free"));

		// load additional views
		int i = 1;
		std::string istr = "1";
		std::string view_name;
		while(c.GetParam("view.name-" + istr, view_name))
		{
			float pos[3], angle[3];
			if (!c.GetParam("view.position-" + istr, pos)) continue;
			if (!c.GetParam("view.angle-" + istr, angle)) continue;
			if (version == 2) COORDINATESYSTEMS::ConvertCarCoordinateSystemV2toV1(pos[0], pos[1], pos[2]);

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
	}

	//load sounds
	if (soundenabled)
	{
		if (!LoadSounds(carpath, carname, sound_device_info, soundbufferlibrary, info_output, error_output))
			return false;
	}

	mz_nominalmax = (GetTireMaxMz(FRONT_LEFT) + GetTireMaxMz(FRONT_RIGHT))*0.5;

	lookbehind = false;

	return true;
}

bool CAR::LoadSounds(
	const std::string & carpath,
	const std::string & carname,
	const SOUNDINFO & sound_device_info,
	const SOUNDBUFFERLIBRARY & soundbufferlibrary,
	std::ostream & info_output,
	std::ostream & error_output)
{
	//check for sound specification file
	CONFIGFILE aud;
	if (aud.Load(carpath+"/"+carname+".aud"))
	{
		std::list <std::string> sections;
		aud.GetSectionList(sections);
		for (std::list <std::string>::iterator i = sections.begin(); i != sections.end(); ++i)
		{
			//load the buffer
			std::string filename;
			if (!aud.GetParam(*i+".filename", filename, error_output)) return false;
			if (!soundbuffers[filename].GetLoaded())
				if (!soundbuffers[filename].Load(carpath+"/"+filename, sound_device_info, error_output))
				{
					error_output << "Error loading sound: " << carpath+"/"+filename << std::endl;
					return false;
				}

			enginesounds.push_back(std::pair <ENGINESOUNDINFO, SOUNDSOURCE> ());
			ENGINESOUNDINFO & info = enginesounds.back().first;
			SOUNDSOURCE & sound = enginesounds.back().second;

			if (!aud.GetParam(*i+".MinimumRPM", info.minrpm, error_output)) return false;
			if (!aud.GetParam(*i+".MaximumRPM", info.maxrpm, error_output)) return false;
			if (!aud.GetParam(*i+".NaturalRPM", info.naturalrpm, error_output)) return false;

			std::string powersetting;
			if (!aud.GetParam(*i+".power", powersetting, error_output)) return false;
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
		if (!soundbuffers["engine.wav"].Load(carpath+"/engine.wav", sound_device_info, error_output))
		{
			error_output << "Unable to load engine sound: "+carpath+"/engine.wav" << std::endl;
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
			error_output << "Can't load tire_squeal sound" << std::endl;
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
			error_output << "Can't load gravel sound" << std::endl;
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
			error_output << "Can't load grass sound" << std::endl;
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
			error_output << "Can't load bump sound: " << i << std::endl;
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
			error_output << "Can't load crash sound" << std::endl;
			return false;
		}
		crashsound.SetBuffer(*buf);
		crashsound.Set3DEffects(true);
		crashsound.SetLoop(false);
		crashsound.SetGain(1.0);
	}

	//set up gear sound
	{
		const SOUNDBUFFER * buf = soundbufferlibrary.GetBuffer("gear"); //TODO: Make this "per car", using carpath+"/"+carname+ in a correct form
		if (!buf)
		{
			error_output << "Can't load gear sound" << std::endl;
			return false;
		}
		gearsound.SetBuffer(*buf);
		gearsound.Set3DEffects(true);
		gearsound.SetLoop(false);
		gearsound.SetGain(1.0);
	}

	//set up brake sound
	{
		const SOUNDBUFFER * buf = soundbufferlibrary.GetBuffer("brake"); //TODO: Make this "per car", using carpath+"/"+carname+ in a correct form
		if (!buf)
		{
			error_output << "Can't load brake sound" << std::endl;
			return false;
		}
		brakesound.SetBuffer(*buf);
		brakesound.Set3DEffects(true);
		brakesound.SetLoop(false);
		brakesound.SetGain(1.0);
	}

	//set up handbrake sound
	{
		const SOUNDBUFFER * buf = soundbufferlibrary.GetBuffer("handbrake"); //TODO: Make this "per car", using carpath+"/"+carname+ in a correct form
		if (!buf)
		{
			error_output << "Can't load handbrake sound" << std::endl;
			return false;
		}
		handbrakesound.SetBuffer(*buf);
		handbrakesound.Set3DEffects(true);
		handbrakesound.SetLoop(false);
		handbrakesound.SetGain(1.0);
	}

	{
		const SOUNDBUFFER * buf = soundbufferlibrary.GetBuffer("wind");
		if (!buf)
		{
			error_output << "Can't load wind sound" << std::endl;
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

bool CAR::LoadInto (
	SCENENODE & parentnode,
	keyed_container <SCENENODE>::handle & output_scenenode,
	keyed_container <DRAWABLE>::handle & output_drawable,
	const std::string & joefile,
	MODEL_JOE03 & output_model,
	TEXTUREMANAGER & textures,
	const std::string & texfile,
	const std::string & misc1texfile,
	const std::string & texsize,
	int anisotropy,
	bool blend,
	const MATHVECTOR <float, 3> & scale,
	std::ostream & error_output )
{
	SCENENODE * node = &parentnode;
	if (!output_scenenode.valid())
	{
		output_scenenode = parentnode.AddNode();
		node = &parentnode.GetNode(output_scenenode);
	}

	// create the drawable in the correct layer depending on blend status
	DRAWABLE * draw = NULL;
	if (blend)
	{
		output_drawable = GetDrawlistBlend(*node).insert(DRAWABLE());
		draw = &GetDrawlistBlend(*node).get(output_drawable);
	}
	else
	{
		output_drawable = GetDrawlistNoBlend(*node).insert(DRAWABLE());
		draw = &GetDrawlistNoBlend(*node).get(output_drawable);
	}
	assert(draw);
	
	// load model
	if (!output_model.Loaded())
	{
		std::stringstream nullout;
		if (!output_model.ReadFromFile(joefile.substr(0,std::max((long unsigned int)0,(long unsigned int) joefile.size()-3))+"ova", nullout))
		{
			if (!output_model.Load(joefile, error_output))
			{
				error_output << "Error loading model: " << joefile << std::endl;
				return false;
			}
			// car mesh orientation fixer
			output_model.Scale(scale[0], scale[1], scale[2]);
			output_model.Rotate(-M_PI_2, 0, 0, 1);
 			output_model.GenerateMeshMetrics();
			output_model.GenerateListID(error_output);
		}
	}
	draw->AddDrawList(output_model.GetListID());
	draw->SetObjectCenter(output_model.GetCenter());

	// load textures
	if(!texfile.empty())
	{
		TEXTUREINFO texinfo;
		texinfo.SetName(texfile);
		texinfo.SetMipMap(true);
		texinfo.SetAnisotropy(anisotropy);
		texinfo.SetSize(texsize);
		TEXTUREPTR diffuse = textures.Get(texinfo);
		if (!diffuse->Loaded())
		{
			error_output << "Error loading texture: " << texfile << std::endl;
			return false;
		}
		draw->SetDiffuseMap(diffuse);
	}

	if (!misc1texfile.empty())
	{
		std::ifstream filecheck(misc1texfile.c_str());
		if (filecheck)
		{
			TEXTUREINFO texinfo;
			texinfo.SetName(misc1texfile);
			texinfo.SetMipMap(true);
			texinfo.SetAnisotropy(anisotropy);
			texinfo.SetSize(texsize);
			TEXTUREPTR misc1 = textures.Get(texinfo);
			if (!misc1->Loaded())
			{
				error_output << "Error loading texture: " << texfile << std::endl;
				return false;
			}
			draw->SetMiscMap1(misc1);
		}
	}
	
	draw->SetBlur(false);
	draw->SetPartialTransparency(blend);
	return true;
}

void CAR::SetColor(float r, float g, float b)
{
	SCENENODE & bodynoderef = topnode.GetNode(bodynode);
	DRAWABLE & bodydrawref = GetDrawlistNoBlend(bodynoderef).get(bodydraw);
	bodydrawref.SetColor(r, g, b, 1);
}

void CAR::SetPosition(const MATHVECTOR <float, 3> & new_position)
{
	MATHVECTOR <double,3> newpos;
	newpos = new_position;
	dynamics.SetPosition(newpos);
	dynamics.AlignWithGround();

	QUATERNION <float> rot;
	rot = dynamics.GetOrientation();

	cameras.Active()->Reset(newpos, rot);
}

void CAR::CopyPhysicsResultsIntoDisplay()
{
	if (!bodynode.valid())
		return;

	MATHVECTOR <float, 3> vec;
	vec = dynamics.GetPosition();
	SCENENODE & bodynoderef = topnode.GetNode(bodynode);
	bodynoderef.GetTransform().SetTranslation(vec);

	vec = dynamics.GetCenterOfMassPosition();
	roadnoise.SetPosition(vec[0],vec[1],vec[2]);
	crashsound.SetPosition(vec[0],vec[1],vec[2]);
	gearsound.SetPosition(vec[0],vec[1],vec[2]);
	brakesound.SetPosition(vec[0],vec[1],vec[2]);
	handbrakesound.SetPosition(vec[0],vec[1],vec[2]);

	QUATERNION <float> quat;
	quat = dynamics.GetOrientation();
	bodynoderef.GetTransform().SetRotation(quat);

	for (int i = 0; i < 4; i++)
	{
		vec = dynamics.GetWheelPosition(WHEEL_POSITION(i));
		SCENENODE & wheelnoderef = topnode.GetNode(wheelnode[i]);
		wheelnoderef.GetTransform().SetTranslation(vec);
		tirebump[i].SetPosition(vec[0],vec[1],vec[2]);
		QUATERNION <float> wheelquat;
		wheelquat = dynamics.GetWheelOrientation(WHEEL_POSITION(i));
		wheelnoderef.GetTransform().SetRotation(wheelquat);

		if (floatingnode[i].valid())
		{
			SCENENODE & floatingnoderef = topnode.GetNode(floatingnode[i]);
			floatingnoderef.GetTransform().SetTranslation(vec);
			QUATERNION <float> floatquat;
			floatquat = dynamics.GetUprightOrientation(WHEEL_POSITION(i));
			floatingnoderef.GetTransform().SetRotation(floatquat);
		}
	}
	
	// update brake/reverse lights
	DRAWABLE & bodydrawref = GetDrawlistNoBlend(bodynoderef).get(bodydraw);
	bodydrawref.EnableAdditiveMap1(applied_brakes > 0);
	bodydrawref.EnableAdditiveMap2(GetGear() < 0);
	bodydrawref.SetSelfIllumination(true);
}

void CAR::UpdateCameras(float dt)
{
	QUATERNION <float> rot;
	rot = dynamics.GetOrientation();
	MATHVECTOR <float, 3> pos = dynamics.GetPosition();
	MATHVECTOR <float, 3> acc = dynamics.GetLastBodyForce() / dynamics.GetMass();

	// reverse the camera direction
	if (lookbehind)
	{
		rot.Rotate(3.141593, 0, 0, 1);
	}

	cameras.Active()->Update(pos, rot, acc, dt);
}

void CAR::Update(double dt)
{
	dynamics.Update();
	CopyPhysicsResultsIntoDisplay();
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
	float clutch = 1 - inputs[CARINPUT::CLUTCH]; // 

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
			float gain = 0.1;

			if (!brakesound.Audible())
			{
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
			float gain = 0.1;

			if (!handbrakesound.Audible())
			{
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
		vel = dynamics.GetVelocity();
		float gain = vel.Magnitude();
		if (gain < 0)
			gain = -gain;
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
			float gain = GetEngineRPMLimit() / GetEngineRPM();
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

float CAR::GetFeedback()
{
	return dynamics.GetFeedback() / (mz_nominalmax * 0.025);
}

float CAR::GetTireSquealAmount(WHEEL_POSITION i) const
{
	const TRACKSURFACE & surface = dynamics.GetWheelContact(WHEEL_POSITION(i)).GetSurface();
	if (surface.type == TRACKSURFACE::NONE)
		return 0;

	MATHVECTOR <float, 3> groundvel;
	groundvel = dynamics.GetWheelVelocity(WHEEL_POSITION(i));
	QUATERNION <float> wheelspace;
	wheelspace = dynamics.GetUprightOrientation(WHEEL_POSITION(i));
	(-wheelspace).RotateVector(groundvel);
	float wheelspeed = dynamics.GetWheel(WHEEL_POSITION(i)).GetAngularVelocity()*dynamics.GetTire(WHEEL_POSITION(i)).GetRadius();
	groundvel[0] -= wheelspeed;
	groundvel[1] *= 2.0;
	groundvel[2] = 0;
	float squeal = (groundvel.Magnitude() - 3.0) * 0.2;

	std::pair <double, double> slideslip = dynamics.GetTire(i).GetSlideSlipRatios();
	double maxratio = std::max(std::abs(slideslip.first), std::abs(slideslip.second));
	float squealfactor = std::max(0.0, maxratio - 1.0);
	squeal *= squealfactor;
	if (squeal < 0)
		squeal = 0;
	if (squeal > 1)
		squeal = 1;

	return squeal;
}

void CAR::EnableGlass(bool enable)
{
	if (!glassdraw.valid())
		return;
	
	SCENENODE & bodynoderef = topnode.GetNode(bodynode);
	DRAWABLE & glassdrawref = GetDrawlistBlend(bodynoderef).get(glassdraw);
	glassdrawref.SetDrawEnable(enable);
}

bool CAR::Serialize(joeserialize::Serializer & s)
{
	_SERIALIZE_(s,dynamics);
	_SERIALIZE_(s,last_steer);
	return true;
}

