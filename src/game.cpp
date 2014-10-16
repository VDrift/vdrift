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
#include "physics/carwheelposition.h"
#include "physics/tracksurface.h"
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
#include "tobullet.h"
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

Game::Game(std::ostream & info_out, std::ostream & error_out) :
	info_output(info_out),
	error_output(error_out),
	frame(0),
	displayframe(0),
	clocktime(0),
	target_time(0),
	timestep(1/90.0),
	graphics(NULL),
	content(error_out),
	carupdater(autoupdate, info_out, error_out),
	trackupdater(autoupdate, info_out, error_out),
	fps_track(10, 0),
	fps_position(0),
	fps_min(0),
	fps_max(0),
	multithreaded(false),
	profilingmode(false),
	benchmode(false),
	dumpfps(false),
	pause(false),
	controlgrab_id(0),
	controlgrab(false),
	garage_camera("garagecam"),
	active_camera(0),
	car_info(1),
	player_car_id(0),
	car_edit_id(0),
	race_laps(0),
	practice(true),
	collisiondispatch(
		&collisionconfig),
	dynamics(
		&collisiondispatch,
		&collisionbroadphase,
		&collisionsolver,
		&collisionconfig,
		timestep),
	dynamics_drawmode(0),
	particle_timer(0),
	track(),
	replay(timestep),
	http("/tmp")
{
	carcontrols_local.first = NULL;
	dynamics.setContactAddedCallback(&CarDynamics::WheelContactCallback);
	RegisterActions();
}

Game::~Game()
{
	// dtor
}

/* Start the game with the given arguments... */
void Game::Start(std::list <std::string> & args)
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

	if (!InitCoreSubsystems())
	{
		return;
	}

	// Load controls.
	info_output << "Loading car controls from: " << pathmanager.GetCarControlsFile() << std::endl;
	if (!carcontrols_local.second.Load(pathmanager.GetCarControlsFile(), info_output, error_output))
	{
		info_output << "Car control file " << pathmanager.GetCarControlsFile() << " doesn't exist; using defaults" << std::endl;
		carcontrols_local.second.Load(pathmanager.GetDefaultCarControlsFile(), info_output, error_output);
		carcontrols_local.second.Save(pathmanager.GetCarControlsFile());
	}

	// Load player car info from settings
	InitPlayerCar();

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
		PopulateCarList(carupdater.GetValueList(), true);
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
		PopulateTrackList(trackupdater.GetValueList());
	}

	// If sound initialization fails, that's okay, it'll disable itself...
	InitSound();

	// Load font data.
	if (!LoadFonts())
	{
		error_output << "Error loading fonts" << std::endl;
		return;
	}

	// Load GUI.
	if (!InitGUI("Main"))
	{
		error_output << "Error initializing graphical user interface" << std::endl;
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
	if (!hud.Init(
		pathmanager.GetGUITextureDir(settings.GetSkin()),
		gui.GetLanguageDict(), gui.GetFont(), fonts["lcd"],
		window.GetW(), window.GetH(),
		content, error_output))
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

	// Load particle system.
	Vec3 smokedir(0.4, 0.2, 1.0);
	tire_smoke.Load(pathmanager.GetEffectsTextureDir(), "smoke.png", settings.GetAnisotropy(), content);
	tire_smoke.SetParameters(settings.GetParticles(), 0.4,0.9, 1,4, 0.3,0.6, 0.02,0.06, smokedir);

	// Initialize force feedback.
	forcefeedback.reset(new ForceFeedback(settings.GetFFDevice(), error_output, info_output));
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
void Game::End()
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

	graphics->Deinit();
	delete graphics;
}

/* Initialize the most important, basic subsystems... */
bool Game::InitCoreSubsystems()
{
	pathmanager.Init(info_output, error_output);
	http.SetTemporaryFolder(pathmanager.GetTemporaryFolder());

	settings.Load(pathmanager.GetSettingsFile(), error_output);

	// global texture size override
	int texture_size = TextureInfo::LARGE;
	if (settings.GetTextureSize() == "small")
	{
		texture_size = TextureInfo::SMALL;
	}
	else if (settings.GetTextureSize() == "medium")
	{
		texture_size = TextureInfo::MEDIUM;
	}

	if (!LastStartWasSuccessful())
	{
		info_output << "The last VDrift startup was unsuccessful.\n";
		info_output << "Settings have been set to failsafe defaults.\n";
		info_output << "Your original VDrift.config file was backed up to VDrift.config.backup" << std::endl;
		settings.Save(pathmanager.GetSettingsFile()+".backup", error_output);
		settings.SetFailsafeSettings();
	}
	BeginStartingUp();

	// choose renderer
	std::string renderer = settings.GetRenderer();
	if (!renderconfigfile.empty())
		renderer = renderconfigfile;

	std::string render_ver, render_cfg;
	std::istringstream render_str(renderer);
	std::getline(render_str, render_ver, '/');
	std::getline(render_str, render_cfg);

	bool using_gl3 = (render_ver == "gl3");

	// disable antialiasing for the GL3 path because we're using image-based AA...
	unsigned antialiasing = using_gl3 ? 0 : settings.GetAntialiasing();

	// make sure to use at least 24bit depth buffer for shadows
	unsigned depth_bpp = settings.GetDepthBpp();
	if (settings.GetShadows() && depth_bpp < 24)
		depth_bpp = 24;

	window.Init(
		"VDrift",
		settings.GetResolutionX(),
		settings.GetResolutionY(),
		depth_bpp,
		antialiasing,
		settings.GetFullscreen(),
		settings.GetVsync(),
		info_output,
		error_output);

	if (!window.Initialized())
	{
		error_output << "Failed to create window." << std::endl;
		return false;
	}

	const int renderer_count = 2;
	for (int i = 0; i < renderer_count; i++)
	{
		// Attempt to enable the GL3 renderer...
		if (using_gl3)
		{
			graphics = new GraphicsGL3(stringMap);
		}
		else
		{
			graphics = new GraphicsGL2();
		}

		bool success = graphics->Init(
			pathmanager.GetShaderPath() + "/" + render_ver,
			settings.GetResolutionX(), settings.GetResolutionY(),
			settings.GetDepthBpp(), settings.GetFullscreen(),
			settings.GetAntialiasing(), settings.GetShadows(),
			settings.GetShadowDistance(), settings.GetShadowQuality(), settings.GetReflections(),
			pathmanager.GetStaticReflectionMap(), pathmanager.GetStaticAmbientMap(),
			settings.GetAnisotropic(), texture_size,
			settings.GetLighting(), settings.GetBloom(),
			settings.GetNormalMaps(), settings.GetSkyDynamic(),
			render_cfg, info_output, error_output);

		if (success)
		{
			break;
		}
		else
		{
			delete graphics;
			graphics = NULL;
			render_ver = "gl2";
			using_gl3 = false;
		}
	}

	if (!graphics)
	{
		error_output << "Failed to create renderer." << std::endl;
		return false;
	}

	Vec3 ldir(-0.250, -0.588, 0.769);
	ldir = ldir.Normalize();
	graphics->SetSunDirection(ldir);
	graphics->SetLocalTime(settings.GetSkyTime());
	graphics->SetLocalTimeSpeed(settings.GetSkyTimeSpeed());

	// Init content factories
	content.getFactory<Texture>().init(texture_size, using_gl3, settings.GetTextureCompress());
	content.getFactory<PTree>().init(read_ini, write_ini, content);

	// Init content paths
	// Always add writeable data paths first so they are checked first
	content.addPath(pathmanager.GetWriteableDataPath());
	content.addPath(pathmanager.GetDataPath());
	content.addSharedPath(pathmanager.GetCarPartsPath());
	content.addSharedPath(pathmanager.GetTrackPartsPath());

	eventsystem.Init(info_output);

	return true;
}

void Game::InitPlayerCar()
{
	Vec3 hsv;
	hsv[0] = settings.GetCarColorHue();
	hsv[1] = settings.GetCarColorSat();
	hsv[2] = settings.GetCarColorVal();

	car_info[0].driver = "user";
	car_info[0].name = settings.GetCar();
	car_info[0].paint = settings.GetCarPaint();
	car_info[0].ailevel = settings.GetAILevel();
	car_info[0].hsv = hsv;
	player_car_id = 0;
}

bool Game::InitGUI(const std::string & pagename)
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

	std::map<std::string, GuiOption::List> valuelists;
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

	// Set driver/edit num to update gui explicitely
	// as they are not stored in settings
	gui.SetOptionValue("game.car_edit", "0");
	gui.SetOptionValue("game.driver", "user");

	// Set input control labels
	LoadControlsIntoGUI();

	// Show main page.
	gui.ActivatePage(pagename, 0.5, error_output);
	if (settings.GetMouseGrab())
		window.ShowMouseCursor(true);

	return true;
}

bool Game::InitSound()
{
	if (sound.Init(2048, info_output, error_output))
	{
		sound.SetVolume(settings.GetSoundVolume());
		content.getFactory<SoundBuffer>().init(sound.GetDeviceInfo());
	}
	else
	{
		error_output << "Sound initialization failed" << std::endl;
		return false;
	}

	info_output << "Sound initialization successful" << std::endl;
	return true;
}

