#include "car.h"

#include "carwheelposition.h"
#include "configfile.h"
#include "coordinatesystems.h"
#include "collision_world.h"
#include "tracksurface.h"
#include "configfile.h"
#include "carinput.h"
#include "mesh_gen.h"
#include "texturemanager.h"
#include "modelmanager.h"
#include "soundmanager.h"
#include "camera_fixed.h"
#include "camera_free.h"
#include "camera_chase.h"
#include "camera_orbit.h"
#include "camera_mount.h"

#include <fstream>
#include <map>
#include <list>
#include <vector>
#include <sstream>
#include <string>

#if defined(_WIN32) || defined(__APPLE__)
bool isnan(float number) {return (number != number);}
bool isnan(double number) {return (number != number);}
#endif

enum WHICHDRAWLIST
{
	BLEND,
	NOBLEND,
	EMISSIVE,
	OMNI
};

static keyed_container <DRAWABLE> & GetDrawlist(SCENENODE & node, WHICHDRAWLIST which)
{
	switch (which)
	{
		case BLEND:
		return node.GetDrawlist().normal_blend;
		
		case NOBLEND:
		return node.GetDrawlist().car_noblend;
		
		case EMISSIVE:
		return node.GetDrawlist().lights_emissive;
		
		case OMNI:
		return node.GetDrawlist().lights_omni;
	};
	assert(0);
	return node.GetDrawlist().car_noblend;
}

// load textures, order diffuse, misc1, misc2
static bool LoadTextures(
	TEXTUREMANAGER & textures,
	const std::vector<std::string> & texname,
	const std::string & texpath,
	const std::string & texsize,
	int anisotropy,
	DRAWABLE & draw,
	std::ostream & error_output)
{
	if(texname.size() == 0)
	{
		error_output << "No texture defined" << std::endl;
		return false;
	}
	
	TEXTUREINFO info;
	info.mipmap = true;
	info.anisotropy = anisotropy;
	info.size = texsize;
	
	std::tr1::shared_ptr<TEXTURE> tex;
	if(texname.size() > 0)
	{
		if (!textures.Load(texpath + texname[0], info, tex)) return false;
		draw.SetDiffuseMap(tex);
	}
	if(texname.size() > 1)
	{
		if (!textures.Load(texpath + texname[1], info, tex)) return false;
		draw.SetMiscMap1(tex);
	}
	if(texname.size() > 2)
	{
		if (!textures.Load(texpath + texname[1], info, tex)) return false;
		draw.SetMiscMap2(tex);
	}
	
	return true;
}

/// takes a initialized drawable => mesh+textures (copies it into corresponding drawlist)
static void AddDrawable(
	WHICHDRAWLIST whichdrawlist,
	SCENENODE & parentnode,
	DRAWABLE & draw,
	keyed_container <SCENENODE>::handle & output_scenenode,
	keyed_container <DRAWABLE>::handle & output_drawable,
	std::ostream & error_output)
{
	SCENENODE * node = &parentnode;
	if (!output_scenenode.valid())
	{
		output_scenenode = parentnode.AddNode();
		node = &parentnode.GetNode(output_scenenode);
	}
	
	if (whichdrawlist == EMISSIVE)
	{
		draw.SetDecal(true);
	}
	
	// create the drawable in the correct layer depending on blend status
	output_drawable = GetDrawlist(*node, whichdrawlist).insert(draw);
	assert(&GetDrawlist(*node, whichdrawlist).get(output_drawable));
}
	
/// take the parentnode, add a scenenode (if output_scenenode isn't yet valid), add a drawable to the
/// scenenode, load a model, load a texture, and set up the drawable with the model and texture.
/// the given TEXTURE textures will not be reloaded if they are already loaded
/// returns true if successful
static bool LoadInto(
	const WHICHDRAWLIST whichdrawlist,
	const std::string & modelname,
	const std::vector<std::string> & texname,
	const std::string & texpath,
	const std::string & texsize,
	const int anisotropy,
	SCENENODE & parentnode,
	TEXTUREMANAGER & textures,
	MODELMANAGER & models,
	std::list <std::tr1::shared_ptr<MODEL_JOE03> > & modellist,
	keyed_container <SCENENODE>::handle & output_scenenode,
	keyed_container <DRAWABLE>::handle & output_drawable,
	std::ostream & error_output)
{
	std::tr1::shared_ptr<MODEL_JOE03> model;
	if (!models.Load(modelname, model)) return false;
	modellist.push_back(model);
	
	DRAWABLE draw;
	draw.AddDrawList(model->GetListID());
	if (!LoadTextures(textures, texname, texpath, texsize, anisotropy, draw, error_output)) return false;
	AddDrawable(whichdrawlist, parentnode, draw, output_scenenode, output_drawable, error_output);
	
	return true;
}

