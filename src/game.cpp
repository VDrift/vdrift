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
#include "minmax.h"
#include "tobullet.h"
#include "hsvtorgb.h"
#include "camera_orbit.h"
#include "tokenize.h"

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

static std::string GetTimeString(float time)
{
	if (time != 0)
	{
		int minutes = time * (1 / 60.0f);
		float seconds = time - minutes * 60;
		std::ostringstream s;
		s << std::setfill('0');
		s << std::setw(2) << minutes << ":";
		s << std::fixed << std::setprecision(3) << std::setw(6) << seconds;
		return s.str();
	}
	return "--:--.---";
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
	pause(true),
	controlgrab_id(0),
	controlgrab(false),
	garage_camera("garagecam"),
	active_camera(0),
	car_info(1),
	player_car_id(0),
	camera_car_id(0),
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
	http("/tmp"),
	ff_update_time(0)
{
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
	if (!car_controls_local.Load(pathmanager.GetCarControlsFile(), info_output, error_output))
	{
		info_output << "Car control file " << pathmanager.GetCarControlsFile() << " doesn't exist; using defaults" << std::endl;
		car_controls_local.Load(pathmanager.GetDefaultCarControlsFile(), info_output, error_output);
		car_controls_local.Save(pathmanager.GetCarControlsFile());
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
	if (!InitGUI())
	{
		error_output << "Error initializing graphical user interface" << std::endl;
		return;
	}

	// Load particle system.
	Vec3 smokedir(0.4, 0.2, 1.0);
	tire_smoke.Load(pathmanager.GetEffectsTextureDir(), "smoke.png", settings.GetAnisotropy(), content);
	tire_smoke.SetParameters(settings.GetParticles(), 0.4,0.9, 1,4, 0.3,0.6, 0.02,0.06, smokedir);

	// Initialize force feedback.
	forcefeedback.reset(new ForceFeedback(settings.GetFFDevice(), error_output, info_output));
	ff_update_time = 0;

	if (benchmode)
	{
		assert(!car_info.empty());
		car_info[player_car_id].driver = Ai::default_type;

		if (!NewGame(false, false, 1))
		{
			error_output << "Error loading benchmark" << std::endl;
			return;
		}
	}
	else
	{
		LoadGarage();
	}

	DoneStartingUp();

	Run();

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
			settings.GetAntialiasing(), settings.GetShadows(),
			settings.GetShadowDistance(), settings.GetShadowQuality(),
			settings.GetReflections(),pathmanager.GetStaticReflectionMap(),
			pathmanager.GetStaticAmbientMap(),
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

	car_info[0].driver = "";
	car_info[0].name = settings.GetCar();
	car_info[0].paint = settings.GetCarPaint();
	car_info[0].ailevel = settings.GetAILevel();
	car_info[0].hsv = hsv;
	player_car_id = 0;
}

bool Game::InitGUI()
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
		auto i = menufiles.begin();
		while (i != menufiles.end())
		{
			if (i->find("~") != std::string::npos || *i == "SConscript")
				i = menufiles.erase(i);
			else
				i++;
		}
	}

	std::map<std::string, GuiOption::List> valuelists;
	PopulateValueLists(valuelists);

	std::map<std::string, Signal1<const std::string &>*> vsignalmap;
	std::map<std::string, Slot0*> actionmap;
	InitSignalMap(vsignalmap);
	InitActionMap(actionmap);

	if (!gui.Load(
		menufiles,
		valuelists,
		pathmanager.GetDataPath(),
		pathmanager.GetOptionsFile(),
		settings.GetSkin(),
		settings.GetLanguage(),
		(float)window.GetH() / window.GetW(),
		vsignalmap,
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
	gui.SetOptionValue("game.driver", "");

	// Set input control labels
	LoadControlsIntoGUI();

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
	for (auto i = args.begin(); i != args.end(); ++i)
	{
		if ((*i)[0] == '-')
		{
			argmap[*i] = "";
		}

		auto n = i;
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
		for (const auto & arg : arghelp)
			if (arg.first.size() > longest)
				longest = arg.first.size();
		for (const auto & arg : arghelp)
		{
			helpstr.append(arg.first);
			for (unsigned int n = 0; n < longest+3-arg.first.size(); n++)
				helpstr.push_back(' ');
			helpstr.append(arg.second + "\n");
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
	nodes.reserve(5);

	nodes.push_back(&dynamicsdraw.getNode());
	nodes.push_back(&trackmap.GetNode());
	nodes.push_back(&tire_smoke.GetNode());

	if (gui.GetNodes().first)
		nodes.push_back(gui.GetNodes().first);

	if (gui.GetNodes().second)
		nodes.push_back(gui.GetNodes().second);

	graphics->BindDynamicVertexData(nodes);

	graphics->ClearDynamicDrawables();
	graphics->AddDynamicNode(dynamicsdraw.getNode());
	graphics->AddDynamicNode(track.GetBodyNode());
	graphics->AddDynamicNode(track.GetRacinglineNode());
	graphics->AddDynamicNode(trackmap.GetNode());
	graphics->AddDynamicNode(tire_smoke.GetNode());

	for (auto & car : car_graphics)
		graphics->AddDynamicNode(car.GetNode());

	if (gui.GetNodes().first)
		graphics->AddDynamicNode(*gui.GetNodes().first);

	if (gui.GetNodes().second)
		graphics->AddDynamicNode(*gui.GetNodes().second);

	PROFILER.endBlock("scenegraph");

	// Send scene information to the graphics subsystem.
	PROFILER.beginBlock("render setup");
	graphics->SetContrast(settings.GetContrast());
	if (active_camera)
	{
		float fov = active_camera->GetFOV() > 0 ? active_camera->GetFOV() : settings.GetFOV();

		Vec3 reflection_location = active_camera->GetPosition();
		if (camera_car_id < unsigned(car_dynamics.size()))
			reflection_location = ToMathVector<float>(car_dynamics[camera_car_id].GetCenterOfMass());

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

void Game::Run()
{
	while (!eventsystem.GetQuit())
		Advance();
}

void Game::Advance()
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

/* Deltat is in seconds... */
void Game::Tick(float deltat)
{
	// This is the minimum fps the game will run at before it starts slowing down time.
	const float minfps = 10;
	// Slow the game down if we can't process fast enough.
	const unsigned int maxticks = (int) (1 / (minfps * timestep));
	// Slow the game down if we can't process fast enough.
	const float maxtime = 1 / minfps;
	unsigned int curticks = 0;

	// Throw away wall clock time if necessary to keep the framerate above the minimum.
	if (deltat > maxtime)
		deltat = maxtime;

	target_time += deltat;

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

	float car_speed = !pause ? car_dynamics[player_car_id].GetSpeed() : 0;
	car_controls_local.ProcessInput(
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

	if (!pause)
	{
		PROFILER.beginBlock("ai");
		ai.Visualize();
		ai.Update(timestep, &car_dynamics[0], car_dynamics.size());
		PROFILER.endBlock("ai");

		//PROFILER.beginBlock("input");
		ProcessCarInputs();
		//PROFILER.endBlock("input");

		PROFILER.beginBlock("physics");
		dynamics.update(timestep);
		PROFILER.endBlock("physics");

		PROFILER.beginBlock("car");
		ProcessCameraInputs();
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
		sound.Update(pause);
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
	if (car_controls_local.GetInput(GameInput::SCREENSHOT) == 1)
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

	if (car_controls_local.GetInput(GameInput::RELOAD_SHADERS) == 1)
	{
		info_output << "Reloading shaders" << std::endl;
		if (!graphics->ReloadShaders(info_output, error_output))
		{
			error_output << "Error reloading shaders" << std::endl;
		}
	}

	if (car_controls_local.GetInput(GameInput::RELOAD_GUI) == 1)
	{
		info_output << "Reloading GUI" << std::endl;

		// First, save the active page name so we can get back to in...
		std::string currentPage = gui.GetActivePageName();

		// Reload GUI
		if (!InitGUI())
		{
			error_output << "Error reloading GUI" << std::endl;
		}

		gui.ActivatePage(currentPage, 0.5, error_output);
	}
}

void Game::UpdateTimer()
{
	// Check for cars doing a lap.
	for (int i = 0; i < car_dynamics.size(); ++i)
	{
		const CarDynamics & car = car_dynamics[i];
		bool advance = false;
		int nextsector = 0;
		if (track.GetSectors() > 0)
		{
			nextsector = (timer.GetLastSector(i) + 1) % track.GetSectors();
			for (int p = 0; p < 4; ++p)
			{
				const RoadPatch * patch = car.GetWheelContact(WheelPosition(p)).GetPatch();
				if (patch == track.GetSectorPatch(nextsector))
				{
					advance = true;
				}
			}
		}

		if (advance)
			timer.Lap(i, nextsector);

		// Update how far the car is on the track...
		// Find the patch under the front left wheel...
		const RoadPatch * curpatch = car.GetWheelContact(FRONT_LEFT).GetPatch();
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

			if (forwardvec.MagnitudeSquared() > 1E-8f)
				dist_from_back = relative_pos.dot(forwardvec.Normalize());

			timer.UpdateDistance(i, curpatch->GetDistFromStart() + dist_from_back);
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
	std::vector<Vec3> positions(car_dynamics.size());
	for (unsigned i = 0; i < positions.size(); ++i)
	{
		positions[i] = ToMathVector<float>(car_dynamics[i].GetCenterOfMass());
	}
	trackmap.Update(settings.GetTrackmap(), camera_car_id, positions);
}

void Game::ProcessGUIInputs()
{
	gui.ProcessInput(
		eventsystem.GetMousePosition()[0] / (float)window.GetW(),
		eventsystem.GetMousePosition()[1] / (float)window.GetH(),
		eventsystem.GetMouseButtonState(1).GetState(),
		eventsystem.GetMouseButtonState(1).GetImpulseFalling(),
		car_controls_local.GetInput(GameInput::GUI_LEFT),
		car_controls_local.GetInput(GameInput::GUI_RIGHT),
		car_controls_local.GetInput(GameInput::GUI_UP),
		car_controls_local.GetInput(GameInput::GUI_DOWN),
		car_controls_local.GetInput(GameInput::GUI_SELECT),
		car_controls_local.GetInput(GameInput::GUI_CANCEL));

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
	for (const auto & key : eventsystem.GetKeyMap())
	{
		if (key.second.GetImpulseRising())
		{
			controlgrab_control.device = CarControlMap::Control::KEYBOARD;
			controlgrab_control.id = key.first;
			car_controls_local.SetControl(controlgrab_input, controlgrab_id, controlgrab_control);
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
				controlgrab_control.device = j;
				controlgrab_control.id = i;
				controlgrab_control.type = CarControlMap::Control::BUTTON;
				car_controls_local.SetControl(controlgrab_input, controlgrab_id, controlgrab_control);
				return true;
			}
		}

		// Check for joystick axes.
		for (int i = 0; i < eventsystem.GetNumAxes(j); ++i)
		{
			assert(j < (int)controlgrab_joystick_state.size());
			assert(i < controlgrab_joystick_state[j].GetNumAxes());

			float delta = eventsystem.GetJoyAxis(j, i) - controlgrab_joystick_state[j].GetAxis(i);
			if (delta > 0.4f || delta < -0.4f)
			{
				controlgrab_control.device = j;
				controlgrab_control.id = i;
				controlgrab_control.type = CarControlMap::Control::AXIS;
				controlgrab_control.negative = delta < 0;
				car_controls_local.SetControl(controlgrab_input, controlgrab_id, controlgrab_control);
				return true;
			}
		}
	}

	// Check for mouse button inputs.
	for (int i = 1; i < 4; ++i)
	{
		if (eventsystem.GetMouseButtonState(i).GetImpulseRising())
		{
			controlgrab_control.device = CarControlMap::Control::MOUSE;
			controlgrab_control.type = CarControlMap::Control::BUTTON;
			controlgrab_control.id = i;
			car_controls_local.SetControl(controlgrab_input, controlgrab_id, controlgrab_control);
			return true;
		}
	}

	// Check for mouse motion inputs.
	int dx = eventsystem.GetMousePosition()[0] - controlgrab_mouse_coords.first;
	int dy = eventsystem.GetMousePosition()[1] - controlgrab_mouse_coords.second;
	int threshold = 200;
	if (dx < -threshold || dx > threshold || dy < -threshold || dy > threshold)
	{
		controlgrab_control.device = CarControlMap::Control::MOUSE;
		controlgrab_control.type = CarControlMap::Control::AXIS;
		if (dx < -threshold)
		{
			controlgrab_control.id = CarControlMap::Control::MOUSEX;
			controlgrab_control.negative = true;
		}
		else if (dx > threshold)
		{
			controlgrab_control.id = CarControlMap::Control::MOUSEX;
			controlgrab_control.negative = false;
		}
		else if (dy < -threshold)
		{
			controlgrab_control.id = CarControlMap::Control::MOUSEY;
			controlgrab_control.negative = true;
		}
		else if (dy > threshold)
		{
			controlgrab_control.id = CarControlMap::Control::MOUSEY;
			controlgrab_control.negative = false;
		}
		car_controls_local.SetControl(controlgrab_input, controlgrab_id, controlgrab_control);
		return true;
	}

	return false;
}

void Game::LoadControlsIntoGUIPage(const std::string & page_name)
{
	std::map<std::string, std::string> label_text;
	car_controls_local.GetControlsInfo(label_text);
	gui.SetLabelText(page_name, label_text);
}

void Game::LoadControlsIntoGUI()
{
	std::map<std::string, std::string> label_text;
	car_controls_local.GetControlsInfo(label_text);
	gui.SetLabelText(label_text);
}

void Game::UpdateStartList()
{
	GuiOption::List startlist;
	PopulateStartList(startlist);
	gui.SetOptionValues("game.startlist", cast(car_edit_id + 1), startlist, error_output);
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

void Game::UpdateCarSpecs()
{
	GuiOption & spec_option = gui.GetOption("game.car_specs");
	GuiOption::List spec_list = spec_option.GetValueList();
	UpdateCarSpecList(spec_list);
	spec_option.SetValues("", spec_list);
}

void Game::UpdateCarSpecList(GuiOption::List & spec_list)
{
	assert(car_dynamics.size() > 0);
	std::vector<float> specs = car_dynamics[0].GetSpecs();

	if (specs.size() < 7 || spec_list.size() < 8)
		return;

	bool imperial = settings.GetMPH();

	std::ostringstream s;
	s << std::fixed << std::setprecision(0);
	s << round(specs[0] * (imperial ? 2.2369f : 3.6f)) << (imperial ? " mph" : " km/h");
	spec_list[0].first = s.str();

	spec_list[1].first = (specs[1] == 1 ? "FWD" : (specs[1] == 2 ? "RWD" : "AWD"));

	s.str("");
	s << round(specs[2] * 1E6f) << " cc";
	spec_list[2].first = s.str();

	s.str("");
	s << round(specs[3] * 1.341E-3f) << " hp";
	spec_list[3].first = s.str();

	s.str("");
	s << round(specs[4] * (imperial ? 0.73756f : 1)) << (imperial ? " ft-lb" : " Nm");
	spec_list[4].first = s.str();

	s.str("");
	s << round(specs[5] * (imperial ? 2.20462f : 1)) << (imperial ? " lb" : " kg");
	spec_list[5].first = s.str();

	s.str("");
	s << round(specs[6] * 100) << " %";
	spec_list[6].first = s.str();

	s.str("");
	s << std::setprecision(1);
	s << specs[5] / specs[3] * (imperial ? 1644 : 745.7f) << (imperial ? " lb/hp" : " kg/hp");
	spec_list[7].first = s.str();
}

void Game::UpdateCars(float dt)
{
	for (int i = 0; i < car_dynamics.size(); ++i)
	{
		car_graphics[i].Update(car_dynamics[i]);

		car_sounds[i].Update(car_dynamics[i], dt);

		AddTireSmokeParticles(car_dynamics[i], dt);

		UpdateDriftScore(i, dt);
	}
}

void Game::ProcessCarInputs()
{
	bool player_control = car_info[player_car_id].driver.empty();
	#ifdef VISUALIZE_AI_DEBUG
	if (!player_control)
	{
		// It allows to deactivate the AI on the player car with F9 button.
		// This is useful for bringing the car in strange
		// situations and test how the AI solves it.
		static bool override_ai = false;
		static bool button_pressed = false;
		if (button_pressed != eventsystem.GetKeyState(SDLK_F9).just_down)
		{
			button_pressed = !button_pressed;
			if (button_pressed)
			{
				override_ai = !override_ai;
				if (override_ai)
					info_output << "Switching to user controlled player car." << std::endl;
				else
					info_output << "Switching to ai controlled player car." << std::endl;
			}
		}
		player_control = override_ai;
	}
	#endif

	for (unsigned carid = 0, aiid = 0; carid < unsigned(car_dynamics.size()); ++carid)
	{
		CarDynamics & car = car_dynamics[carid];
		CarGraphics & car_gfx = car_graphics[carid];

		std::vector <float> carinputs(CarInput::INVALID, 0.0f);
		if (replay.GetPlaying())
			carinputs = replay.PlayFrame(carid, car);
		else if (carid == player_car_id && player_control)
			carinputs = car_controls_local.GetInputs();
		else
			carinputs = ai.GetInputs(aiid++);

		assert(carinputs.size() >= CarInput::INVALID);

		// Force brake at start and once the race is over.
		if (timer.Staging())
		{
			carinputs[CarInput::BRAKE] = 1.0;
			carinputs[CarInput::CLUTCH] = 1.0;
		}
		else if (race_laps > 0 && (int)timer.GetCurrentLap(carid) > race_laps)
		{
			carinputs[CarInput::BRAKE] = 1.0;
			carinputs[CarInput::CLUTCH] = 1.0;
			carinputs[CarInput::THROTTLE] = 0.0;

			if (benchmode)
				eventsystem.Quit();
		}

		car.Update(carinputs);
		car_gfx.Update(carinputs);

		// Record car state.
		if (replay.GetRecording())
			replay.RecordFrame(carid, carinputs, car);

		if (carid == camera_car_id && settings.GetHUD() != "NoHud")
			UpdateHUD(carid, carinputs);
	}
}

void Game::ProcessCameraInputs()
{
	CarControlMap & carcontrol = car_controls_local;

	// Handle camera focus
	unsigned car_count = car_dynamics.size();
	if (carcontrol.GetInput(GameInput::FOCUS_NEXT))
		camera_car_id = (camera_car_id < car_count - 1) ? camera_car_id + 1 : 0;
	else if (carcontrol.GetInput(GameInput::FOCUS_PREV))
		camera_car_id = (camera_car_id > 0) ? camera_car_id - 1 : car_count - 1;

	CarDynamics & car = car_dynamics[camera_car_id];
	CarGraphics & car_gfx = car_graphics[camera_car_id];
	CarSound & car_snd = car_sounds[camera_car_id];

	// Handle camera mode change inputs.
	unsigned camera_id = settings.GetCamera();
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
	unsigned camera_count = car_gfx.GetCameras().size();
	if (camera_id > camera_count)
		camera_id = camera_count - 1;
	else if (camera_id == camera_count)
		camera_id = 0;

	// set active camera
	Camera * old_camera = active_camera;
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

void Game::UpdateHUD(const size_t carid, const std::vector<float> & carinputs)
{
	const CarDynamics & car = car_dynamics[carid];
	const GuiLanguage & lang = gui.GetLanguageDict();

	if (settings.GetDebugInfo())
	{
		if (!profilingmode)
		{
			std::ostringstream debug_info[4];
			debug_info[0] << std::fixed << std::setprecision(2);
			debug_info[1] << std::fixed << std::setprecision(2);
			debug_info[2] << std::fixed << std::setprecision(2);
			debug_info[3] << std::fixed << std::setprecision(2);

			car.DebugPrint(debug_info[0], true, false, false, false);
			car.DebugPrint(debug_info[1], false, true, false, false);
			car.DebugPrint(debug_info[2], false, false, true, false);
			car.DebugPrint(debug_info[3], false, false, false, true);

			signal_debug_info[0](debug_info[0].str());
			signal_debug_info[1](debug_info[1].str());
			signal_debug_info[2](debug_info[2].str());
			signal_debug_info[3](debug_info[3].str());
		}
		else if (frame % 10 == 0)
		{
			std::ostringstream gpu_profile;
			graphics->printProfilingInfo(gpu_profile);

			signal_debug_info[0](PROFILER.getAvgSummary(quickprof::MICROSECONDS));
			signal_debug_info[1](gpu_profile.str());
		}
	}

	if (settings.GetInputGraph())
	{
		std::ostringstream steeringstr, throttlestr, brakestr;
		steeringstr << carinputs[CarInput::STEER_RIGHT] - carinputs[CarInput::STEER_LEFT];
		throttlestr << carinputs[CarInput::THROTTLE];
		brakestr << carinputs[CarInput::BRAKE];

		signal_steering(steeringstr.str());
		signal_throttle(throttlestr.str());
		signal_brake(brakestr.str());
	}

	std::pair <int, int> curplace = timer.GetCarPlace(carid);
	std::ostringstream placestr;
	placestr << curplace.first << " / " << curplace.second;

	int cur_lap = Clamp(timer.GetCurrentLap(carid), 1, race_laps);
	std::ostringstream lapstr;
	if (race_laps > 0)
		lapstr << cur_lap << " / " << race_laps;
	else
		lapstr << "0 / 0";

	int score = timer.GetDriftScore(carid);
	std::ostringstream scorestr;
	scorestr << score;

	std::ostringstream msgstr;
	if (race_laps > 0)
	{
		float stagingtimeleft = timer.GetStagingTimeLeft();
		if (stagingtimeleft > 0.5f)
			msgstr << (int)stagingtimeleft + 1;
		else if (stagingtimeleft > 0)
			msgstr << lang("Ready");
		else if (stagingtimeleft < 0 && stagingtimeleft > -1)
			msgstr << lang("GO");
		else if (timer.GetCurrentLap(carid) > race_laps)
			msgstr << ((curplace.first == 1) ? lang("You won!") : lang("You lost"));
	}
	if (msgstr.tellp() <= 0 && timer.GetIsDrifting(carid))
		msgstr << "+" << (int)timer.GetThisDriftScore(carid);

	int gear = car.GetTransmission().GetGear();
	std::ostringstream gearstr;
	if (gear == -1)
		gearstr << "R";
	else if (gear == 0)
		gearstr << "N";
	else
		gearstr << gear;

	float speed_scale = (settings.GetMPH() ? 2.237f : 3.6f);
	float speed = std::abs(car.GetSpeedMPS()) * speed_scale;
	float speedometer = car.GetMaxSpeedMPS() * speed_scale;
	speedometer = Clamp(std::ceil(speedometer / 40.0f) * 40.0f, 120.0f, 320.0f);

	float rpm = car.GetTachoRPM();
	float rpmred = car.GetEngine().GetRedline();
	float tachometer = car.GetEngine().GetRPMLimit();
	tachometer = Clamp(std::ceil(tachometer / 2000.0f) * 2000.0f, 8000.0f, 20000.0f);

	std::ostringstream speedostr, speednstr, speedstr;
	speedostr << int(speedometer);
	speednstr << speed / speedometer;
	speedstr << std::setfill('0') << std::setw(3) << int(speed);

	std::ostringstream shiftstr, tachostr, rpmnstr, rpmrstr, rpmstr;
	shiftstr << int(rpm >= rpmred);
	tachostr << int(tachometer);
	rpmnstr << rpm / tachometer;
	rpmrstr << rpmred / tachometer;
	rpmstr << int(rpm);

	std::ostringstream absstr, tcsstr, gasstr, nosstr;
	absstr << (car.GetABSActive() ? 1.0 : 0.3);
	tcsstr << (car.GetTCSActive() ? 1.0 : 0.3);
	gasstr << (car.GetFuelAmount() ? 0.3 : 1.0);
	nosstr << ((car.GetNosAmount() && carinputs[CarInput::NOS]) ? 1.0 : 0.3);

	signal_lap_time[0](GetTimeString(timer.GetTime(carid)));
	signal_lap_time[1](GetTimeString(timer.GetLastLap(carid)));
	signal_lap_time[2](GetTimeString(timer.GetBestLap(carid)));

	signal_pos(placestr.str());
	signal_lap(lapstr.str());
	signal_score(scorestr.str());
	signal_message(msgstr.str());

	signal_gear(gearstr.str());
	signal_shift(shiftstr.str());

	signal_speedometer(speedostr.str());
	signal_speed_norm(speednstr.str());
	signal_speed(speedstr.str());

	signal_tachometer(tachostr.str());
	signal_rpm_norm(rpmnstr.str());
	signal_rpm_red(rpmrstr.str());
	signal_rpm(rpmstr.str());

	signal_abs(absstr.str());
	signal_tcs(tcsstr.str());
	signal_gas(gasstr.str());
	signal_nos(nosstr.str());
}

bool Game::NewGame(bool playreplay, bool addopponents, int num_laps)
{
	// This should clear out all data.
	LeaveGame();

	// Cache number of laps for gui.
	race_laps = num_laps;

	// Start out with no camera.
	camera_car_id = player_car_id;
	active_camera = NULL;

	// Get cars count.
	size_t cars_num = addopponents ? car_info.size() : 1;

	// Set track, car config file.
	std::string trackname = settings.GetTrack();

	if (playreplay)
	{
		std::string replayfilename = pathmanager.GetReplayPath() + "/" + settings.GetSelectedReplay();
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
		timer.AddCar(car_info[i].name);
	}
	timer.SetPlayerCarId(
		car_info[player_car_id].driver.empty() ? player_car_id : car_info.size());

	// Bind vertex data.
	std::vector<SceneNode *> nodes;
	nodes.push_back(&track.GetRacinglineNode());
	nodes.push_back(&track.GetTrackNode());
	nodes.push_back(&track.GetBodyNode());
	for (auto & car : car_graphics)
	{
		nodes.push_back(&car.GetNode());
	}
	graphics->BindStaticVertexData(nodes);

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
				std::shared_ptr<PTree> carcfg;
				content.load(carcfg, cardir, carname + ".car");

				std::ostringstream carstream;
				write_ini(*carcfg, carstream);
				info.config = carstream.str();

				prev_car_name = info.name;
			}
		}

		replay.StartRecording(car_info, settings.GetTrack(), error_output);
	}

	// Clean up asset cache.
	content.sweep();

	// Set up GUI.
	gui.SetInGame(true);
	gui.ActivatePage("Hud", 0.25, error_output);

	// not strictly needed, is expected to be called by Hud page onfocus event
	ContinueGame();

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

	std::shared_ptr<PTree> carconf;
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
	unsigned carid = car_dynamics.size() - 1;
	CarDynamics & car = car_dynamics[carid];
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

	if (!info.driver.empty())
	{
		ai.AddCar(carid, info.ailevel, info.driver);
		car.SetSteeringAssist(true);
		car.SetAutoReverse(true);
		car.SetAutoClutch(true);
		car.SetAutoShift(true);
		car.SetABS(true);
		car.SetTCS(true);
	}
	else
	{
		car.SetSteeringAssist(settings.GetSteeringAssist());
		car.SetAutoReverse(settings.GetAutoReverse());
		car.SetAutoClutch(settings.GetAutoClutch());
		car.SetAutoShift(settings.GetAutoShift());
		car.SetABS(settings.GetABS());
		car.SetTCS(settings.GetTCS());
	}

	info_output << "Car loading was successful: " << info.name << std::endl;

	return true;
}

bool Game::LoadTrack(const std::string & trackname)
{
	gui.ActivatePage("Loading", 0.5, error_output);

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
	int count_max = track.ObjectsNum();
	int displayevery = count_max / 50;
	while (!track.Loaded() && success)
	{
		if (displayevery == 0 || count % displayevery == 0)
			ShowLoadingScreen(count, count_max, "");

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
	graphics->ClearStaticDrawables();
	graphics->AddStaticNode(track.GetTrackNode());
	graphics->SetFixedSkybox(track.IsFixedSkybox());

	return true;
}

void Game::LoadGarage()
{
	LeaveGame();

	// Load track explicitly to avoid track reversed car orientation issue.
	// Proper fix would be to support reversed car orientation in garage.

	gui.ActivatePage("Loading", 0.5, error_output);

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
	int count_max = track.ObjectsNum();
	int displayevery = count_max / 50;
	while (!track.Loaded() && success)
	{
		if (displayevery == 0 || count % displayevery == 0)
			ShowLoadingScreen(count, count_max, "");

		success = track.ContinueDeferredLoad();
		count++;
	}

	if (!success)
	{
		error_output << "Error loading garage: " << settings.GetSkin() << std::endl;
		return;
	}

	// Build static drawlist.
	graphics->ClearStaticDrawables();
	graphics->AddStaticNode(track.GetTrackNode());
	graphics->SetFixedSkybox(track.IsFixedSkybox());

	// Load car.
	SetGarageCar();

	// Show main page.
	gui.ActivatePage("Main", 0.5, error_output);
}

void Game::SetGarageCar()
{
	if (gui.GetInGame() || !track.Loaded())
		return;

	// clear previous car
	car_dynamics.clear();
	car_graphics.clear();
	car_sounds.clear();

	// load car
	std::vector<SceneNode *> nodes;
	Vec3 car_pos = track.GetStart(0).first;
	Quat car_rot = track.GetStart(0).second;
	if (LoadCar(car_info[car_edit_id], car_pos, car_rot, false))
	{
		car_graphics.back().Update(car_dynamics[car_dynamics.size() - 1]);
		nodes.push_back(&car_graphics.back().GetNode());
	}
	nodes.push_back(&track.GetTrackNode());
	graphics->BindStaticVertexData(nodes);

	// camera setup
	Vec3 offset(0.5, -3.0, 0.5);
	Vec3 pos = car_pos + offset;
	Quat rot = LookAt(pos, car_pos, Direction::Up);
	garage_camera.SetOffset(offset);
	garage_camera.Reset(pos, rot);
	active_camera = &garage_camera;

	UpdateCarSpecs();
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
		std::ostringstream fpsstr;
		fpsstr << (int)fps_avg;
		signal_fps(fpsstr.str());
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
		for (const auto & folder : folders)
		{
			std::ifstream file((path + "/" + folder + "/" + folder + ext).c_str());
			if (file)
				set.insert(std::make_pair(folder, folder));
		}
	}
	else
	{
		for (const auto & folder : folders)
		{
			std::list<std::string> files;
			pathmanager.GetFileList(path + "/" + folder, files, ext);
			for (const auto & file : files)
			{
				const std::string opt = file.substr(0, file.length() - ext.length());
				const std::string val = folder + "/" + opt;
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
	std::list<std::string> folders;
	pathmanager.GetFileList(path, folders);
	for (const auto & folder : folders)
	{
		std::ifstream file((path + "/" + folder + "/about.txt").c_str());
		if (file)
		{
			std::string name;
			getline(file, name);
			set.insert(std::make_pair(folder, name));
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
	for (const auto & track : trackset)
	{
		tracklist.push_back(track);
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
	for (const auto & car : carset)
	{
		carlist.push_back(car);
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
		for (const auto & file : filelist)
		{
			std::string paintname = file.substr(0, file.find('.'));
			paintlist.push_back(std::make_pair("skins/" + file, paintname));
		}
	}
}

void Game::PopulateCarTireList(const std::string & /*carname*/, GuiOption::List & tirelist)
{
	tirelist.clear();
	tirelist.push_back(std::make_pair("default", "default"));

	std::list<std::string> filelist;
	if (pathmanager.GetFileList(pathmanager.GetCarPartsPath() + "/tire", filelist, ".tire"))
	{
		for (const auto & file : filelist)
		{
			std::string name = file.substr(0, file.find('.'));
			tirelist.push_back(std::make_pair("tire/" + file, name));
		}
	}
}

void Game::PopulateCarWheelList(const std::string & /*carname*/, GuiOption::List & wheellist)
{
	wheellist.clear();
	wheellist.push_back(std::make_pair("default", "default"));

	std::list<std::string> filelist;
	if (pathmanager.GetFileList(pathmanager.GetCarPartsPath() + "/wheel", filelist, ".wheel"))
	{
		for (const auto & file : filelist)
		{
			std::string name = file.substr(0, file.find('.'));
			wheellist.push_back(std::make_pair("wheel/" + file, name));
		}
	}
}

void Game::PopulateDriverList(GuiOption::List & driverlist)
{
	const std::vector<std::string> aitypes = ai.ListFactoryTypes();
	driverlist.clear();
	driverlist.push_back(std::make_pair("", "player"));
	for (const auto & aitype : aitypes)
	{
		driverlist.push_back(std::make_pair(aitype, aitype));
	}
}

void Game::PopulateStartList(GuiOption::List & startlist)
{
	startlist.clear();
	for (size_t i = 0; i < car_info.size(); ++i)
	{
		auto & info = car_info[i];
		const size_t n0 = info.name.find("/") + 1;
		const size_t n1 = info.name.length();
		const std::string entry = info.name.substr(n0, n1 - n0) + " / "
			+ (info.driver.empty() ? "player" : info.driver);
		startlist.push_back(std::make_pair(cast(i + 1), entry));
	}
}

void Game::PopulateReplayList(GuiOption::List & replaylist)
{
	replaylist.clear();
	int numreplays = 0;
	std::list<std::string> replayfolder;
	if (pathmanager.GetFileList(pathmanager.GetReplayPath(), replayfolder))
	{
		for (auto & file : replayfolder)
		{
			// Replay expects a formatted string: "MM-DD-hh-mm-trackname.vdr".
			if (file != "benchmark.vdr" &&
				file.find(".vdr") == file.length() - 4 &&
				file.length() > 16)
			{
				// Parse replay name.
				std::string str = file.substr(0, file.length() - 4);
				str[2] = '/';
				str[5] = ' ';
				str[8] = ':';
				str[11] = ' ';

				replaylist.push_back(std::make_pair(file, str));
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
	for (const auto & skin : skins)
	{
		if (pathmanager.FileExists(pathmanager.GetSkinsPath() + "/" + skin + "/menus/Main"))
		{
			skinlist.push_back(std::make_pair(skin, skin));
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
	for (const auto & language : languages)
	{
		if (pathmanager.FileExists(path + language))
		{
			std::string value = language.substr(0, language.length()-3);
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

	if (player_car_id < unsigned(car_dynamics.size()) &&
		car_info[player_car_id].driver.empty())
	{
		car_dynamics[player_car_id].SetAutoClutch(settings.GetAutoClutch());
		car_dynamics[player_car_id].SetAutoShift(settings.GetAutoShift());
		car_dynamics[player_car_id].SetAutoReverse(settings.GetAutoReverse());
		car_dynamics[player_car_id].SetSteeringAssist(settings.GetSteeringAssist());
	}

	sound.SetVolume(settings.GetSoundVolume());
	sound.SetMaxActiveSources(settings.GetMaxSoundSources());
	sound.SetAttenuation(settings.GetSoundAttenuation());
}

void Game::ShowLoadingScreen(float progress, float progress_max, const std::string & /*optional_text*/)
{
	assert(progress_max > 0);

	std::ostringstream loadstr;
	loadstr << progress / progress_max;
	signal_loading(loadstr.str());

	// Tick the gui.
	eventsystem.BeginFrame();
	gui.Update(eventsystem.Get_dt());
	eventsystem.EndFrame();

	std::vector<SceneNode*> nodes;
	nodes.reserve(2);
	if (gui.GetNodes().first)
		nodes.push_back(gui.GetNodes().first);
	if (gui.GetNodes().second)
		nodes.push_back(gui.GetNodes().second);
	graphics->BindDynamicVertexData(nodes);

	graphics->ClearDynamicDrawables();
	if (gui.GetNodes().first)
		graphics->AddDynamicNode(*gui.GetNodes().first);
	if (gui.GetNodes().second)
		graphics->AddDynamicNode(*gui.GetNodes().second);

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

	for (const auto & url : urls)
	{
		bool requestSuccess = http.Request(url, error_output);
		if (!requestSuccess)
		{
			http.CancelAllRequests();
			return false;
		}
		else
		{
			info_output << "Requesting URL " << url << std::endl;
		}

		while (http.Tick())
		{
			if (eventsystem.GetQuit())
			{
				http.CancelAllRequests();
				return false;
			}

			HttpInfo info;
			http.GetRequestInfo(url, info);

			if (info.state == HttpInfo::FAILED)
			{
				http.CancelAllRequests();
				error_output << "Failed when downloading URL: " << url << " with error: " << info.error << std::endl;
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

			std::ostringstream loadstr;
			loadstr << fmod(info.downloaded, total) / total;
			signal_loading(loadstr.str());

			Advance();
		}

		HttpInfo info;
		http.GetRequestInfo(url, info);
		if (info.state == HttpInfo::FAILED)
		{
			http.CancelAllRequests();
			error_output << "Failed when downloading URL: " << url << " with error: " << info.error << std::endl;
			return false;
		}
	}

	return true;
}

void Game::UpdateForceFeedback(float dt)
{
	const float ffdt = 0.02f;
	float feedback = 0.0f;
	if (!pause)
	{
		ff_update_time += dt;
		if (ff_update_time >= ffdt)
		{
			ff_update_time = 0;

			feedback = car_dynamics[player_car_id].GetFeedback();

			// scale
			feedback = feedback * settings.GetFFGain();

			// invert
			if (settings.GetFFInvert()) feedback = -feedback;

			// clamp
			feedback = Clamp(feedback, -1.0f, 1.0f);
		}
	}
	forcefeedback->update(feedback, ffdt, error_output);
}

void Game::AddTireSmokeParticles(const CarDynamics & car, float dt)
{
	// Only spawn particles every so often...
	unsigned int interval = 0.2f / dt;
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
	particle_timer = (particle_timer + 1) % (unsigned int)((1 / timestep));
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

void Game::UpdateDriftScore(const int carid, const float dt)
{
	assert(carid >= 0 && carid < car_dynamics.size());
	const CarDynamics & car = car_dynamics[carid];

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
		if (car_speed > 10 && dir_mag > 0.01f)
		{
			// Angle between car's direction and velocity.
			float cos_angle = car_direction.dot(car_velocity) / (car_speed * dir_mag);
			if (cos_angle > 1) cos_angle = 1;
			else if (cos_angle < -1) cos_angle = -1;
			float car_angle = std::acos(cos_angle);

			// Drift starts when the angle > 0.2 (around 11.5 degrees).
			// Drift ends when the angle < 0.1 (aournd 5.7 degrees).
			float angle_threshold(0.2);
			if (timer.GetIsDrifting(carid)) angle_threshold = 0.1f;

			is_drifting = (car_angle > angle_threshold && car_angle <= float(M_PI_2));
			spin_out = (car_angle > float(M_PI_2));

			// Calculate score.
			if (is_drifting)
			{
				// Base score is the drift distance.
				timer.IncrementThisDriftScore(carid, dt * car_speed);

				// Bonus score calculation is now done in TIMER.
				timer.UpdateMaxDriftAngleSpeed(carid, car_angle, car_speed);
			}
		}
	}

	timer.SetIsDrifting(carid, is_drifting, on_track && !spin_out);
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
	gui.SetInGame(false);

	PauseGame();

	ai.ClearCars();

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

	graphics->ClearStaticDrawables();

	tire_smoke.Clear();
	track.Clear();
	car_dynamics.clear();
	car_graphics.clear();
	car_sounds.clear();
	sound.Update(true);
	trackmap.Unload();
	timer.Unload();
	active_camera = NULL;
	camera_car_id = 0;
	race_laps = 0;
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

void Game::PauseGame()
{
	if (settings.GetMouseGrab())
		window.ShowMouseCursor(true);

	gui.ActivatePage("Main", 0.25, error_output);

	pause = true;
}

void Game::ContinueGame()
{
	if (settings.GetMouseGrab())
		window.ShowMouseCursor(false);

	gui.ActivatePage(settings.GetHUD(), 0.25, error_output);

	pause = false;
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
	car_controls_local.DeleteControl(controlgrab_input, controlgrab_id);

	gui.ActivatePage(controlgrab_page, 0.25, error_output);
	LoadControlsIntoGUIPage(controlgrab_page);
}

void Game::SetButtonControl()
{
	controlgrab_control.onetime = (gui.GetOptionValue("controledit.once") == "true");
	controlgrab_control.pushdown = (gui.GetOptionValue("controledit.down") == "true");
	car_controls_local.SetControl(controlgrab_input, controlgrab_id, controlgrab_control);

	gui.ActivatePage(controlgrab_page, 0.25, error_output);
	LoadControlsIntoGUIPage(controlgrab_page);
}

void Game::SetAnalogControl()
{
	controlgrab_control.deadzone = cast<float>(gui.GetOptionValue("controledit.deadzone"));
	controlgrab_control.exponent = cast<float>(gui.GetOptionValue("controledit.exponent"));
	controlgrab_control.gain = cast<float>(gui.GetOptionValue("controledit.gain"));
	car_controls_local.SetControl(controlgrab_input, controlgrab_id, controlgrab_control);

	gui.ActivatePage(controlgrab_page, 0.25, error_output);
	LoadControlsIntoGUIPage(controlgrab_page);
}

void Game::LoadControls()
{
	car_controls_local.Load(
		pathmanager.GetCarControlsFile(),
		info_output,
		error_output);

	LoadControlsIntoGUIPage(gui.GetActivePageName());
}

void Game::SaveControls()
{
	car_controls_local.Save(pathmanager.GetCarControlsFile());
}

void Game::SyncOptions()
{
	std::map<std::string, std::string> optionmap;
	settings.Get(optionmap);

	// hack: store player car info only
	CarInfo & info = car_info[player_car_id];
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

	// hack: store player car info only
	const CarInfo & info = car_info[player_car_id];
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
	SetCarToEdit(cast(player_car_id + 1));
}

void Game::SetCarToEdit(const std::string & value)
{
	// set current car as active car
	size_t prev_edit_id = car_edit_id;
	car_edit_id = cast<size_t>(value) - 1;
	assert(car_edit_id < car_info.size());

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

	// swap driver with player
	if (value.empty())
	{
		CarInfo & pinfo = car_info[player_car_id];
		pinfo.driver = info.driver;
		player_car_id = car_edit_id;
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
	if (cars_num == car_info.size())
		return;

	if (cars_num < car_info.size())
	{
		if (car_edit_id >= cars_num)
			car_edit_id = cars_num - 1;

		if (player_car_id >= cars_num)
		{
			car_info[cars_num - 1] = car_info[player_car_id];
			player_car_id = cars_num - 1;
		}

		car_info.resize(cars_num);
	}
	else
	{
		car_info.reserve(cars_num);
		for (size_t i = car_info.size(); i < cars_num; ++i)
		{
			// variate color, lame version
			float hue = car_info.back().hsv[0] + 0.07f;
			if (hue > 1) hue -= 1;
			Vec3 hsv(hue, 0.95f, 0.5f);

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

	controlgrab_control = car_controls_local.GetControl(inputstr, id);
	controlgrab_page = gui.GetActivePageName();
	controlgrab_input = inputstr;
	controlgrab_id = id;

	if (controlgrab_control.device == CarControlMap::Control::UNKNOWN)
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
	actions[2].call.bind<Game, &Game::StartRace>(this);
	actions[3].call.bind<Game, &Game::PauseGame>(this);
	actions[4].call.bind<Game, &Game::ContinueGame>(this);
	actions[5].call.bind<Game, &Game::RestartGame>(this);
	actions[6].call.bind<Game, &Game::StartReplay>(this);
	actions[7].call.bind<Game, &Game::StartCheckForUpdates>(this);
	actions[8].call.bind<Http, &Http::CancelAllRequests>(&http);
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
	actionmap["StartRace"] = &actions[2];
	actionmap["PauseGame"] = &actions[3];
	actionmap["ContinueGame"] = &actions[4];
	actionmap["RestartGame"] = &actions[5];
	actionmap["StartReplay"] = &actions[6];
	actionmap["StartCheckForUpdates"] = &actions[7];
	actionmap["CancelDownload"] = &actions[8];
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

void Game::InitSignalMap(std::map<std::string, Signal1<const std::string &>*> & signalmap)
{
	signalmap["game.loading"] = &signal_loading;
	signalmap["game.fps"] = &signal_fps;

	signalmap["car.debug0"] = &signal_debug_info[0];
	signalmap["car.debug1"] = &signal_debug_info[1];
	signalmap["car.debug2"] = &signal_debug_info[2];
	signalmap["car.debug3"] = &signal_debug_info[3];
	signalmap["car.message"] = &signal_message;
	signalmap["car.cur_lap_time"] = &signal_lap_time[0];
	signalmap["car.last_lap_time"] = &signal_lap_time[1];
	signalmap["car.best_lap_time"] = &signal_lap_time[2];
	signalmap["car.lap"] = &signal_lap;
	signalmap["car.pos"] = &signal_pos;
	signalmap["car.score"] = &signal_score;
	signalmap["car.steering"] = &signal_steering;
	signalmap["car.throttle"] = &signal_throttle;
	signalmap["car.brake"] = &signal_brake;
	signalmap["car.gear"] = &signal_gear;
	signalmap["car.shift"] = &signal_shift;
	signalmap["car.speedometer"] = &signal_speedometer;
	signalmap["car.speed.norm"] = &signal_speed_norm;
	signalmap["car.speed"] = &signal_speed;
	signalmap["car.tachometer"] = &signal_tachometer;
	signalmap["car.rpm.norm"] = &signal_rpm_norm;
	signalmap["car.rpm.red"] = &signal_rpm_red;
	signalmap["car.rpm"] = &signal_rpm;
	signalmap["car.abs"] = &signal_abs;
	signalmap["car.tcs"] = &signal_tcs;
	signalmap["car.gas"] = &signal_gas;
	signalmap["car.nos"] = &signal_nos;
}