bool Game::ParseArguments(std::list <std::string> & args)
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

	if (!argmap["-cartest"].empty())
	{
		pathmanager.Init(info_output, error_output);
		content.getFactory<PTree>().init(read_ini, write_ini, content);
		content.addPath(pathmanager.GetWriteableDataPath());
		content.addPath(pathmanager.GetDataPath());
		content.addSharedPath(pathmanager.GetCarPartsPath());
		content.addSharedPath(pathmanager.GetTrackPartsPath());

		const std::string carname = argmap["-cartest"];
		const std::string cardir = pathmanager.GetCarsDir() + "/" + carname;

		PerformanceTesting perftest(dynamics);
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

	arghelp["-render FILE"] = "Load the specified render configuration file instead of the default gl3/deferred.conf.";
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

void Game::Test()
{
	QT_SET_OUTPUT(&info_output);

	QT_RUN_TESTS;

	info_output << std::endl;
}

void Game::Draw(float dt)
{
	PROFILER.beginBlock("scenegraph");
	std::vector<SceneNode*> nodes;
	nodes.reserve(7);
	nodes.push_back(&debugnode);
	nodes.push_back(&gui.GetNode());
	nodes.push_back(&hud.GetNode());
	nodes.push_back(&inputgraph.GetNode());
	nodes.push_back(&dynamicsdraw.getNode());
	nodes.push_back(&trackmap.GetNode());
	nodes.push_back(&tire_smoke.GetNode());
	graphics->BindDynamicVertexData(nodes);

	graphics->ClearDynamicDrawables();
	graphics->AddDynamicNode(debugnode);
	graphics->AddDynamicNode(gui.GetNode());
	graphics->AddDynamicNode(hud.GetNode());
	graphics->AddDynamicNode(inputgraph.GetNode());
	graphics->AddDynamicNode(dynamicsdraw.getNode());
#ifndef USE_STATIC_OPTIMIZATION_FOR_TRACK
	graphics->AddDynamicNode(track.GetTrackNode());
#endif
	graphics->AddDynamicNode(track.GetBodyNode());
	graphics->AddDynamicNode(track.GetRacinglineNode());
	graphics->AddDynamicNode(trackmap.GetNode());
	graphics->AddDynamicNode(tire_smoke.GetNode());
	for (std::vector<CarGraphics>::iterator i = car_graphics.begin(); i != car_graphics.end(); ++i)
	{
		graphics->AddDynamicNode(i->GetNode());
	}
	//gui.GetNode().DebugPrint(info_output);
	PROFILER.endBlock("scenegraph");

	// Send scene information to the graphics subsystem.
	PROFILER.beginBlock("render setup");
	graphics->SetContrast(settings.GetContrast());
	if (active_camera)
	{
		float fov = active_camera->GetFOV() > 0 ? active_camera->GetFOV() : settings.GetFOV();

		Vec3 reflection_location = active_camera->GetPosition();
		if (carcontrols_local.first)
			reflection_location = ToMathVector<float>(carcontrols_local.first->GetCenterOfMass());

		Quat camlook;
		camlook.Rotate(M_PI_2, 1, 0, 0);
		Quat cam_orientation = -(active_camera->GetOrientation() * camlook);

		graphics->SetupScene(
			fov, settings.GetViewDistance(),
			active_camera->GetPosition(),
			cam_orientation,
			reflection_location,
			error_output);
	}
	else
	{
		graphics->SetupScene(
			settings.GetFOV(), settings.GetViewDistance(),
			Vec3(), Quat(), Vec3(),
			error_output);
	}
	graphics->UpdateScene(dt);
	PROFILER.endBlock("render setup");

	// Sync CPU and GPU (flip the page).
	PROFILER.beginBlock("render sync");
	window.SwapBuffers();
	PROFILER.endBlock("render sync");

	PROFILER.beginBlock("render draw");
	graphics->DrawScene(error_output);
	PROFILER.endBlock("render draw");
}

/* The main game loop... */
void Game::MainLoop()
{
	while (!eventsystem.GetQuit() && (!benchmode || replay.GetPlaying()))
	{
		CalculateFPS();

		clocktime += eventsystem.Get_dt();

		eventsystem.BeginFrame();

		// Do CPU intensive stuff in parallel with the GPU...
		Tick(eventsystem.Get_dt());

		Draw(eventsystem.Get_dt());

		eventsystem.EndFrame();

		PROFILER.endCycle();

		displayframe++;
	}
}

/* Deltat is in seconds... */
void Game::Tick(float deltat)
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
	while (target_time - timestep * frame > timestep && curticks < maxticks)
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

	UpdateParticleGraphics();

	gui.Update(eventsystem.Get_dt());
}