static bool GenerateWheelMesh(
	const CONFIGFILE & carconf,
	const std::string & id,
	const std::string & carpath,
	const std::string & partspath,
	const std::string & texsize,
	const int anisotropy,
	SCENENODE & topnode,
	TEXTUREMANAGER & textures,
	MODELMANAGER & models,
	std::list <std::tr1::shared_ptr<MODEL_JOE03> > & modellist,
	keyed_container <SCENENODE>::handle & output_scenenode,
	keyed_container <DRAWABLE>::handle & output_drawable,
	std::ostream & error_output)
{
	output_scenenode = topnode.AddNode();
	SCENENODE & node = topnode.GetNode(output_scenenode);

	std::string orientation;
	carconf.GetParam("wheel-"+id+".orientation", orientation, error_output);

	// tire parameters
	std::string tiresize;
	CARTIRESIZE<float> tire;
	if (!carconf.GetParam("tire-"+id+".size", tiresize, error_output)) return false;
	if (!tire.Parse(tiresize, error_output)) return false;
	float aspectRatio = tire.aspect_ratio * 100.f;
	float rim_diameter = (tire.radius - tire.sidewall_width * tire.aspect_ratio) * 2.f;
	float rim_width = tire.sidewall_width;
	float sectionWidth_mm = tire.sidewall_width * 1000.f;
	float rimDiameter_in = rim_diameter / 0.0254f;

	// create tire
	std::vector<std::string> tiretexname;
	if (!carconf.GetParam("tire-"+id+".texture", tiretexname, error_output)) return false;
	
	const std::string tiremodelname(tiresize + orientation);
	std::tr1::shared_ptr<MODEL_JOE03> tiremodel;
	if (!models.Get(tiremodelname, tiremodel))
	{
		VERTEXARRAY output_varray;
		MESHGEN::mg_tire(output_varray, sectionWidth_mm, aspectRatio, rimDiameter_in);
		//output_varray.Rotate(-M_PI_2, 0, 0, 1);
		if (orientation != "left") output_varray.Scale(-1, 1, 1); // mirror mesh

		tiremodel.reset(new MODEL_JOE03());
		tiremodel->SetVertexArray(output_varray);
		tiremodel->GenerateMeshMetrics();
		tiremodel->GenerateListID(error_output);
		models.Set(tiremodelname, tiremodel);
	}
	if (!LoadInto(
		NOBLEND, tiremodelname, tiretexname, partspath + "/tire/textures/", texsize, anisotropy,
		node, textures, models, modellist, output_scenenode, output_drawable,
		error_output)) return false;

	// wheel parameters
	std::string rimmodelname;
	std::vector<std::string> wheeltexname;
	if (!carconf.GetParam("wheel-"+id+".mesh", rimmodelname, error_output)) return false;
	if (!carconf.GetParam("wheel-"+id+".texture", wheeltexname, error_output)) return false;
	
	// create wheel
	std::tr1::shared_ptr<MODEL_JOE03> wheelmodel;
	std::string wheeltexpath(carpath + "/textures/");
	std::string wheelmodelname(carpath + rimmodelname + tiresize + orientation);
	if (!models.Get(wheelmodelname, wheelmodel))
	{
		std::string modelname(carpath + "/" + rimmodelname);
		if (!std::ifstream((models.GetPath() + "/" + modelname).c_str()))
		{
			modelname = partspath + "/wheel/" + rimmodelname;
			wheeltexpath = partspath + "/wheel/textures/";
			wheelmodelname = rimmodelname + tiresize + orientation;
		}
		if (!models.Get(wheelmodelname, wheelmodel))
		{
			// load wheel mesh, scale and translate(wheel model offset rim_width/2)
			std::tr1::shared_ptr<MODEL_JOE03> temp;
			if (!models.Load(modelname, temp)) return false;

			// create a new wheel model
			wheelmodel.reset(new MODEL_JOE03());
			wheelmodel->SetVertexArray(temp->GetVertexArray());
			wheelmodel->Translate(-0.75 * 0.5, 0, 0);
			wheelmodel->Scale(rim_width, rim_diameter, rim_diameter);
			
			// create wheel rim
			const float flangeDisplacement_mm = 10;
			VERTEXARRAY varray;
			MESHGEN::mg_rim(varray, sectionWidth_mm, aspectRatio, rimDiameter_in, flangeDisplacement_mm);
			//rim_varray.Rotate(-M_PI_2, 0, 0, 1);

			// add rim to wheel mesh
			varray = varray + wheelmodel->GetVertexArray();
			if (orientation != "left") varray.Scale(-1, 1, 1); // mirror mesh

			wheelmodel->SetVertexArray(varray);
			wheelmodel->GenerateMeshMetrics();
			wheelmodel->GenerateListID(error_output);
			models.Set(wheelmodelname, wheelmodel);
		}
	}
	keyed_container <DRAWABLE>::handle wheeldraw;
	if (!LoadInto(
		NOBLEND, wheelmodelname, wheeltexname, wheeltexpath, texsize, anisotropy,
		node, textures, models, modellist, output_scenenode, wheeldraw,
		error_output)) return false;

	// create brake rotor(optional)
	std::string radius;
	std::vector<std::string> rotortexname;
	if (!carconf.GetParam("brake-"+id+".texture", rotortexname)) return true;
	if (!carconf.GetParam("brake-"+id+".radius", radius, error_output)) return false;

	std::tr1::shared_ptr<MODEL_JOE03> brakemodel;
	std::string rotorname("rotor"+radius+orientation);
	if (!models.Get(rotorname, brakemodel))
	{
		float r(0.25);
		std::stringstream s;
		s << radius;
		s >> r;
		float diameter_mm = r * 2 * 1000;
		float thickness_mm = 25;

		VERTEXARRAY rotor_varray;
		MESHGEN::mg_brake_rotor(&rotor_varray, diameter_mm, thickness_mm);
		if (orientation != "left") rotor_varray.Scale(-1, 1, 1); // mirror mesh
		//rotor_varray->Rotate(-M_PI_2, 0, 0, 1);
		
		brakemodel.reset(new MODEL_JOE03());
		brakemodel->SetVertexArray(rotor_varray);
		brakemodel->GenerateMeshMetrics();
		brakemodel->GenerateListID(error_output);
		models.Set(rotorname, brakemodel);
	}
	keyed_container <DRAWABLE>::handle rotor_draw;
	std::string rotortexpath(partspath + "/brake/textures/");
	if (!LoadInto(
		NOBLEND, rotorname, rotortexname, rotortexpath, texsize, anisotropy,
		node, textures, models, modellist, output_scenenode, rotor_draw, 
		error_output)) return false;

	return true;
}

