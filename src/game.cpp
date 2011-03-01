#include "game.h"
#include "unittest.h"
#include "definitions.h"
#include "joepack.h"
#include "matrix4.h"
#include "config.h"
#include "carwheelposition.h"
#include "numprocessors.h"
#include "parallel_task.h"
#include "performance_testing.h"
#include "widget_label.h"
#include "quickprof.h"
#include "tracksurface.h"
#include "utils.h"
#include "graphics_fallback.h"
#include "graphics_gl3v.h"

#include <fstream>
using std::ifstream;

#include <string>
using std::string;

#include <map>
using std::map;
using std::pair;

#include <list>
using std::list;

#include <iostream>
using std::cout;
using std::endl;

#include <sstream>
using std::stringstream;

#include <algorithm>
using std::sort;

#include <cstdio>

#define _PRINTSIZE_(x) {std::cout << #x << ": " << sizeof(x) << std::endl;}

#define USE_STATIC_OPTIMIZATION_FOR_TRACK

GAME::GAME(std::ostream & info_out, std::ostream & error_out) :
	info_output(info_out),
	error_output(error_out),
	frame(0),
	displayframe(0),
	clocktime(0),
	target_time(0),
	timestep(1/90.0),
	textures(error_out),
	models(error_out),
	sounds(error_out),
	graphics_interface(NULL),
	enableGL3(false),
	fps_track(10, 0),
	fps_position(0),
	fps_min(0),
	fps_max(0),
	multithreaded(false),
	benchmode(false),
	dumpfps(false),
	active_camera(0),
	pause(false),
	particle_timer(0),
	race_laps(0),
	debugmode(false),
	profilingmode(false),
	renderconfigfile("render.conf.deferred"),
	collision(timestep),
	track(info_out, error_out),
	replay(timestep),
	http("/tmp")
{
	carcontrols_local.first = 0;
}

///start the game with the given arguments
void GAME::Start(list <string> & args)
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

	info_output << "Starting VDrift: " << VERSION << ", Version: " << REVISION << ", O/S: ";
#ifdef _WIN32
	info_output << "Windows" << endl;
#elif defined(__APPLE__)
	info_output << "Apple" << endl;
#else
	info_output << "Unix-like" << endl;
#endif

	InitCoreSubsystems();

	//load controls
	info_output << "Loading car controls from: " << pathmanager.GetCarControlsFile() << endl;
	if (!pathmanager.FileExists(pathmanager.GetCarControlsFile()))
	{
		info_output << "Car control file " << pathmanager.GetCarControlsFile() << " doesn't exist; using defaults" << endl;
		carcontrols_local.second.Load(pathmanager.GetDefaultCarControlsFile(), info_output, error_output);
		carcontrols_local.second.Save(pathmanager.GetCarControlsFile(), info_output, error_output);
	}
	else
	{
		carcontrols_local.second.Load(pathmanager.GetCarControlsFile(), info_output, error_output);
	}
	
	InitSound(); //if sound initialization fails, that's okay, it'll disable itself

	//load font data
	if (!LoadFonts())
	{
		error_output << "Error loading fonts" << endl;
		return;
	}
	
	//load loading screen assets
	if (!loadingscreen.Init(
			pathmanager.GetGUITextureDir(settings.GetSkin()),
			window.GetW(),
			window.GetH(),
			settings.GetTextureSize(),
			textures,
			fonts["futuresans"]))
	{
		error_output << "Error loading the loading screen" << endl; //ironic
		return;
	}

	//initialize HUD
	if (!hud.Init(pathmanager.GetGUITextureDir(settings.GetSkin()), settings.GetTextureSize(), textures, fonts["lcd"], fonts["futuresans"], window.GetW(), window.GetH(), debugmode, error_output))
	{
		error_output << "Error initializing HUD" << endl;
		return;
	}
	hud.Hide();

	//initialise input graph
	if (!inputgraph.Init(pathmanager.GetGUITextureDir(settings.GetSkin()), settings.GetTextureSize(), textures, error_output))
	{
		error_output << "Error initializing input graph" << endl;
		return;
	}
	inputgraph.Hide();

	//initialize GUI
	if (!InitGUI()) return;

	//initialize FPS counter
	{
		float screenhwratio = (float)window.GetH()/window.GetW();
		float w = 0.06;
		fps_draw.Init(debugnode, fonts["futuresans"], "", 0.5-w*0.5,1.0-0.02, screenhwratio*0.03,0.03);
		fps_draw.SetDrawOrder(debugnode, 150);
	}
	
	//initialize profiling text
	if (profilingmode)
	{
		float screenhwratio = (float)window.GetH()/window.GetW();
		profiling_text.Init(debugnode, fonts["futuresans"], "", 0.01, 0.25, screenhwratio*0.03, 0.03);
		profiling_text.SetDrawOrder(debugnode, 150);
	}

	//load particle systems
	list <string> smoketexlist;
	string smoketexpath = pathmanager.GetDataPath()+"/"+pathmanager.GetTireSmokeTextureDir();
	pathmanager.GetFileList(smoketexpath, smoketexlist, ".png");
	if (!tire_smoke.Load(
			smoketexlist,
			pathmanager.GetTireSmokeTextureDir(),
			settings.GetTextureSize(),
			settings.GetAnisotropy(),
			textures,
			error_output))
	{
		error_output << "Error loading tire smoke particle system" << endl;
		return;
	}
	tire_smoke.SetParameters(0.125,0.25, 5,14, 0.3,1, 0.5,1, MATHVECTOR<float,3>(0,0,1));

	//initialize force feedback
#ifdef ENABLE_FORCE_FEEDBACK
	forcefeedback.reset(new FORCEFEEDBACK(settings.GetFFDevice(), error_output, info_output));
	ff_update_time = 0;