/* Increment game logic by one frame... */
void Game::AdvanceGameLogic()
{
	//PROFILER.beginBlock("input-processing");

	eventsystem.ProcessEvents();

	float car_speed = 0;
	if (carcontrols_local.first)
		car_speed = carcontrols_local.first->GetSpeed();

	carcontrols_local.second.ProcessInput(
			settings.GetJoyType(),
			eventsystem,
			timestep,
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
		ai.Update(timestep, &car_dynamics[0], car_dynamics.size());
		PROFILER.endBlock("ai");

		PROFILER.beginBlock("physics");
		dynamics.update(timestep);
		PROFILER.endBlock("physics");

		PROFILER.beginBlock("car");
		UpdateCars(timestep);
		PROFILER.endBlock("car");

		// Update dynamic track objects.
		track.Update();

		//PROFILER.beginBlock("timer");
		UpdateTimer();
		//PROFILER.endBlock("timer");

		//PROFILER.beginBlock("particles");
		UpdateParticles(timestep);
		//PROFILER.endBlock("particles");

		//PROFILER.beginBlock("trackmap-update");
		UpdateTrackMap();
		//PROFILER.endBlock("trackmap-update");
	}

	if (sound.Enabled())
	{
		bool pause_sound = pause || gui.Active();
		PROFILER.beginBlock("sound");
		Vec3 pos;
		Quat rot;
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
	UpdateForceFeedback(timestep);
	//PROFILER.endBlock("force-feedback");
}

/* Process inputs used only for higher level game functions... */
void Game::ProcessGameInputs()
{
	// Most game inputs are allowed whether or not there's a car in the game.
	if (carcontrols_local.second.GetInput(GameInput::SCREENSHOT) == 1.0)
	{
		// Determine filename.
		std::string shotfile;
		for (int i = 1; i < 999; i++)
		{
			std::ostringstream s;
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

	if (carcontrols_local.second.GetInput(GameInput::RELOAD_SHADERS) == 1.0)
	{
		info_output << "Reloading shaders" << std::endl;
		if (!graphics->ReloadShaders(info_output, error_output))
		{
			error_output << "Error reloading shaders" << std::endl;
		}
	}

	if (carcontrols_local.second.GetInput(GameInput::RELOAD_GUI) == 1.0)
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
		if (carcontrols_local.second.GetInput(GameInput::PAUSE) == 1.0)
		{
			pause = !pause;
		}
	}
}

void Game::UpdateTimer()
{
	// Check for cars doing a lap.
	for (int i = 0; i != car_dynamics.size(); ++i)
	{
		const CarDynamics & car = car_dynamics[i];
		int carid = cartimerids[&car];

		bool advance = false;
		int nextsector = 0;
		if (track.GetSectors() > 0)
		{
			nextsector = (timer.GetLastSector(carid) + 1) % track.GetSectors();
			for (int p = 0; p < 4; ++p)
			{
				const Bezier * patch = car.GetWheelContact(WheelPosition(p)).GetPatch();
				if (patch == track.GetSectorPatch(nextsector))
				{
					advance = true;
				}
			}
		}

		if (advance)
			timer.Lap(carid, nextsector);

		// Update how far the car is on the track...
		// Find the patch under the front left wheel...
		const Bezier * curpatch = car.GetWheelContact(FRONT_LEFT).GetPatch();
		if (!curpatch)
			curpatch = car.GetWheelContact(FRONT_RIGHT).GetPatch();

		// Only update if car is on track.
		if (curpatch)
		{
			Vec3 pos = ToMathVector<float>(car.GetCenterOfMass());
			Vec3 back_left, back_right, front_left;
			if (!track.IsReversed())
			{
				back_left = curpatch->GetBL();
				back_right = curpatch->GetBR();
				front_left = curpatch->GetFL();
			}
			else
			{
				back_left = curpatch->GetFL();
				back_right = curpatch->GetFR();
				front_left = curpatch->GetBL();
			}

			Vec3 forwardvec = front_left - back_left;
			Vec3 relative_pos = pos - back_left;
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
			info_output << w << "=" << i->GetCurPatch(WHEEL_POSITION(w)) << ", ";
		}
		info_output << std::endl;*/
	}

	timer.Tick(timestep);
	//timer.DebugPrint(info_output);
}

void Game::UpdateTrackMap()
{
	std::list <std::pair<Vec3, bool> > carpositions;
	for (int i = 0; i != car_dynamics.size(); ++i)
	{
		const CarDynamics & car = car_dynamics[i];
		bool player = (carcontrols_local.first == &car);
		Vec3 carpos = ToMathVector<float>(car.GetCenterOfMass());
		carpositions.push_back(std::make_pair(carpos, player));
	}

	trackmap.Update(settings.GetTrackmap(), carpositions);
}

void Game::ProcessGUIInputs()
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

	gui.ProcessInput(
		eventsystem.GetMousePosition()[0] / (float)window.GetW(),
		eventsystem.GetMousePosition()[1] / (float)window.GetH(),
		eventsystem.GetMouseButtonState(1).down,
		eventsystem.GetMouseButtonState(1).just_up,
		carcontrols_local.second.GetInput(GameInput::GUI_LEFT),
		carcontrols_local.second.GetInput(GameInput::GUI_RIGHT),
		carcontrols_local.second.GetInput(GameInput::GUI_UP),
		carcontrols_local.second.GetInput(GameInput::GUI_DOWN),
		carcontrols_local.second.GetInput(GameInput::GUI_SELECT),
		carcontrols_local.second.GetInput(GameInput::GUI_CANCEL));

	if (controlgrab && AssignControl())
	{
		controlgrab = false;
		ActivateEditControlPage();
	}
}

bool Game::AssignControl()
{
	assert(controlgrab && "Trying to assign control with no control grabbed.");

	// Check for key inputs.
	std::map <SDL_Keycode, Toggle> & keymap = eventsystem.GetKeyMap();
	for (std::map <SDL_Keycode, Toggle>::iterator i = keymap.begin(); i != keymap.end(); ++i)
	{
		if (i->second.GetImpulseRising())
		{
			controlgrab_control.type = CarControlMap::Control::KEY;
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
				controlgrab_control.type = CarControlMap::Control::JOY;
				controlgrab_control.joynum = j;
				controlgrab_control.joytype = CarControlMap::Control::JOYBUTTON;
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
				controlgrab_control.type = CarControlMap::Control::JOY;
				controlgrab_control.joytype = CarControlMap::Control::JOYAXIS;
				controlgrab_control.joynum = j;
				controlgrab_control.joyaxis = i;
				if (axis > 0)
					controlgrab_control.joyaxistype = CarControlMap::Control::POSITIVE;
				else if (axis < 0)
					controlgrab_control.joyaxistype = CarControlMap::Control::NEGATIVE;
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
			controlgrab_control.type = CarControlMap::Control::MOUSE;
			controlgrab_control.mousetype = CarControlMap::Control::MOUSEBUTTON;
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
		controlgrab_control.type = CarControlMap::Control::MOUSE;
		controlgrab_control.mousetype = CarControlMap::Control::MOUSEMOTION;
		if (dx < -threshold)
			controlgrab_control.mdir = CarControlMap::Control::LEFT;
		else if (dx > threshold)
			controlgrab_control.mdir = CarControlMap::Control::RIGHT;
		else if (dy < -threshold)
			controlgrab_control.mdir = CarControlMap::Control::UP;
		else if (dy > threshold)
			controlgrab_control.mdir = CarControlMap::Control::DOWN;
		carcontrols_local.second.SetControl(controlgrab_input, controlgrab_id, controlgrab_control);
		return true;
	}

	return false;
}

void Game::LoadControlsIntoGUIPage(const std::string & page_name)
{
	std::map<std::string, std::string> label_text;
	carcontrols_local.second.GetControlsInfo(label_text);
	gui.SetLabelText(page_name, label_text);
}

void Game::LoadControlsIntoGUI()
{
	std::map<std::string, std::string> label_text;
	carcontrols_local.second.GetControlsInfo(label_text);
	gui.SetLabelText(label_text);
}

void Game::UpdateStartList()
{
	GuiOption::List startlist;
	PopulateStartList(startlist);
	gui.SetOptionValues("game.startlist", cast(car_edit_id), startlist, error_output);
}

void Game::UpdateCarPosList()
{
	const std::string startpos = cast(car_edit_id);
	GuiOption::List startpos_list;
	startpos_list.reserve(car_info.size());
	for (size_t i = 0; i < car_info.size(); ++i)
	{
		startpos_list.push_back(std::make_pair(cast(i), cast(i + 1)));
	}
	gui.SetOptionValues("game.car_startpos", startpos, startpos_list, error_output);
}

void Game::UpdateCarInfo()
{
	const CarInfo & info = car_info[car_edit_id];
	gui.SetOptionValue("game.driver", info.driver);
	gui.SetOptionValue("game.car", info.name);
	gui.SetOptionValue("game.car_paint", info.paint);
	gui.SetOptionValue("game.car_color_hue", cast(info.hsv[0]));
	gui.SetOptionValue("game.car_color_sat", cast(info.hsv[1]));
	gui.SetOptionValue("game.car_color_val", cast(info.hsv[2]));
	gui.SetOptionValue("game.car_startpos", cast(car_edit_id));
	gui.SetOptionValue("game.ai_level", cast(info.ailevel));
}

void Game::UpdateCars(float dt)
{
	for (int i = 0; i < car_dynamics.size(); ++i)
	{
		UpdateCarInputs(i);

		car_graphics[i].Update(car_dynamics[i]);

		car_sounds[i].Update(car_dynamics[i], dt);

		AddTireSmokeParticles(car_dynamics[i], dt);

		UpdateDriftScore(car_dynamics[i], dt);
	}
}

void Game::UpdateCarInputs(int carid)
{
	CarDynamics & car = car_dynamics[carid];
	CarGraphics & car_gfx = car_graphics[carid];
	CarSound & car_snd = car_sounds[carid];

	std::vector <float> carinputs(CarInput::INVALID, 0.0f);
	if (replay.GetPlaying())
	{
		const std::vector<float> inputs = replay.PlayFrame(carid, car);
		assert(inputs.size() <= carinputs.size());
		for (size_t i = 0; i < inputs.size(); ++i)
		{
			carinputs[i] = inputs[i];
		}
	}
	else if (carcontrols_local.first == &car)
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
					ai.AddCar(&car, 1.0);
				}
				else
				{
					info_output << "Switching to user controlled player." << std::endl;
					ai.RemoveCar(&car);
				}
			}
		}
		if (aiControlled)
		{
			carinputs = ai.GetInputs(&car);
			assert(carinputs.size() == CarInput::INVALID);
		}
#endif
	}
	else
	{
		carinputs = ai.GetInputs(&car);
		assert(carinputs.size() == CarInput::INVALID);
	}

	// Force brake at start and once the race is over.
	if (timer.Staging())
	{
		carinputs[CarInput::BRAKE] = 1.0;
	}
	else if (race_laps > 0 && (int)timer.GetCurrentLap(cartimerids[&car]) > race_laps)
	{
		carinputs[CarInput::BRAKE] = 1.0;
		carinputs[CarInput::THROTTLE] = 0.0;
	}

	car.Update(carinputs);
	car_gfx.Update(carinputs);

	// Record car state.
	if (replay.GetRecording())
	{
		replay.RecordFrame(carid, carinputs, car);
	}

	// Local player input processing starts here.
	if (carcontrols_local.first != &car)
		return;

	inputgraph.Update(carinputs);

	std::ostringstream debug_info1, debug_info2, debug_info3, debug_info4;
	if (settings.GetDebugInfo())
	{
		car.DebugPrint(debug_info1, true, false, false, false);
		car.DebugPrint(debug_info2, false, true, false, false);
		car.DebugPrint(debug_info3, false, false, true, false);
		car.DebugPrint(debug_info4, false, false, false, true);
	}

	const std::pair <int, int> curplace = timer.GetPlayerPlace();
	const int tid = cartimerids[&car];
	hud.Update(
		gui.GetFont(), fonts["lcd"], window.GetW(), window.GetH(),
		timer.GetPlayerTime(), timer.GetLastLap(), timer.GetBestLap(), timer.GetStagingTimeLeft(),
		timer.GetPlayerCurrentLap(), race_laps, curplace.first, curplace.second, settings.GetMPH(),
		car.GetTachoRPM(), car.GetEngine().GetRedline(), car.GetEngine().GetRPMLimit(),
		car.GetSpeedMPS(), car.GetMaxSpeedMPS(),
		car.GetClutch().GetPosition(), car.GetTransmission().GetGear(),
		car.GetFuelAmount(), car.GetNosAmount(), carinputs[CarInput::NOS],
		car.GetABSEnabled(), car.GetABSActive(), car.GetTCSEnabled(), car.GetTCSActive(),
		timer.GetIsDrifting(tid), timer.GetDriftScore(tid), timer.GetThisDriftScore(tid),
		debug_info1.str(), debug_info2.str(), debug_info3.str(), debug_info4.str());

	// Handle camera mode change inputs.
	Camera * old_camera = active_camera;
	CarControlMap & carcontrol = carcontrols_local.second;
	unsigned int camera_id = settings.GetCamera();
	if (carcontrol.GetInput(GameInput::VIEW_HOOD))
		camera_id = 0;
	else if (carcontrol.GetInput(GameInput::VIEW_INCAR))
		camera_id = 1;
	else if (carcontrol.GetInput(GameInput::VIEW_CHASERIGID))
		camera_id = 2;
	else if (carcontrol.GetInput(GameInput::VIEW_CHASE))
		camera_id = 3;
	else if (carcontrol.GetInput(GameInput::VIEW_ORBIT))
		camera_id = 4;
	else if (carcontrol.GetInput(GameInput::VIEW_FREE))
		camera_id = 5;
	else if (carcontrol.GetInput(GameInput::VIEW_NEXT))
		camera_id++;
	else if (carcontrol.GetInput(GameInput::VIEW_PREV))
		camera_id--;

	// wrap around
	const unsigned int camera_count = car_gfx.GetCameras().size();
	if (camera_id > camera_count)
		camera_id = camera_count - 1;
	else if (camera_id == camera_count)
		camera_id = 0;

	// set active camear
	active_camera = car_gfx.GetCameras()[camera_id];
	settings.SetCamera(camera_id);

	// handle rear view
	Vec3 pos = ToMathVector<float>(car.GetPosition());
	Quat rot = ToQuaternion<float>(car.GetOrientation());
	if (carcontrol.GetInput(GameInput::VIEW_REAR))
		rot.Rotate(M_PI, 0, 0, 1);

	// reset camera on change
	if (old_camera != active_camera)
		active_camera->Reset(pos, rot);
	else
		active_camera->Update(pos, rot, timestep);

	// Handle camera inputs.
	float left = timestep * (carcontrol.GetInput(GameInput::PAN_LEFT) - carcontrol.GetInput(GameInput::PAN_RIGHT));
	float up = timestep * (carcontrol.GetInput(GameInput::PAN_UP) - carcontrol.GetInput(GameInput::PAN_DOWN));
	float dy = timestep * (carcontrol.GetInput(GameInput::ZOOM_IN) - carcontrol.GetInput(GameInput::ZOOM_OUT));
	Vec3 zoom(Direction::Forward * 4 * dy);
	active_camera->Rotate(up, left);
	active_camera->Move(zoom[0], zoom[1], zoom[2]);

	// Hide glass if we're inside the car, adjust sounds.
	bool incar = (camera_id == 0 || camera_id == 1);
	car_gfx.EnableInteriorView(incar);
	car_snd.EnableInteriorSound(incar);

	// Move up the close shadow distance if we're in the cockpit.
	graphics->SetCloseShadow(incar ? 1.0 : 5.0);
}

