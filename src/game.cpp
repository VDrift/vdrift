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

#include "game.h"
#include "unittest.h"
#include "definitions.h"
#include "joepack.h"
#include "matrix4.h"
#include "physics/surface.h"
#include "numprocessors.h"
#include "performance_testing.h"
#include "quickprof.h"
#include "utils.h"
#include "graphics/graphics_gl2.h"
#include "graphics/graphics_gl3v.h"
#include "cfg/ptree.h"
#include "svn_sourceforge.h"
#include "game_downloader.h"
#include "containeralgorithm.h"
#include "hsvtorgb.h"
#include "camera_orbit.h"

#include <fstream>
#include <string>
#include <map>
#include <list>
#include <iostream>
#include <sstream>
#include <algorithm>
#include <cstdio>

#if defined(unix) || defined(__unix) || defined(__unix__)
#include <GL/glx.h>
#include <SDL/SDL_syswm.h>
#endif

#ifdef _WIN32
	#define OS_NAME "Windows"
#elif defined(__APPLE__)
	#define OS_NAME "OS X"
#else
	#define OS_NAME "Unix"
#endif

#define _PRINTSIZE_(x) {std::cout << #x << ": " << sizeof(x) << std::endl;}

#define USE_STATIC_OPTIMIZATION_FOR_TRACK


template <typename T>
static std::string cast(const T &t) {
	std::ostringstream os;
	os << t;
	return os.str();
}

template <typename T>
static T cast(const std::string &str) {
	std::istringstream is(str);
	T t;
	is >> t;
	return t;
}

GAME::GAME(std::ostream & info_out, std::ostream & error_out) :
	info_output(info_out),
	error_output(error_out),
	frame(0),
	displayframe(0),
	clocktime(0),
	target_time(0),
	timestep(1/90.0),
	graphics_interface(NULL),
	enableGL3(true),
	usingGL3(false),
	content(error_out),
	carupdater(autoupdate, info_out, error_out),
	trackupdater(autoupdate, info_out, error_out),
	renderconfigfile("render.conf.deferred"),
	fps_track(10, 0),
	fps_position(0),
	fps_min(0),
	fps_max(0),
	multithreaded(false),
	profilingmode(false),
	debugmode(false),
	benchmode(false),
	dumpfps(false),
	pause(false),
	controlgrab_id(0),
	controlgrab(false),
	garage_camera("garagecam"),
	cars_name(1),
	cars_paint(1),
	cars_color_hsv(1),
	cars_ai_type(1),
	cars_ai_level(1),
	car_edit_id(0),
	active_camera(0),
	race_laps(0),
	practice(true),
	dynamics_config(timestep, 8),
	dynamics(dynamics_config),
	dynamics_drawmode(0),
	particle_timer(0),
	track(),
	replay(timestep),
	http("/tmp")
{
	carcontrols_local.first = 0;
	dynamics.setContactAddedCallback(&sim::Vehicle::WheelContactCallback);
	RegisterActions();
}

GAME::~GAME()
{
	// dtor
}

/* Start the game with the given arguments... */
void GAME::Start(std::list <std::string> & args)
{
	/*_PRINTSIZE_(graphics);
	_PRINTSIZE_(eventsystem);
	_PRINTSIZE_(sound);
	_PRINTSIZE_(generic_sounds);
	_PRINTSIZE_(settings);
	_PRINTSIZE_(pathmanager);
	_PRINTSIZE_(track);
	_PRINTSIZE_(trackmap);
	_PRINTSIZE_(gui);
	_PRINTSIZE_(rootnode);
	_PRINTSIZE_(collision);
	_PRINTSIZE_(hud);
	_PRINTSIZE_(inputgraph);
	_PRINTSIZE_(loadingscreen);
	_PRINTSIZE_(loadingscreen_node);
	_PRINTSIZE_(timer);
	_PRINTSIZE_(replay);
	_PRINTSIZE_(tire_smoke);
	_PRINTSIZE_(ai);*/

	if (!ParseArguments(args))
	{
		return;
	}

	info_output << "Starting VDrift: " << VERSION << ", Revision: " << REVISION << ", O/S: " << OS_NAME << std::endl;

	InitCoreSubsystems();

	// Load controls.
	info_output << "Loading car controls from: " << pathmanager.GetCarControlsFile() << std::endl;
	if (!carcontrols_local.second.Load(pathmanager.GetCarControlsFile(), info_output, error_output))
	{
		info_output << "Car control file " << pathmanager.GetCarControlsFile() << " doesn't exist; using defaults" << std::endl;
		carcontrols_local.second.Load(pathmanager.GetDefaultCarControlsFile(), info_output, error_output);
		carcontrols_local.second.Save(pathmanager.GetCarControlsFile());
	}

	// Init car update manager
	if (!carupdater.Init(
			pathmanager.GetUpdateManagerFileBase(),
			pathmanager.GetUpdateManagerFile(),
			pathmanager.GetUpdateManagerFileBackup(),
			"CarManager",
			pathmanager.GetCarsDir()))
	{
		// non-fatal error, just log it
		error_output << "Car update manager failed to initialize; this error is not fatal" << std::endl;
	}
	else
	{
		// send GUI value lists to the carupdater so it knows about the cars on disk
		PopulateValueLists(carupdater.GetValueLists());
	}

	// Init track update manager
	if (!trackupdater.Init(
			pathmanager.GetUpdateManagerFileBase(),
			pathmanager.GetUpdateManagerFile(),
			pathmanager.GetUpdateManagerFileBackup(),
			"TrackManager",
			pathmanager.GetTracksDir()))
	{
		// non-fatal error, just log it
		error_output << "Track update manager failed to initialize; this error is not fatal" << std::endl;
	}
	else
	{
		// send GUI value lists to the carupdater so it knows about the tracks on disk
		PopulateValueLists(trackupdater.GetValueLists());
	}

	// If sound initialization fails, that's okay, it'll disable itself...
	InitSound();

	// Load font data.
	if (!LoadFonts())
	{
		error_output << "Error loading fonts" << std::endl;
		return;
	}

	// Load loading screen assets.
	if (!loadingscreen.Init(
			pathmanager.GetGUITextureDir(settings.GetSkin()),
			window.GetW(), window.GetH(),
			content, fonts["futuresans"]))
	{
		error_output << "Error loading the loading screen" << std::endl;
		return;
	}

	// Initialize HUD.
	if (!hud.Init(pathmanager.GetGUITextureDir(settings.GetSkin()), content, fonts["lcd"], fonts["futuresans"], fonts["futuresans-noshader"], window.GetW(), window.GetH(), debugmode, error_output))
	{
		error_output << "Error initializing HUD" << std::endl;
		return;
	}
	hud.SetVisible(false);

	// Initialise input graph.
	if (!inputgraph.Init(pathmanager.GetGUITextureDir(settings.GetSkin()), content))
	{
		error_output << "Error initializing input graph" << std::endl;
		return;
	}
	inputgraph.Hide();

	// Initialize GUI.
	if (!InitGUI("Main"))
	{
		error_output << "Error initializing graphical user interface" << std::endl;
		return;
	}

	// Initialize FPS counter.
	{
		float screenhwratio = (float)window.GetH()/window.GetW();
		float w = 0.06;
		fps_draw.Init(debugnode, fonts["futuresans"], "", 0.5-w*0.5,1.0-0.02, screenhwratio*0.03,0.03);
		fps_draw.SetDrawOrder(debugnode, 150);
	}

	// Initialize profiling text.
	if (profilingmode)
	{
		float screenhwratio = (float)window.GetH()/window.GetW();
		float profilingtextsize = 0.02;
		profiling_text.Init(debugnode, fonts["futuresans"], "", 0.01, 0.25, screenhwratio*profilingtextsize, profilingtextsize);
		profiling_text.SetDrawOrder(debugnode, 150);
	}

	// Load particle systems.
	std::list <std::string> smoketexlist;
	std::string smoketexpath = pathmanager.GetDataPath()+"/"+pathmanager.GetTireSmokeTextureDir();
	pathmanager.GetFileList(smoketexpath, smoketexlist, ".png");
	if (!tire_smoke.Load(
			smoketexlist,
			pathmanager.GetTireSmokeTextureDir(),
			settings.GetAnisotropy(),
			content))
	{
		error_output << "Error loading tire smoke particle system" << std::endl;
		return;
	}
	tire_smoke.SetParameters(0.4,0.9, 1,2, 0.3,0.6, 0.02,0.06, MATHVECTOR<float,3>(0.4,0.2,1));

	// Initialize force feedback.
	forcefeedback.reset(new FORCEFEEDBACK(settings.GetFFDevice(), error_output, info_output));
	ff_update_time = 0;

	LoadGarage();

	if (benchmode && !NewGame(true))
	{
		error_output << "Error loading benchmark" << std::endl;
	}

	DoneStartingUp();

	MainLoop();

	End();
}

/* Do any necessary cleanup... */
void GAME::End()
{
	if (benchmode)
	{
		float mean_fps = displayframe / clocktime;
		info_output << "Elapsed time: " << clocktime << " seconds\n";
		info_output << "Average frame-rate: " << mean_fps << " frames per second\n";
		info_output << "Min / Max frame-rate: " << fps_min << " / " << fps_max << " frames per second" << std::endl;
	}

	if (profilingmode)
		info_output << "Profiling summary:\n" << PROFILER.getSummary(quickprof::PERCENT) << std::endl;

	info_output << "Shutting down..." << std::endl;

	LeaveGame();

	// Save settings first incase later deinits cause crashes.
	settings.Save(pathmanager.GetSettingsFile(), error_output);

	graphics_interface->Deinit();
	delete graphics_interface;
}

/* Initialize the most important, basic subsystems... */
void GAME::InitCoreSubsystems()
{
	pathmanager.Init(info_output, error_output);
	http.SetTemporaryFolder(pathmanager.GetTemporaryFolder());

	settings.Load(pathmanager.GetSettingsFile(), error_output);

	// global texture size override
	int texture_size = TEXTUREINFO::LARGE;
	if (settings.GetTextureSize() == "small")
	{
		texture_size = TEXTUREINFO::SMALL;
	}
	else if (settings.GetTextureSize() == "medium")
	{
		texture_size = TEXTUREINFO::MEDIUM;
	}

	if (!LastStartWasSuccessful())
	{
		info_output << "The last VDrift startup was unsuccessful.\nSettings have been set to failsafe defaults.\nYour original VDrift.config file was backed up to VDrift.config.backup" << std::endl;
		settings.Save(pathmanager.GetSettingsFile()+".backup", error_output);
		settings.SetFailsafeSettings();
	}
	BeginStartingUp();

	window.Init("VDrift",
		settings.GetResolutionX(), settings.GetResolutionY(),
		settings.GetBpp(),
		settings.GetShadows() ? std::max(settings.GetDepthbpp(),(unsigned int)24) : settings.GetDepthbpp(),
		settings.GetFullscreen(),
		// Explicitly disable antialiasing for the GL3 path because we're using image-based AA...
		usingGL3 ? 0 : settings.GetAntialiasing(),
		info_output, error_output);

	const int rendererCount = 2;
	for (int i = 0; i < rendererCount; i++)
	{
		// Attempt to enable the GL3 renderer...
		if (enableGL3 && i == 0 && settings.GetShaders())
		{
			graphics_interface = new GRAPHICS_GL3V(stringMap);
			usingGL3 = true;
		}
		else
		{
			graphics_interface = new GRAPHICS_GL2();
		}

		bool success = graphics_interface->Init(pathmanager.GetShaderPath(),
			settings.GetResolutionX(), settings.GetResolutionY(),
			settings.GetBpp(), settings.GetDepthbpp(), settings.GetFullscreen(),
			settings.GetShaders(), settings.GetAntialiasing(), settings.GetShadows(),
			settings.GetShadowDistance(), settings.GetShadowQuality(),
			settings.GetReflections(), pathmanager.GetStaticReflectionMap(),
			pathmanager.GetStaticAmbientMap(),
			settings.GetAnisotropic(), texture_size,
			settings.GetLighting(), settings.GetBloom(), settings.GetNormalMaps(),
			renderconfigfile,
			info_output, error_output);

		if (success)
		{
			break;
		}
		else
		{
			delete graphics_interface;
			graphics_interface = NULL;
		}
	}

	QUATERNION <float> ldir;
	ldir.Rotate(M_PI * 0.1, 0, 1, 0);
	ldir.Rotate(-M_PI * 0.2, 1, 0, 0);
	graphics_interface->SetSunDirection(ldir);

	// Init content factories
	content.getFactory<TEXTURE>().init(texture_size, usingGL3);
	content.getFactory<MODEL>().init(usingGL3);
	content.getFactory<PTree>().init(read_ini, write_ini, content);

	// Init content paths
	// Always add writeable data paths first so they are checked first
	content.addPath(pathmanager.GetWriteableDataPath());
	content.addPath(pathmanager.GetDataPath());
	content.addSharedPath(pathmanager.GetCarPartsPath());
	content.addSharedPath(pathmanager.GetTrackPartsPath());

	eventsystem.Init(info_output);
}