bool LoadCameras(
	const CONFIGFILE & cfg,
	const float camerabounce,
	CAMERA_SYSTEM & cameras,
	std::ostream & error_output)
{
	CAMERA_MOUNT * hood_cam = new CAMERA_MOUNT("hood");
	CAMERA_MOUNT * driver_cam = new CAMERA_MOUNT("incar");
	driver_cam->SetEffectStrength(camerabounce);
	hood_cam->SetEffectStrength(camerabounce);

	float pos[3], hoodpos[3];
	if (!cfg.GetParam("camera.view-position", pos, error_output)) return false;
	COORDINATESYSTEMS::ConvertCarCoordinateSystemV2toV1(pos[0], pos[1], pos[2]);
	MATHVECTOR <float, 3> cam_offset;
	cam_offset.Set(pos);
	driver_cam->SetOffset(cam_offset);

	if (!cfg.GetParam("camera.hood-mounted-view-position", hoodpos, error_output))
	{
		pos[1] = 0;
		pos[0] += 1.0;
		cam_offset.Set(pos);
	}
	else
	{
		COORDINATESYSTEMS::ConvertCarCoordinateSystemV2toV1(hoodpos[0],hoodpos[1],hoodpos[2]);
		cam_offset.Set(hoodpos);
	}
	hood_cam->SetOffset(cam_offset);

	float view_stiffness = 0.0;
	cfg.GetParam("camera.view-stiffness", view_stiffness);
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
/*
	// load additional views
	int i = 1;
	std::string istr = "1";
	std::string view_name;
	while(cfg.GetParam("view.name-" + istr, view_name))
	{
		float pos[3], angle[3];
		if (!cfg.GetParam("view.position-" + istr, pos)) continue;
		if (!cfg.GetParam("view.angle-" + istr, angle)) continue;
		COORDINATESYSTEMS::ConvertCarCoordinateSystemV2toV1(pos[0], pos[1], pos[2]);

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
*/
	return true;
}