bool Game::NewGame(bool playreplay, bool addopponents, int num_laps)
{
	// This should clear out all data.
	LeaveGame();

	// Cache number of laps for gui.
	race_laps = num_laps;

	// Start out with no camera.
	active_camera = NULL;

	// Get cars count.
	size_t cars_num = addopponents ? car_info.size() : 1;

	// Set track, car config file.
	std::string trackname = settings.GetTrack();

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
		car_info = replay.GetCarInfo();
		cars_num = car_info.size();
	}

	// Load track.
	if (!LoadTrack(trackname))
	{
		error_output << "Error during track loading: " << trackname << std::endl;
		return false;
	}

	// Load cars.
	car_dynamics.reserve(cars_num);
	car_graphics.reserve(cars_num);
	car_sounds.reserve(cars_num);
	for (size_t i = 0; i < cars_num; ++i)
	{
		if (!LoadCar(car_info[i], track.GetStart(i).first, track.GetStart(i).second, sound.Enabled()))
			return false;
	}

	// Load timer.
	float pretime = (num_laps > 0) ? 3.0f : 0.0f;
	if (!timer.Load(pathmanager.GetTrackRecordsPath()+"/"+trackname+".txt", pretime))
	{
		error_output << "Unable to load timer" << std::endl;
		return false;
	}

	// Add cars to the timer system.
	for (int i = 0; i < car_dynamics.size(); ++i)
	{
		cartimerids[&car_dynamics[i]] = timer.AddCar(car_info[i].name);
		if (carcontrols_local.first == &car_dynamics[i])
			timer.SetPlayerCarId(i);
	}

	// bind vertex data
	std::vector<SceneNode *> nodes;
	nodes.push_back(&track.GetRacinglineNode());
	nodes.push_back(&track.GetTrackNode());
	nodes.push_back(&track.GetBodyNode());
	for (std::vector<CarGraphics>::iterator i = car_graphics.begin(); i != car_graphics.end(); ++i)
	{
		nodes.push_back(&i->GetNode());
	}
	graphics->BindStaticVertexData(nodes);

	// Set up GUI.
	gui.SetInGame(true);
	gui.Deactivate();
	ShowHUD(true);
	if (settings.GetMouseGrab())
	{
		window.ShowMouseCursor(false);
	}

	// Record a replay.
	if (settings.GetRecordReplay() && !playreplay)
	{
		std::string prev_car_name;
		for (size_t i = 0; i < car_info.size(); ++i)
		{
			// load car config
			CarInfo & info = car_info[i];
			if (prev_car_name == info.name)
			{
				info.config = car_info[i - 1].config;
			}
			else
			{
				const size_t n0 = info.name.find("/");
				const size_t n1 = info.name.length();
				const std::string carname = info.name.substr(n0 + 1, n1 - n0 - 1);
				const std::string cardir = pathmanager.GetCarsDir() + "/" + info.name.substr(0, n0);
				std::tr1::shared_ptr<PTree> carcfg;
				content.load(carcfg, cardir, carname + ".car");

				std::ostringstream carstream;
				write_ini(*carcfg, carstream);
				info.config = carstream.str();

				prev_car_name = info.name;
			}
		}

		replay.StartRecording(car_info, settings.GetTrack(), error_output);
	}

	content.sweep();
	return true;
}

std::string Game::GetReplayRecordingFilename()
{
	// Get time.
	time_t curtime = time(0);
	tm now = *localtime(&curtime);

	// Time string.
	char timestr[]= "MM-DD-hh-mm";
	const char format[] = "%m-%d-%H-%M";
	strftime(timestr, sizeof(timestr), format, &now);

	// Replay file name.
	std::ostringstream s;
	s << pathmanager.GetReplayPath() << "/" << timestr << "-" << settings.GetTrack() << ".vdr";
	return s.str();
}

bool Game::LoadCar(
	const CarInfo & info,
	const Vec3 & position,
	const Quat & orientation,
	const bool sound_enabled)
{
	const size_t n0 = info.name.find("/");
	const size_t n1 = info.name.length();
	const std::string carname = info.name.substr(n0 + 1, n1 - n0 - 1);
	const std::string cardir = pathmanager.GetCarsDir() + "/" + info.name.substr(0, n0);

	std::tr1::shared_ptr<PTree> carconf;
	if (info.config.empty())
	{
		content.load(carconf, cardir, carname + ".car");
		if (!carconf->size())
		{
			error_output << "Failed to load car config: " << info.name << std::endl;
			return false;
		}
	}
	else
	{
		carconf.reset(new PTree());
		std::istringstream carstream(info.config);
		read_ini(carstream, *carconf);
	}

	Vec3 color;
	HSVtoRGB(info.hsv[0], info.hsv[1], info.hsv[2], color[0], color[1], color[2]);

	car_graphics.push_back(CarGraphics());
	CarGraphics & car_gfx = car_graphics.back();
	if (!car_gfx.Load(
		*carconf, cardir, carname, info.wheel, info.paint, color,
		settings.GetAnisotropy(), settings.GetCameraBounce(),
		content, error_output))
	{
		error_output << "Failed to load graphics for car: " << info.name << std::endl;
		car_graphics.pop_back();
		return false;
	}

	car_sounds.push_back(CarSound());
	CarSound & car_snd = car_sounds.back();
	if (sound_enabled && !car_snd.Load(cardir, carname, sound, content, error_output))
	{
		error_output << "Failed to load sounds for car: " << info.name << std::endl;
		car_graphics.pop_back();
		car_sounds.pop_back();
		return false;
	}

	car_dynamics.push_back(CarDynamics());
	CarDynamics & car = car_dynamics[car_dynamics.size() - 1];
	if (!car.Load(
		*carconf, cardir, info.tire,
		ToBulletVector(position),
		ToBulletQuaternion(orientation),
		settings.GetVehicleDamage(),
		dynamics, content, error_output))
	{
		error_output << "Failed to load physics for car: " << info.name << std::endl;
		car_graphics.pop_back();
		car_sounds.pop_back();
		car_dynamics.pop_back();
		return false;
	}

	bool isai = (info.driver != "user");
	if (!isai)
		carcontrols_local.first = &car;
	else
		ai.AddCar(&car, info.ailevel, info.driver);

	car.SetAutoClutch(settings.GetAutoClutch() || isai);
	car.SetAutoShift(settings.GetAutoShift() || isai);
	car.SetABS(settings.GetABS() || isai);
	car.SetTCS(settings.GetTCS() || isai);

	info_output << "Car loading was successful: " << info.name << std::endl;

	return true;
}

bool Game::LoadTrack(const std::string & trackname)
{
	ShowLoadingScreen(0.0, 1.0, false, "", 0.5, 0.5);

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
		graphics->GetShadows()))
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
			ShowLoadingScreen(count, track.ObjectsNum(), false, "", 0.5, 0.5);
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
			window.GetW(),
			window.GetH(),
			track.GetRoadList(),
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
	graphics->ClearStaticDrawables();
	graphics->AddStaticNode(track.GetTrackNode());
#endif

	return true;
}