/* Write the scenegraph to the output drawlist... */
template <bool clearfirst>
void TraverseScene(SCENENODE & node, GRAPHICS::dynamicdrawlist_type & output)
{
	if (clearfirst)
	{
		output.clear();
	}

	MATRIX4 <float> identity;
	node.Traverse(output, identity);

	//std::cout << output.size() << std::endl;
	//std::cout << node.Nodes() << "," << node.Drawables() << std::endl;
}

bool GAME::InitGUI(const std::string & pagename)
{
	std::list <std::string> menufiles;
	std::string menufolder = pathmanager.GetGUIMenuPath(settings.GetSkin());
	if (!pathmanager.GetFileList(menufolder, menufiles))
	{
		error_output << "Error retreiving contents of folder: " << menufolder << std::endl;
		return false;
	}
	else
	{
		// Remove any pages that have ~ characters.
		std::list <std::list <std::string>::iterator> todel;
		for (std::list <std::string>::iterator i = menufiles.begin(); i != menufiles.end(); ++i)
		{
			if (i->find("~") != std::string::npos || *i == "SConscript")
			{
				todel.push_back(i);
			}
		}
		for (std::list <std::list <std::string>::iterator>::iterator i = todel.begin(); i != todel.end(); ++i)
		{
			menufiles.erase(*i);
		}
	}

	std::map<std::string, std::list <std::pair <std::string, std::string> > > valuelists;
	PopulateValueLists(valuelists);

	std::map<std::string, Slot0*> actionmap;
	InitActionMap(actionmap);

	if (!gui.Load(
			menufiles,
			valuelists,
			pathmanager.GetDataPath(),
			pathmanager.GetOptionsFile(),
			settings.GetSkin(),
			settings.GetLanguage(),
			settings.GetTextureSize(),
			(float)window.GetH() / window.GetW(),
			actionmap,
			content,
			info_output,
			error_output))
	{
		error_output << "Error loading GUI files" << std::endl;
		return false;
	}

	// Connect game actions to gui options
	BindActionsToGUI();

	// Set options from game settings.
	std::map<std::string, std::string> optionmap;
	settings.Get(optionmap);
	gui.SetOptions(optionmap);

	// Set input control labels
	LoadControlsIntoGUI();

	// Show main page.
	gui.ActivatePage(pagename, 0.5, error_output);
	if (settings.GetMouseGrab())
		window.ShowMouseCursor(true);

	return true;
}

bool GAME::InitSound()
{
	if (sound.Init(2048, info_output, error_output))
	{
		sound.SetVolume(settings.GetSoundVolume());
		content.getFactory<SOUNDBUFFER>().init(sound.GetDeviceInfo());
	}
	else
	{
		error_output << "Sound initialization failed" << std::endl;
		return false;
	}

	info_output << "Sound initialization successful" << std::endl;
	return true;
}

bool GAME::ParseArguments(std::list <std::string> & args)
{
	bool continue_game(true);

	std::map <std::string, std::string> arghelp;
	std::map <std::string, std::string> argmap;

	// Generate an argument map.
	for (std::list <std::string>::iterator i = args.begin(); i != args.end(); ++i)
	{
		if ((*i)[0] == '-')
		{
			argmap[*i] = "";
		}

		std::list <std::string>::iterator n = i;
		n++;
		if (n != args.end())
		{
			if ((*n)[0] != '-')
				argmap[*i] = *n;
		}
	}

	// Check for arguments.
	if (argmap.find("-test") != argmap.end())
	{
		Test();
		continue_game = false;
	}
	arghelp["-test"] = "Run unit tests.";

	if (argmap.find("-debug") != argmap.end())
	{
		debugmode = true;
	}
	arghelp["-debug"] = "Display car debugging information.";

	if (argmap.find("-gl2") != argmap.end())
	{
		enableGL3 = false;
	}
	else if (argmap.find("-gl3") != argmap.end())
	{
		enableGL3 = true;
	}
	arghelp["-gl2"] = "Prefer OpenGL2 rendering if supported.";
	arghelp["-gl3"] = "Prefer OpenGL3 rendering if supported.";

	if (!argmap["-cartest"].empty())
	{
		const std::string carname = argmap["-cartest"];
		const std::string cardir = pathmanager.GetCarsDir() + "/" + carname;
		pathmanager.Init(info_output, error_output);
		PERFORMANCE_TESTING perftest(dynamics);
		perftest.Test(cardir, carname, content, info_output, error_output);
		continue_game = false;
	}
	arghelp["-cartest CAR"] = "Run car performance testing on given CAR.";

	if (!argmap["-profile"].empty())
	{
		pathmanager.SetProfile(argmap["-profile"]);
	}
	arghelp["-profile NAME"] = "Store settings, controls, and records under a separate profile.";

	if (argmap.find("-profiling") != argmap.end() || argmap.find("-benchmark") != argmap.end())
	{
		PROFILER.init(20);
		profilingmode = true;
	}
	arghelp["-profiling"] = "Display game performance data.";

	if (argmap.find("-dumpfps") != argmap.end())
	{
		info_output << "Dumping the frame-rate to log." << std::endl;
		dumpfps = true;
	}
	arghelp["-dumpfps"] = "Continually dump the framerate to the log.";


	if (!argmap["-resolution"].empty())
	{
		std::string res(argmap["-resolution"]);
		std::vector <std::string> restoken = Tokenize(res, "x,");
		if (restoken.size() != 2)
		{
			error_output << "Expected resolution to be in the form 640x480" << std::endl;
			continue_game = false;
		}
		else
		{
			int xres = cast<int>(restoken[0]);
			int yres = cast<int>(restoken[1]);
			settings.SetResolution(xres, yres);
			settings.SetResolutionOverride(true);
		}
	}
	arghelp["-resolution WxH"] = "Use the specified display resolution.";

	#ifndef _WIN32
	int processors = NUMPROCESSORS::GetNumProcessors();
	if (argmap.find("-multithreaded") != argmap.end())
	{
		multithreaded = true;

		if (processors > 1)
		{
			info_output << "Multithreading enabled: " << processors << " processors" << std::endl;
			//info_output << "Note that multithreading is currently NOT RECOMMENDED for use and is likely to decrease performance significantly." << std::endl;
		}
		else
		{
			info_output << "Multithreading forced on, but only 1 processor!" << std::endl;
		}
	}
	else if (continue_game)
	{
		if (processors > 1)
			info_output << "Multi-processor system detected.  Run with -multithreaded argument to enable multithreading (EXPERIMENTAL)." << std::endl;
	}
	arghelp["-multithreaded"] = "Use multithreading where possible.";
	#endif

	if (argmap.find("-nosound") != argmap.end())
		sound.Disable();
	arghelp["-nosound"] = "Disable all sound.";

	if (argmap.find("-benchmark") != argmap.end())
	{
		info_output << "Entering benchmark mode." << std::endl;
		benchmode = true;
	}
	arghelp["-benchmark"] = "Run in benchmark mode.";

	arghelp["-render FILE"] = "Load the specified render configuration file instead of the default " + renderconfigfile + ".";
	if (!argmap["-render"].empty())
	{
		renderconfigfile = argmap["-render"];
	}

	dynamics_drawmode = 0;
	if (argmap.find("-drawaabbs") != argmap.end())
	{
		dynamics_drawmode |= btIDebugDraw::DBG_DrawAabb;
	}
	arghelp["-drawaabbs"] = "Draw collision object axis aligned bounding boxes.";

	if (argmap.find("-drawcontacts") != argmap.end())
	{
		dynamics_drawmode |= btIDebugDraw::DBG_DrawContactPoints;
	}
	arghelp["-drawcontacts"] = "Draw collision object contact normals.";

	if (argmap.find("-drawshapes") != argmap.end())
	{
		dynamics_drawmode |= btIDebugDraw::DBG_DrawWireframe;
	}
	arghelp["-drawshapes"] = "Draw collision object shape wireframes.";

	arghelp["-help"] = "Display command-line help.";
	if (argmap.find("-help") != argmap.end() || argmap.find("-h") != argmap.end() || argmap.find("--help") != argmap.end() || argmap.find("-?") != argmap.end())
	{
		std::string helpstr;
		unsigned int longest = 0;
		for (std::map <std::string, std::string>::iterator i = arghelp.begin(); i != arghelp.end(); ++i)
			if (i->first.size() > longest)
				longest = i->first.size();
		for (std::map <std::string, std::string>::iterator i = arghelp.begin(); i != arghelp.end(); ++i)
		{
			helpstr.append(i->first);
			for (unsigned int n = 0; n < longest+3-i->first.size(); n++)
				helpstr.push_back(' ');
			helpstr.append(i->second + "\n");
		}
		info_output << "Command-line help:\n\n" << helpstr << std::endl;
		continue_game = false;
	}

	return continue_game;
}

void GAME::Test()
{
	QT_RUN_TESTS;

	info_output << std::endl;
}

void GAME::BeginDraw()
{
	PROFILER.beginBlock("render");
	// Send scene information to the graphics subsystem.
	if (active_camera)
	{
		float fov = active_camera->GetFOV() > 0 ? active_camera->GetFOV() : settings.GetFOV();

		MATHVECTOR <float, 3> reflection_sample_location = active_camera->GetPosition();
		if (carcontrols_local.first)
			reflection_sample_location = carcontrols_local.first->GetPosition();

		QUATERNION <float> camlook;
		camlook.Rotate(M_PI_2, 1, 0, 0);
		QUATERNION <float> camorient = -(active_camera->GetOrientation() * camlook);
		graphics_interface->SetupScene(fov, settings.GetViewDistance(), active_camera->GetPosition(), camorient, reflection_sample_location);
	}
	else
		graphics_interface->SetupScene(settings.GetFOV(), settings.GetViewDistance(), MATHVECTOR <float, 3> (), QUATERNION <float> (), MATHVECTOR <float, 3> ());

	graphics_interface->SetContrast(settings.GetContrast());
	graphics_interface->BeginScene(error_output);
	PROFILER.endBlock("render");

	PROFILER.beginBlock("scenegraph");

	TraverseScene<true>(debugnode, graphics_interface->GetDynamicDrawlist());
	TraverseScene<false>(gui.GetNode(), graphics_interface->GetDynamicDrawlist());
	TraverseScene<false>(track.GetRacinglineNode(), graphics_interface->GetDynamicDrawlist());
	TraverseScene<false>(dynamicsdraw.getNode(), graphics_interface->GetDynamicDrawlist());
#ifndef USE_STATIC_OPTIMIZATION_FOR_TRACK
	TraverseScene<false>(track.GetTrackNode(), graphics_interface->GetDynamicDrawlist());
#endif
	TraverseScene<false>(track.GetBodyNode(), graphics_interface->GetDynamicDrawlist());
	TraverseScene<false>(hud.GetNode(), graphics_interface->GetDynamicDrawlist());
	TraverseScene<false>(trackmap.GetNode(), graphics_interface->GetDynamicDrawlist());
	TraverseScene<false>(inputgraph.GetNode(), graphics_interface->GetDynamicDrawlist());
	TraverseScene<false>(tire_smoke.GetNode(), graphics_interface->GetDynamicDrawlist());
	for (std::list <CAR>::iterator i = cars.begin(); i != cars.end(); ++i)
	{
		TraverseScene<false>(i->GetNode(), graphics_interface->GetDynamicDrawlist());
	}

	//gui.GetNode().DebugPrint(info_output);
	PROFILER.endBlock("scenegraph");
	PROFILER.beginBlock("render");
	graphics_interface->DrawScene(error_output);
	PROFILER.endBlock("render");
}