#endif

	if (benchmode && !NewGame(true))
	{
		error_output << "Error loading benchmark" << endl;
	}
	
	DoneStartingUp();

	//begin
	MainLoop();
	End();
}
#if defined(unix) || defined(__unix) || defined(__unix__)
#include <GL/glx.h>
#include <SDL/SDL_syswm.h>
#endif
///initialize the most important, basic subsystems
void GAME::InitCoreSubsystems()
{
	pathmanager.Init(info_output, error_output);
	http.SetTemporaryFolder(pathmanager.GetTemporaryFolder());
	
	textures.SetBasePath(pathmanager.GetDataPath());
	textures.SetSharedPath(pathmanager.GetSharedDataPath());
	
	models.SetBasePath(pathmanager.GetDataPath());
	models.SetSharedPath(pathmanager.GetSharedDataPath());
	
	sounds.SetBasePath(pathmanager.GetDataPath());
	sounds.SetSharedPath(pathmanager.GetSharedDataPath());
	
	settings.Load(pathmanager.GetSettingsFile(), error_output);
	
	if (!LastStartWasSuccessful())
	{
		info_output << "The last VDrift startup was unsuccessful.\nSettings have been set to failsafe defaults.\nYour original VDrift.config file was backed up to VDrift.config.backup" << endl;
		settings.Save(pathmanager.GetSettingsFile()+".backup", error_output);
		settings.SetFailsafeSettings();
	}
	BeginStartingUp();
	
	window.Init("VDrift - open source racing simulation",
		settings.GetResolutionX(), settings.GetResolutionY(),
		settings.GetBpp(),
		settings.GetShadows() ? std::max(settings.GetDepthbpp(),(unsigned int)24) : settings.GetDepthbpp(),
		settings.GetFullscreen(),
		settings.GetAntialiasing(),
		info_output, error_output);
	
	const int rendererCount = 2;
	for (int i = 0; i < rendererCount; i++)
	{
		// attempt to enable the GL3 renderer
		if (enableGL3 && i == 0 && settings.GetShaders())
		{
			graphics_interface = new GRAPHICS_GL3V(stringMap);
			models.setGenerateDrawList(false);
		}
		else
		{
			graphics_interface = new GRAPHICS_FALLBACK();
			models.setGenerateDrawList(true);
		}
		
		bool success = graphics_interface->Init(pathmanager.GetShaderPath(),
			settings.GetResolutionX(), settings.GetResolutionY(),
			settings.GetBpp(), settings.GetDepthbpp(), settings.GetFullscreen(),
			settings.GetShaders(), settings.GetAntialiasing(), settings.GetShadows(),
			settings.GetShadowDistance(), settings.GetShadowQuality(),
			settings.GetReflections(), pathmanager.GetStaticReflectionMap(),
			pathmanager.GetStaticAmbientMap(),
			settings.GetAnisotropic(), settings.GetTextureSize(),
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
	ldir.Rotate(3.141593*0.05,0,1,0);
	ldir.Rotate(-3.141593*0.1,1,0,0);
	graphics_interface->SetSunDirection(ldir);

	eventsystem.Init(info_output);
}

///write the scenegraph to the output drawlist
template <bool clearfirst>
void TraverseScene(SCENENODE & node, GRAPHICS_INTERFACE::dynamicdrawlist_type & output)
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

bool GAME::InitGUI()
{
	list <string> menufiles;
	string menufolder = pathmanager.GetGUIMenuPath(settings.GetSkin());
	if (!pathmanager.GetFileList(menufolder, menufiles))
	{
		error_output << "Error retreiving contents of folder: " << menufolder << endl;
		return false;
	}
	else
	{
		//remove any pages that have ~ characters
		list <list <string>::iterator> todel;
		for (list <string>::iterator i = menufiles.begin(); i != menufiles.end(); ++i)
		{
			if (i->find("~") != string::npos)
			{
				todel.push_back(i);
			}
		}
		for (list <list <string>::iterator>::iterator i = todel.begin(); i != todel.end(); ++i)
		{
			menufiles.erase(*i);
		}
	}
	
	std::map<std::string, std::list <std::pair <std::string, std::string> > > valuelists;
	PopulateValueLists(valuelists);
	
	if (!gui.Load(
			menufiles,
			valuelists,
			pathmanager.GetOptionsFile(),
			pathmanager.GetCarControlsFile(),
			menufolder,
			pathmanager.GetGUILanguageDir(settings.GetSkin()),
			settings.GetLanguage(),
			pathmanager.GetGUITextureDir(settings.GetSkin()),
			pathmanager.GetDataPath(),
			settings.GetTextureSize(),
			(float)window.GetH()/window.GetW(),
			fonts,
			textures,
			models,
			info_output,
			error_output))
	{
		error_output << "Error loading GUI files" << endl;
		return false;
	}
	
	std::map<std::string, std::string> optionmap;
	GetOptions(optionmap);
	gui.SyncOptions(true, optionmap, error_output);
	gui.ActivatePage("Main", 0.5, error_output); //nice, slow fade-in
	if (settings.GetMouseGrab()) eventsystem.SetMouseCursorVisibility(true);

	return true;
}

bool GAME::InitSound()
{
	if (sound.Init(2048, info_output, error_output))
	{
		sound.SetMasterVolume(settings.GetMasterVolume());
		sound.Pause(false);
	}
	else
	{
		error_output << "Sound initialization failed" << endl;
		return false;
	}

	info_output << "Sound initialization successful" << endl;
	return true;
}

bool GAME::ParseArguments(std::list <std::string> & args)
{
	bool continue_game(true);
	
	map <string, string> arghelp;
	map <string, string> argmap;

	//generate an argument map
	for (list <string>::iterator i = args.begin(); i != args.end(); ++i)
	{
		if ((*i)[0] == '-')
		{
			argmap[*i] = "";
		}

		list <string>::iterator n = i;
		n++;
		if (n != args.end())
		{
			if ((*n)[0] != '-')
				argmap[*i] = *n;
		}
	}

	//check for arguments

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
	
	if (argmap.find("-gl3") != argmap.end())
	{
		enableGL3 = true;
	}
	arghelp["-gl3"] = "Attempt to enable OpenGL3 rendering.";

	if (!argmap["-cartest"].empty())
	{
		pathmanager.Init(info_output, error_output);
		PERFORMANCE_TESTING perftest;
		perftest.Test(pathmanager.GetCarPath(), argmap["-cartest"], info_output, error_output);
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
		info_output << "Dumping the frame-rate to log." << endl;
		dumpfps = true;
	}
	arghelp["-dumpfps"] = "Continually dump the framerate to the log.";
	
	
	if (!argmap["-resolution"].empty())
	{
		string res(argmap["-resolution"]);
		std::vector <std::string> restoken = Tokenize(res, "x,");
		if (restoken.size() != 2)
		{
			error_output << "Expected resolution to be in the form 640x480" << endl;
			continue_game = false;
		}
		else
		{
			stringstream sx(restoken[0]);
			stringstream sy(restoken[1]);
			int xres, yres;
			sx >> xres;
			sy >> yres;
			settings.SetResolutionX(xres);
			settings.SetResolutionY(yres);
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
			info_output << "Multithreading enabled: " << processors << " processors" << endl;
			//info_output << "Note that multithreading is currently NOT RECOMMENDED for use and is likely to decrease performance significantly." << endl;
		}
		else
		{
			info_output << "Multithreading forced on, but only 1 processor!" << endl;
		}
	}
	else if (continue_game)
	{
		if (processors > 1)
			info_output << "Multi-processor system detected.  Run with -multithreaded argument to enable multithreading (EXPERIMENTAL)." << endl;
	}
	arghelp["-multithreaded"] = "Use multithreading where possible.";
	#endif

	if (argmap.find("-nosound") != argmap.end())
		sound.Disable();
	arghelp["-nosound"] = "Disable all sound.";

	if (argmap.find("-benchmark") != argmap.end())
	{
		info_output << "Entering benchmark mode." << endl;
		benchmode = true;
	}
	arghelp["-benchmark"] = "Run in benchmark mode.";
	
	arghelp["-render FILE"] = "Load the specified render configuration file instead of the default " + renderconfigfile + ".";
	if (!argmap["-render"].empty())
	{
		renderconfigfile = argmap["-render"];
	}
	
	
	arghelp["-help"] = "Display command-line help.";
	if (argmap.find("-help") != argmap.end() || argmap.find("-h") != argmap.end() || argmap.find("--help") != argmap.end() || argmap.find("-?") != argmap.end())
	{
		string helpstr;
		unsigned int longest = 0;
		for (std::map <string,string>::iterator i = arghelp.begin(); i != arghelp.end(); ++i)
			if (i->first.size() > longest)
				longest = i->first.size();
		for (std::map <string,string>::iterator i = arghelp.begin(); i != arghelp.end(); ++i)
		{
			helpstr.append(i->first);
			for (unsigned int n = 0; n < longest+3-i->first.size(); n++)
				helpstr.push_back(' ');
			helpstr.append(i->second + "\n");
		}
		info_output << "Command-line help:\n\n" << helpstr << endl;
		continue_game = false;
	}

	return continue_game;
}

///do any necessary cleanup
void GAME::End()
{
	if (benchmode)
	{
		float mean_fps = displayframe / clocktime;
		info_output << "Elapsed time: " << clocktime << " seconds\n";
		info_output << "Average frame-rate: " << mean_fps << " frames per second\n";
		info_output << "Min / Max frame-rate: " << fps_min << " / " << fps_max << " frames per second" << endl;
	}
	
	if (profilingmode)
		info_output << "Profiling summary:\n" << PROFILER.getSummary(quickprof::PERCENT) << endl;
	
	info_output << "Shutting down..." << endl;

	LeaveGame();

	if (sound.Enabled())
		sound.Pause(true); //stop the sound thread

	settings.Save(pathmanager.GetSettingsFile(), error_output); //save settings first incase later deinits cause crashes
	
	graphics_interface->Deinit();
	delete graphics_interface;
	window.Deinit();
}

void GAME::Test()
{
	QT_RUN_TESTS;

	info_output << endl;
}

void GAME::BeginDraw()
{
	PROFILER.beginBlock("render");
	//send scene information to the graphics subsystem
	if (active_camera)
	{
		MATHVECTOR <float, 3> reflection_sample_location = active_camera->GetPosition();
		if (carcontrols_local.first)
			reflection_sample_location = carcontrols_local.first->GetCenterOfMassPosition();
		
		QUATERNION <float> camlook;
		camlook.Rotate(3.141593*0.5,1,0,0);
		camlook.Rotate(-3.141593*0.5,0,0,1);
		QUATERNION <float> camorient = -(active_camera->GetOrientation()*camlook);
		graphics_interface->SetupScene(settings.GetFOV(), settings.GetViewDistance(), active_camera->GetPosition(), camorient, reflection_sample_location);
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
	#ifndef USE_STATIC_OPTIMIZATION_FOR_TRACK
	TraverseScene<false>(track.GetTrackNode(), graphics_interface->GetDynamicDrawlist());
	#endif
	TraverseScene<false>(hud.GetNode(), graphics_interface->GetDynamicDrawlist());
	TraverseScene<false>(trackmap.GetNode(), graphics_interface->GetDynamicDrawlist());
	TraverseScene<false>(inputgraph.GetNode(), graphics_interface->GetDynamicDrawlist());
	TraverseScene<false>(tire_smoke.GetNode(), graphics_interface->GetDynamicDrawlist());
	for (list <CAR>::iterator i = cars.begin(); i != cars.end(); ++i)
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
	window.SwapBuffers(error_output);
	PROFILER.endBlock("render");
}

///the main game loop
void GAME::MainLoop()
{
	while (!eventsystem.GetQuit() && (!benchmode || replay.GetPlaying()))
	{
		CalculateFPS();
		
		clocktime += eventsystem.Get_dt();

		eventsystem.BeginFrame();

		Tick(eventsystem.Get_dt()); //do CPU intensive stuff in parallel with the GPU

		gui.Update(eventsystem.Get_dt());
		
		FinishDraw(); //sync CPU and GPU (flip the page)
		BeginDraw();
		
		eventsystem.EndFrame();
		
		PROFILER.endCycle();
		
		displayframe++;
	}
}

///deltat is in seconds
void GAME::Tick(float deltat)
{
	const float minfps = 10.0f; //this is the minimum fps the game will run at before it starts slowing down time
	const unsigned int maxticks = (int) (1.0f / (minfps * timestep)); //slow the game down if we can't process fast enough
	const float maxtime = 1.0 / minfps; //slow the game down if we can't process fast enough
	unsigned int curticks = 0;

	//throw away wall clock time if necessary to keep the framerate above the minimum
	if (deltat > maxtime) deltat = maxtime;

	target_time += deltat;
	
	http.Tick();

	//increment game logic by however many tick periods have passed since the last GAME::Tick
	while (target_time - TickPeriod()*frame > TickPeriod() && curticks < maxticks)
	{
		frame++;

		AdvanceGameLogic();

		curticks++;
	}

	if (dumpfps && curticks > 0 && frame % 100 == 0)
	{
		info_output << "Current FPS: " << eventsystem.GetFPS() << endl;
	}
}

///increment game logic by one frame
void GAME::AdvanceGameLogic()
{
	//PROFILER.beginBlock("input-processing");
	
	eventsystem.ProcessEvents();

	ProcessGUIInputs();

	ProcessGameInputs();
	
	//PROFILER.endBlock("input-processing");

	if (track.Loaded())
	{
		if (pause && carcontrols_local.first)
		{
			sound.Pause(true);
			//cout << "Paused" << endl;

			//this next line is required so that the game will see the unpause key
			carcontrols_local.second.ProcessInput(
				settings.GetJoyType(),
				eventsystem,
				carcontrols_local.first->GetLastSteer(),
				TickPeriod(),
				settings.GetJoy200(),
				carcontrols_local.first->GetSpeed(),
				settings.GetSpeedSensitivity(),
				window.GetW(),
				window.GetH(),
				settings.GetButtonRamp(),
				settings.GetHGateShifter());
		}
		else
		{
			//cout << "Not paused" << endl;
			if (gui.Active()) //keep the game paused when the gui is up
			{
				if (sound.Enabled())
					sound.Pause(true); //stop sounds when the gui is up
			}
			else
			{
				if (sound.Enabled())
					sound.Pause(false);
				
				PROFILER.beginBlock("ai");
				ai.Visualize();
				ai.update(TickPeriod(), &track, cars);
				PROFILER.endBlock("ai");
				
				PROFILER.beginBlock("physics");
				collision.Update(TickPeriod());
				PROFILER.endBlock("physics");
				
				PROFILER.beginBlock("car-update");
				for (list <CAR>::iterator i = cars.begin(); i != cars.end(); ++i)
				{
					UpdateCar(*i, TickPeriod());
				}
				PROFILER.endBlock("car-update");
				
				//PROFILER.beginBlock("timer");
				UpdateTimer();
				//PROFILER.endBlock("timer");

				//PROFILER.beginBlock("particles");
				UpdateParticleSystems(TickPeriod());
				//PROFILER.endBlock("particles");
			}
		}
	}

	//PROFILER.beginBlock("trackmap-update");
	UpdateTrackMap();
	//PROFILER.endBlock("trackmap-update");

	if (sound.Enabled())
	{
		if (active_camera)
			sound.SetListener(active_camera->GetPosition(), -active_camera->GetOrientation(), MATHVECTOR <float, 3>());
		else
			sound.SetListener(MATHVECTOR <float, 3> (), QUATERNION <float> (), MATHVECTOR <float, 3>());
	}

	//PROFILER.beginBlock("force-feedback");
	UpdateForceFeedback(TickPeriod());
	//PROFILER.endBlock("force-feedback");
}

///process inputs used only for higher level game functions
void GAME::ProcessGameInputs()
{
	if (carcontrols_local.first)
	{
		if (carcontrols_local.second.GetInput(CARINPUT::SCREENSHOT) == 1.0)
		{
			//determine filename
			string shotfile;
			for (int i = 1; i < 999; i++)
			{
				stringstream s;
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
				info_output << "Capturing screenshot to " << shotfile << endl;
				window.Screenshot(shotfile);
			}
			else
				error_output << "Couldn't find a file to which to save the captured screenshot" << endl;
		}
		
		if (carcontrols_local.second.GetInput(CARINPUT::PAUSE) == 1.0)
		{
			//cout << "Pause input; changing " << pause << " to " << !pause << endl;
			pause = !pause;
		}
		
		if (carcontrols_local.second.GetInput(CARINPUT::RELOAD_SHADERS) == 1.0)
		{
			info_output << "Reloading shaders" << endl;
			if (!graphics_interface->ReloadShaders(pathmanager.GetShaderPath(), info_output, error_output))
			{
				error_output << "Error reloading shaders" << endl;
			}
		}
	}
}

void GAME::UpdateTimer()
{
	//check for cars doing a lap
	for (list <CAR>::iterator i = cars.begin(); i != cars.end(); ++i)
	{
		int carid = cartimerids[&(*i)];

		bool advance = false;
		int nextsector = 0;
		if (track.GetSectors() > 0)
		{
			nextsector = (i->GetSector() + 1) % track.GetSectors();
			//cout << "next " << nextsector << ", cur " << i->GetSector() << ", track " << track.GetSectors() << endl;
			for (int p = 0; p < 4; p++)
			{
				if (i->GetCurPatch(p) == track.GetLapSequence(nextsector))
				{
					advance = true;
					//cout << "Drove over new sector " << nextsector << " patch " << i->GetCurPatch(p) << endl;
					//cout << p << ". " << i->GetCurPatch(p) << ", " << track.GetLapSequence(nextsector) << endl;
				}
				//else cout << p << ". " << i->GetCurPatch(p) << ", " << track.GetLapSequence(nextsector) << endl;
			}
		}

		if (advance)
		{
			// only count it if the car's current sector isn't -1
			// which is the default value when the car is loaded
			timer.Lap(carid, i->GetSector(), nextsector, (i->GetSector() >= 0)); 
			i->SetSector(nextsector);
		}

		//update how far the car is on the track
		const BEZIER * curpatch = i->GetCurPatch(0); //find the patch under the front left wheel
		if (!curpatch)
			curpatch = i->GetCurPatch(1); //try the other wheel
		if (curpatch) //only update if car is on track
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

			//float dist_from_back = (back_left - back_right).perp_distance (back_left, pos);

			MATHVECTOR <float, 3> forwardvec = front_left - back_left;
			MATHVECTOR <float, 3> relative_pos = pos - back_left;
			float dist_from_back = 0;
			
			if (forwardvec.Magnitude() > 0.0001)
				dist_from_back = relative_pos.dot(forwardvec.Normalize());

			timer.UpdateDistance(carid, curpatch->GetDistFromStart() + dist_from_back);
			//std::cout << curpatch->GetDistFromStart() << ", " << dist_from_back << std::endl;
			//std::cout << curpatch->GetDistFromStart() + dist_from_back << std::endl;
		}

		/*info_output << "sector=" << i->GetSector() << ", next=" << track.GetLapSequence(nextsector) << ", ";
		for (int w = 0; w < 4; w++)
		{
			info_output << w << "=" << i->GetCurPatch(w) << ", ";
		}
		info_output << std::endl;*/
	}

	timer.Tick(TickPeriod());
	//timer.DebugPrint(info_output);
}

void GAME::UpdateTrackMap()
{
	list <pair<MATHVECTOR <float, 3>, bool> > carpositions;
	for (list <CAR>::iterator i = cars.begin(); i != cars.end(); ++i)
	{
		bool playercar = (carcontrols_local.first == &(*i));
		carpositions.push_back(pair<MATHVECTOR <float, 3>, bool> (i->GetCenterOfMassPosition(), playercar));
	}

	trackmap.Update(settings.GetTrackmap(), carpositions);
}

///check eventsystem state and make updates to the GUI
void GAME::ProcessGUIInputs()
{
	//handle the ESCAPE key with dedicated logic
	if (eventsystem.GetKeyState(SDLK_ESCAPE).just_down)
	{
		if (gui.Active() && gui.GetActivePageName() == "AssignControl")
		{
			if (controlgrab_page.empty())
				gui.ActivatePage("Main", 0.25, error_output); //uh, dunno what to do so go to the main menu
			else
				gui.ActivatePage(controlgrab_page, 0.25, error_output);
			if (settings.GetMouseGrab()) eventsystem.SetMouseCursorVisibility(true);
			gui.SetControlsNeedLoading(false);
		}
		else
		{
			if (track.Loaded())
			{
				if (gui.Active())
				{
					gui.DeactivateAll();
					if (settings.GetMouseGrab()) eventsystem.SetMouseCursorVisibility(false);
				}
				else
				{
					gui.ActivatePage("InGameMenu", 0.25, error_output);
					if (settings.GetMouseGrab()) eventsystem.SetMouseCursorVisibility(true);
				}
			}
		}
	}
	
	/*// handle F12 with dedicated logic because we want to be able to do this outside of the game
	if (eventsystem.GetKeyState(SDLK_F12).just_down)
	{
		info_output << "Reloading shaders" << endl;
		if (!graphics_interface->ReloadShaders(pathmanager.GetShaderPath(), info_output, error_output))
		{
			error_output << "Error reloading shaders" << endl;
		}
	}*/

	list <string> gui_actions;

	//handle inputs when we're waiting to assign a control
	if (gui.Active() && gui.GetActivePageName() == "AssignControl")
	{
		bool have_a_control_input = false;

		//check for key inputs
		std::map <SDLKey, TOGGLE> & keymap = eventsystem.GetKeyMap();
		for (std::map <SDLKey, TOGGLE>::iterator i = keymap.begin(); i != keymap.end() && !have_a_control_input; ++i)
		{
			if (i->second.GetImpulseRising())
			{
				have_a_control_input = true;

				carcontrols_local.second.AddInputKey(controlgrab_input, controlgrab_analog,
					controlgrab_only_one, i->first, error_output);

				//info_output << "Adding new key input for " << controlgrab_input << endl;
			}
		}

		//check for joystick inputs
		for (int j = 0; j < eventsystem.GetNumJoysticks() && !have_a_control_input; j++)
		{
			//check for joystick buttons
			for (int i = 0; i < eventsystem.GetNumButtons(j) && !have_a_control_input; i++)
			{
				if (eventsystem.GetJoyButton(j, i).GetImpulseRising())
				{
					have_a_control_input = true;

					carcontrols_local.second.AddInputJoyButton(controlgrab_input, controlgrab_analog,
							controlgrab_only_one, j, i, error_output);
				}
			}

			//check for joystick axes
			//if (settings.GetJoystickCalibrated()) //only allow joystick inputs if the joystick is calibrated //commented out because the axis detection code is smarter and only looks for deltas
			{
				for (int i = 0; i < eventsystem.GetNumAxes(j) && !have_a_control_input; i++)
				{
					//std::cout << "joy " << j << " axis " << i << ": " << eventsystem.GetJoyAxis(j, i) << endl;

					assert(j < (int)controlgrab_joystick_state.size());
					assert(i < controlgrab_joystick_state[j].GetNumAxes());

					if (eventsystem.GetJoyAxis(j, i) - controlgrab_joystick_state[j].GetAxis(i) > 0.4 && !have_a_control_input)
					{
						have_a_control_input = true;

						carcontrols_local.second.AddInputJoyAxis(controlgrab_input, controlgrab_analog,
								controlgrab_only_one, j, i, "positive", error_output);
					}

					if (eventsystem.GetJoyAxis(j, i) - controlgrab_joystick_state[j].GetAxis(i) < -0.4 && !have_a_control_input)
					{
						have_a_control_input = true;

						carcontrols_local.second.AddInputJoyAxis(controlgrab_input, controlgrab_analog,
								controlgrab_only_one, j, i, "negative", error_output);
					}
				}
			}
		}

		//check for mouse button inputs
		for (int i = 1; i <= 3 && !have_a_control_input; i++)
		{
			//std::cout << "mouse button " << i << ": " << eventsystem.GetMouseButtonState(i).down << std::endl;

			if (eventsystem.GetMouseButtonState(i).just_down)
			{
				have_a_control_input = true;

				carcontrols_local.second.AddInputMouseButton(controlgrab_input, controlgrab_analog,
					controlgrab_only_one, i, error_output);
			}
		}

		//check for mouse motion inputs
		if (!have_a_control_input)
		{
			MATHVECTOR <float, 2> orig;
			orig.Set(controlgrab_mouse_coords.first, controlgrab_mouse_coords.second);
			MATHVECTOR <float, 2> cur;
			cur.Set(eventsystem.GetMousePosition()[0],eventsystem.GetMousePosition()[1]);
			MATHVECTOR <float, 2> diff;
			diff = cur - orig;

			int threshold = 200;

			std::string motion;

			if (diff[0] < -threshold)
				motion = "left";
			else if (diff[0] > threshold)
				motion = "right";
			else if (diff[1] < -threshold)
				motion = "up";
			else if (diff[1] > threshold)
				motion = "down";

			if (!motion.empty())
			{
				have_a_control_input = true;

				carcontrols_local.second.AddInputMouseMotion(controlgrab_input, controlgrab_analog,
						controlgrab_only_one, motion, error_output);
			}
		}

		if (have_a_control_input)
			RedisplayControlPage();
	}
	else
	{
		if (gui.Active())
		{
			//send input to the gui and get output into the gui_actions list
			gui_actions = gui.ProcessInput(eventsystem.GetKeyState(SDLK_UP).just_down, eventsystem.GetKeyState(SDLK_DOWN).just_down,
					eventsystem.GetMousePosition()[0]/(float)window.GetW(), eventsystem.GetMousePosition()[1]/(float)window.GetH(),
							eventsystem.GetMouseButtonState(1).down, eventsystem.GetMouseButtonState(1).just_up, (float)window.GetH()/window.GetW(), error_output);
		}
	}

	if (gui.Active())
	{
		//if the user did something that requires loading or saving options, do a sync
		bool neededsync = false;
		if (gui.OptionsNeedSync())
		{
			std::map<std::string, std::string> optionmap;
			GetOptions(optionmap);
			gui.SyncOptions(false, optionmap, error_output);
			SetOptions(optionmap);
			neededsync = true;
		}

		if (gui.ControlsNeedLoading())
		{
			carcontrols_local.second.Load(pathmanager.GetCarControlsFile(), info_output, error_output);
			//std::cout << "Control files are being loaded: " << gui.GetActivePageName() << ", " << gui.GetLastPageName() << std::endl;
			if (!gui.GetLastPageName().empty())
			{
				LoadControlsIntoGUIPage(gui.GetLastPageName());
			}
		}

		//process gui actions
		for (list <string>::iterator i = gui_actions.begin(); i != gui_actions.end(); ++i)
		{
			ProcessGUIAction(*i);
		}

		if (neededsync &&
			gui.GetActivePageName() != "AssignControl" &&
			gui.GetActivePageName() != "EditButtonControl" &&
			gui.GetLastPageName() != "EditButtonControl" &&
			gui.GetActivePageName() != "EditAnalogControl" &&
			gui.GetLastPageName() != "EditAnalogControl")
		{
			//write out controls
			carcontrols_local.second.Save(pathmanager.GetCarControlsFile(), info_output, error_output);
			//std::cout << "Control files are being saved: " << gui.GetActivePageName() << ", " << gui.GetLastPageName() << std::endl;
		}
	}
}

void GAME::RedisplayControlPage()
{
	if (controlgrab_page.empty())
	{
		gui.ActivatePage("Main", 0.25, error_output); //uh, dunno what to do so go to the main menu
	}
	else
	{
		gui.ActivatePage(controlgrab_page, 0.25, error_output);
		LoadControlsIntoGUIPage(controlgrab_page);
	}
	gui.SetControlsNeedLoading(false);
	if (settings.GetMouseGrab()) eventsystem.SetMouseCursorVisibility(true);
}

void GAME::LoadControlsIntoGUIPage(const std::string & pagename)
{
	CONFIG controlfile;
	std::map<std::string, std::list <std::pair <std::string, std::string> > > valuelists;
	PopulateValueLists(valuelists);
	carcontrols_local.second.Save(controlfile, info_output, error_output);
	gui.UpdateControls(pagename, controlfile);
}

///process the action string from the GUI
void GAME::ProcessGUIAction(const std::string & action)
{
	if (action == "Quit")
	{
		info_output << "Got quit message from GUI.  Shutting down..." << endl;
		eventsystem.Quit();
	}
	else if (action == "StartPracticeGame")
	{
		if (!NewGame())
		{
			LeaveGame();
		}
	}
	else if (action.substr(0,14) == "controlgrabadd")
	{
		controlgrab_page = gui.GetActivePageName();
		string setting = action.substr(19);
		controlgrab_input = setting;
		controlgrab_analog = (action.substr(15,1) == "y");
		controlgrab_only_one = (action.substr(17,1) == "y");
		
		//info_output << "Controlgrab action: " << action << ", " << action.substr(12,1) << ", " << action.substr(14,1) << endl;
		controlgrab_mouse_coords = std::pair <int,int> (eventsystem.GetMousePosition()[0],eventsystem.GetMousePosition()[1]);
		controlgrab_joystick_state = eventsystem.GetJoysticks();
		gui.ActivatePage("AssignControl", 0.25, error_output); //nice, slow fade-in
		gui.SetControlsNeedLoading(false);
	}
	else if (action.substr(0,15) == "controlgrabedit")
	{
		//determine edit parameters
		controlgrab_page = gui.GetActivePageName();
		string controlstr = action.substr(16);
		//info_output << "Controledit action: " << controlstr << endl;
		stringstream controlstream(controlstr);
		controlgrab_editcontrol.ReadFrom(controlstream);
		assert(action.find('\n') != string::npos);
		controlgrab_input = action.substr(action.find('\n')+1);
		assert(!controlgrab_input.empty());
		//info_output << "Controledit input: " << controlgrab_input << endl;

		std::map<std::string, GUIOPTION> tempoptionmap;

		//determine which edit page to show
		bool analog = false;
		if (controlgrab_editcontrol.type == CARCONTROLMAP_LOCAL::CONTROL::JOY &&
			controlgrab_editcontrol.joytype == CARCONTROLMAP_LOCAL::CONTROL::JOYAXIS)
			analog = true;
		if (controlgrab_editcontrol.type == CARCONTROLMAP_LOCAL::CONTROL::MOUSE &&
			controlgrab_editcontrol.mousetype == CARCONTROLMAP_LOCAL::CONTROL::MOUSEMOTION)
			analog = true;

		//display the page and load up the gui state
		if (!analog)
		{
			gui.ActivatePage("EditButtonControl", 0.25, error_output);
			tempoptionmap["controledit.button.held_once"].SetCurrentValue(controlgrab_editcontrol.onetime?"true":"false");

			if (controlgrab_editcontrol.type == CARCONTROLMAP_LOCAL::CONTROL::KEY)
				tempoptionmap["controledit.button.up_down"].SetCurrentValue(controlgrab_editcontrol.keypushdown?"true":"false");
			else if (controlgrab_editcontrol.type == CARCONTROLMAP_LOCAL::CONTROL::JOY)
				tempoptionmap["controledit.button.up_down"].SetCurrentValue(controlgrab_editcontrol.joypushdown?"true":"false");
			else if (controlgrab_editcontrol.type == CARCONTROLMAP_LOCAL::CONTROL::MOUSE)
				tempoptionmap["controledit.button.up_down"].SetCurrentValue(controlgrab_editcontrol.mouse_push_down?"true":"false");
			else
				assert(0);
		}
		else
		{
			gui.ActivatePage("EditAnalogControl", 0.25, error_output);

			string deadzone, exponent, gain;
			{
				stringstream tempstr;
				tempstr << controlgrab_editcontrol.deadzone;
				//cout << "Deadzone: " << controlgrab_editcontrol.deadzone << std::endl;
				deadzone = tempstr.str();
			}
			{
				stringstream tempstr;
				tempstr << controlgrab_editcontrol.exponent;
				exponent = tempstr.str();
			}
			{
				stringstream tempstr;
				tempstr << controlgrab_editcontrol.gain;
				gain = tempstr.str();
			}
			tempoptionmap["controledit.analog.deadzone"].SetCurrentValue(deadzone);
			tempoptionmap["controledit.analog.exponent"].SetCurrentValue(exponent);
			tempoptionmap["controledit.analog.gain"].SetCurrentValue(gain);
		}
		//std::cout << "Updating options..." << endl;
		gui.GetPage(gui.GetActivePageName()).UpdateOptions(gui.GetNode(), false, tempoptionmap, error_output);
		//std::cout << "Done updating options." << endl;
		gui.SetControlsNeedLoading(false);
	}
	else if (action == "ButtonControlOK" || action == "AnalogControlOK")
	{
		std::map<std::string, GUIOPTION> tempoptionmap;

		if (action == "ButtonControlOK")
		{
			//get current GUI state
			tempoptionmap["controledit.button.held_once"].SetCurrentValue(controlgrab_editcontrol.onetime?"true":"false");
			tempoptionmap["controledit.button.up_down"].SetCurrentValue(controlgrab_editcontrol.keypushdown?"true":"false");
			gui.GetPage(gui.GetActivePageName()).UpdateOptions(gui.GetNode(), true, tempoptionmap, error_output);

			//save GUI state to our control
			if (tempoptionmap["controledit.button.held_once"].GetCurrentDisplayValue() == "true")
				controlgrab_editcontrol.onetime = true;
			else
				controlgrab_editcontrol.onetime = false;
			
			bool down = false;
			if (tempoptionmap["controledit.button.up_down"].GetCurrentDisplayValue() == "true")
				down = true;
			
			controlgrab_editcontrol.joypushdown = down;
			controlgrab_editcontrol.keypushdown = down;
			controlgrab_editcontrol.mouse_push_down = down;
		}
		else //analog control
		{
			//get current GUI state
			tempoptionmap["controledit.analog.deadzone"].SetCurrentValue("0");
			tempoptionmap["controledit.analog.exponent"].SetCurrentValue("1");
			tempoptionmap["controledit.analog.gain"].SetCurrentValue("1");
			gui.GetPage(gui.GetActivePageName()).UpdateOptions(gui.GetNode(), true, tempoptionmap, error_output);

			//save GUI state to our control
			{
				stringstream tempstr(tempoptionmap["controledit.analog.deadzone"].GetCurrentDisplayValue());
				tempstr >> controlgrab_editcontrol.deadzone;
				//std::cout << controlgrab_editcontrol.deadzone << endl;
			}
			{
				stringstream tempstr(tempoptionmap["controledit.analog.exponent"].GetCurrentDisplayValue());
				tempstr >> controlgrab_editcontrol.exponent;
			}
			{
				stringstream tempstr(tempoptionmap["controledit.analog.gain"].GetCurrentDisplayValue());
				tempstr >> controlgrab_editcontrol.gain;
			}
		}

		//send our control update to the control maintainer
		carcontrols_local.second.UpdateControl(controlgrab_editcontrol, controlgrab_input, error_output);

		//go back to the previous page
		if (controlgrab_page.empty())
		{
			gui.ActivatePage("Main", 0.25, error_output); //uh, dunno what to do so go to the main menu
		}
		else
		{
			gui.ActivatePage(controlgrab_page, 0.25, error_output);
			LoadControlsIntoGUIPage(controlgrab_page);
		}
		gui.SetControlsNeedLoading(false);
	}
	else if (action == "ButtonControlCancel" || action == "AnalogControlCancel")
	{
		if (controlgrab_page.empty())
		{
			gui.ActivatePage("Main", 0.25, error_output); //uh, dunno what to do so go to the main menu
		}
		else
		{
			gui.ActivatePage(controlgrab_page, 0.25, error_output);
			LoadControlsIntoGUIPage(controlgrab_page);
		}
		gui.SetControlsNeedLoading(false);
	}
	else if (action == "ButtonControlDelete" || action == "AnalogControlDelete")
	{
		carcontrols_local.second.DeleteControl(controlgrab_editcontrol, controlgrab_input, error_output);
		if (controlgrab_page.empty())
		{
			gui.ActivatePage("Main", 0.25, error_output); //uh, dunno what to do so go to the main menu
		}
		else
		{
			gui.ActivatePage(controlgrab_page, 0.25, error_output);
			LoadControlsIntoGUIPage(controlgrab_page);
		}
		gui.SetControlsNeedLoading(false);
	}
	else if (action == "ReturnToGame")
	{
		if (gui.Active())
		{
			gui.DeactivateAll();
			if (settings.GetMouseGrab()) eventsystem.SetMouseCursorVisibility(false);
		}
	}
	else if (action == "LeaveGame")
	{
		LeaveGame();
		gui.ActivatePage("Main", 0.25, error_output);
	}
	else if (action == "StartReplay")
	{
		if (settings.GetSelectedReplay() != 0 && !NewGame(true))
		{
			gui.ActivatePage("ReplayStartError", 0.25, error_output);
		}
		//cout << settings.GetSelectedReplay() << endl;
	}
	else if (action == "PlayerCarChange") //this means the player clicked the GUI to change their car
	{
		std::list <std::pair <std::string, std::string> > carpaintlist;
		PopulateCarPaintList(settings.GetSelectedCar(), carpaintlist);
		gui.ReplaceOptionValues("game.player_paint", carpaintlist, error_output);
	}
	else if (action == "OpponentCarChange") //this means the player clicked the GUI to change the opponent car
	{
		std::list <std::pair <std::string, std::string> > carpaintlist;
		PopulateCarPaintList(settings.GetOpponentCar(), carpaintlist);
		gui.ReplaceOptionValues("game.opponent_paint", carpaintlist, error_output);
	}
	else if (action == "AddOpponent")
	{
		if (opponents.size() == 3)
		{
			opponents.clear();
			opponents_paint.clear();
			opponents_color.clear();
		}
		
		opponents.push_back(settings.GetOpponentCar());
		opponents_paint.push_back(settings.GetOpponentCarPaint());
		MATHVECTOR <float, 3> color(0);
		settings.GetOpponentColor(color[0], color[1], color[2]);
		opponents_color.push_back(color);

		std::string opponentstr;
		for (std::vector<std::string>::iterator i = opponents.begin(); i != opponents.end(); ++i)
		{
			if (i != opponents.begin())
			{
				opponentstr += ", ";
			}
			opponentstr += *i;
		}
		SCENENODE & pagenode = gui.GetPageNode("SingleRace");
		gui.GetPage("SingleRace").GetLabel("OpponentsList").get().SetText(pagenode, opponentstr);
	}
	else if (action == "RestartGame")
	{
		bool play_replay = false;
		bool add_opponents = !opponents.empty();
		int num_laps = race_laps;
		if (!NewGame(play_replay, add_opponents, num_laps))
		{
			LeaveGame();
		}
	}
	else if (action == "StartRace")
	{
		//handle a single race
		if (opponents.empty())
		{
			gui.ActivatePage("NoOpponentsError", 0.25, error_output);
		}
		else
		{
			bool play_replay = false;
			bool add_opponents = true;
			int num_laps = settings.GetNumberOfLaps();
			if (!NewGame(play_replay, add_opponents, num_laps))
			{
				LeaveGame();
			}
		}
	}
	else if (action == "HandleOnlineClicked")
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
	else if (action == "StartDataManager")
	{
		gui.ActivatePage("Downloading", 0.25, error_output);
		std::string url = "http://vdrift.svn.sourceforge.net/viewvc/vdrift/vdrift-data/cars/350Z/?view=tar";
		bool success = Download(url);
		if (success)
		{
			gui.ActivatePage("DataManager", 0.25, error_output);
		}
		else
		{
			gui.ActivatePage("DataConnectionError", 0.25, error_output);
		}
	}
	else
	{
		error_output << "Unhandled GUI event: " << action << endl;
	}
}

///send inputs to the car, check for collisions, and so on
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
			for (unsigned int i = 0; i < inputarray.size(); i++)
			{
				carinputs[i] = inputarray[i];
			}
		}
		else
		{
			//carinputs = carcontrols_local.second.GetInputs();
			carinputs = carcontrols_local.second.ProcessInput(
					settings.GetJoyType(),
					eventsystem,
					car.GetLastSteer(),
					TickPeriod(),
					settings.GetJoy200(),
					car.GetSpeed(),
					settings.GetSpeedSensitivity(),
					window.GetW(),
					window.GetH(),
					settings.GetButtonRamp(),
					settings.GetHGateShifter());
		}
	}
	else
	{
		carinputs = ai.GetInputs(&car);
		assert(carinputs.size() == CARINPUT::INVALID);
	}

	//force brake and clutch during staging and once the race is over
	if (timer.Staging() || ((int)timer.GetCurrentLap(cartimerids[&car]) > race_laps && race_laps > 0))
	{
		carinputs[CARINPUT::BRAKE] = 1.0;
	}

	car.HandleInputs(carinputs, TickPeriod());

	if (carcontrols_local.first == &car)
	{
		if (replay.GetRecording())
			replay.RecordFrame(carinputs, car);

		inputgraph.Update(carinputs);

		if (replay.GetPlaying())
		{
			//this next line allows game inputs to be processed
			carcontrols_local.second.ProcessInput(
				settings.GetJoyType(),
				eventsystem,
				car.GetLastSteer(),
				TickPeriod(),
				settings.GetJoy200(),
				car.GetSpeed(),
				settings.GetSpeedSensitivity(),
				window.GetW(),
				window.GetH(),
				settings.GetButtonRamp(),
				settings.GetHGateShifter());
		}
		
		stringstream debug_info1, debug_info2, debug_info3, debug_info4;
		if (debugmode)
		{
			car.DebugPrint(debug_info1, true, false, false, false);
			car.DebugPrint(debug_info2, false, true, false, false);
			car.DebugPrint(debug_info3, false, false, true, false);
			car.DebugPrint(debug_info4, false, false, false, true);
		}
		std::pair <int, int> curplace = timer.GetPlayerPlace();
		hud.Update(fonts["lcd"], fonts["futuresans"], timer.GetPlayerTime(), timer.GetLastLap(),
			timer.GetBestLap(), timer.GetStagingTimeLeft(),
			timer.GetPlayerCurrentLap(), race_laps, curplace.first, curplace.second,
			car.GetClutch(), car.GetGear(), car.GetEngineRPM(),
			car.GetEngineRedline(), car.GetEngineRPMLimit(), car.GetSpeedometer(),
			settings.GetMPH(), debug_info1.str(), debug_info2.str(), debug_info3.str(),
			debug_info4.str(), window.GetW(), window.GetH(), car.GetABSEnabled(),
			car.GetABSActive(), car.GetTCSEnabled(), car.GetTCSActive(),
			timer.GetIsDrifting(cartimerids[&car]), timer.GetDriftScore(cartimerids[&car]),
			timer.GetThisDriftScore(cartimerids[&car]));

		//handle camera mode change inputs
		CAMERA * old_camera = active_camera;
		CARCONTROLMAP_LOCAL & carcontrol = carcontrols_local.second;
		if (carcontrol.GetInput(CARINPUT::VIEW_HOOD))
		{
			active_camera = car.Cameras().Select("hood");
		}
		else if (carcontrol.GetInput(CARINPUT::VIEW_INCAR))
		{
			active_camera = car.Cameras().Select("incar");
		}
		else if (carcontrol.GetInput(CARINPUT::VIEW_FREE))
		{
			active_camera = car.Cameras().Select("free");
		}
		else if (carcontrol.GetInput(CARINPUT::VIEW_ORBIT))
		{
			active_camera = car.Cameras().Select("orbit");
		}
		else if (carcontrol.GetInput(CARINPUT::VIEW_CHASERIGID))
		{
			active_camera = car.Cameras().Select("chaserigid");
		}
		else if (carcontrol.GetInput(CARINPUT::VIEW_CHASE))
		{
			active_camera = car.Cameras().Select("chase");
		}
		else if (carcontrol.GetInput(CARINPUT::VIEW_NEXT))
		{
			active_camera = car.Cameras().Next();
		}
		else if (carcontrol.GetInput(CARINPUT::VIEW_PREV))
		{
			active_camera = car.Cameras().Prev();
		}
		settings.SetCameraMode(active_camera->GetName());
		
		if(old_camera != active_camera)
		{
			active_camera->Reset(car.GetPosition(), car.GetOrientation());
		}

		//handle camera inputs
		float left = TickPeriod() * (carcontrol.GetInput(CARINPUT::PAN_LEFT) - carcontrol.GetInput(CARINPUT::PAN_RIGHT));
		float up = TickPeriod() * (carcontrol.GetInput(CARINPUT::PAN_UP) - carcontrol.GetInput(CARINPUT::PAN_DOWN));
		float dx = TickPeriod() * (carcontrol.GetInput(CARINPUT::ZOOM_IN) - carcontrol.GetInput(CARINPUT::ZOOM_OUT));
		active_camera->Rotate(-up, left); //up is inverted
		active_camera->Move(4 * dx, 0, 0);

		//set cockpit sounds
		bool incar = (active_camera->GetName() == "hood" || active_camera->GetName() == "incar");
		{
			std::list <SOUNDSOURCE *> soundlist;
			car.GetEngineSoundList(soundlist);
			for (std::list <SOUNDSOURCE *>::iterator s = soundlist.begin(); s != soundlist.end(); s++)
			{
				(*s)->Enable3D(!incar);
			}
		}
		
		//hide glass if we're inside the car
		car.EnableGlass(!incar);
		
		//move up the close shadow distance if we're in the cockpit
		graphics_interface->SetCloseShadow(incar ? 1.0 : 5.0);
	}
}