void Game::LoadGarage()
{
	LeaveGame();

	// Load track explicitly to avoid track reversed car orientation issue.
	// Proper fix would be to support reversed car orientation in garage.

	ShowLoadingScreen(0.0, 1.0, false, "", 0.5, 0.5);

	bool track_reverse = false;
	bool track_dynamic = false;
	if (!track.DeferredLoad(
		content, dynamics,
		info_output, error_output,
		pathmanager.GetSkinsPath() + "/" + settings.GetSkin(),
		pathmanager.GetSkinsDir() + "/" + settings.GetSkin(),
		pathmanager.GetEffectsTextureDir(),
		pathmanager.GetTrackPartsPath(),
		settings.GetAnisotropy(),
		track_reverse, track_dynamic,
		graphics->GetShadows()))
	{
		error_output << "Error loading garage: " << settings.GetSkin() << std::endl;
		return;
	}

	bool success = true;
	int count = 0;
	while (!track.Loaded() && success)
	{
		int displayevery = track.ObjectsNum() / 50;
		if (displayevery == 0 || count % displayevery == 0)
		{
			ShowLoadingScreen(count, track.ObjectsNum(), false, "", 0.5, 0.5);
		}
		success = track.ContinueDeferredLoad();
		count++;
	}

	if (!success)
	{
		error_output << "Error loading garage: " << settings.GetSkin() << std::endl;
		return;
	}

	// Build static drawlist.
#ifdef USE_STATIC_OPTIMIZATION_FOR_TRACK
	graphics->ClearStaticDrawables();
	graphics->AddStaticNode(track.GetTrackNode());
#endif

	// Load car.
	SetGarageCar();
}

void Game::SetGarageCar()
{
	if (gui.GetInGame() || !track.Loaded())
		return;

	// clear previous car
	carcontrols_local.first = NULL;
	car_dynamics.clear();
	car_graphics.clear();
	car_sounds.clear();

	// load car
	std::vector<SceneNode *> nodes;
	Vec3 car_pos = track.GetStart(0).first;
	Quat car_rot = track.GetStart(0).second;
	if (LoadCar(car_info[car_edit_id], car_pos, car_rot, false))
	{
		// update car position
		dynamics.update(timestep);
		car_graphics.back().Update(car_dynamics[car_dynamics.size() - 1]);
		nodes.push_back(&car_graphics.back().GetNode());
	}
	nodes.push_back(&track.GetTrackNode());
	graphics->BindStaticVertexData(nodes);

	// camera setup
	Vec3 offset(1.5, 3.0, 0.5);
	Vec3 pos = car_pos + offset;
	Quat rot = LookAt(pos, car_pos, Direction::Up);
	garage_camera.SetOffset(offset);
	garage_camera.Reset(pos, rot);
	active_camera = &garage_camera;
}

void Game::SetCarColor()
{
	if (!gui.GetInGame() && !car_graphics.empty())
	{
		float r, g, b;
		const Vec3 & hsv = car_info[car_edit_id].hsv;
		HSVtoRGB(hsv[0], hsv[1], hsv[2], r, g, b);
		car_graphics.back().SetColor(r, g, b);
	}
}

bool Game::LoadFonts()
{
	const std::string fontdir = pathmanager.GetFontDir(settings.GetSkin());
	const std::string fontpath = pathmanager.GetDataPath()+"/"+fontdir;

	if (!fonts["freesans"].Load(fontpath+"/freesans.txt",fontdir, "freesans.png", content, error_output)) return false;
	if (!fonts["lcd"].Load(fontpath+"/lcd.txt",fontdir, "lcd.png", content, error_output)) return false;
	if (!fonts["futuresans"].Load(fontpath+"/futuresans.txt",fontdir, "futuresans.png", content, error_output)) return false;
	if (!fonts["futuresans-noshader"].Load(fontpath+"/futuresans.txt",fontdir, "futuresans_noshaders.png", content, error_output)) return false;

	info_output << "Loaded fonts successfully" << std::endl;

	return true;
}

void Game::CalculateFPS()
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

	std::ostringstream fpsstr;
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
		std::ostringstream summary;
		summary << "CPU:\n" << cpuProfile << "\n\nGPU:\n";
		graphics->printProfilingInfo(summary);
		profiling_text.Revise(summary.str());
	}
}

static void PopulateCarSet(
	std::set<std::pair<std::string, std::string> > & set,
	const std::string & path,
	const PathManager & pathmanager,
	const bool cardironly)
{
	const std::string ext(".car");
	std::list<std::string> folders;
	pathmanager.GetFileList(path, folders);
	if (cardironly)
	{
		for (std::list<std::string>::iterator i = folders.begin(); i != folders.end(); ++i)
		{
			std::ifstream file((path + "/" + *i + "/" + *i + ext).c_str());
			if (file)
				set.insert(std::make_pair(*i, *i));
		}
	}
	else
	{
		for (std::list<std::string>::iterator i = folders.begin(); i != folders.end(); ++i)
		{
			std::list<std::string> files;
			pathmanager.GetFileList(path + "/" + *i, files, ext);
			for (std::list<std::string>::iterator j = files.begin(); j != files.end(); ++j)
			{
				const std::string opt = j->substr(0, j->length() - ext.length());
				const std::string val = *i + "/" + opt;
				set.insert(std::make_pair(val, opt));
			}
		}
	}
}

static void PopulateTrackSet(
	std::set<std::pair<std::string, std::string> > & set,
	const std::string & path,
	const PathManager & pathmanager)
{
	std::list<std::string> folderlist;
	pathmanager.GetFileList(path, folderlist);
	for (std::list <std::string>::iterator i = folderlist.begin(); i != folderlist.end(); ++i)
	{
		std::ifstream file((path + "/" + *i + "/about.txt").c_str());
		if (file)
		{
			std::string name;
			getline(file, name);
			set.insert(std::make_pair(*i, name));
		}
	}
}

template <class T0, class T1>
struct SortPairBySecond
{
	bool operator()(const std::pair<T0, T1> & first, const std::pair<T0, T1> & second)
	{
		return first.second < second.second;
	}
};

void Game::PopulateTrackList(GuiOption::List & tracklist)
{
	// Use set to avoid duplicate entries.
	std::set<std::pair<std::string, std::string> > trackset;
	PopulateTrackSet(trackset, pathmanager.GetReadOnlyTracksPath(), pathmanager);
	PopulateTrackSet(trackset, pathmanager.GetWriteableTracksPath(), pathmanager);

	tracklist.clear();
	for (std::set<std::pair<std::string, std::string> >::const_iterator i = trackset.begin(); i != trackset.end(); i++)
	{
		tracklist.push_back(*i);
	}

	std::sort(tracklist.begin(), tracklist.end(), SortPairBySecond<std::string, std::string>());
}

void Game::PopulateCarList(GuiOption::List & carlist, bool cardironly)
{
	// Use set to avoid duplicate entries.
	std::set <std::pair<std::string, std::string> > carset;
	PopulateCarSet(carset, pathmanager.GetReadOnlyCarsPath(), pathmanager, cardironly);
	PopulateCarSet(carset, pathmanager.GetWriteableCarsPath(), pathmanager, cardironly);

	carlist.clear();
	for (std::set<std::pair<std::string, std::string> >::const_iterator i = carset.begin(); i != carset.end(); i++)
	{
		carlist.push_back(*i);
	}
}

void Game::PopulateCarPaintList(const std::string & carname, GuiOption::List & paintlist)
{
	paintlist.clear();
	paintlist.push_back(std::make_pair("default", "default"));

	std::list<std::string> filelist;
	const std::string cardir = carname.substr(0, carname.find("/"));
	const std::string paintdir = pathmanager.GetCarPaintPath(cardir);
	if (pathmanager.GetFileList(paintdir, filelist, ".png"))
	{
		for (std::list<std::string>::iterator i = filelist.begin(); i != filelist.end(); ++i)
		{
			std::string paintname = i->substr(0, i->find('.'));
			paintlist.push_back(std::make_pair("skins/" + *i, paintname));
		}
	}
}

void Game::PopulateCarTireList(const std::string & carname, GuiOption::List & tirelist)
{
	tirelist.clear();
	tirelist.push_back(std::make_pair("default", "default"));

	std::list<std::string> filelist;
	if (pathmanager.GetFileList(pathmanager.GetCarPartsPath() + "/tire", filelist, ".tire"))
	{
		for (std::list<std::string>::iterator i = filelist.begin(); i != filelist.end(); ++i)
		{
			std::string name = i->substr(0, i->find('.'));
			tirelist.push_back(std::make_pair("tire/" + *i, name));
		}
	}
}

void Game::PopulateCarWheelList(const std::string & carname, GuiOption::List & wheellist)
{
	wheellist.clear();
	wheellist.push_back(std::make_pair("default", "default"));

	std::list<std::string> filelist;
	if (pathmanager.GetFileList(pathmanager.GetCarPartsPath() + "/wheel", filelist, ".wheel"))
	{
		for (std::list<std::string>::iterator i = filelist.begin(); i != filelist.end(); ++i)
		{
			std::string name = i->substr(0, i->find('.'));
			wheellist.push_back(std::make_pair("wheel/" + *i, name));
		}
	}
}

void Game::PopulateDriverList(GuiOption::List & driverlist)
{
	const std::vector<std::string> aitypes = ai.ListFactoryTypes();
	driverlist.clear();
	driverlist.push_back(std::make_pair("user", "user"));
	for (size_t i = 0; i < aitypes.size(); ++i)
	{
		driverlist.push_back(std::make_pair(aitypes[i], aitypes[i]));
	}
}

void Game::PopulateStartList(GuiOption::List & startlist)
{
	startlist.clear();
	for (size_t i = 0; i < car_info.size(); ++i)
	{
		const size_t n0 = car_info[i].name.find("/") + 1;
		const size_t n1 = car_info[i].name.length();
		const std::string carname = car_info[i].name.substr(n0, n1 - n0);
		startlist.push_back(std::make_pair(cast(i), carname + " / " + car_info[i].driver));
	}
}