void GAME::FinishDraw()
{
	PROFILER.beginBlock("render");
	graphics_interface->EndScene(error_output);
	window.SwapBuffers();
	PROFILER.endBlock("render");
}

/* The main game loop... */
void GAME::MainLoop()
{
	while (!eventsystem.GetQuit() && (!benchmode || replay.GetPlaying()))
	{
		CalculateFPS();

		clocktime += eventsystem.Get_dt();

		eventsystem.BeginFrame();

		// Do CPU intensive stuff in parallel with the GPU...
		Tick(eventsystem.Get_dt());

		gui.Update(eventsystem.Get_dt());

		// Sync CPU and GPU (flip the page).
		FinishDraw();
		BeginDraw();

		eventsystem.EndFrame();

		PROFILER.endCycle();

		displayframe++;
	}
}

/* Deltat is in seconds... */
void GAME::Tick(float deltat)
{
	// This is the minimum fps the game will run at before it starts slowing down time.
	const float minfps = 10.0f;
	// Slow the game down if we can't process fast enough.
	const unsigned int maxticks = (int) (1.0f / (minfps * timestep));
	// Slow the game down if we can't process fast enough.
	const float maxtime = 1.0 / minfps;
	unsigned int curticks = 0;

	// Throw away wall clock time if necessary to keep the framerate above the minimum.
	if (deltat > maxtime)
        deltat = maxtime;

	target_time += deltat;

	http.Tick();

	// Increment game logic by however many tick periods have passed since the last GAME::Tick...
	while (target_time - TickPeriod() * frame > TickPeriod() && curticks < maxticks)
	{
		frame++;

		AdvanceGameLogic();

		curticks++;
	}

	// Debug draw dynamics
	if (dynamics_drawmode && track.Loaded())
	{
		dynamicsdraw.clear();
		dynamics.debugDrawWorld();
	}

	if (dumpfps && curticks > 0 && frame % 100 == 0)
	{
		info_output << "Current FPS: " << eventsystem.GetFPS() << std::endl;
	}
}

/* Increment game logic by one frame... */
void GAME::AdvanceGameLogic()
{
	//PROFILER.beginBlock("input-processing");

	eventsystem.ProcessEvents();

	float last_steer = 0;
	float car_speed = 0;
	if (carcontrols_local.first)
	{
		last_steer = carcontrols_local.first->GetLastSteer();
		car_speed = carcontrols_local.first->GetSpeed();
	}
	carcontrols_local.second.ProcessInput(
			settings.GetJoyType(),
			eventsystem,
			last_steer,
			TickPeriod(),
			settings.GetJoy200(),
			car_speed,
			settings.GetSpeedSensitivity(),
			window.GetW(),
			window.GetH(),
			settings.GetButtonRamp(),
			settings.GetHGateShifter());

	ProcessGUIInputs();

	ProcessGameInputs();

	//PROFILER.endBlock("input-processing");

	if (track.Loaded() && !pause && !gui.Active())
	{
		PROFILER.beginBlock("ai");
		ai.Visualize();
		ai.update(TickPeriod(), cars);
		PROFILER.endBlock("ai");

		PROFILER.beginBlock("physics");
		dynamics.update(TickPeriod());
		PROFILER.endBlock("physics");

		PROFILER.beginBlock("car");
		for (std::list <CAR>::iterator i = cars.begin(); i != cars.end(); ++i)
		{
			UpdateCar(*i, TickPeriod());
		}
		PROFILER.endBlock("car");

		// Update dynamic track objects.
		track.Update();

		//PROFILER.beginBlock("timer");
		UpdateTimer();
		//PROFILER.endBlock("timer");

		//PROFILER.beginBlock("particles");
		UpdateParticleSystems(TickPeriod());
		//PROFILER.endBlock("particles");

		//PROFILER.beginBlock("trackmap-update");
		UpdateTrackMap();
		//PROFILER.endBlock("trackmap-update");
	}

	if (sound.Enabled())
	{
		bool pause_sound = pause || gui.Active();
		PROFILER.beginBlock("sound");
		MATHVECTOR <float, 3> pos;
		QUATERNION <float> rot;
		if (active_camera)
		{
			pos = active_camera->GetPosition();
			rot = active_camera->GetOrientation();
		}
		sound.SetListenerPosition(pos[0], pos[1], pos[2]);
		sound.SetListenerRotation(rot[0], rot[1], rot[2], rot[3]);
		sound.Update(pause_sound);
		PROFILER.endBlock("sound");
	}

	//PROFILER.beginBlock("force-feedback");
	UpdateForceFeedback(TickPeriod());
	//PROFILER.endBlock("force-feedback");
}

/* Process inputs used only for higher level game functions... */
void GAME::ProcessGameInputs()
{
	// Most game inputs are allowed whether or not there's a car in the game.
	if (carcontrols_local.second.GetInput(CARINPUT::SCREENSHOT) == 1.0)
	{
		// Determine filename.
		std::string shotfile;
		for (int i = 1; i < 999; i++)
		{
			std::stringstream s;
			s << pathmanager.GetScreenshotPath() << "/shot";
			s.width(3);
			s.fill('0');
			s << i << ".bmp";
			if (!pathmanager.FileExists(s.str()))
			{
				shotfile = s.str();
				break;
			}
		}
		if (!shotfile.empty())
		{
			info_output << "Capturing screenshot to " << shotfile << std::endl;
			window.Screenshot(shotfile);
		}
		else
			error_output << "Couldn't find a file to which to save the captured screenshot" << std::endl;
	}

	if (carcontrols_local.second.GetInput(CARINPUT::RELOAD_SHADERS) == 1.0)
	{
		info_output << "Reloading shaders" << std::endl;
		if (!graphics_interface->ReloadShaders(pathmanager.GetShaderPath(), info_output, error_output))
		{
			error_output << "Error reloading shaders" << std::endl;
		}
	}

	if (carcontrols_local.second.GetInput(CARINPUT::RELOAD_GUI) == 1.0)
	{
		info_output << "Reloading GUI" << std::endl;

		// First, save the active page name so we can get back to in...
		std::string currentPage = gui.GetActivePageName();

		// Reload GUI
		if (!InitGUI(currentPage))
		{
			error_output << "Error reloading GUI" << std::endl;
		}
	}

	// Some game inputs are only allowed when there's a car in the game.
	if (carcontrols_local.first)
	{
		if (carcontrols_local.second.GetInput(CARINPUT::PAUSE) == 1.0)
		{
			pause = !pause;
		}
	}
}

void GAME::UpdateTimer()
{
	// Check for cars doing a lap.
	for (std::list <CAR>::iterator i = cars.begin(); i != cars.end(); ++i)
	{
		int carid = cartimerids[&(*i)];

		bool advance = false;
		int nextsector = 0;
		if (track.GetSectors() > 0)
		{
			nextsector = (i->GetSector() + 1) % track.GetSectors();
			//cout << "next " << nextsector << ", cur " << i->GetSector() << ", track " << track.GetSectors() << std::endl;
			for (int p = 0; p < 4; ++p)
			{
				if (i->GetCurPatch(p) == track.GetSectorPatch(nextsector))
				{
					advance = true;
					//info_output << "New sector " << nextsector << "/" << track.GetSectors();
					//info_output << " patch " << i->GetCurPatch(p) << std::endl;
					//info_output <<  ", " << track.GetSectorPatch(nextsector) << std::endl;
				}
				//else cout << p << ". " << i->GetCurPatch(p) << ", " << track.GetSectorPatch(nextsector) << std::endl;
			}
		}

		if (advance)
		{
			// Only count it if the car's current sector isn't -1 which is the default value when the car is loaded...
			timer.Lap(carid, nextsector, (i->GetSector() >= 0));
			i->SetSector(nextsector);
		}

		// Update how far the car is on the track...
		// Find the patch under the front left wheel...
		const BEZIER * curpatch = i->GetCurPatch(0);
		if (!curpatch)
			// Try the other wheel...
			curpatch = i->GetCurPatch(1);

		// Only update if car is on track.
		if (curpatch)
		{
			MATHVECTOR <float, 3> pos = i->GetCenterOfMassPosition();
			MATHVECTOR <float, 3> back_left, back_right, front_left;

			if (!track.IsReversed())
			{
				back_left = MATHVECTOR <float, 3> (curpatch->GetBL()[2], curpatch->GetBL()[0], curpatch->GetBL()[1]);
				back_right = MATHVECTOR <float, 3> (curpatch->GetBR()[2], curpatch->GetBR()[0], curpatch->GetBR()[1]);
				front_left = MATHVECTOR <float, 3> (curpatch->GetFL()[2], curpatch->GetFL()[0], curpatch->GetFL()[1]);
			}
			else
			{
				back_left = MATHVECTOR <float, 3> (curpatch->GetFL()[2], curpatch->GetFL()[0], curpatch->GetFL()[1]);
				back_right = MATHVECTOR <float, 3> (curpatch->GetFR()[2], curpatch->GetFR()[0], curpatch->GetFR()[1]);
				front_left = MATHVECTOR <float, 3> (curpatch->GetBL()[2], curpatch->GetBL()[0], curpatch->GetBL()[1]);
			}

			MATHVECTOR <float, 3> forwardvec = front_left - back_left;
			MATHVECTOR <float, 3> relative_pos = pos - back_left;
			float dist_from_back = 0;

			if (forwardvec.Magnitude() > 0.0001)
				dist_from_back = relative_pos.dot(forwardvec.Normalize());

			timer.UpdateDistance(carid, curpatch->GetDistFromStart() + dist_from_back);
			//std::cout << curpatch->GetDistFromStart() << ", " << dist_from_back << std::endl;
			//std::cout << curpatch->GetDistFromStart() + dist_from_back << std::endl;
		}

		/*info_output << "sector=" << i->GetSector() << ", next=" << track.GetSectorPatch(nextsector) << ", ";
		for (int w = 0; w < 4; w++)
		{
			info_output << w << "=" << i->GetCurPatch(0) << ", ";
		}
		info_output << std::endl;*/
	}

	timer.Tick(TickPeriod());
	//timer.DebugPrint(info_output);
}

void GAME::UpdateTrackMap()
{
	std::list <std::pair<MATHVECTOR <float, 3>, bool> > carpositions;
	for (std::list <CAR>::iterator i = cars.begin(); i != cars.end(); ++i)
	{
		bool playercar = (carcontrols_local.first == &(*i));
		carpositions.push_back(std::make_pair(i->GetCenterOfMassPosition(), playercar));
	}

	trackmap.Update(settings.GetTrackmap(), carpositions);
}

void GAME::ProcessGUIInputs()
{
	if (!gui.Active())
	{
		// Handle the ESCAPE key with dedicated logic...
		if (eventsystem.GetKeyState(SDLK_ESCAPE).just_up)
		{
			// Show in-game GUI
			ShowHUD(false);
			gui.ActivatePage("InGameMain", 0.25, error_output);
			if (settings.GetMouseGrab())
				window.ShowMouseCursor(true);
		}
		return;
	}

	if (controlgrab)
	{
		// Handle control assignment
		if (AssignControl())
		{
			controlgrab = false;
			EditControl();
		}
		return;
	}

	gui.ProcessInput(
		eventsystem.GetMousePosition()[0] / (float)window.GetW(),
		eventsystem.GetMousePosition()[1] / (float)window.GetH(),
		eventsystem.GetMouseButtonState(1).down,
		eventsystem.GetMouseButtonState(1).just_up,
		carcontrols_local.second.GetInput(CARINPUT::GUI_LEFT),
		carcontrols_local.second.GetInput(CARINPUT::GUI_RIGHT),
		carcontrols_local.second.GetInput(CARINPUT::GUI_UP),
		carcontrols_local.second.GetInput(CARINPUT::GUI_DOWN),
		carcontrols_local.second.GetInput(CARINPUT::GUI_SELECT),
		carcontrols_local.second.GetInput(CARINPUT::GUI_CANCEL));
}