///start a new game.  LeaveGame() is called first thing, which should take care of clearing out all current data.
bool GAME::NewGame(bool playreplay, bool addopponents, int num_laps)
{
	LeaveGame(); //this should clear out all data
	
	race_laps = num_laps; // cache number of laps for gui

	if (playreplay)
	{
		stringstream replayfilenamestream;

		if(benchmode)
			replayfilenamestream << pathmanager.GetReplayPath() << "/benchmark.vdr";
		else
		{
			std::list <std::pair <std::string, std::string> > replaylist;

			unsigned sel_index = settings.GetSelectedReplay() - 1;

			PopulateReplayList(replaylist);

			std::list<std::pair <std::string, std::string> >::iterator it = replaylist.begin();
			advance(it, sel_index);

			replayfilenamestream << pathmanager.GetReplayPath() << "/" << it->second;
		}

		string replayfilename = replayfilenamestream.str();
		info_output << "Loading replay file " << replayfilename << endl;
		if (!replay.StartPlaying(replayfilename, error_output))
			return false;
	}

	//set the track name
	std::string trackname;
	if (playreplay)
	{
		trackname = replay.GetTrack();
	}
	else
	{
		trackname = settings.GetTrack();
	}
	
	if (!LoadTrack(trackname))
	{
		error_output << "Error during track loading: " << trackname << endl;
		return false;
	}
	
	//start out with no camera
	active_camera = NULL;

	//load the local player's car
	//cout << "About to load car..." << endl;
	MATHVECTOR<float, 3> carcolor(0);
	string carname, carpaint("00"), carfile;
	if (playreplay)
	{
		carname = replay.GetCarType();
		carpaint = replay.GetCarPaint();
		carfile = replay.GetCarFile();
		replay.GetCarColor(carcolor[0], carcolor[1], carcolor[2]);
	}
	else
	{
		carname = settings.GetSelectedCar();
		carpaint = settings.GetPlayerCarPaint();
		settings.GetPlayerColor(carcolor[0], carcolor[1], carcolor[2]);
	}
	if (!LoadCar(carname, carpaint, carcolor, track.GetStart(0).first, track.GetStart(0).second, true, false, carfile))
	{
		return false;
	}
	//cout << "After load car: " << carcontrols_local.first << endl;

	//load AI cars
	if (addopponents)
	{
		int carcount = 1;
		for (unsigned int i = 0; i < opponents.size(); ++i)
		{
			//int startplace = std::min(carcount, track.GetNumStartPositions()-1);
			int startplace = carcount;
			if (!LoadCar(opponents[i], opponents_paint[i], opponents_color[i], track.GetStart(startplace).first, track.GetStart(startplace).second, false, true))
				return false;
			ai.add_car(&cars.back(), settings.GetAIDifficulty());
			carcount++;
		}
	}
	else
	{
		opponents.clear();
	}

	//send car sounds to the sound subsystem
	for (std::list <CAR>::iterator i = cars.begin(); i != cars.end(); ++i)
	{
		std::list <SOUNDSOURCE *> soundlist;
		i->GetSoundList(soundlist);
		for (std::list <SOUNDSOURCE *>::iterator s = soundlist.begin(); s != soundlist.end(); ++s)
		{
			sound.AddSource(**s);
		}
	}

	//enable HUD display
	if (settings.GetShowHUD())
		hud.Show();
	if (settings.GetInputGraph())
		inputgraph.Show();

	//load the timer
	float pretime = 0.0f;
	if (num_laps > 0)
		pretime = 3.0f;
	if (!timer.Load(pathmanager.GetTrackRecordsPath()+"/"+trackname+".txt", pretime, error_output))
		return false;

	//add cars to the timer system
	int count = 0;
	for (std::list <CAR>::iterator i = cars.begin(); i != cars.end(); ++i)
	{
		cartimerids[&(*i)] = timer.AddCar(i->GetCarType());
		if (carcontrols_local.first == &(*i))
			timer.SetPlayerCarID(count);
		count++;
	}

	//set up the GUI
	gui.SetInGame(true);
	gui.DeactivateAll();
	if (settings.GetMouseGrab())
		eventsystem.SetMouseCursorVisibility(false);

	//record a replay
	if (settings.GetRecordReplay() && !playreplay)
	{
		assert(carcontrols_local.first);
		std::string cartype = carcontrols_local.first->GetCarType();
		std::string carpath = pathmanager.GetCarPath()+"/"+cartype+"/"+cartype+".car";
		
		float r(0), g(0), b(0);
		settings.GetPlayerColor(r, g, b);
		
		replay.StartRecording(
			cartype,
			settings.GetPlayerCarPaint(),
			r, g, b,
			carpath,
			settings.GetTrack(),
			error_output);
	}

	textures.Sweep();
	models.Sweep();
	sounds.Sweep();
	return true;
}