CAR::CAR() :
	gearsound_check(0),
	brakesound_check(false),
	handbrakesound_check(false),
	last_steer(0),
	sector(-1),
	applied_brakes(0)
{
	// ctor
	modelrotation.Rotate(-M_PI_2, 0, 0, 1);
}

bool CAR::LoadLight(
	const CONFIGFILE & cfg,
	const std::string & name,
	std::ostream & error_output)
{
	float pos[] = {0, 0, 0};
	float col[] = {0, 0, 0};
	float size;
	if (!cfg.GetParam(name + ".position", pos, error_output)) return false;
	if (!cfg.GetParam(name + ".color", col, error_output)) return false;
	if (!cfg.GetParam(name + ".radius", size, error_output)) return false;

//	COORDINATESYSTEMS::ConvertCarCoordinateSystemV2toV1(pos[0], pos[1], pos[2]);

	lights.push_back(LIGHT());
	SCENENODE & bodynoderef = topnode.GetNode(bodynode);
	lights.back().node = bodynoderef.AddNode();
	SCENENODE & node = bodynoderef.GetNode(lights.back().node);
	VERTEXARRAY & varray = lights.back().varray;
	MODEL & model = lights.back().model;
	varray.SetToUnitCube();
	varray.Scale(size, size, size);
	//varray.SetToBillboard(-1,-1,1,1);
	node.GetTransform().SetTranslation(MATHVECTOR<float,3>(pos[0], pos[1], pos[2]));
	model.BuildFromVertexArray(varray, error_output);

	keyed_container <DRAWABLE> & dlist = GetDrawlist(node, OMNI);
	lights.back().draw = dlist.insert(DRAWABLE());
	DRAWABLE & draw = dlist.get(lights.back().draw);
	draw.SetColor(col[0], col[1], col[2]);
	draw.AddDrawList(model.GetListID());
	//draw.SetVertArray(&model.GetVertexArray());
	draw.SetCull(true, true);
	//draw.SetCull(false, false);
	draw.SetDrawEnable(false);

	return true;
}