bool GAME::AssignControl()
{
	// Check for key inputs.
	std::map <SDLKey, TOGGLE> & keymap = eventsystem.GetKeyMap();
	for (std::map <SDLKey, TOGGLE>::iterator i = keymap.begin(); i != keymap.end(); ++i)
	{
		if (i->second.GetImpulseRising())
		{
			controlgrab_control.type = CARCONTROLMAP::CONTROL::KEY;
			controlgrab_control.keycode = i->first;
			carcontrols_local.second.SetControl(controlgrab_input, controlgrab_id, controlgrab_control);
			return true;
		}
	}

	// Check for joystick inputs.
	for (int j = 0; j < eventsystem.GetNumJoysticks(); ++j)
	{
		// Check for joystick buttons.
		for (int i = 0; i < eventsystem.GetNumButtons(j); ++i)
		{
			if (eventsystem.GetJoyButton(j, i).GetImpulseRising())
			{
				controlgrab_control.type = CARCONTROLMAP::CONTROL::JOY;
				controlgrab_control.joynum = j;
				controlgrab_control.joytype = CARCONTROLMAP::CONTROL::JOYBUTTON;
				controlgrab_control.keycode = i;
				carcontrols_local.second.SetControl(controlgrab_input, controlgrab_id, controlgrab_control);
				return true;
			}
		}

		// Check for joystick axes.
		for (int i = 0; i < eventsystem.GetNumAxes(j); ++i)
		{
			assert(j < (int)controlgrab_joystick_state.size());
			assert(i < controlgrab_joystick_state[j].GetNumAxes());

			float delta = eventsystem.GetJoyAxis(j, i) - controlgrab_joystick_state[j].GetAxis(i);
			int axis = 0;
			if (delta > 0.4) axis = 1;
			if (delta < -0.4) axis = -1;
			if (axis)
			{
				controlgrab_control.type = CARCONTROLMAP::CONTROL::JOY;
				controlgrab_control.joytype = CARCONTROLMAP::CONTROL::JOYAXIS;
				controlgrab_control.joynum = j;
				controlgrab_control.joyaxis = i;
				if (axis > 0)
					controlgrab_control.joyaxistype = CARCONTROLMAP::CONTROL::POSITIVE;
				else if (axis < 0)
					controlgrab_control.joyaxistype = CARCONTROLMAP::CONTROL::NEGATIVE;
				carcontrols_local.second.SetControl(controlgrab_input, controlgrab_id, controlgrab_control);
				return true;
			}
		}
	}

	// Check for mouse button inputs.
	for (int i = 1; i < 4; ++i)
	{
		if (eventsystem.GetMouseButtonState(i).just_down)
		{
			controlgrab_control.type = CARCONTROLMAP::CONTROL::MOUSE;
			controlgrab_control.mousetype = CARCONTROLMAP::CONTROL::MOUSEBUTTON;
			controlgrab_control.keycode = i;
			carcontrols_local.second.SetControl(controlgrab_input, controlgrab_id, controlgrab_control);
			return true;
		}
	}

	// Check for mouse motion inputs.
	int dx = eventsystem.GetMousePosition()[0] - controlgrab_mouse_coords.first;
	int dy = eventsystem.GetMousePosition()[1] - controlgrab_mouse_coords.second;
	int threshold = 200;
	if (dx < -threshold || dx > threshold || dy < -threshold || dy > threshold)
	{
		controlgrab_control.type = CARCONTROLMAP::CONTROL::MOUSE;
		controlgrab_control.mousetype = CARCONTROLMAP::CONTROL::MOUSEMOTION;
		if (dx < -threshold)
			controlgrab_control.mdir = CARCONTROLMAP::CONTROL::LEFT;
		else if (dx > threshold)
			controlgrab_control.mdir = CARCONTROLMAP::CONTROL::RIGHT;
		else if (dy < -threshold)
			controlgrab_control.mdir = CARCONTROLMAP::CONTROL::UP;
		else if (dy > threshold)
			controlgrab_control.mdir = CARCONTROLMAP::CONTROL::DOWN;
		carcontrols_local.second.SetControl(controlgrab_input, controlgrab_id, controlgrab_control);
		return true;
	}

	return false;
}

void GAME::LoadControlsIntoGUIPage()
{
	std::map<std::string, std::string> label_text;
	carcontrols_local.second.GetControlsInfo(label_text);
	gui.SetLabelText(gui.GetActivePageName(), label_text);
}

void GAME::LoadControlsIntoGUI()
{
	std::map<std::string, std::string> label_text;
	carcontrols_local.second.GetControlsInfo(label_text);
	gui.SetLabelText(label_text);
}

void GAME::UpdateStartList(unsigned i, const std::string & value)
{
	std::stringstream s;
	s << "Racer" << i;
	gui.SetLabelText("SingleRace", s.str(), value);
}

void GAME::UpdateCar(CAR & car, double dt)
{
	car.Update(dt);
	UpdateCarInputs(car);
	AddTireSmokeParticles(dt, car);
	UpdateDriftScore(car, dt);
}

void GAME::UpdateCarInputs(CAR & car)
{
	std::vector <float> carinputs(CARINPUT::INVALID, 0.0f);
	if (carcontrols_local.first == &car)
	{
		if (replay.GetPlaying())
		{
			const std::vector <float> & inputarray = replay.PlayFrame(car);
			assert(inputarray.size() <= carinputs.size());
			for (size_t i = 0; i < inputarray.size(); ++i)
			{
				carinputs[i] = inputarray[i];
			}
		}
		else
		{
			carinputs = carcontrols_local.second.GetInputs();

#ifdef VISUALIZE_AI_DEBUG
			// It allows to activate the AI on the player car with F9 button.
			// AI will override player inputs.
			// This is useful for bringing the car in strange
			// situations and test how the AI solves it.
			static bool aiControlled = false;
			static bool buttonPressed = false;
			if (buttonPressed != eventsystem.GetKeyState(SDLK_F9).just_down)
			{
				buttonPressed = !buttonPressed;
				if (buttonPressed)
				{
					aiControlled = !aiControlled;
					if (aiControlled)
					{
						info_output << "Switching to AI controlled player." << std::endl;
						ai.add_car(&car, 1.0);
					}
					else
					{
						info_output << "Switching to user controlled player." << std::endl;
						ai.remove_car(&car);
					}
				}
			}
			if(aiControlled)
			{
				carinputs = ai.GetInputs(&car);
				assert(carinputs.size() == CARINPUT::INVALID);
			}
#endif
		}
	}
	else
	{
		carinputs = ai.GetInputs(&car);
		assert(carinputs.size() == CARINPUT::INVALID);
	}

	// Force brake and clutch during staging and once the race is over.
	if (timer.Staging() || ((int)timer.GetCurrentLap(cartimerids[&car]) > race_laps && race_laps > 0))
	{
		carinputs[CARINPUT::BRAKE] = 1.0;
	}

	car.ProcessInputs(carinputs);

	if (carcontrols_local.first != &car)
		return;

	if (replay.GetRecording())
		replay.RecordFrame(carinputs, car);

	inputgraph.Update(carinputs);

	std::stringstream debug_info1, debug_info2, debug_info3, debug_info4;
	if (debugmode)
	{
		car.DebugPrint(debug_info1, true, false, false, false);
		car.DebugPrint(debug_info2, false, true, false, false);
		car.DebugPrint(debug_info3, false, false, true, false);
		car.DebugPrint(debug_info4, false, false, false, true);
	}

	std::pair <int, int> curplace = timer.GetPlayerPlace();
	int tid = cartimerids[&car];
	hud.Update(
		fonts["lcd"], fonts["futuresans"], fonts["futuresans-noshader"], window.GetW(), window.GetH(),
		timer.GetPlayerTime(), timer.GetLastLap(), timer.GetBestLap(), timer.GetStagingTimeLeft(),
		timer.GetPlayerCurrentLap(), race_laps, curplace.first, curplace.second,
		car.GetEngineRPM(), car.GetEngineRedline(), car.GetEngineRPMLimit(),
		car.GetSpeedMPS(), car.GetMaxSpeedMPS(), settings.GetMPH(), car.GetClutch(), car.GetGear(),
		debug_info1.str(), debug_info2.str(), debug_info3.str(), debug_info4.str(),
		car.GetABSEnabled(), car.GetABSActive(), car.GetTCSEnabled(), car.GetTCSActive(),
		car.GetFuelAmount() > 1E-5, car.GetNosActive(), car.GetNosAmount(),
		timer.GetIsDrifting(tid), timer.GetDriftScore(tid), timer.GetThisDriftScore(tid));

	// Handle camera mode change inputs.
	CAMERA * old_camera = active_camera;
	CARCONTROLMAP & carcontrol = carcontrols_local.second;
	unsigned camera_id = settings.GetCamera();
	if (carcontrol.GetInput(CARINPUT::VIEW_HOOD))
	{
		camera_id = 0;
	}
	else if (carcontrol.GetInput(CARINPUT::VIEW_INCAR))
	{
		camera_id = 1;
	}
	else if (carcontrol.GetInput(CARINPUT::VIEW_CHASERIGID))
	{
		camera_id = 2;
	}
	else if (carcontrol.GetInput(CARINPUT::VIEW_CHASE))
	{
		camera_id = 3;
	}
	else if (carcontrol.GetInput(CARINPUT::VIEW_ORBIT))
	{
		camera_id = 4;
	}
	else if (carcontrol.GetInput(CARINPUT::VIEW_FREE))
	{
		camera_id = 5;
	}
	else if (carcontrol.GetInput(CARINPUT::VIEW_NEXT))
	{
		++camera_id;
	}
	else if (carcontrol.GetInput(CARINPUT::VIEW_PREV))
	{
		--camera_id;
	}
	if (camera_id > car.GetCameras().size())
	{
		camera_id = car.GetCameras().size() - 1;
	}
	else if (camera_id == car.GetCameras().size())
	{
		camera_id = 0;
	}
	active_camera = car.GetCameras()[camera_id];
	settings.SetCamera(camera_id);
	bool incar = (camera_id == 0 || camera_id == 1);

	MATHVECTOR<float, 3> pos = car.GetPosition();
	QUATERNION<float> rot = car.GetOrientation();
	if (carcontrol.GetInput(CARINPUT::VIEW_REAR))
	{
		rot.Rotate(M_PI, 0, 0, 1);
	}
	if (old_camera != active_camera)
	{
		active_camera->Reset(pos, rot);
	}
	else
	{
		active_camera->Update(pos, rot, TickPeriod());
	}

	// Handle camera inputs.
	float left = TickPeriod() * (carcontrol.GetInput(CARINPUT::PAN_LEFT) - carcontrol.GetInput(CARINPUT::PAN_RIGHT));
	float up = TickPeriod() * (carcontrol.GetInput(CARINPUT::PAN_UP) - carcontrol.GetInput(CARINPUT::PAN_DOWN));
	float dy = TickPeriod() * (carcontrol.GetInput(CARINPUT::ZOOM_IN) - carcontrol.GetInput(CARINPUT::ZOOM_OUT));
	MATHVECTOR<float, 3> zoom(direction::Forward * 4 * dy);
	active_camera->Rotate(up, left);
	active_camera->Move(zoom[0], zoom[1], zoom[2]);

	// Hide glass if we're inside the car, adjust sounds.
	car.SetInteriorView(incar);

	// Move up the close shadow distance if we're in the cockpit.
	graphics_interface->SetCloseShadow(incar ? 1.0 : 5.0);
}