std::string GAME::GetReplayRecordingFilename()
{
	//determine replay filename
	int replay_number = 1;
	for (int i = 1; i < 99; i++)
	{
		stringstream s;
		s << pathmanager.GetReplayPath() << "/" << i << ".vdr";
		if (!pathmanager.FileExists(s.str()))
		{
			replay_number = i;
			break;
		}
	}
	stringstream s;
	s << pathmanager.GetReplayPath() << "/" << replay_number << ".vdr";
	return s.str();
}

///clean up all game data
void GAME::LeaveGame()
{
	ai.clear_cars();

	carcontrols_local.first = NULL;

	if (replay.GetRecording())
	{
		info_output << "Saving replay to " << GetReplayRecordingFilename() << endl;
		replay.StopRecording(GetReplayRecordingFilename());
		
		std::list <std::pair <std::string, std::string> > replaylist;
		PopulateReplayList(replaylist);
		gui.ReplaceOptionValues("game.selected_replay", replaylist, error_output);
	}
	if (replay.GetPlaying()) replay.StopPlaying();

	gui.SetInGame(false);
	
	// clear out the static drawables
	SCENENODE empty;
	graphics_interface->AddStaticNode(empty, true);

	if (sound.Enabled())
	{
		for (std::list <CAR>::iterator i = cars.begin(); i != cars.end(); ++i)
		{
			std::list <SOUNDSOURCE *> soundlist;
			i->GetSoundList(soundlist);
			for (std::list <SOUNDSOURCE *>::iterator s = soundlist.begin(); s != soundlist.end(); s++)
			{
				sound.RemoveSource(*s);
			}
		}
	}
	
	collision.Clear();
	track.Unload();
	cars.clear();
	hud.Hide();
	inputgraph.Hide();
	trackmap.Unload();
	timer.Unload();
	active_camera = 0;
	pause = false;
	race_laps = 0;
	tire_smoke.Clear();
}