bool CAR::LoadGraphics(
	const CONFIGFILE & carconf,
	const std::string & carpath,
	const std::string & carname,
	const std::string & partspath,
	const MATHVECTOR <float, 3> & carcolor,
	const std::string & carpaint,
	const std::string & texsize,
	const int anisotropy,
	const float camerabounce,
	const bool loaddriver,
	const bool debugmode,
	TEXTUREMANAGER & textures,
	MODELMANAGER & models,
	std::ostream & info_output,
	std::ostream & error_output)
{
	cartype = carname;
	std::string texpath(carpath + "/textures/");
	
	//load car body graphics
	std::string bodymodelname;
	std::vector<std::string> bodytexname;
	if (!carconf.GetParam("body.mesh", bodymodelname, error_output)) return false;
	if (!carconf.GetParam("body.texture", bodytexname, error_output)) return false;
	
	bodytexname[0] = "body"+carpaint+".png"; 
	if (!LoadInto(
		NOBLEND, carpath+"/"+bodymodelname, bodytexname, texpath, texsize, anisotropy,
		topnode, textures, models, modellist, bodynode, bodydraw,
		error_output)) return false;
	
	//load car interior graphics (optional)
	std::string intmodelname;
	if (carconf.GetParam("interior.mesh", intmodelname))
	{
		keyed_container <DRAWABLE>::handle interiordraw;
		std::vector<std::string> texname;
		if (!carconf.GetParam("interior.texture", texname, error_output)) return false;
		if (!LoadInto(
			NOBLEND, carpath+"/"+intmodelname, texname, texpath, texsize, anisotropy,
			topnode.GetNode(bodynode), textures, models, modellist, bodynode, interiordraw,
			error_output)) return false;
	}

	//load car glass graphics (optional)
	std::string glassmodelname;
	if (carconf.GetParam("glass.mesh", glassmodelname))
	{
		std::vector<std::string> texname;
		if (!carconf.GetParam("glass.texture", texname, error_output)) return false;
		if (!LoadInto(
			BLEND, carpath+"/"+glassmodelname, texname, texpath, texsize, anisotropy,
			topnode.GetNode(bodynode), textures, models, modellist, bodynode, glassdraw,
			error_output)) return false;
	}

	// load driver graphics (optional)
	if (loaddriver)
	{
		std::string drivermodelname;
		if (carconf.GetParam("driver.mesh", drivermodelname))
		{
			keyed_container <DRAWABLE>::handle driverdraw;
			std::vector<std::string> texname;
			if (!carconf.GetParam("driver.texture", texname, error_output)) return false;
			if (LoadInto(
				NOBLEND, partspath+"/driver/"+drivermodelname, texname, partspath+"/driver/textures/", texsize, anisotropy,
				topnode.GetNode(bodynode), textures, models, modellist, drivernode, driverdraw,
				error_output))
			{
				float pos[3] = {0, 0, 0};
				if (!carconf.GetParam("driver.position", pos, error_output)) return false;
				//COORDINATESYSTEMS::ConvertCarCoordinateSystemV2toV1(pos[0], pos[1], pos[2]);
				SCENENODE & drivernoderef = topnode.GetNode(bodynode).GetNode(drivernode);
				MATHVECTOR <float, 3> floatpos(pos[0], pos[1], pos[2]);
				drivernoderef.GetTransform().SetTranslation(floatpos);
			}
			else
			{
				error_output << "Error loading driver graphics: " << partspath + "/driver/" + drivermodelname << std::endl;
			}
		}
	}

	// load wheel graphics
	const std::string wheelid[] = {"fl", "fr", "rl", "rr"};
	for (int i = 0; i < WHEEL_POSITION_SIZE; ++i)
	{
		keyed_container <DRAWABLE>::handle wheeldraw;
		if (!GenerateWheelMesh(
			carconf, wheelid[i], carpath, partspath, texsize, anisotropy, 
			topnode, textures, models, modellist, wheelnode[i], wheeldraw, error_output))
		{
			error_output << "Error generating wheel mesh for wheel " << i << std::endl;
			return false;
		}

		std::string fendermodel;
		if (carconf.GetParam("cycle-fender-"+wheelid[i]+".mesh", fendermodel))
		{
			keyed_container <DRAWABLE>::handle fenderdraw;
			std::vector<std::string> fendertex;
			if (!carconf.GetParam("cycle-fender-"+wheelid[i]+".texture", fendertex)) return false;
			LoadInto(
				NOBLEND, carpath+"/"+fendermodel, fendertex, texpath, texsize, anisotropy,
				topnode, textures, models, modellist, floatingnode[i], fenderdraw, error_output);
		}

		// set wheel positions(for widget_spinningcar)
		float pos[3];
		if (!carconf.GetParam("wheel-"+wheelid[i]+".position", pos, error_output)) return false;
		//COORDINATESYSTEMS::ConvertCarCoordinateSystemV2toV1(pos[0], pos[1], pos[2]);

		MATHVECTOR <float, 3> wheelpos(pos[0], pos[1], pos[2]);
		SCENENODE & wheelnoderef = topnode.GetNode(wheelnode[i]);
		wheelnoderef.GetTransform().SetTranslation(wheelpos);
		if (floatingnode[i].valid())
		{
			SCENENODE & floatingnoderef = topnode.GetNode(floatingnode[i]);
			floatingnoderef.GetTransform().SetTranslation(wheelpos);
		}
	}
	
	{
		// load brake light point light sources (optional)
		float r;
		int i = 0;
		std::string istr = "0";
		while (carconf.GetParam("light-brake-" + istr + ".radius", r))
		{
			if (!LoadLight(carconf, "light-brake-" + istr, error_output)) return false;

			std::stringstream sstr;
			sstr << ++i;
			istr = sstr.str();
		}

		// load car brake graphics (optional)
		std::string brakemodelname;
		if (carconf.GetParam("light-brake.mesh", brakemodelname))
		{
			std::vector<std::string> texname;
			if (!carconf.GetParam("light-brake.texture", texname, error_output)) return false;
			if (!LoadInto(
				EMISSIVE, carpath+"/"+brakemodelname, texname, texpath, texsize, anisotropy,
				topnode.GetNode(bodynode), textures, models, modellist, bodynode, brakelights,
				error_output)) return false;
		}
		
		// load reverse lights (optional)
		i = 0;
		istr = "0";
		while (carconf.GetParam("light-reverse-" + istr + ".radius", r))
		{
			if (!LoadLight(carconf, "light-reverse-" + istr, error_output)) return false;

			std::stringstream sstr;
			sstr << ++i;
			istr = sstr.str();
		}
		
		// load car reverse graphics (optional)
		std::string revmodelname;
		if (carconf.GetParam("light-reverse.mesh", revmodelname))
		{
			std::vector<std::string> texname;
			if (!carconf.GetParam("light-reverse.texture", texname, error_output)) return false;
			if (!LoadInto(
				EMISSIVE, carpath+"/"+revmodelname, texname, texpath, texsize, anisotropy,
				topnode.GetNode(bodynode), textures, models, modellist, bodynode, reverselights,
				error_output)) return false;
		}
	}

	if (!LoadCameras(carconf, camerabounce, cameras, error_output)) return false;
	
	SetColor(carcolor[0], carcolor[1], carcolor[2]);
	
	mz_nominalmax = (GetTireMaxMz(FRONT_LEFT) + GetTireMaxMz(FRONT_RIGHT))*0.5;

	lookbehind = false;

	return true;
}