bool GAME::NewGame(bool playreplay, bool addopponents, int num_laps)
{
	// This should clear out all data.
	LeaveGame();

	// Cache number of laps for gui.
	race_laps = num_laps;

	// Start out with no camera.
	active_camera = NULL;

	// Set track, car config file.
	std::string trackname = settings.GetTrack();
	std::string carfile;

	if (playreplay)
	{
		// Load replay.
		std::string replayfilename = pathmanager.GetReplayPath();
		if (benchmode)
			replayfilename += "/benchmark.vdr";
		else
			replayfilename += "/" + settings.GetSelectedReplay();

		info_output << "Loading replay file: " << replayfilename << std::endl;

		if (!replay.StartPlaying(replayfilename, error_output))
			return false;

		trackname = replay.GetTrack();
		carfile = replay.GetCarFile();
		cars_name[0] = replay.GetCarType();
		cars_paint[0] = replay.GetCarPaint();
		cars_color_hsv[0] = replay.GetCarColorHSV();
	}

	// Load track.
	if (!LoadTrack(trackname))
	{
		error_output << "Error during track loading: " << trackname << std::endl;
		return false;
	}

	// Load cars.
	size_t cars_num = (addopponents) ? cars_name.size() : 1;
	for (size_t i = 0; i < cars_num; ++i)
	{
		bool isai = (i > 0);

		if (!LoadCar(cars_name[i], cars_paint[i], cars_color_hsv[i],
			track.GetStart(i).first, track.GetStart(i).second, !isai, isai, carfile))
			return false;

		if (isai)
			ai.add_car(&cars.back(), cars_ai_level[i], cars_ai_type[i]);
		else
			carfile.clear();
	}

	// Load timer.
	float pretime = (num_laps > 0) ? 3.0f : 0.0f;
	if (!timer.Load(pathmanager.GetTrackRecordsPath()+"/"+trackname+".txt", pretime))
	{
		error_output << "Unable to load timer" << std::endl;
		return false;
	}

	// Add cars to the timer system.
	int count = 0;
	for (std::list<CAR>::iterator i = cars.begin(); i != cars.end(); ++i)
	{
		cartimerids[&(*i)] = timer.AddCar(i->GetCarType());
		if (carcontrols_local.first == &(*i))
			timer.SetPlayerCarID(count);
		count++;
	}

	// Set up GUI.
	gui.SetInGame(true);
	gui.Deactivate();
	ShowHUD(true);
	if (settings.GetMouseGrab())
		window.ShowMouseCursor(false);

	// Record a replay.
	if (settings.GetRecordReplay() && !playreplay)
	{
		assert(carcontrols_local.first);
		const std::string cartype = carcontrols_local.first->GetCarType();
		const std::string cardir = pathmanager.GetCarsDir() + "/" + cars_name[0];

		std::tr1::shared_ptr<PTree> carconfig;
		content.load(carconfig, cardir, cars_name[0] + ".car");

		replay.StartRecording(
			cartype,
			cars_paint[0],
			cars_color_hsv[0],
			*carconfig,
			settings.GetTrack(),
			error_output);
	}

	content.sweep();
	return true;
}

std::string GAME::GetReplayRecordingFilename()
{
	// Get time.
	time_t curtime = time(0);
	tm now = *localtime(&curtime);

	// Time string.
	char timestr[]= "MM-DD-hh-mm";
	const char format[] = "%m-%d-%H-%M";
	strftime(timestr, sizeof(timestr), format, &now);

	// Replay file name.
	std::stringstream s;
	s << pathmanager.GetReplayPath() << "/" << timestr << "-" << settings.GetTrack() << ".vdr";
	return s.str();
}

bool GAME::LoadCar(
	const std::string & car_name,
	const std::string & car_paint,
	const MATHVECTOR <float, 3> & car_color_hsv,
	const MATHVECTOR <float, 3> & start_position,
	const QUATERNION <float> & start_orientation,
	bool islocal, bool isai,
	const std::string & carfile)
{
	if (islocal)
	{
		// Reset local car pointer.
		carcontrols_local.first = 0;
	}

	std::string car_dir = pathmanager.GetCarsDir() + "/" + car_name;
	std::tr1::shared_ptr<PTree> carconf;
	if (carfile.empty())
	{
		content.load(carconf, car_dir, car_name + ".car");
		if (!carconf->size())
		{
			error_output << "Failed to load " << car_name << std::endl;
			return false;
		}
	}
	else
	{
		carconf.reset(new PTree());
		std::stringstream carstream(carfile);
		read_ini(carstream, *carconf);
	}

	MATHVECTOR<float, 3> car_color_rgb;
	HSVtoRGB(
		car_color_hsv[0], car_color_hsv[1], car_color_hsv[2],
		car_color_rgb[0], car_color_rgb[1], car_color_rgb[2]);

	cars.push_back(CAR());
	CAR & car = cars.back();
	if (!car.LoadGraphics(
		*carconf, car_dir, car_name, car_paint, car_color_rgb,
		settings.GetAnisotropy(), settings.GetCameraBounce(),
		content, error_output))
	{
		error_output << "Error loading car: " << car_name << std::endl;
		cars.pop_back();
		return false;
	}

	if (sound.Enabled() && !car.LoadSounds(
		*carconf, car_dir, car_name, sound,
		content, error_output))
	{
		error_output << "Failed to load sounds for car " << car_name << std::endl;
		return false;
	}

	if (!car.LoadPhysics(
		*carconf, car_dir, start_position, start_orientation,
		settings.GetABS() || isai, settings.GetTCS() || isai,
		settings.GetVehicleDamage(), dynamics,
        content, error_output))
	{
		error_output << "Failed to load physics for car " << car_name << std::endl;
		return false;
	}

	info_output << "Car loading was successful: " << car_name << std::endl;
	if (islocal)
	{
		// Load local controls.
		carcontrols_local.first = &cars.back();

		// Setup auto clutch and auto shift.
		ProcessNewSettings();

		// Shift into first gear if autoshift enabled.
		if (carcontrols_local.first && settings.GetAutoShift())
			carcontrols_local.first->SetGear(1);
	}

	return true;
}

bool GAME::LoadTrack(const std::string & trackname)
{
	LoadingScreen(0.0, 1.0, false, "", 0.5, 0.5);

	if (!track.DeferredLoad(
			content, dynamics,
			info_output, error_output,
			pathmanager.GetTracksPath(trackname),
			pathmanager.GetTracksDir()+"/"+trackname,
			pathmanager.GetEffectsTextureDir(),
			pathmanager.GetTrackPartsPath(),
			settings.GetAnisotropy(),
			settings.GetTrackReverse(),
			settings.GetTrackDynamic(),
			graphics_interface->GetShadows(),
			settings.GetBatchGeometry()))
	{
		error_output << "Error loading track: " << trackname << std::endl;
		return false;
	}

	bool success = true;
	int count = 0;
	while (!track.Loaded() && success)
	{
		int displayevery = track.ObjectsNum() / 50;
		if (displayevery == 0 || count % displayevery == 0)
		{
			LoadingScreen(count, track.ObjectsNum(), false, "", 0.5, 0.5);
		}
		success = track.ContinueDeferredLoad();
		count++;
	}

	if (!success)
	{
		error_output << "Error loading track (deferred): " << trackname << std::endl;
		return false;
	}

	// Set racing line visibility.
	track.SetRacingLineVisibility(settings.GetRacingline());

	// Generate the track map.
	if (!trackmap.BuildMap(
			track.GetRoadList(),
			window.GetW(),
			window.GetH(),
			trackname,
			pathmanager.GetHUDTextureDir(),
			content,
			error_output))
	{
		error_output << "Error loading track map: " << trackname << std::endl;
		return false;
	}

	// Set dynamics debug draw mode
	if (dynamics_drawmode)
	{
		dynamicsdraw.setDebugMode(dynamics_drawmode);
		dynamics.setDebugDrawer(&dynamicsdraw);
	}
	else
	{
		dynamics.setDebugDrawer(0);
	}

	// Build static drawlist.
#ifdef USE_STATIC_OPTIMIZATION_FOR_TRACK
	graphics_interface->AddStaticNode(track.GetTrackNode());
#endif

	return true;
}

void GAME::LoadGarage()
{
	LeaveGame();

	// Load track explicitly to avoid track reversed car orientation issue.
	// Proper fix would be to support reversed car orientation in garage.

	LoadingScreen(0.0, 1.0, false, "", 0.5, 0.5);

	std::string trackname = "garage";
	bool track_reverse = false;
	bool track_dynamic = false;
	if (!track.DeferredLoad(
			content, dynamics,
			info_output, error_output,
			pathmanager.GetTracksPath(trackname),
			pathmanager.GetTracksDir() + "/" + trackname,
			pathmanager.GetEffectsTextureDir(),
			pathmanager.GetTrackPartsPath(),
			settings.GetAnisotropy(),
			track_reverse, track_dynamic,
			graphics_interface->GetShadows(),
			settings.GetBatchGeometry()))
	{
		error_output << "Error loading track: " << trackname << std::endl;
		return;
	}

	bool success = true;
	int count = 0;
	while (!track.Loaded() && success)
	{
		int displayevery = track.ObjectsNum() / 50;
		if (displayevery == 0 || count % displayevery == 0)
		{
			LoadingScreen(count, track.ObjectsNum(), false, "", 0.5, 0.5);
		}
		success = track.ContinueDeferredLoad();
		count++;
	}

	if (!success)
	{
		error_output << "Error loading track (deferred): " << trackname << std::endl;
		return;
	}

	// Build static drawlist.
#ifdef USE_STATIC_OPTIMIZATION_FOR_TRACK
	graphics_interface->AddStaticNode(track.GetTrackNode());
#endif

	// Load car.
	SetGarageCar();
}

void GAME::SetGarageCar()
{
	if (gui.GetInGame() || !track.Loaded())
		return;

	// get car start position marker for camera setup
	MATHVECTOR<float, 3> car_pos = track.GetStart(0).first;

	// clear previous car
	cars.clear();

	// remove previous car sounds
	sound.Update(true);

	if (LoadCar(
		cars_name[car_edit_id],
		cars_paint[car_edit_id],
		cars_color_hsv[car_edit_id],
		car_pos, track.GetStart(0).second,
		true, false))
	{
		// set car
		CAR & car = cars.back();
		dynamics.update(timestep);
		car.Update(timestep);

		// add car sounds
		sound.Update(true);

		// use car shape center for camera setup
		car_pos = car.GetPosition();
	}

	// camera setup
	MATHVECTOR<float, 3> offset(1.75, 5.75, 0.75);
	track.GetStart(0).second.RotateVector(offset);
	MATHVECTOR<float, 3> pos = track.GetStart(0).first + offset;
	QUATERNION<float> rot = LookAt(pos, car_pos, direction::Up);
	garage_camera.SetOffset(MATHVECTOR<float, 3>(0));
	garage_camera.Reset(pos, rot);
	active_camera = &garage_camera;
}

void GAME::SetCarColor()
{
	if (!gui.GetInGame() && !cars.empty())
	{
		float r, g, b;
		const MATHVECTOR<float, 3> & hsv = cars_color_hsv[car_edit_id];
		HSVtoRGB(hsv[0], hsv[1], hsv[2], r, g, b);
		cars.back().SetColor(r, g, b);
	}
}

bool GAME::LoadFonts()
{
	std::string fontdir = pathmanager.GetFontDir(settings.GetSkin());
	std::string fontpath = pathmanager.GetDataPath()+"/"+fontdir;

	if (graphics_interface->GetUsingShaders())
	{
		if (!fonts["freesans"].Load(fontpath+"/freesans.txt",fontdir, "freesans.png", content, error_output)) return false;
		if (!fonts["lcd"].Load(fontpath+"/lcd.txt",fontdir, "lcd.png", content, error_output)) return false;
		if (!fonts["futuresans"].Load(fontpath+"/futuresans.txt",fontdir, "futuresans.png", content, error_output)) return false;
		if (!fonts["futuresans-noshader"].Load(fontpath+"/futuresans.txt",fontdir, "futuresans_noshaders.png", content, error_output)) return false;
	}
	else
	{
		if (!fonts["freesans"].Load(fontpath+"/freesans.txt",fontdir, "freesans_noshaders.png", content, error_output)) return false;
		if (!fonts["lcd"].Load(fontpath+"/lcd.txt",fontdir, "lcd_noshaders.png", content,  error_output)) return false;
		if (!fonts["futuresans"].Load(fontpath+"/futuresans.txt",fontdir, "futuresans_noshaders.png", content, error_output)) return false;
	}

	info_output << "Loaded fonts successfully" << std::endl;

	return true;
}