///add a car, optionally controlled by the local player
bool GAME::LoadCar(
	const std::string & carname, const std::string & carpaint,
	const MATHVECTOR <float, 3> & carcolor,
	const MATHVECTOR <float, 3> & start_position,
	const QUATERNION <float> & start_orientation,
	bool islocal, bool isai, const string & carfile)
{
	std::string partspath = pathmanager.GetCarSharedDir();
	std::string carpath = pathmanager.GetCarDir()+"/"+carname;
	std::string cfgpath = pathmanager.GetDataPath()+"/"+carpath+"/"+carname+".car";
	
	CONFIG carconf;
	if (carfile.empty()) //if no file is passed in, then load it from disk
	{
		if (!carconf.Load(cfgpath)) return false;
	}
	else
	{
		stringstream carstream(carfile);
		if (!carconf.Load(carstream)) return false;
	}
	
	cars.push_back(CAR());
	CAR & car = cars.back();
	bool loaddriver = true;
	
	if (!car.LoadGraphics(
		carconf, carpath, carname, partspath,
		carcolor, carpaint, settings.GetTextureSize(), settings.GetAnisotropy(),
		settings.GetCameraBounce(), loaddriver, debugmode,
		textures, models, info_output, error_output))
	{
		error_output << "Error loading car: " << carname << endl;
		cars.pop_back();
		return false;
	}
	
	if(sound.Enabled() && !car.LoadSounds(carpath, carname, sound.GetDeviceInfo(), sounds, info_output, error_output))
	{
		return false;
	}
	
	if (!car.LoadPhysics(
		carconf, carpath,
		start_position, start_orientation,
		settings.GetABS() || isai, settings.GetTCS() || isai,
		models,  collision,
		info_output, error_output))
	{
		return false;
	}
	
	info_output << "Car loading was successful: " << carname << endl;
	if (islocal)
	{
		//load local controls
		carcontrols_local.first = &cars.back();

		//set the active camera
		active_camera = car.Cameras().Select(settings.GetCameraMode());
		active_camera->Reset(car.GetPosition(), car.GetOrientation());

		// setup auto clutch and auto shift
		ProcessNewSettings();
		
		// shift into first gear if autoshift enabled
		if (carcontrols_local.first && settings.GetAutoShift())
			carcontrols_local.first->SetGear(1);
	}
	
	return true;
}