bool CAR::LoadPhysics(
	const CONFIGFILE & carconf,
	const std::string & carpath,
	const MATHVECTOR <float, 3> & initial_position,
	const QUATERNION <float> & initial_orientation,
	const bool defaultabs,
	const bool defaulttcs,
	MODELMANAGER & models,
	COLLISION_WORLD & world,
	std::ostream & info_output,
	std::ostream & error_output)
{
	if (!dynamics.Load(carconf, error_output)) return false;

	std::string carmodel;
	std::tr1::shared_ptr<MODEL_JOE03> modelptr;
	if (!carconf.GetParam("body.mesh", carmodel, error_output)) return false;
	if (!models.Load(carpath+"/"+carmodel, modelptr)) return false;

	typedef CARDYNAMICS::T T;
	MATHVECTOR <T, 3> size;
	MATHVECTOR <T, 3> center;
	MATHVECTOR <T, 3> position;
	QUATERNION <T> orientation;

	position = initial_position;
	orientation = initial_orientation;
	size = modelptr->GetAABB().GetSize();
	center = modelptr->GetAABB().GetCenter();
	
	// fix model rotation
	modelrotation.RotateVector(size);
	modelrotation.RotateVector(center);
	
	dynamics.Init(world, size, center, position, orientation);
	dynamics.SetABS(defaultabs);
	dynamics.SetTCS(defaulttcs);

	return true;
}