void GAME::CalculateFPS()
{
	if (eventsystem.Get_dt() > 0)
	{
		fps_track[fps_position] = 1.0/eventsystem.Get_dt();
	}

	fps_position = (fps_position + 1) % 10;
	float fps_avg = 0.0;
	for (int i = 0; i < 10; i++)
	{
		fps_avg += fps_track[i];
	}
	fps_avg /= 10.0;

	std::stringstream fpsstr;
	fpsstr << "FPS: " << (int)fps_avg;

	// Don't start looking an min/max until we've put out a few frames.
	if (fps_min == 0 && frame > 20)
	{
		fps_max = fps_avg;
		fps_min = fps_avg;
	}
	else if (fps_avg > fps_max)
	{
		fps_max = fps_avg;
	}
	else if (fps_avg < fps_min)
	{
		fps_min = fps_avg;
	}

	if (settings.GetShowFps())
	{
		float screenhwratio = (float)window.GetH() / window.GetW();
		float scaley = 0.03;
		float scalex = scaley * screenhwratio;
		float w = fps_draw.GetWidth("FPS: 100") * screenhwratio;
		float x = 0.5 - w * 0.5;
		float y = 1 - scaley;
		fps_draw.Revise(fpsstr.str(), x, y, scalex, scaley);
		fps_draw.SetDrawEnable(debugnode, true);
	}
	else
	{
		fps_draw.SetDrawEnable(debugnode, false);
	}

	if (profilingmode && frame % 10 == 0)
	{
		std::string cpuProfile = PROFILER.getAvgSummary(quickprof::MICROSECONDS);
		std::stringstream summary;
		summary << "CPU:\n" << cpuProfile << "\n\nGPU:\n";
		graphics_interface->printProfilingInfo(summary);
		profiling_text.Revise(summary.str());
	}
}

bool SortStringPairBySecond (const std::pair<std::string, std::string> & first, const std::pair<std::string, std::string> & second)
{
	return first.second < second.second;
}

void GAME::PopulateReplayList(std::list <std::pair <std::string, std::string> > & replaylist)
{
	replaylist.clear();
	int numreplays = 0;
	std::list <std::string> replayfoldercontents;
	if (pathmanager.GetFileList(pathmanager.GetReplayPath(), replayfoldercontents))
	{
		for (std::list<std::string>::iterator i = replayfoldercontents.begin(); i != replayfoldercontents.end(); ++i)
		{
			// Replay expects a formatted string: "MM-DD-hh-mm-trackname.vdr".
			if (*i != "benchmark.vdr" &&
				i->find(".vdr") == i->length() - 4 &&
				i->length() > 16)
			{
				// Parse replay name.
				std::string str = i->substr(0, i->length() - 4);
				str[2] = '/';
				str[5] = ' ';
				str[8] = ':';
				str[11] = ' ';

				replaylist.push_back(std::make_pair(*i, str));
				++numreplays;
			}
		}
	}

	if (numreplays == 0)
		replaylist.push_back(std::make_pair("none", "None"));

	settings.SetSelectedReplay(replaylist.begin()->first);
}

void GAME::PopulateCarPaintList(const std::string & carname, std::list <std::pair <std::string, std::string> > & carpaintlist)
{
	carpaintlist.clear();
	carpaintlist.push_back(std::make_pair("default", "default"));

	std::list <std::string> paintfolder;
	std::string paintdir = pathmanager.GetCarPaintPath(carname);
	if (pathmanager.GetFileList(paintdir, paintfolder, ".png"))
	{
		for (std::list <std::string>::iterator i = paintfolder.begin(); i != paintfolder.end(); ++i)
		{
			std::string paintname = i->substr(0, i->length() - 4);
			carpaintlist.push_back(std::make_pair("skins/" + *i, paintname));
		}
	}
}

static void PopulateCarSet(std::set <std::pair<std::string, std::string> > & set, const std::string & path, const PATHMANAGER & pathmanager)
{
	std::list <std::string> folderlist;
	pathmanager.GetFileList(path, folderlist);
	for (std::list <std::string>::iterator i = folderlist.begin(); i != folderlist.end(); ++i)
	{
		std::ifstream check((path + "/" + *i + "/" + *i + ".car").c_str());
		if (check)
		{
			set.insert(std::make_pair(*i, *i));
		}
	}
}

static void PopulateTrackSet(std::set <std::pair<std::string, std::string> > & set, const std::string & path, const PATHMANAGER & pathmanager)
{
	std::list <std::string> folderlist;
	pathmanager.GetFileList(path, folderlist);
	for (std::list <std::string>::iterator i = folderlist.begin(); i != folderlist.end(); ++i)
	{
		std::ifstream check((path+"/"+*i+"/about.txt").c_str());
		if (check)
		{
			std::string name;
			getline(check, name);
			set.insert(std::make_pair(*i, name));
		}
	}
}

void GAME::PopulateValueLists(std::map<std::string, std::list <std::pair <std::string, std::string> > > & valuelists)
{
	// Populate track list. Use set to avoid duplicate entries.
	std::set <std::pair<std::string, std::string> > trackset;
	PopulateTrackSet(trackset, pathmanager.GetReadOnlyTracksPath(), pathmanager);
	PopulateTrackSet(trackset, pathmanager.GetWriteableTracksPath(), pathmanager);
	std::list <std::pair<std::string, std::string> > tracklist;
	for (std::set <std::pair<std::string, std::string> >::const_iterator i = trackset.begin(); i != trackset.end(); i++)
	{
		tracklist.push_back(*i);
	}
	tracklist.sort(SortStringPairBySecond);
	valuelists["tracks"] = tracklist;

	// Populate car list. Use set to avoid duplicate entries.
	std::set <std::pair<std::string, std::string> > carset;
	PopulateCarSet(carset, pathmanager.GetReadOnlyCarsPath(), pathmanager);
	PopulateCarSet(carset, pathmanager.GetWriteableCarsPath(), pathmanager);
	std::list <std::pair<std::string, std::string> > carlist;
	for (std::set <std::pair<std::string, std::string> >::const_iterator i = carset.begin(); i != carset.end(); i++)
	{
		carlist.push_back(*i);
	}
	valuelists["cars"] = carlist;

	std::vector <std::string> AI_Types(ai.ListFactoryTypes());
	std::list <std::pair<std::string, std::string> > AI_Type_list;
	for (size_t i=0; i < AI_Types.size(); i++)
	{
		AI_Type_list.push_back(std::make_pair(AI_Types[i], AI_Types[i]));
	}
	valuelists["ai_types"] = AI_Type_list;

	// Populate car paints.
	PopulateCarPaintList(cars_name[0], valuelists["car_paints"]);

	// Populate anisotropy list.
	int max_aniso = graphics_interface->GetMaxAnisotropy();
	valuelists["anisotropy"].push_back(std::make_pair("0","Off"));
	int cur = 1;
	while (cur <= max_aniso)
	{
		std::string anisostr = cast(cur);
		valuelists["anisotropy"].push_back(std::make_pair(anisostr,anisostr+"X"));
		cur *= 2;
	}

	// Populate antialiasing list.
	valuelists["antialiasing"].push_back(std::make_pair("0","Off"));
	if (graphics_interface->AntialiasingSupported())
	{
		valuelists["antialiasing"].push_back(std::make_pair("2","2X"));
		valuelists["antialiasing"].push_back(std::make_pair("4","4X"));
	}

	// Populate replays list.
	PopulateReplayList(valuelists["replays"]);

	// Populate other lists.
	valuelists["joy_indices"].push_back(std::make_pair("0","0"));

	// Populate skins.
	std::list <std::string> skinlist;
	pathmanager.GetFileList(pathmanager.GetSkinPath(), skinlist);
	for (std::list <std::string>::iterator i = skinlist.begin(); i != skinlist.end(); ++i)
	{
		if (pathmanager.FileExists(pathmanager.GetSkinPath()+*i+"/menus/Main"))
		{
			valuelists["skins"].push_back(std::make_pair(*i,*i));
		}
	}

	// Populate languages.
	std::list <std::string> languages;
	std::string skinfolder = pathmanager.GetDataPath() + "/" + pathmanager.GetGUILanguageDir(settings.GetSkin()) + "/";
	pathmanager.GetFileList(skinfolder, languages, ".txt");
	for (std::list <std::string>::iterator i = languages.begin(); i != languages.end(); ++i)
	{
		if (pathmanager.FileExists(skinfolder + *i))
		{
			std::string value = i->substr(0, i->length()-4);
			valuelists["languages"].push_back(std::make_pair(value, value));
		}
	}
}

/* Update the game with any new setting changes that have just been made... */
void GAME::ProcessNewSettings()
{
	if (track.Loaded())
	{
		track.SetRacingLineVisibility(settings.GetRacingline());
	}

	if (carcontrols_local.first)
	{
		carcontrols_local.first->SetAutoClutch(settings.GetAutoClutch());
		carcontrols_local.first->SetAutoShift(settings.GetAutoShift());
	}

	sound.SetVolume(settings.GetSoundVolume());
	sound.SetMaxActiveSources(settings.GetMaxSoundSources());
}

void GAME::ShowHUD(bool value)
{
	hud.SetVisible(value && settings.GetShowHUD());

	if (value && settings.GetInputGraph())
		inputgraph.Show();
	else
		inputgraph.Hide();
}

void GAME::LoadingScreen(float progress, float max, bool drawGui, const std::string & optionalText, float x, float y)
{
	assert(max > 0);
	loadingscreen.Update(progress/max, optionalText, x, y);

	graphics_interface->GetDynamicDrawlist().clear();
	if (drawGui)
	{
		TraverseScene<false>(gui.GetNode(), graphics_interface->GetDynamicDrawlist());
	}
	TraverseScene<false>(loadingscreen.GetNode(), graphics_interface->GetDynamicDrawlist());

	graphics_interface->SetupScene(45.0, 100.0, MATHVECTOR <float, 3> (), QUATERNION <float> (), MATHVECTOR <float, 3> ());
	graphics_interface->BeginScene(error_output);
	graphics_interface->DrawScene(error_output);
	graphics_interface->EndScene(error_output);
	window.SwapBuffers();
}

bool GAME_DOWNLOADER::operator()(const std::string & file)
{
	return game.Download(file);
}

bool GAME_DOWNLOADER::operator()(const std::vector <std::string> & urls)
{
	return game.Download(urls);
}

bool GAME::Download(const std::string & file)
{
	std::vector <std::string> files;
	files.push_back(file);
	return Download(files);
}

bool GAME::Download(const std::vector <std::string> & urls)
{
	// Make sure we're not currently downloading something in the background.
	if (http.Downloading())
	{
		error_output << "Unable to download additional files; currently already downloading something in the background." << std::endl;
		return false;
	}

	for (unsigned int i = 0; i < urls.size(); i++)
	{
		std::string url = urls[i];
		bool requestSuccess = http.Request(url, error_output);
		if (!requestSuccess)
		{
			http.CancelAllRequests();
			return false;
		}
		bool userCancel = false;
		while (http.Tick() && !userCancel)
		{
			eventsystem.ProcessEvents();
			if (eventsystem.GetKeyState(SDLK_ESCAPE).just_down || eventsystem.GetQuit())
			{
				http.CancelAllRequests();
				return false;
			}

			HTTPINFO info;
			http.GetRequestInfo(url, info);

			if (info.state == HTTPINFO::FAILED)
			{
				http.CancelAllRequests();
				error_output << "Failed when downloading URL: " << url << std::endl;
				return false;
			}

			std::stringstream text;
			text << HTTPINFO::GetString(info.state);
			if (info.state == HTTPINFO::DOWNLOADING)
			{
				text << " " << HTTP::ExtractFilenameFromUrl(url);
				text << " " << HTTPINFO::FormatSize(info.downloaded);
				//text << " " << HTTPINFO::FormatSpeed(info.speed);
			}
			double total = 1000000;
			if (info.totalsize > 0)
				total = info.totalsize;

			// Tick the GUI...
			eventsystem.BeginFrame();
			gui.Update(eventsystem.Get_dt());
			eventsystem.EndFrame();

			LoadingScreen(fmod(info.downloaded,total), total, true, text.str(), 0.5, 0.5);
		}

		HTTPINFO info;
		http.GetRequestInfo(url, info);
		if (info.state == HTTPINFO::FAILED)
		{
			http.CancelAllRequests();
			error_output << "Failed when downloading URL: " << url << std::endl;
			return false;
		}
	}

	return true;
}