bool GAME::LoadTrack(const std::string & trackname)
{
	LoadingScreen(0.0, 1.0, false, "", 0.5, 0.5);

	//load the track
	if (!track.DeferredLoad(
			textures, models,
			pathmanager.GetTrackPath()+"/"+trackname,
			pathmanager.GetTrackDir()+"/"+trackname,
			pathmanager.GetEffectsTextureDir(),
			settings.GetTextureSize(),
			settings.GetAnisotropy(),
			settings.GetTrackReverse(),
			graphics_interface->GetShadows(),
			false))
	{
		error_output << "Error loading track: " << trackname << endl;
		return false;
	}
	
	bool success = true;
	int count = 0;
	while (!track.Loaded() && success)
	{
		int displayevery = track.DeferredLoadTotalObjects() / 50;
		if (displayevery == 0 || count % displayevery == 0)
		{
			LoadingScreen(count, track.DeferredLoadTotalObjects(), false, "", 0.5, 0.5);
		}
		success = track.ContinueDeferredLoad();
		count++;
	}
	
	if (!success)
	{
		error_output << "Error loading track (deferred): " << trackname << endl;
		return false;
	}

	//set racing line visibility
	track.SetRacingLineVisibility(settings.GetRacingline());

	//generate the track map
	if (!trackmap.BuildMap(
			track.GetRoadList(),
			window.GetW(),
			window.GetH(),
			trackname,
			pathmanager.GetHUDTextureDir(),
			settings.GetTextureSize(),
			textures,
			error_output))
	{
		error_output << "Error loading track map: " << trackname << endl;
		return false;
	}

	//setup track collision
	collision.Reset(track);
	collision.DebugPrint(info_output);
	
	//build static drawlist
	#ifdef USE_STATIC_OPTIMIZATION_FOR_TRACK
	graphics_interface->AddStaticNode(track.GetTrackNode());
	#endif

	return true;
}