bool CAR::LoadSounds(
	const std::string & carpath,
	const std::string & carname,
	const SOUNDINFO & soundinfo,
	SOUNDMANAGER & sounds,
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
			std::tr1::shared_ptr<SOUNDBUFFER> soundptr;
			if (!aud.GetParam(*i+".filename", filename, error_output)) return false;
			if (!sounds.Load(carpath+"/"+filename, soundinfo, soundptr)) return false;

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

			sound.SetBuffer(soundptr);
			sound.Enable3D(true);
			sound.Loop(true);
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
		std::tr1::shared_ptr<SOUNDBUFFER> soundptr;
		if (!sounds.Load(carpath+"/engine", soundinfo, soundptr)) return false;
		enginesounds.push_back(std::pair <ENGINESOUNDINFO, SOUNDSOURCE> ());
		SOUNDSOURCE & enginesound = enginesounds.back().second;
		enginesound.SetBuffer(soundptr);
		enginesound.Enable3D(true);
		enginesound.Loop(true);
		enginesound.SetGain(0);
		enginesound.Play();
	}

	//set up tire squeal sounds
	for (int i = 0; i < 4; ++i)
	{
		std::tr1::shared_ptr<SOUNDBUFFER> soundptr;
		if (!sounds.Load("sounds/tire_squeal", soundinfo, soundptr)) return false;
		tiresqueal[i].SetBuffer(soundptr);
		tiresqueal[i].Enable3D(true);
		tiresqueal[i].Loop(true);
		tiresqueal[i].SetGain(0);
		int samples = tiresqueal[i].GetSoundTrack().GetSoundInfo().samples;
		tiresqueal[i].SeekToSample((samples/4)*i);
		tiresqueal[i].Play();
	}

	//set up tire gravel sounds
	for (int i = 0; i < 4; ++i)
	{
		std::tr1::shared_ptr<SOUNDBUFFER> soundptr;
		if (!sounds.Load("sounds/gravel", soundinfo, soundptr)) return false;
		gravelsound[i].SetBuffer(soundptr);
		gravelsound[i].Enable3D(true);
		gravelsound[i].Loop(true);
		gravelsound[i].SetGain(0);
		int samples = gravelsound[i].GetSoundTrack().GetSoundInfo().samples;
		gravelsound[i].SeekToSample((samples/4)*i);
		gravelsound[i].Play();
	}

	//set up tire grass sounds
	for (int i = 0; i < 4; ++i)
	{
		std::tr1::shared_ptr<SOUNDBUFFER> soundptr;
		if (!sounds.Load("sounds/grass", soundinfo, soundptr)) return false;
		grasssound[i].SetBuffer(soundptr);
		grasssound[i].Enable3D(true);
		grasssound[i].Loop(true);
		grasssound[i].SetGain(0);
		int samples = grasssound[i].GetSoundTrack().GetSoundInfo().samples;
		grasssound[i].SeekToSample((samples/4)*i);
		grasssound[i].Play();
	}

	//set up bump sounds
	for (int i = 0; i < 4; ++i)
	{
		std::tr1::shared_ptr<SOUNDBUFFER> soundptr;
		if (i >= 2)
		{
			if (!sounds.Load("sounds/bump_rear", soundinfo, soundptr)) return false;
		}
		else
		{
			if (!sounds.Load("sounds/bump_front", soundinfo, soundptr)) return false;
		}
		tirebump[i].SetBuffer(soundptr);
		tirebump[i].Enable3D(true);
		tirebump[i].Loop(false);
		tirebump[i].SetGain(1.0);
	}

	//set up crash sound
	{
		std::tr1::shared_ptr<SOUNDBUFFER> soundptr;
		if (!sounds.Load("sounds/crash", soundinfo, soundptr)) return false;
		crashsound.SetBuffer(soundptr);
		crashsound.Enable3D(true);
		crashsound.Loop(false);
		crashsound.SetGain(1.0);
	}

	//set up gear sound
	{
		std::tr1::shared_ptr<SOUNDBUFFER> soundptr;
		if (!sounds.Load("sounds/gear", soundinfo, soundptr)) return false;
		gearsound.SetBuffer(soundptr);
		gearsound.Enable3D(true);
		gearsound.Loop(false);
		gearsound.SetGain(1.0);
	}

	//set up brake sound
	{
		std::tr1::shared_ptr<SOUNDBUFFER> soundptr;
		if (!sounds.Load("sounds/brake", soundinfo, soundptr)) return false;
		brakesound.SetBuffer(soundptr);
		brakesound.Enable3D(true);
		brakesound.Loop(false);
		brakesound.SetGain(1.0);
	}

	//set up handbrake sound
	{
		std::tr1::shared_ptr<SOUNDBUFFER> soundptr;
		if (!sounds.Load("sounds/handbrake", soundinfo, soundptr)) return false;
		handbrakesound.SetBuffer(soundptr);
		handbrakesound.Enable3D(true);
		handbrakesound.Loop(false);
		handbrakesound.SetGain(1.0);
	}

	{
		std::tr1::shared_ptr<SOUNDBUFFER> soundptr;
		if (!sounds.Load("sounds/wind", soundinfo, soundptr)) return false;
		roadnoise.SetBuffer(soundptr);
		roadnoise.Enable3D(true);
		roadnoise.Loop(true);
		roadnoise.SetGain(0);
		roadnoise.SetPitch(1.0);
		roadnoise.Play();
	}

	return true;
}