void GAME::UpdateForceFeedback(float dt)
{
	if (carcontrols_local.first)
	{
		//static ofstream file("ff_output.txt");
		ff_update_time += dt;
		const double ffdt = 0.02;
		if (ff_update_time >= ffdt )
		{
			ff_update_time = 0.0;
			double feedback = -carcontrols_local.first->GetFeedback();

			// scale
			feedback = feedback * settings.GetFFGain() * 0.2;

			// invert
			if (settings.GetFFInvert()) feedback = -feedback;

			// clamp
			if (feedback > 1.0) feedback = 1.0;
			if (feedback < -1.0) feedback = -1.0;

			double force = feedback;
			forcefeedback->update(force, &feedback, ffdt, error_output);
		}
	}

	if (pause && dt == 0)
	{
		double pos=0;
		forcefeedback->update(0, &pos, 0.02, error_output);
	}
}

void GAME::AddTireSmokeParticles(float dt, CAR & car)
{
	// Only spawn particles every so often...
	unsigned int interval = 0.2 / dt;
	if (particle_timer % interval == 0)
	{
		for (int i = 0; i < 4; i++)
		{
			float squeal = car.GetTireSquealAmount(i);
			if (squeal > 0)
			{
				tire_smoke.AddParticle(
					car.GetWheelPosition(i) - MATHVECTOR<float,3>(0,0,car.GetTireRadius(i)),
					0.5);
			}
		}
	}
}

void GAME::UpdateParticleSystems(float dt)
{
	if (track.Loaded() && active_camera)
	{
		QUATERNION <float> camlook;
		camlook.Rotate(3.141593*0.5, 1, 0, 0);
		QUATERNION <float> camorient = -(active_camera->GetOrientation() * camlook);

		tire_smoke.Update(dt, camorient, active_camera->GetPosition());
	}

	particle_timer++;
	particle_timer = particle_timer % (unsigned int)((1.0/TickPeriod()));
}

void GAME::UpdateDriftScore(CAR & car, double dt)
{
	// Assert that the car is registered with the timer system.
	assert(cartimerids.find(&car) != cartimerids.end());

	// Make sure the car is not off track.
	int wheel_count = 0;
	for (int i=0; i < 4; i++)
	{
		if ( car.GetCurPatch ( i ) ) wheel_count++;
	}

	bool on_track = ( wheel_count > 1 );
	bool is_drifting = false;
	bool spin_out = false;
	if ( on_track )
	{
		// Car's velocity on the horizontal plane (should use surface plane here).
		MATHVECTOR <float, 3> car_velocity = car.GetVelocity();
		car_velocity[2] = 0;
		float car_speed = car_velocity.Magnitude();

		// Car's direction on the horizontal plane.
		MATHVECTOR <float, 3> car_direction = direction::Forward;
		car.GetOrientation().RotateVector(car_direction);
		car_direction[2] = 0;
		float dir_mag = car_direction.Magnitude();

		// Speed must be above 10 m/s and orientation must be valid.
		if ( car_speed > 10 && dir_mag > 0.01)
		{
			// Angle between car's direction and velocity.
			float cos_angle = car_direction.dot(car_velocity) / (car_speed * dir_mag);
			if (cos_angle > 1) cos_angle = 1;
			else if (cos_angle < -1) cos_angle = -1;
			float car_angle = acos(cos_angle);

			// Drift starts when the angle > 0.2 (around 11.5 degrees).
			// Drift ends when the angle < 0.1 (aournd 5.7 degrees).
			float angle_threshold(0.2);
			if ( timer.GetIsDrifting(cartimerids[&car]) ) angle_threshold = 0.1;

			is_drifting = ( car_angle > angle_threshold && car_angle <= M_PI/2.0 );
			spin_out = ( car_angle > M_PI/2.0 );

			// Calculate score.
			if ( is_drifting )
			{
				// Base score is the drift distance.
				timer.IncrementThisDriftScore(cartimerids[&car], dt * car_speed);

				// Bonus score calculation is now done in TIMER.
				timer.UpdateMaxDriftAngleSpeed(cartimerids[&car], car_angle, car_speed);
				//std::cout << timer.GetDriftScore(cartimerids[&car]) << " + " << timer.GetThisDriftScore(cartimerids[&car]) << std::endl;
			}
		}
	}

	timer.SetIsDrifting(cartimerids[&car], is_drifting, on_track && !spin_out);
	//std::cout << is_drifting << ", " << on_track << ", " << car_angle << std::endl;
}

void GAME::BeginStartingUp()
{
	std::ofstream f(pathmanager.GetStartupFile().c_str());
}

void GAME::DoneStartingUp()
{
	std::remove(pathmanager.GetStartupFile().c_str());
}

bool GAME::LastStartWasSuccessful() const
{
	return !pathmanager.FileExists(pathmanager.GetStartupFile());
}

void GAME::QuitGame()
{
	info_output << "Got quit message from GUI. Shutting down..." << std::endl;
	eventsystem.Quit();
}

void GAME::LeaveGame()
{
	ai.clear_cars();

	carcontrols_local.first = NULL;

	if (replay.GetRecording())
	{
		std::string replayname = GetReplayRecordingFilename();
		info_output << "Saving replay to " << replayname << std::endl;
		replay.StopRecording(replayname);

		std::list <std::pair <std::string, std::string> > replaylist;
		PopulateReplayList(replaylist);
		gui.SetOptionValues("game.selected_replay", "", replaylist, error_output);
	}

	if (replay.GetPlaying())
		replay.StopPlaying();

	gui.SetInGame(false);
	gui.ActivatePage("Main", 0.25, error_output);

	// Clear out the static drawables.
	SCENENODE empty;
	graphics_interface->AddStaticNode(empty, true);

	track.Clear();
	cars.clear();
	sound.Update(true);
	hud.SetVisible(false);
	inputgraph.Hide();
	trackmap.Unload();
	timer.Unload();
	active_camera = 0;
	pause = false;
	race_laps = 0;
	tire_smoke.Clear();
}

void GAME::StartPractice()
{
	practice = true;
	if (!NewGame())
	{
		LoadGarage();
	}
}

void GAME::StartRace()
{
	// Handle a single race.
	if (cars_name.size() < 2)
	{
		gui.ActivatePage("NoOpponentsError", 0.25, error_output);
	}
	else
	{
		practice = false;
		bool play_replay = false;
		int num_laps = settings.GetNumberOfLaps();
		if (!NewGame(play_replay, !practice, num_laps))
		{
			LoadGarage();
		}
	}
}

void GAME::ReturnToGame()
{
	if (gui.Active())
	{
		if (settings.GetMouseGrab())
			window.ShowMouseCursor(false);
		gui.Deactivate();
		ShowHUD(true);
	}
}

void GAME::RestartGame()
{
	bool play_replay = false;
	int num_laps = race_laps;
	if (!NewGame(play_replay, !practice, num_laps))
	{
		LoadGarage();
	}
}

void GAME::StartReplay()
{
	if (settings.GetSelectedReplay() != "none"  && !NewGame(true))
	{
		gui.ActivatePage("ReplayStartError", 0.25, error_output);
	}
}

void GAME::HandleOnlineClicked()
{
	std::string motdUrl = "vdrift.net/online/motd.txt";
	bool success = Download(motdUrl);
	if (success)
	{
		gui.ActivatePage("Online", 0.25, error_output);
		std::string motdFile = http.GetDownloadPath(motdUrl);
		std::string motd = UTILS::LoadFileIntoString(motdFile, error_output);
		gui.SetLabelText("Online", "Motd", motd);
	}
}

void GAME::StartCheckForUpdates()
{
	carupdater.StartCheckForUpdates(GAME_DOWNLOADER(*this, http), gui);
	trackupdater.StartCheckForUpdates(GAME_DOWNLOADER(*this, http), gui);
	gui.ActivatePage("UpdatesFound", 0.25, error_output);
}

void GAME::StartCarManager()
{
	carupdater.Reset();
	carupdater.Show(gui);
}

void GAME::CarManagerNext()
{
	carupdater.Increment();
	carupdater.Show(gui);
}

void GAME::CarManagerPrev()
{
	carupdater.Decrement();
	carupdater.Show(gui);
}

void GAME::ApplyCarUpdate()
{
	carupdater.ApplyUpdate(GAME_DOWNLOADER(*this, http), gui, pathmanager);
}

void GAME::StartTrackManager()
{
	trackupdater.Reset();
	trackupdater.Show(gui);
}

void GAME::TrackManagerNext()
{
	trackupdater.Increment();
	trackupdater.Show(gui);
}

void GAME::TrackManagerPrev()
{
	trackupdater.Decrement();
	trackupdater.Show(gui);
}

void GAME::ApplyTrackUpdate()
{
	trackupdater.ApplyUpdate(GAME_DOWNLOADER(*this, http), gui, pathmanager);
}

void GAME::EditControl()
{
	if (controlgrab_control.IsAnalog())
	{
		// edit analog control
		std::string control = controlgrab_control.GetInfo();
		std::string deadzone = cast(controlgrab_control.deadzone);
		std::string exponent = cast(controlgrab_control.exponent);
		std::string gain = cast(controlgrab_control.gain);

		gui.SetOptionValue("controledit.control", control);
		gui.SetOptionValue("controledit.deadzone", deadzone);
		gui.SetOptionValue("controledit.exponent", exponent);
		gui.SetOptionValue("controledit.gain", gain);

		gui.ActivatePage("EditAnalogControl", 0.25, error_output);
	}
	else
	{
		// edit button control
		std::string control = controlgrab_control.GetInfo();
		bool down = controlgrab_control.pushdown;
		bool once = controlgrab_control.onetime;

		gui.SetOptionValue("controledit.control", control);
		gui.SetOptionValue("controledit.once", once ? "true" : "false");
		gui.SetOptionValue("controledit.down", down ? "true" : "false");

		gui.ActivatePage("EditButtonControl", 0.25, error_output);
	}
}

void GAME::CancelControl()
{
	gui.ActivatePage(controlgrab_page, 0.25, error_output);
	LoadControlsIntoGUIPage();
}

void GAME::DeleteControl()
{
	carcontrols_local.second.DeleteControl(controlgrab_input, controlgrab_id);
	gui.ActivatePage(controlgrab_page, 0.25, error_output);
	LoadControlsIntoGUIPage();
}

void GAME::SetButtonControl()
{
	controlgrab_control.onetime = (gui.GetOptionValue("controledit.once") == "true");
	controlgrab_control.pushdown = (gui.GetOptionValue("controledit.down") == "true");
	carcontrols_local.second.SetControl(controlgrab_input, controlgrab_id, controlgrab_control);
	gui.ActivatePage(controlgrab_page, 0.25, error_output);
	LoadControlsIntoGUIPage();
}

void GAME::SetAnalogControl()
{
	controlgrab_control.deadzone = cast<float>(gui.GetOptionValue("controledit.deadzone"));
	controlgrab_control.exponent = cast<float>(gui.GetOptionValue("controledit.exponent"));
	controlgrab_control.gain = cast<float>(gui.GetOptionValue("controledit.gain"));
	carcontrols_local.second.SetControl(controlgrab_input, controlgrab_id, controlgrab_control);
	gui.ActivatePage(controlgrab_page, 0.25, error_output);
	LoadControlsIntoGUIPage();
}

void GAME::LoadControls()
{
	carcontrols_local.second.Load(
		pathmanager.GetCarControlsFile(),
		info_output,
		error_output);

	LoadControlsIntoGUIPage();
}

void GAME::SaveControls()
{
	carcontrols_local.second.Save(pathmanager.GetCarControlsFile());
}

void GAME::SyncOptions()
{
	std::map<std::string, std::string> optionmap;
	settings.Get(optionmap);

	// hack: store first car info only
	cars_name[0] = optionmap["game.car"];
	cars_paint[0] = optionmap["game.car_paint"];
	cars_color_hsv[0][0] = cast<float>(optionmap["game.car_color_hue"]);
	cars_color_hsv[0][1] = cast<float>(optionmap["game.car_color_sat"]);
	cars_color_hsv[0][2] = cast<float>(optionmap["game.car_color_val"]);
	if (car_edit_id > 0)
	{
		optionmap.erase("game.car");
		optionmap.erase("game.car_paint");
		optionmap.erase("game.car_color_hue");
		optionmap.erase("game.car_color_sat");
		optionmap.erase("game.car_color_val");
	}

	gui.SetOptions(optionmap);
}