bool GAME::LoadFonts()
{
	string fontdir = pathmanager.GetFontDir(settings.GetSkin());
	string fontpath = pathmanager.GetDataPath()+"/"+fontdir;

	if (graphics_interface->GetUsingShaders())
	{
		if (!fonts["freesans"].Load(fontpath+"/freesans.txt",fontdir, "freesans.png", settings.GetTextureSize(), textures, error_output)) return false;
		if (!fonts["lcd"].Load(fontpath+"/lcd.txt",fontdir, "lcd.png", settings.GetTextureSize(), textures, error_output)) return false;
		if (!fonts["futuresans"].Load(fontpath+"/futuresans.txt",fontdir, "futuresans.png", settings.GetTextureSize(), textures, error_output)) return false;
	}
	else
	{
		if (!fonts["freesans"].Load(fontpath+"/freesans.txt",fontdir, "freesans_noshaders.png", settings.GetTextureSize(), textures, error_output)) return false;
		if (!fonts["lcd"].Load(fontpath+"/lcd.txt",fontdir, "lcd_noshaders.png", settings.GetTextureSize(), textures,  error_output)) return false;
		if (!fonts["futuresans"].Load(fontpath+"/futuresans.txt",fontdir, "futuresans_noshaders.png", settings.GetTextureSize(), textures, error_output)) return false;
	}

	info_output << "Loaded fonts successfully" << endl;

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

	stringstream fpsstr;
	fpsstr << "FPS: " << (int)fps_avg;

	if(fps_min == 0 && frame > 20) //don't start looking an min/max until we've put out a few frames
	{
		fps_max = fps_avg;
		fps_min = fps_avg;
	}
	else if(fps_avg > fps_max)
	{
		fps_max = fps_avg;
	}
	else if(fps_avg < fps_min)
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
		profiling_text.Revise(PROFILER.getAvgSummary(quickprof::MICROSECONDS));
	}
}

bool SortStringPairBySecond (const pair<string,string> & first, const pair<string,string> & second)
{
	return first.second < second.second;
}

bool UnsignedNumericSort (const string a, const string b)
{
	unsigned i = 0;
	while ((i < a.length()) && (i < b.length()))
	{
		if(a[i] >= '0' && a[i] <= '9' && b[i] >= '0' && b[i] <= '9')
		{
			int an = atoi(a.c_str() + i),	bn = atoi(b.c_str() + i);
			
			if (an < bn)
				return true;
			else if (an > bn)
				return false;
			
			for(; bn; i++, bn/=10);

			continue;
		}
		if(a[i] < b[i])
			return true;
		else if(a[i] > b[i])
			return false;
		i++;
	}
	return true;
}

void GAME::PopulateReplayList(std::list <std::pair <std::string, std::string> > & replaylist)
{
	replaylist.clear();
	int numreplays = 0;
	std::list <std::string> replayfoldercontents;
	if (pathmanager.GetFileList(pathmanager.GetReplayPath(),replayfoldercontents))
	{
		replayfoldercontents.sort(UnsignedNumericSort);
		for (std::list <std::string>::iterator i = replayfoldercontents.begin(); i != replayfoldercontents.end(); ++i)
		{
			if (*i != "benchmark.vdr" && i->find(".vdr") == i->length()-4)
			{
				stringstream rnumstr;
				rnumstr << numreplays+1;
				replaylist.push_back(pair<string,string>(rnumstr.str(),*i));
				numreplays++;
			}
		}
	}

	if (numreplays == 0)
	{
		replaylist.push_back(pair<string,string>("0","None"));
		settings.SetSelectedReplay(0); //replay zero is a special value that the GAME class interprets as "None"
	}
	else
	{
		settings.SetSelectedReplay(1);
	}
}

void GAME::PopulateCarPaintList(const std::string & carname, std::list <std::pair <std::string, std::string> > & carpaintlist)
{
	carpaintlist.clear();
	bool exists = true;
	int paintnum = 0;
	while (exists)
	{
		exists = false;

		stringstream paintstr;
		paintstr.width(2);
		paintstr.fill('0');
		paintstr << paintnum;

		std::string cartexfile =pathmanager.GetCarPath()+"/"+carname+"/body"+paintstr.str()+".png";
		//std::cout << cartexfile << std::endl;
		ifstream check(cartexfile.c_str());
		if (check)
		{
			exists = true;
			carpaintlist.push_back(pair<string,string>(paintstr.str(),paintstr.str()));
			//std::cout << carname << ": " << paintstr.str() << std::endl;
			paintnum++;
		}
	}
}

void GAME::PopulateValueLists(std::map<std::string, std::list <std::pair <std::string, std::string> > > & valuelists)
{
	//populate track list
	list <pair<string,string> > tracklist;
	list <string> trackfolderlist;
	pathmanager.GetFileList(pathmanager.GetTrackPath(), trackfolderlist);
	for (list <string>::iterator i = trackfolderlist.begin(); i != trackfolderlist.end(); ++i)
	{
		ifstream check((pathmanager.GetTrackPath()+"/"+*i+"/about.txt").c_str());
		if (check)
		{
			string displayname;
			getline(check, displayname);
			tracklist.push_back(pair<string,string>(*i,displayname));
		}
	}
	tracklist.sort(SortStringPairBySecond);
	valuelists["tracks"] = tracklist;

	//populate car list
	list <pair<string,string> > carlist;
	list <string> carfolderlist;
	pathmanager.GetFileList(pathmanager.GetCarPath(), carfolderlist);
	for (list <string>::iterator i = carfolderlist.begin(); i != carfolderlist.end(); ++i)
	{
		ifstream check((pathmanager.GetCarPath()+"/"+*i+"/about.txt").c_str());
		if (check)
		{
			carlist.push_back(pair<string,string>(*i,*i));
		}
	}
	valuelists["cars"] = carlist;

	//populate car paints
	PopulateCarPaintList(settings.GetSelectedCar(), valuelists["player_paints"]);
	PopulateCarPaintList(settings.GetOpponentCar(), valuelists["opponent_paints"]);

	//populate video mode list
	list <pair<string,string> > modelistx;
	list <pair<string,string> > modelisty;
	ifstream modes(pathmanager.GetVideoModeFile().c_str());
	while (modes.good())
	{
		string x, y;
		modes >> x;
		modes >> y;
		if (!x.empty() && !y.empty())
		{
			modelistx.push_back(pair<string,string>(x,x));
			modelisty.push_back(pair<string,string>(y,y));
		}
	}
	modelistx.reverse();
	modelisty.reverse();
	valuelists["resolution_widths"] = modelistx;
	valuelists["resolution_heights"] = modelisty;

	//populate anisotropy list
	int max_aniso = graphics_interface->GetMaxAnisotropy();
	valuelists["anisotropy"].push_back(pair<string,string>("0","Off"));
	int cur = 1;
	while (cur <= max_aniso)
	{
		stringstream anisostr;
		anisostr << cur;
		valuelists["anisotropy"].push_back(pair<string,string>(anisostr.str(),anisostr.str()+"X"));
		cur *= 2;
	}

	//populate antialiasing list
	valuelists["antialiasing"].push_back(pair<string,string>("0","Off"));
	if (graphics_interface->AntialiasingSupported())
	{
		valuelists["antialiasing"].push_back(pair<string,string>("2","2X"));
		valuelists["antialiasing"].push_back(pair<string,string>("4","4X"));
	}
	
	//populate replays list
	PopulateReplayList(valuelists["replays"]);
	
	//populate other lists
	valuelists["joy_indeces"].push_back(pair<string,string>("0","0"));
	
	//populate skins
	list <string> skinlist;
	pathmanager.GetFileList(pathmanager.GetSkinPath(), skinlist);
	for (list <string>::iterator i = skinlist.begin(); i != skinlist.end(); ++i)
	{
		if (pathmanager.FileExists(pathmanager.GetSkinPath()+*i+"/menus/Main"))
		{
			valuelists["skins"].push_back(pair<string,string>(*i,*i));
		}
	}
	
	//populate languages
	list <string> languages;
	string skinfolder = pathmanager.GetDataPath() + "/" + pathmanager.GetGUILanguageDir(settings.GetSkin()) + "/";
	pathmanager.GetFileList(skinfolder, languages, ".lng");
	for (list <string>::iterator i = languages.begin(); i != languages.end(); ++i)
	{
		if (pathmanager.FileExists(skinfolder + *i))
		{
			size_t n = i->rfind(".lng");
			string value = i->substr(0, n);
			valuelists["languages"].push_back(pair<string,string>(value, value));
		}
	}
}