void CAR::SetColor(float r, float g, float b)
{
	SCENENODE & bodynoderef = topnode.GetNode(bodynode);
	GetDrawlist(bodynoderef, NOBLEND).get(bodydraw).SetColor(r, g, b, 1);
	//std::cout << "color: " << r << ", " << g << ", " << b << std::endl;
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

void CAR::UpdateGraphics()
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
	quat = quat * modelrotation;
	bodynoderef.GetTransform().SetRotation(quat);
	
	for (int i = 0; i < 4; i++)
	{
		vec = dynamics.GetWheelPosition(WHEEL_POSITION(i));
		SCENENODE & wheelnoderef = topnode.GetNode(wheelnode[i]);
		wheelnoderef.GetTransform().SetTranslation(vec);
		tirebump[i].SetPosition(vec[0],vec[1],vec[2]);

		QUATERNION <float> wheelquat;
		wheelquat = dynamics.GetWheelOrientation(WHEEL_POSITION(i));
		wheelquat = wheelquat * modelrotation;
		wheelnoderef.GetTransform().SetRotation(wheelquat);

		if (floatingnode[i].valid())
		{
			SCENENODE & floatingnoderef = topnode.GetNode(floatingnode[i]);
			floatingnoderef.GetTransform().SetTranslation(vec);

			QUATERNION <float> floatquat;
			floatquat = dynamics.GetUprightOrientation(WHEEL_POSITION(i));
			floatquat = floatquat * modelrotation;
			floatingnoderef.GetTransform().SetRotation(floatquat);
		}
	}

	// update brake/reverse lights
	if (brakelights.valid())
	{
		GetDrawlist(bodynoderef, EMISSIVE).get(brakelights).SetDrawEnable(applied_brakes > 0);
	}
	for (std::list <LIGHT>::iterator i = lights.begin(); i != lights.end(); i++)
	{
		SCENENODE & node = bodynoderef.GetNode(i->node);
		DRAWABLE & draw = GetDrawlist(node, OMNI).get(i->draw);
		draw.SetDrawEnable(applied_brakes > 0);
	}
	if (reverselights.valid())
	{
		GetDrawlist(bodynoderef, EMISSIVE).get(reverselights).SetDrawEnable(GetGear() < 0);
	}
}

void CAR::UpdateCameras(float dt)
{
	
	MATHVECTOR <float, 3> pos = dynamics.GetPosition();
	MATHVECTOR <float, 3> acc = dynamics.GetLastBodyForce() / dynamics.GetMass();
	
	QUATERNION <float> rot;
	rot = dynamics.GetOrientation();
	
	// reverse the camera direction
	if (lookbehind)
	{
		rot.Rotate(M_PI, 0, 0, 1);
	}
	
	cameras.Active()->Update(pos, rot, acc, dt);
}

void CAR::Update(double dt)
{
	dynamics.Update();
	UpdateGraphics();
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
	float clutch = 1 - inputs[CARINPUT::CLUTCH];

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
			if (!brakesound.Audible())
			{
				float gain = 0.1;
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
			if (!handbrakesound.Audible())
			{
				float gain = 0.1;
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

		//if (i->second == loudest) std::cout << i->first->GetSoundTrack().GetName() << ": " << i->second << std::endl;
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
//			suspensionbumpdetection[i].Update(
//				dynamics.GetSuspension(WHEEL_POSITION(i)).GetVelocity(),
//				dynamics.GetSuspension(WHEEL_POSITION(i)).GetDisplacementFraction(),
//				dt);
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
			float gain = 0.0;
			if (GetEngineRPM() != 0.0)
				gain = GetEngineRPMLimit() / GetEngineRPM();
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

	double slide = dynamics.GetTire(i).GetSlide() / dynamics.GetTire(i).GetIdealSlide();
	double slip = dynamics.GetTire(i).GetSlip() / dynamics.GetTire(i).GetIdealSlip();
	double maxratio = std::max(std::abs(slide), std::abs(slip));
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
	DRAWABLE & glassdrawref = GetDrawlist(bodynoderef, BLEND).get(glassdraw);
	glassdrawref.SetDrawEnable(enable);
}

bool CAR::Serialize(joeserialize::Serializer & s)
{
	_SERIALIZE_(s,dynamics);
	_SERIALIZE_(s,last_steer);
	return true;
}