void GAME::SyncSettings()
{
	std::map<std::string, std::string> optionmap;
	settings.Get(optionmap);
	gui.GetOptions(optionmap);

	// hack: store first car info only
	optionmap["game.car"] = cars_name[0];
	optionmap["game.car_paint"] = cars_paint[0];
	optionmap["game.car_color_hue"] = cast(cars_color_hsv[0][0]);
	optionmap["game.car_color_sat"] = cast(cars_color_hsv[0][1]);
	optionmap["game.car_color_val"] = cast(cars_color_hsv[0][2]);

	settings.Set(optionmap);
	ProcessNewSettings();
}

void GAME::EditFirstCar()
{
	if (car_edit_id == 0)
		return;

	// set player car as active car
	size_t id = car_edit_id = 0;

	// if player car differs from last car force reload
	if (cars_name[id] != cars_name.back())
	{
		gui.SetOptionValue("game.car", cars_name[id]);
		SetGarageCar();
	}

	// reset paint, color
	gui.SetOptionValue("game.car_paint", cars_paint[id]);
	gui.SetOptionValue("game.car_color_hue", cast(cars_color_hsv[id][0]));
	gui.SetOptionValue("game.car_color_sat", cast(cars_color_hsv[id][1]));
	gui.SetOptionValue("game.car_color_val", cast(cars_color_hsv[id][2]));
}

void GAME::EditLastCar()
{
	if (car_edit_id != 0)
		return;

	// set last car as active car
	size_t id = car_edit_id = cars_name.size() - 1;

	// if player car differs from last car force reload
	if (cars_name[id] != cars_name.back())
	{
		gui.SetOptionValue("game.car", cars_name[id]);
		SetGarageCar();
	}

	// reset paint, color
	gui.SetOptionValue("game.car_paint", cars_paint[id]);
	gui.SetOptionValue("game.car_color_hue", cast(cars_color_hsv[id][0]));
	gui.SetOptionValue("game.car_color_sat", cast(cars_color_hsv[id][1]));
	gui.SetOptionValue("game.car_color_val", cast(cars_color_hsv[id][2]));
}

void GAME::SetCarName(const std::string & value)
{
	if (cars_name[car_edit_id] == value)
		return;

	cars_name[car_edit_id] = value;
	UpdateStartList(car_edit_id, cars_name[car_edit_id]);

	std::list <std::pair <std::string, std::string> > carpaintlist;
	PopulateCarPaintList(value, carpaintlist);
	gui.SetOptionValues("game.car_paint", cars_paint[car_edit_id], carpaintlist, error_output);

	SetGarageCar();
}

void GAME::SetCarPaint(const std::string & value)
{
	if (cars_paint[car_edit_id] != value)
	{
		cars_paint[car_edit_id] = value;
		SetGarageCar();
	}
}

void GAME::SetCarColorHue(const std::string & value)
{
	cars_color_hsv[car_edit_id][0] = cast<float>(value);
	SetCarColor();
}

void GAME::SetCarColorSat(const std::string & value)
{
	cars_color_hsv[car_edit_id][1] = cast<float>(value);
	SetCarColor();
}

void GAME::SetCarColorVal(const std::string & value)
{
	cars_color_hsv[car_edit_id][2] = cast<float>(value);
	SetCarColor();
}

void GAME::SetCarAIType(const std::string & value)
{
	cars_ai_type[car_edit_id] = value;
}

void GAME::SetCarAILevel(const std::string & value)
{
	cars_ai_level[car_edit_id] = cast<float>(value);
}

void GAME::SetCarsNum(const std::string & value)
{
	size_t cars_num = cast<size_t>(value);
	int delta = cars_num - cars_name.size();
	if (!delta)
		return;

	if (delta < 0)
	{
		for (size_t i = cars_name.size(); i > cars_num; --i)
		{
			UpdateStartList(i - 1, "");
			cars_name.pop_back();
			cars_paint.pop_back();
			cars_color_hsv.pop_back();
			cars_ai_type.pop_back();
			cars_ai_level.pop_back();
		}
	}
	else if (delta > 0)
	{
		cars_name.reserve(cars_num);
		cars_paint.reserve(cars_num);
		cars_color_hsv.reserve(cars_num);
		cars_ai_type.reserve(cars_num);
		cars_ai_level.reserve(cars_num);
		for (size_t i = cars_name.size(); i < cars_num; ++i)
		{
			// variate color, lame version
			float hue = cars_color_hsv.back()[0] + 0.07;
			if (hue > 1.0f) hue -= 1.0f;
			MATHVECTOR<float, 3> color(hue, 0.95, 0.5);

			cars_name.push_back(cars_name.back());
			cars_paint.push_back(cars_paint.back());
			cars_color_hsv.push_back(color);
			cars_ai_type.push_back(cars_ai_type.back());
			cars_ai_level.push_back(cars_ai_level.back());
			UpdateStartList(i, cars_name.back());
		}
	}

	size_t id = car_edit_id = cars_num - 1;
	gui.SetOptionValue("game.car", cars_name[id]);
	gui.SetOptionValue("game.car_paint", cars_paint[id]);
	gui.SetOptionValue("game.car_color_hue", cast(cars_color_hsv[id][0]));
	gui.SetOptionValue("game.car_color_sat", cast(cars_color_hsv[id][1]));
	gui.SetOptionValue("game.car_color_val", cast(cars_color_hsv[id][2]));
	gui.SetOptionValue("game.ai_type", cars_ai_type[id]);
	gui.SetOptionValue("game.ai_level", cast(cars_ai_level[id]));
}

void GAME::SetTrackImage(const std::string & value)
{
	gui.SetOptionValue("game.track.image", value + "/trackshot.png");
}

void GAME::SetControl(const std::string & value)
{
	std::stringstream vs(value);
	std::string inputstr, idstr, oncestr, downstr;
	getline(vs, inputstr, ':');
	getline(vs, idstr, ':');
	getline(vs, oncestr, ':');
	getline(vs, downstr);

	size_t id = 0;
	std::stringstream ns(idstr);
	ns >> id;

	controlgrab_control = carcontrols_local.second.GetControl(inputstr, id);
	controlgrab_page = gui.GetActivePageName();
	controlgrab_input = inputstr;
	controlgrab_id = id;

	if (controlgrab_control.type == CARCONTROLMAP::CONTROL::UNKNOWN)
	{
		// assign control
		controlgrab_mouse_coords = std::make_pair(eventsystem.GetMousePosition()[0], eventsystem.GetMousePosition()[1]);
		controlgrab_joystick_state = eventsystem.GetJoysticks();

		// default control settings
		if (!oncestr.empty())
			controlgrab_control.onetime = (oncestr == "once");
		if (!downstr.empty())
			controlgrab_control.pushdown = (downstr == "down");

		gui.ActivatePage("AssignControl", 0.25, error_output);
		controlgrab = true;
		return;
	}

	EditControl();
}

void GAME::BindActionsToGUI()
{
	set_car_name.connect(gui.GetOption("game.car").signal_val);
	set_car_paint.connect(gui.GetOption("game.car_paint").signal_val);
	set_car_color_hue.connect(gui.GetOption("game.car_color_hue").signal_val);
	set_car_color_sat.connect(gui.GetOption("game.car_color_sat").signal_val);
	set_car_color_val.connect(gui.GetOption("game.car_color_val").signal_val);
	set_car_ai_type.connect(gui.GetOption("game.ai_type").signal_val);
	set_car_ai_level.connect(gui.GetOption("game.ai_level").signal_val);
	set_cars_num.connect(gui.GetOption("game.cars_num").signal_val);
	set_track_image.connect(gui.GetOption("game.track").signal_val);
	set_control.connect(gui.GetOption("controledit.string").signal_val);
}

void GAME::RegisterActions()
{
	set_car_name.call.bind<GAME, &GAME::SetCarName>(this);
	set_car_paint.call.bind<GAME, &GAME::SetCarPaint>(this);
	set_car_color_hue.call.bind<GAME, &GAME::SetCarColorHue>(this);
	set_car_color_sat.call.bind<GAME, &GAME::SetCarColorSat>(this);
	set_car_color_val.call.bind<GAME, &GAME::SetCarColorVal>(this);
	set_car_ai_type.call.bind<GAME, &GAME::SetCarAIType>(this);
	set_car_ai_level.call.bind<GAME, &GAME::SetCarAILevel>(this);
	set_cars_num.call.bind<GAME, &GAME::SetCarsNum>(this);
	set_track_image.call.bind<GAME, &GAME::SetTrackImage>(this);
	set_control.call.bind<GAME, &GAME::SetControl>(this);

	actions.resize(27);
	actions[0].call.bind<GAME, &GAME::QuitGame>(this);
	actions[1].call.bind<GAME, &GAME::LoadGarage>(this);
	actions[2].call.bind<GAME, &GAME::StartPractice>(this);
	actions[3].call.bind<GAME, &GAME::StartRace>(this);
	actions[4].call.bind<GAME, &GAME::ReturnToGame>(this);
	actions[5].call.bind<GAME, &GAME::RestartGame>(this);
	actions[6].call.bind<GAME, &GAME::StartReplay>(this);
	actions[7].call.bind<GAME, &GAME::HandleOnlineClicked>(this);
	actions[8].call.bind<GAME, &GAME::StartCheckForUpdates>(this);
	actions[9].call.bind<GAME, &GAME::StartCarManager>(this);
	actions[10].call.bind<GAME, &GAME::CarManagerNext>(this);
	actions[11].call.bind<GAME, &GAME::CarManagerPrev>(this);
	actions[12].call.bind<GAME, &GAME::ApplyCarUpdate>(this);
	actions[13].call.bind<GAME, &GAME::StartTrackManager>(this);
	actions[14].call.bind<GAME, &GAME::TrackManagerNext>(this);
	actions[15].call.bind<GAME, &GAME::TrackManagerPrev>(this);
	actions[16].call.bind<GAME, &GAME::ApplyTrackUpdate>(this);
	actions[17].call.bind<GAME, &GAME::CancelControl>(this);
	actions[18].call.bind<GAME, &GAME::DeleteControl>(this);
	actions[19].call.bind<GAME, &GAME::SetButtonControl>(this);
	actions[20].call.bind<GAME, &GAME::SetAnalogControl>(this);
	actions[21].call.bind<GAME, &GAME::LoadControls>(this);
	actions[22].call.bind<GAME, &GAME::SaveControls>(this);
	actions[23].call.bind<GAME, &GAME::SyncOptions>(this);
	actions[24].call.bind<GAME, &GAME::SyncSettings>(this);
	actions[25].call.bind<GAME, &GAME::EditFirstCar>(this);
	actions[26].call.bind<GAME, &GAME::EditLastCar>(this);
}

void GAME::InitActionMap(std::map<std::string, Slot0*> & actionmap)
{
	actionmap["QuitGame"] = &actions[0];
	actionmap["LeaveGame"] = &actions[1];
	actionmap["StartPractice"] = &actions[2];
	actionmap["StartRace"] = &actions[3];
	actionmap["ReturnToGame"] = &actions[4];
	actionmap["RestartGame"] = &actions[5];
	actionmap["StartReplay"] = &actions[6];
	actionmap["HandleOnlineClicked"] = &actions[7];
	actionmap["StartCheckForUpdates"] = &actions[8];
	actionmap["StartCarManager"] = &actions[9];
	actionmap["CarManagerNext"] = &actions[10];
	actionmap["CarManagerPrev"] = &actions[11];
	actionmap["ApplyCarUpdate"] = &actions[12];
	actionmap["StartTrackManager"] = &actions[13];
	actionmap["TrackManagerNext"] = &actions[14];
	actionmap["TrackManagerPrev"] = &actions[15];
	actionmap["ApplyTrackUpdate"] = &actions[16];
	actionmap["CancelControl"] = &actions[17];
	actionmap["DeleteControl"] = &actions[18];
	actionmap["SetButtonControl"] = &actions[19];
	actionmap["SetAnalogControl"] = &actions[20];
	actionmap["LoadControls"] = &actions[21];
	actionmap["SaveControls"] = &actions[22];
	actionmap["gui.options.load"] = &actions[23];
	actionmap["gui.options.save"] = &actions[24];
	actionmap["EditFirstCar"] = &actions[25];
	actionmap["EditLastCar"] = &actions[26];
}