void Game::PopulateReplayList(GuiOption::List & replaylist)
{
	replaylist.clear();
	int numreplays = 0;
	std::list<std::string> replayfoldercontents;
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

void Game::PopulateGUISkinList(GuiOption::List & skinlist)
{
	std::list<std::string> skins;
	pathmanager.GetFileList(pathmanager.GetSkinsPath(), skins);

	skinlist.clear();
	for (std::list<std::string>::iterator i = skins.begin(); i != skins.end(); ++i)
	{
		if (pathmanager.FileExists(pathmanager.GetSkinsPath() + "/" + *i + "/menus/Main"))
		{
			skinlist.push_back(std::make_pair(*i, *i));
		}
	}
}

void Game::PopulateGUILangList(GuiOption::List & langlist)
{
	std::list<std::string> languages;
	std::string path = pathmanager.GetDataPath() + "/locale/";
	pathmanager.GetFileList(path, languages, ".po");

	langlist.clear();
	langlist.push_back(std::make_pair("en", "en"));
	for (std::list<std::string>::iterator i = languages.begin(); i != languages.end(); ++i)
	{
		if (pathmanager.FileExists(path + *i))
		{
			std::string value = i->substr(0, i->length()-3);
			langlist.push_back(std::make_pair(value, value));
		}
	}
}

void Game::PopulateAnisoList(GuiOption::List & anisolist)
{
	anisolist.clear();
	anisolist.push_back(std::make_pair("0", "Off"));

	int cur = 1;
	int max_aniso = graphics->GetMaxAnisotropy();
	while (cur <= max_aniso)
	{
		std::string anisostr = cast(cur);
		anisolist.push_back(std::make_pair(anisostr, anisostr + "X"));
		cur *= 2;
	}
}

void Game::PopulateAntialiasList(GuiOption::List & antialiaslist)
{
	antialiaslist.clear();
	antialiaslist.push_back(std::make_pair("0", "Off"));
	if (graphics->AntialiasingSupported())
	{
		antialiaslist.push_back(std::make_pair("2", "2X"));
		antialiaslist.push_back(std::make_pair("4", "4X"));
	}
}

void Game::PopulateValueLists(std::map<std::string, GuiOption::List> & valuelists)
{
	PopulateTrackList(valuelists["tracks"]);

	PopulateCarList(valuelists["cars"]);

	PopulateCarPaintList(settings.GetCar(), valuelists["car_paints"]);

	PopulateCarTireList(settings.GetCar(), valuelists["tires"]);

	PopulateCarWheelList(settings.GetCar(), valuelists["wheels"]);

	PopulateDriverList(valuelists["driver"]);

	PopulateStartList(valuelists["startlist"]);

	PopulateReplayList(valuelists["replays"]);

	PopulateGUISkinList(valuelists["skins"]);

	PopulateGUILangList(valuelists["languages"]);

	PopulateAnisoList(valuelists["anisotropy"]);

	PopulateAntialiasList(valuelists["antialiasing"]);

	// PopulateJoystickList
	valuelists["joy_indices"].push_back(std::make_pair("0", "0"));
}

void Game::ProcessNewSettings()
{
	// Update the game with any new setting changes that have just been made.

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
	sound.SetAttenuation(settings.GetSoundAttenuation());
}

void Game::ShowHUD(bool value)
{
	hud.SetVisible(value && settings.GetShowHUD());

	if (value && settings.GetInputGraph())
		inputgraph.Show();
	else
		inputgraph.Hide();
}

void Game::ShowLoadingScreen(float progress, float max, bool drawGui, const std::string & optionalText, float x, float y)
{
	assert(max > 0);
	loadingscreen.Update(progress/max, optionalText, x, y);

	std::vector<SceneNode*> nodes;
	if (drawGui)
		nodes.push_back(&gui.GetNode());
	nodes.push_back(&loadingscreen.GetNode());
	graphics->BindDynamicVertexData(nodes);

	graphics->ClearDynamicDrawables();
	if (drawGui)
		graphics->AddDynamicNode(gui.GetNode());
	graphics->AddDynamicNode(loadingscreen.GetNode());

	graphics->SetupScene(45.0, 100.0, Vec3(), Quat(), Vec3(), error_output);
	window.SwapBuffers();
	graphics->DrawScene(error_output);
}

bool GameDownloader::operator()(const std::string & file)
{
	return game.Download(file);
}

bool GameDownloader::operator()(const std::vector <std::string> & urls)
{
	return game.Download(urls);
}

bool Game::Download(const std::string & file)
{
	std::vector <std::string> files;
	files.push_back(file);
	return Download(files);
}

bool Game::Download(const std::vector <std::string> & urls)
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

			HttpInfo info;
			http.GetRequestInfo(url, info);

			if (info.state == HttpInfo::FAILED)
			{
				http.CancelAllRequests();
				error_output << "Failed when downloading URL: " << url << std::endl;
				return false;
			}

			std::ostringstream text;
			text << HttpInfo::GetString(info.state);
			if (info.state == HttpInfo::DOWNLOADING)
			{
				text << " " << Http::ExtractFilenameFromUrl(url);
				text << " " << HttpInfo::FormatSize(info.downloaded);
				//text << " " << HTTPINFO::FormatSpeed(info.speed);
			}
			double total = 1000000;
			if (info.totalsize > 0)
				total = info.totalsize;

			// Tick the GUI...
			eventsystem.BeginFrame();
			gui.Update(eventsystem.Get_dt());
			eventsystem.EndFrame();

			ShowLoadingScreen(fmod(info.downloaded,total), total, true, text.str(), 0.5, 0.5);
		}

		HttpInfo info;
		http.GetRequestInfo(url, info);
		if (info.state == HttpInfo::FAILED)
		{
			http.CancelAllRequests();
			error_output << "Failed when downloading URL: " << url << std::endl;
			return false;
		}
	}

	return true;
}

void Game::UpdateForceFeedback(float dt)
{
	const float ffdt = 0.02;

	if (carcontrols_local.first)
	{
		ff_update_time += dt;
		if (ff_update_time >= ffdt )
		{
			ff_update_time = 0.0;

			float feedback = carcontrols_local.first->GetFeedback();

			// scale
			feedback = feedback * settings.GetFFGain();

			// invert
			if (settings.GetFFInvert()) feedback = -feedback;

			// clamp
			if (feedback > 1.0) feedback = 1.0;
			if (feedback < -1.0) feedback = -1.0;

			forcefeedback->update(feedback, ffdt, error_output);
		}
	}

	if (pause && dt == 0)
	{
		forcefeedback->update(0.0f, ffdt, error_output);
	}
}

void Game::AddTireSmokeParticles(const CarDynamics & car, float dt)
{
	// Only spawn particles every so often...
	unsigned int interval = 0.2 / dt;
	if (particle_timer % interval == 0)
	{
		for (int i = 0; i < 4; i++)
		{
			float squeal = car.GetTireSquealAmount(WheelPosition(i));
			if (squeal > 0)
			{
				btVector3 p = car.GetWheelContact(WheelPosition(i)).GetPosition();
				tire_smoke.AddParticle(ToMathVector<float>(p), 0.5);
			}
		}
	}
}

void Game::UpdateParticles(float dt)
{
	tire_smoke.Update(dt);
	particle_timer = (particle_timer + 1) % (unsigned int)((1.0 / timestep));
}

void Game::UpdateParticleGraphics()
{
	if (track.Loaded() && active_camera)
	{
		Quat camlook;
		camlook.Rotate(M_PI_2, 1, 0, 0);
		Quat camorient = -(active_camera->GetOrientation() * camlook);
		Vec3 campos = active_camera->GetPosition();
		float znear = 0.1f; // hardcoded in graphics
		float zfar = settings.GetViewDistance();
		tire_smoke.UpdateGraphics(camorient, campos, znear, zfar);
	}
}