// read options from settings
void GAME::GetOptions(std::map<std::string, std::string> & options)
{
	bool write_to = true;
	CONFIG tempconfig;
	settings.Serialize(write_to, tempconfig);
	
	for (CONFIG::const_iterator ic = tempconfig.begin(); ic != tempconfig.end(); ++ic)
	{
		std::string section = ic->first;
		for (CONFIG::SECTION::const_iterator is = ic->second.begin(); is != ic->second.end(); ++is)
		{
			if (section.length() > 0)
				options[section + "." + is->first] = is->second;
			else
				options[is->first] = is->second;
		}
	}
}

// write options to settings
void GAME::SetOptions(const std::map<std::string, std::string> & options)
{
	CONFIG tempconfig;
	for (map<string, string>::const_iterator i = options.begin(); i != options.end(); ++i)
	{
		std::string section;
		std::string param = i->first;
		size_t n = param.find(".");
		if (n < param.length())
		{
			section = param.substr(0, n);
			param.erase(0, n + 1);
		}
		tempconfig.SetParam(section, param, i->second);
	}
	
	bool write_to = false;
	settings.Serialize(write_to, tempconfig);
	
	// account for new settings
	ProcessNewSettings();
}

///update the game with any new setting changes that have just been made
void GAME::ProcessNewSettings()
{
	if (track.Loaded())
	{
		if (settings.GetShowHUD())
			hud.Show();
		else
			hud.Hide();

		if (settings.GetInputGraph())
			inputgraph.Show();
		else
			inputgraph.Hide();

		track.SetRacingLineVisibility(settings.GetRacingline());
	}

	if (carcontrols_local.first)
	{
		carcontrols_local.first->SetAutoClutch(settings.GetAutoClutch());
		carcontrols_local.first->SetAutoShift(settings.GetAutoShift());
	}

	sound.SetMasterVolume(settings.GetMasterVolume());
}

void GAME::LoadingScreen(float progress, float max, bool drawGui, const std::string & optionalText, float x, float y)
{
	assert(max > 0);
	loadingscreen.Update(progress/max, optionalText, x, y);

	graphics_interface->GetDynamicDrawlist().clear();
	if (drawGui)
		TraverseScene<false>(gui.GetNode(), graphics_interface->GetDynamicDrawlist());
	TraverseScene<false>(loadingscreen.GetNode(), graphics_interface->GetDynamicDrawlist());

	graphics_interface->SetupScene(45.0, 100.0, MATHVECTOR <float, 3> (), QUATERNION <float> (), MATHVECTOR <float, 3> ());

	graphics_interface->BeginScene(error_output);
	graphics_interface->DrawScene(error_output);
	graphics_interface->EndScene(error_output);
	window.SwapBuffers(error_output);
}

bool GAME::Download(const std::string & file)
{
	std::vector <std::string> files;
	files.push_back(file);
	return Download(files);
}

bool GAME::Download(const std::vector <std::string> & urls)
{
	// make sure we're not currently downloading something in the background
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
				text << " " << HTTP::ExtractFilenameFromUrl(url) << " " << HTTPINFO::FormatSpeed(info.speed);
			double total = 1000000;
			if (info.totalsize > 0)
				total = info.totalsize;
			
			// tick the GUI
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
#ifdef ENABLE_FORCE_FEEDBACK
	if (carcontrols_local.first)
	{
		//static ofstream file("ff_output.txt");
		ff_update_time += dt;
		const double ffdt = 0.02;
		if (ff_update_time >= ffdt )
		{
			ff_update_time = 0.0;
			double feedback = -carcontrols_local.first->GetFeedback();

			feedback = settings.GetFFGain() * feedback / 100.0;
			if (settings.GetFFInvert()) feedback = -feedback;

			if (feedback > 1.0)
				feedback = 1.0;
			if (feedback < -1.0)
				feedback = -1.0;
			//feedback += 0.5;
			/*
			static double motion_frequency = 0.1;
			static double motion_amplitude = 4.0;
			static double spring_strength = 1.0;
			*/
			//double center = sin( timefactor * 2 * M_PI * motion_frequency ) * motion_amplitude;
			double force = feedback;

			//std::cout << "ff_update_time: " << ff_update_time << " force: " << force << endl;
			forcefeedback->update(force, &feedback, ffdt, error_output);
		}
	}
	
	if (pause && dt == 0)
	{
		double pos=0;
		forcefeedback->update(0, &pos, 0.02, error_output);
	}
#endif
}

void GAME::AddTireSmokeParticles(float dt, CAR & car)
{
	for (int i = 0; i < 4; i++)
	{
		float squeal = car.GetTireSquealAmount(WHEEL_POSITION(i));
		if (squeal > 0)
		{
			unsigned int interval = 0.2 / dt; //only spawn particles every so often
			if (particle_timer % interval == 0)
			{
				tire_smoke.AddParticle(
					car.GetWheelPosition(WHEEL_POSITION(i)) - MATHVECTOR<float,3>(0,0,car.GetTireRadius(WHEEL_POSITION(i))),
					0.5, 0.5, 0.5, 0.5);
			}
		}
	}
}

void GAME::UpdateParticleSystems(float dt)
{
	if (track.Loaded() && active_camera)
	{
		QUATERNION <float> camlook;
		camlook.Rotate(3.141593*0.5,1,0,0);
		camlook.Rotate(-3.141593*0.5,0,0,1);
		QUATERNION <float> camorient = -(active_camera->GetOrientation() * camlook);

		tire_smoke.Update(dt, camorient, active_camera->GetPosition());
	}

	particle_timer++;
	particle_timer = particle_timer % (unsigned int)((1.0/TickPeriod()));
}

void GAME::UpdateDriftScore(CAR & car, double dt)
{
	//assert that the car is registered with the timer system
	assert(cartimerids.find(&car) != cartimerids.end());

	//make sure the car is not off track
	int wheel_count = 0;
	for (int i=0; i < 4; i++)
	{
		if ( car.GetCurPatch ( WHEEL_POSITION(i) ) ) wheel_count++;
	}

	bool on_track = ( wheel_count > 1 );
	bool is_drifting = false;
	bool spin_out = false;
	if ( on_track )
	{
		//car's velocity on the horizontal plane(should use surface plane here)
		MATHVECTOR <float, 3> car_velocity = car.GetVelocity();
		car_velocity[2] = 0;
		float car_speed = car_velocity.Magnitude();

		//car's direction on the horizontal plane
		MATHVECTOR <float, 3> car_orientation = car.GetOrientation().AxisX();
		car_orientation[2] = 0;
		float orient_mag = car_orientation.Magnitude();

		//speed must be above 10 m/s and orientation must be valid
		if ( car_speed > 10 && orient_mag > 0.01)
		{
			//angle between car's direction and velocity
			float cos_angle = car_orientation.dot(car_velocity) / (car_speed * orient_mag);
			if (cos_angle > 1) cos_angle = 1;
			else if (cos_angle < -1) cos_angle = -1;
			float car_angle = acos(cos_angle);

			//drift starts when the angle > 0.2 (around 11.5 degrees)
			//drift ends when the angle < 0.1 (aournd 5.7 degrees)
			float angle_threshold(0.2);
			if ( timer.GetIsDrifting(cartimerids[&car]) ) angle_threshold = 0.1;

			is_drifting = ( car_angle > angle_threshold && car_angle <= M_PI/2.0 );
			spin_out = ( car_angle > M_PI/2.0 );

			//calculate score
			if ( is_drifting )
			{
				//base score is the drift distance
				timer.IncrementThisDriftScore(cartimerids[&car], dt * car_speed);

				//bonus score calculation is now done in TIMER
				timer.UpdateMaxDriftAngleSpeed(cartimerids[&car], car_angle, car_speed);
				//std::cout << timer.GetDriftScore(cartimerids[&car]) << " + " << timer.GetThisDriftScore(cartimerids[&car]) << endl;
			}
		}
	}

	timer.SetIsDrifting(cartimerids[&car], is_drifting, on_track && !spin_out);
	//std::cout << is_drifting << ", " << on_track << ", " << car_angle << endl;
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