void Game::UpdateDriftScore(const CarDynamics & car, float dt)
{
	// Assert that the car is registered with the timer system.
	assert(cartimerids.find(&car) != cartimerids.end());

	// Make sure the car is not off track.
	int wheel_count = 0;
	for (int i = 0; i < 4; i++)
	{
		if (car.GetWheelContact(WheelPosition(i)).GetPatch())
			wheel_count++;
	}

	bool on_track = (wheel_count > 1);
	bool is_drifting = false;
	bool spin_out = false;
	if (on_track)
	{
		// Car's velocity on the horizontal plane (should use surface plane here).
		btVector3 car_velocity = car.GetVelocity();
		car_velocity[2] = 0;
		float car_speed = car_velocity.length();

		// Car's direction on the horizontal plane.
		btVector3 car_direction = quatRotate(car.GetOrientation(), Direction::forward);
		car_direction[2] = 0;
		float dir_mag = car_direction.length();

		// Speed must be above 10 m/s and orientation must be valid.
		if (car_speed > 10 && dir_mag > 0.01)
		{
			// Angle between car's direction and velocity.
			float cos_angle = car_direction.dot(car_velocity) / (car_speed * dir_mag);
			if (cos_angle > 1) cos_angle = 1;
			else if (cos_angle < -1) cos_angle = -1;
			float car_angle = acosf(cos_angle);

			// Drift starts when the angle > 0.2 (around 11.5 degrees).
			// Drift ends when the angle < 0.1 (aournd 5.7 degrees).
			float angle_threshold(0.2);
			if (timer.GetIsDrifting(cartimerids[&car])) angle_threshold = 0.1;

			is_drifting = (car_angle > angle_threshold && car_angle <= M_PI / 2.0);
			spin_out = (car_angle > M_PI / 2.0);

			// Calculate score.
			if (is_drifting)
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

void Game::BeginStartingUp()
{
	std::ofstream f(pathmanager.GetStartupFile().c_str());
}

void Game::DoneStartingUp()
{
	std::remove(pathmanager.GetStartupFile().c_str());
}

bool Game::LastStartWasSuccessful() const
{
	return !pathmanager.FileExists(pathmanager.GetStartupFile());
}

void Game::QuitGame()
{
	info_output << "Got quit message from GUI. Shutting down..." << std::endl;
	eventsystem.Quit();
}

void Game::LeaveGame()
{
	ai.ClearCars();

	carcontrols_local.first = NULL;

	if (replay.GetRecording())
	{
		std::string replayname = GetReplayRecordingFilename();
		info_output << "Saving replay to " << replayname << std::endl;
		replay.StopRecording(replayname);

		GuiOption::List replaylist;
		PopulateReplayList(replaylist);
		gui.SetOptionValues("game.selected_replay", "", replaylist, error_output);
	}

	if (replay.GetPlaying())
		replay.Reset();

	gui.SetInGame(false);
	gui.ActivatePage("Main", 0.25, error_output);

	graphics->ClearStaticDrawables();

	tire_smoke.Clear();
	track.Clear();
	car_dynamics.clear();
	car_graphics.clear();
	car_sounds.clear();
	sound.Update(true);
	hud.SetVisible(false);
	inputgraph.Hide();
	trackmap.Unload();
	timer.Unload();
	active_camera = 0;
	pause = false;
	race_laps = 0;
}

void Game::StartPractice()
{
	practice = true;
	if (!NewGame())
	{
		LoadGarage();
	}
}

void Game::StartRace()
{
	practice = (car_info.size() < 2);
	int num_laps = practice ? 0 : settings.GetNumberOfLaps();
	bool play_replay = false;
	if (!NewGame(play_replay, !practice, num_laps))
	{
		LoadGarage();
	}
}

void Game::ReturnToGame()
{
	if (gui.Active())
	{
		if (settings.GetMouseGrab())
			window.ShowMouseCursor(false);
		gui.Deactivate();
		ShowHUD(true);
	}
}

void Game::RestartGame()
{
	bool play_replay = false;
	int num_laps = race_laps;
	if (!NewGame(play_replay, !practice, num_laps))
	{
		LoadGarage();
	}
}

void Game::StartReplay()
{
	if (settings.GetSelectedReplay() != "none"  && !NewGame(true))
	{
		gui.ActivatePage("ReplayStartError", 0.25, error_output);
	}
}

void Game::HandleOnlineClicked()
{
	std::string motdUrl = "vdrift.net/online/motd.txt";
	bool success = Download(motdUrl);
	if (success)
	{
		gui.ActivatePage("Online", 0.25, error_output);
		std::string motdFile = http.GetDownloadPath(motdUrl);
		std::string motd = Utils::LoadFileIntoString(motdFile, error_output);
		gui.SetLabelText("Online", "Motd", motd);
	}
}

void Game::StartCheckForUpdates()
{
	carupdater.StartCheckForUpdates(GameDownloader(*this, http), gui);
	trackupdater.StartCheckForUpdates(GameDownloader(*this, http), gui);
	gui.SetOptionValue("update.cars", cast(carupdater.GetUpdatesNum()));
	gui.SetOptionValue("update.tracks", cast(trackupdater.GetUpdatesNum()));
	gui.ActivatePage("UpdatesFound", 0.25, error_output);
}

void Game::StartCarManager()
{
	carupdater.Reset();
	carupdater.Show(gui);
}

void Game::CarManagerNext()
{
	carupdater.Increment();
	carupdater.Show(gui);
}

void Game::CarManagerPrev()
{
	carupdater.Decrement();
	carupdater.Show(gui);
}

void Game::ApplyCarUpdate()
{
	carupdater.ApplyUpdate(GameDownloader(*this, http), gui, pathmanager);
}

void Game::StartTrackManager()
{
	trackupdater.Reset();
	trackupdater.Show(gui);
}

void Game::TrackManagerNext()
{
	trackupdater.Increment();
	trackupdater.Show(gui);
}

void Game::TrackManagerPrev()
{
	trackupdater.Decrement();
	trackupdater.Show(gui);
}

void Game::ApplyTrackUpdate()
{
	trackupdater.ApplyUpdate(GameDownloader(*this, http), gui, pathmanager);
}

void Game::ActivateEditControlPage()
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

void Game::CancelControl()
{
	gui.ActivatePage(controlgrab_page, 0.25, error_output);
	LoadControlsIntoGUIPage(controlgrab_page);
}

void Game::DeleteControl()
{
	carcontrols_local.second.DeleteControl(controlgrab_input, controlgrab_id);

	gui.ActivatePage(controlgrab_page, 0.25, error_output);
	LoadControlsIntoGUIPage(controlgrab_page);
}

void Game::SetButtonControl()
{
	controlgrab_control.onetime = (gui.GetOptionValue("controledit.once") == "true");
	controlgrab_control.pushdown = (gui.GetOptionValue("controledit.down") == "true");
	carcontrols_local.second.SetControl(controlgrab_input, controlgrab_id, controlgrab_control);

	gui.ActivatePage(controlgrab_page, 0.25, error_output);
	LoadControlsIntoGUIPage(controlgrab_page);
}

void Game::SetAnalogControl()
{
	controlgrab_control.deadzone = cast<float>(gui.GetOptionValue("controledit.deadzone"));
	controlgrab_control.exponent = cast<float>(gui.GetOptionValue("controledit.exponent"));
	controlgrab_control.gain = cast<float>(gui.GetOptionValue("controledit.gain"));
	carcontrols_local.second.SetControl(controlgrab_input, controlgrab_id, controlgrab_control);

	gui.ActivatePage(controlgrab_page, 0.25, error_output);
	LoadControlsIntoGUIPage(controlgrab_page);
}

void Game::LoadControls()
{
	carcontrols_local.second.Load(
		pathmanager.GetCarControlsFile(),
		info_output,
		error_output);

	LoadControlsIntoGUIPage(gui.GetActivePageName());
}

void Game::SaveControls()
{
	carcontrols_local.second.Save(pathmanager.GetCarControlsFile());
}

void Game::SyncOptions()
{
	std::map<std::string, std::string> optionmap;
	settings.Get(optionmap);

	// hack: store player car info only
	CarInfo & info = car_info[player_car_id];
	assert(info.driver == "user");
	info.name = optionmap["game.car"];
	info.paint = optionmap["game.car_paint"];
	info.tire = optionmap["game.car_tire"];
	info.wheel = optionmap["game.car_wheel"];
	info.hsv[0] = cast<float>(optionmap["game.car_color_hue"]);
	info.hsv[1] = cast<float>(optionmap["game.car_color_sat"]);
	info.hsv[2] = cast<float>(optionmap["game.car_color_val"]);
	if (car_edit_id > 0)
	{
		optionmap.erase("game.driver");
		optionmap.erase("game.car");
		optionmap.erase("game.car_paint");
		optionmap.erase("game.car_tire");
		optionmap.erase("game.car_wheel");
		optionmap.erase("game.car_color_hue");
		optionmap.erase("game.car_color_sat");
		optionmap.erase("game.car_color_val");
	}

	gui.SetOptions(optionmap);
}

void Game::SyncSettings()
{
	std::map<std::string, std::string> optionmap;
	settings.Get(optionmap);
	gui.GetOptions(optionmap);

	// hack: store first car info only
	const CarInfo & info = car_info[player_car_id];
	assert(info.driver == "user");
	optionmap["game.driver"] = info.driver;
	optionmap["game.car"] = info.name;
	optionmap["game.car_paint"] = info.paint;
	optionmap["game.car_tire"] = info.tire;
	optionmap["game.car_wheel"] = info.wheel;
	optionmap["game.car_color_hue"] = cast(info.hsv[0]);
	optionmap["game.car_color_sat"] = cast(info.hsv[1]);
	optionmap["game.car_color_val"] = cast(info.hsv[2]);

	settings.Set(optionmap);
	ProcessNewSettings();
}

void Game::SelectPlayerCar()
{
	SetCarToEdit(cast(player_car_id));
}

void Game::SetCarToEdit(const std::string & value)
{
	// set current car as active car
	size_t prev_edit_id = car_edit_id;
	car_edit_id = cast<size_t>(value);

	if (car_edit_id == prev_edit_id)
		return;

	// if car differs from last car force reload
	const CarInfo & info_prev = car_info[prev_edit_id];
	const CarInfo & info = car_info[car_edit_id];
	if (info.name != info_prev.name)
	{
		SetGarageCar();
	}

	UpdateCarInfo();
}

void Game::SetCarStartPos(const std::string & value)
{
	const size_t car_pos_old = car_edit_id;
	const size_t car_pos_new = cast<size_t>(value);
	if (car_pos_new == car_pos_old)
		return;

	assert(car_pos_new < car_info.size());
	CarInfo info = car_info[car_pos_new];
	car_info[car_pos_new] = car_info[car_pos_old];
	car_info[car_pos_old] = info;

	if (player_car_id == car_pos_old)
		player_car_id = car_pos_new;
	else if (player_car_id == car_pos_new)
		player_car_id = car_pos_old;

	car_edit_id = car_pos_new;

	UpdateStartList();
}

void Game::SetCarName(const std::string & value)
{
	CarInfo & info = car_info[car_edit_id];
	if (info.name == value)
		return;

	car_info[car_edit_id].name = value;

	GuiOption::List paintlist;
	PopulateCarPaintList(value, paintlist);
	gui.SetOptionValues("game.car_paint", info.paint, paintlist, error_output);

	UpdateStartList();

	SetGarageCar();
}

void Game::SetCarPaint(const std::string & value)
{
	CarInfo & info = car_info[car_edit_id];
	if (info.paint != value)
	{
		info.paint = value;
		SetGarageCar();
	}
}

void Game::SetCarTire(const std::string & value)
{
	CarInfo & info = car_info[car_edit_id];
	if (info.tire != value)
	{
		info.tire = value;
		SetGarageCar();
	}
}

void Game::SetCarWheel(const std::string & value)
{
	CarInfo & info = car_info[car_edit_id];
	if (info.wheel != value)
	{
		info.wheel = value;
		SetGarageCar();
	}
}

void Game::SetCarColorHue(const std::string & value)
{
	car_info[car_edit_id].hsv[0] = cast<float>(value);
	SetCarColor();
}

void Game::SetCarColorSat(const std::string & value)
{
	car_info[car_edit_id].hsv[1] = cast<float>(value);
	SetCarColor();
}

void Game::SetCarColorVal(const std::string & value)
{
	car_info[car_edit_id].hsv[2] = cast<float>(value);
	SetCarColor();
}

void Game::SetCarDriver(const std::string & value)
{
	CarInfo & info = car_info[car_edit_id];
	if (value == info.driver)
		return;

	// reset option in the no opponents case
	if (car_info.size() == 1)
	{
		gui.SetOptionValue("game.driver", "user");
		return;
	}

	// swap driver with user
	if (value == "user")
	{
		CarInfo & pinfo = car_info[player_car_id];
		pinfo.driver = info.driver;
		player_car_id = car_edit_id;
	}
	else if (player_car_id == car_edit_id)
	{
		player_car_id = (car_edit_id + 1) % car_info.size();
		CarInfo & pinfo = car_info[player_car_id];
		pinfo.driver = "user";
	}
	info.driver = value;

	UpdateStartList();
}

void Game::SetCarAILevel(const std::string & value)
{
	car_info[car_edit_id].ailevel = cast<float>(value);
}

void Game::SetCarsNum(const std::string & value)
{
	size_t cars_num = cast<size_t>(value);
	int delta = cars_num - car_info.size();
	if (delta == 0)
		return;

	if (delta < 0)
	{
		for (size_t i = car_info.size(); i > cars_num; --i)
		{
			if (player_car_id == i - 1)
			{
				CarInfo & info = car_info[--player_car_id];
				info.driver = "user";
			}
			car_info.pop_back();
		}

		if (car_edit_id >= cars_num)
			car_edit_id = cars_num;
	}
	else if (delta > 0)
	{
		car_info.reserve(cars_num);
		for (size_t i = car_info.size(); i < cars_num; ++i)
		{
			// variate color, lame version
			float hue = car_info.back().hsv[0] + 0.07;
			if (hue > 1.0f) hue -= 1.0f;
			Vec3 hsv(hue, 0.95, 0.5);

			car_info.push_back(car_info.back());
			CarInfo & info = car_info.back();
			info.driver = ai.default_type;
			info.hsv = hsv;
		}
	}

	UpdateCarPosList();

	UpdateStartList();

	UpdateCarInfo();
}

void Game::SetControl(const std::string & value)
{
	if (controlgrab)
	{
		assert(0 && "Trying to set a control while assigning one.");
		return;
	}

	std::istringstream vs(value);
	std::string inputstr, idstr, oncestr, downstr;
	getline(vs, inputstr, ':');
	getline(vs, idstr, ':');
	getline(vs, oncestr, ':');
	getline(vs, downstr);

	size_t id = 0;
	std::istringstream ns(idstr);
	ns >> id;

	controlgrab_control = carcontrols_local.second.GetControl(inputstr, id);
	controlgrab_page = gui.GetActivePageName();
	controlgrab_input = inputstr;
	controlgrab_id = id;

	if (controlgrab_control.type == CarControlMap::Control::UNKNOWN)
	{
		// assign control
		controlgrab_mouse_coords = std::make_pair(eventsystem.GetMousePosition()[0], eventsystem.GetMousePosition()[1]);
		controlgrab_joystick_state = eventsystem.GetJoysticks();

		// default control settings
		if (!oncestr.empty())
			controlgrab_control.onetime = (oncestr == "once");
		if (!downstr.empty())
			controlgrab_control.pushdown = (downstr == "down");

		controlgrab = true;

		gui.ActivatePage("AssignControl", 0.25, error_output);
	}
	else
	{
		ActivateEditControlPage();
	}
}

void Game::BindActionsToGUI()
{
	set_car_toedit.connect(gui.GetOption("game.startlist").signal_val);
	set_car_startpos.connect(gui.GetOption("game.car_startpos").signal_val);
	set_car_name.connect(gui.GetOption("game.car").signal_val);
	set_car_paint.connect(gui.GetOption("game.car_paint").signal_val);
	set_car_tire.connect(gui.GetOption("game.car_tire").signal_val);
	set_car_wheel.connect(gui.GetOption("game.car_wheel").signal_val);
	set_car_color_hue.connect(gui.GetOption("game.car_color_hue").signal_val);
	set_car_color_sat.connect(gui.GetOption("game.car_color_sat").signal_val);
	set_car_color_val.connect(gui.GetOption("game.car_color_val").signal_val);
	set_car_driver.connect(gui.GetOption("game.driver").signal_val);
	set_car_ailevel.connect(gui.GetOption("game.ai_level").signal_val);
	set_cars_num.connect(gui.GetOption("game.cars_num").signal_val);
	set_control.connect(gui.GetOption("controledit.string").signal_val);
}

void Game::RegisterActions()
{
	set_car_toedit.call.bind<Game, &Game::SetCarToEdit>(this);
	set_car_startpos.call.bind<Game, &Game::SetCarStartPos>(this);
	set_car_name.call.bind<Game, &Game::SetCarName>(this);
	set_car_paint.call.bind<Game, &Game::SetCarPaint>(this);
	set_car_tire.call.bind<Game, &Game::SetCarTire>(this);
	set_car_wheel.call.bind<Game, &Game::SetCarWheel>(this);
	set_car_color_hue.call.bind<Game, &Game::SetCarColorHue>(this);
	set_car_color_sat.call.bind<Game, &Game::SetCarColorSat>(this);
	set_car_color_val.call.bind<Game, &Game::SetCarColorVal>(this);
	set_car_driver.call.bind<Game, &Game::SetCarDriver>(this);
	set_car_ailevel.call.bind<Game, &Game::SetCarAILevel>(this);
	set_cars_num.call.bind<Game, &Game::SetCarsNum>(this);
	set_control.call.bind<Game, &Game::SetControl>(this);

	actions.resize(26);
	actions[0].call.bind<Game, &Game::QuitGame>(this);
	actions[1].call.bind<Game, &Game::LoadGarage>(this);
	actions[2].call.bind<Game, &Game::StartPractice>(this);
	actions[3].call.bind<Game, &Game::StartRace>(this);
	actions[4].call.bind<Game, &Game::ReturnToGame>(this);
	actions[5].call.bind<Game, &Game::RestartGame>(this);
	actions[6].call.bind<Game, &Game::StartReplay>(this);
	actions[7].call.bind<Game, &Game::HandleOnlineClicked>(this);
	actions[8].call.bind<Game, &Game::StartCheckForUpdates>(this);
	actions[9].call.bind<Game, &Game::StartCarManager>(this);
	actions[10].call.bind<Game, &Game::CarManagerNext>(this);
	actions[11].call.bind<Game, &Game::CarManagerPrev>(this);
	actions[12].call.bind<Game, &Game::ApplyCarUpdate>(this);
	actions[13].call.bind<Game, &Game::StartTrackManager>(this);
	actions[14].call.bind<Game, &Game::TrackManagerNext>(this);
	actions[15].call.bind<Game, &Game::TrackManagerPrev>(this);
	actions[16].call.bind<Game, &Game::ApplyTrackUpdate>(this);
	actions[17].call.bind<Game, &Game::CancelControl>(this);
	actions[18].call.bind<Game, &Game::DeleteControl>(this);
	actions[19].call.bind<Game, &Game::SetButtonControl>(this);
	actions[20].call.bind<Game, &Game::SetAnalogControl>(this);
	actions[21].call.bind<Game, &Game::LoadControls>(this);
	actions[22].call.bind<Game, &Game::SaveControls>(this);
	actions[23].call.bind<Game, &Game::SyncOptions>(this);
	actions[24].call.bind<Game, &Game::SyncSettings>(this);
	actions[25].call.bind<Game, &Game::SelectPlayerCar>(this);
}

void Game::InitActionMap(std::map<std::string, Slot0*> & actionmap)
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
	actionmap["SelectPlayerCar"] = &actions[25];
}
