#include "game.h"
#include "unittest.h"
#include "definitions.h"
#include "joepack.h"
#include "matrix4.h"
#include "configfile.h"
#include "carwheelposition.h"
#include "numprocessors.h"
#include "parallel_task.h"
#include "performance_testing.h"
#include "widget_label.h"
#include "quickprof.h"
#include "tracksurfacetype.h"

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

	InitializeCoreSubsystems();

	//load loading screen assets
	if (!loadingscreen.Initialize(loadingscreen_node, pathmanager.GetGUITexturePath(settings.GetSkin()),
				 graphics.GetW(), graphics.GetH(), settings.GetTextureSize(), error_output))
	{
		error_output << "Error loading the loading screen" << endl; //ironic
		return;
	}

	//load controls
	info_output << "Loading car controls from: " << pathmanager.GetCarControlsFile() << endl;
	if (!pathmanager.FileExists(pathmanager.GetCarControlsFile()))
	{
		info_output << "Car control file " << pathmanager.GetCarControlsFile() << " doesn't exist; using defaults" << endl;
		carcontrols_local.second.Load(pathmanager.GetDefaultCarControlsFile(), info_output, error_output);
		carcontrols_local.second.Save(pathmanager.GetCarControlsFile(), info_output, error_output);
	}
	else
		carcontrols_local.second.Load(pathmanager.GetCarControlsFile(), info_output, error_output);

	InitializeSound(); //if sound initialization fails, that's okay, it'll disable itself

	//load font data
	if (!LoadFonts())
	{
		error_output << "Error loading fonts" << endl;
		return;
	}

	//initialize HUD
	if (!hud.Init(rootnode, pathmanager.GetGUITexturePath(settings.GetSkin()), fonts["lcd"], fonts["futuresans"], error_output, graphics.GetW(), graphics.GetH(), settings.GetTextureSize(), debugmode))
	{
		error_output << "Error initializing HUD" << endl;
		return;
	}
	hud.Hide();

	//initialise input graph
	if (!inputgraph.Init(rootnode, pathmanager.GetGUITexturePath(settings.GetSkin()), error_output, settings.GetTextureSize()))
	{
		error_output << "Error initializing input graph" << endl;
		return;
	}
	inputgraph.Hide();

	//initialize GUI
	if (!InitializeGUI())
		return;

	//initialize FPS counter
	fps_draw = &rootnode.AddDrawable();
	fps_draw->SetDrawOrder(150);
	
	//initialize profiling text
	if (profilingmode)
	{
		float screenhwratio = (float)graphics.GetH()/graphics.GetW();
		profiling_text.Init(rootnode, fonts["futuresans"], "", 0.01, 0.25, screenhwratio*0.2,0.2);
		profiling_text.GetDrawable()->SetDrawOrder(150);
	}

	//load particle systems
	list <string> smoketexlist;
	pathmanager.GetFolderIndex(pathmanager.GetTireSmokeTexturePath(), smoketexlist, ".png");
	for (list <string>::iterator i = smoketexlist.begin(); i != smoketexlist.end(); ++i)
		*i = pathmanager.GetTireSmokeTexturePath() + "/" + *i;
	if (!tire_smoke.Load(rootnode, smoketexlist, settings.GetAnisotropy(), settings.GetTextureSize(), error_output))
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

	if (benchmode)
	{
		if(!NewGame(true))
		{
			error_output << "Error loading benchmark" << endl;
		}
		//End();
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
void GAME::InitializeCoreSubsystems()
{
	pathmanager.Init(info_output, error_output);
	settings.Load(pathmanager.GetSettingsFile());
	
	if (!LastStartWasSuccessful())
	{
		info_output << "The last VDrift startup was unsuccessful.\nSettings have been set to failsafe defaults.\nYour original VDrift.config file was backed up to VDrift.config.backup" << endl;
		settings.Save(pathmanager.GetSettingsFile()+".backup");
		settings.SetFailsafeSettings();
	}
	BeginStartingUp();
	
	graphics.Init(pathmanager.GetShaderPath(), "VDrift - open source racing simulation",
		settings.GetResolution_x(), settings.GetResolution_y(),
		settings.GetBpp(), settings.GetDepthbpp(), settings.GetFullscreen(),
		settings.GetShaders(), settings.GetAntialiasing(), settings.GetShadows(),
		settings.GetShadowDistance(), settings.GetShadowQuality(),
		settings.GetReflections(), pathmanager.GetStaticReflectionMap(),
		pathmanager.GetStaticAmbientMap(),
		settings.GetAnisotropic(), settings.GetTextureSize(),
		settings.GetLighting(), settings.GetBloom(),
		info_output, error_output);
	
	QUATERNION <float> ldir;
	ldir.Rotate(3.141593*0.05,0,1,0);
	ldir.Rotate(-3.141593*0.1,1,0,0);
	graphics.SetSunDirection(ldir);

	eventsystem.Init(info_output);
}

bool GAME::InitializeGUI()
{
	list <string> menufiles;
	string menufolder = pathmanager.GetGUIMenuPath(settings.GetSkin());
	if (!pathmanager.GetFolderIndex(menufolder, menufiles))
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
			menufiles.erase(*i);
	}
	std::map<std::string, std::list <std::pair <std::string, std::string> > > valuelists;
	PopulateValueLists(valuelists);
	if (!gui.Load(menufiles, valuelists, pathmanager.GetOptionsFile(), pathmanager.GetCarControlsFile(), menufolder, pathmanager.GetGUITexturePath(settings.GetSkin()), pathmanager.GetDataPath(), &rootnode.AddNode(), fonts, (float)graphics.GetH()/graphics.GetW(), settings.GetTextureSize(), info_output, error_output))
	{
		error_output << "Error loading GUI files" << endl;
		return false;
	}
	std::map<std::string, std::string> optionmap;
	LoadSaveOptions(LOAD, optionmap);
	gui.SyncOptions(true, optionmap, error_output);
	gui.ActivatePage("Main", 0.5, error_output); //nice, slow fade-in
	if (settings.GetMouseGrab()) eventsystem.SetMouseCursorVisibility(true);

	return true;
}

bool GAME::InitializeSound()
{
	if (sound.Init(512, info_output, error_output))
	{
		generic_sounds.SetLibraryPath(pathmanager.GetGenericSoundPath());
		
		if (!generic_sounds.Load("tire_squeal", sound.GetDeviceInfo(), error_output)) return false;
		if (!generic_sounds.Load("grass", sound.GetDeviceInfo(), error_output)) return false;
		if (!generic_sounds.Load("gravel", sound.GetDeviceInfo(), error_output)) return false;
		if (!generic_sounds.Load("bump_front", sound.GetDeviceInfo(), error_output)) return false;
		if (!generic_sounds.Load("bump_rear", sound.GetDeviceInfo(), error_output)) return false;
		if (!generic_sounds.Load("wind", sound.GetDeviceInfo(), error_output)) return false;
		if (!generic_sounds.Load("crash", sound.GetDeviceInfo(), error_output)) return false;
		
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

///break up the input into a vector of strings using the token characters given
vector <string> Tokenize(const string & input, const string & tokens)
{
	vector <string> out;
	
	unsigned int pos = 0;
	unsigned int lastpos = 0;
	
	while (pos != (unsigned int) string::npos)
	{
		pos = input.find_first_of(tokens, pos);
		string thisstr = input.substr(lastpos,pos-lastpos);
		if (!thisstr.empty())
			out.push_back(thisstr);
		pos = input.find_first_not_of(tokens, pos);
		lastpos = pos;
	}
	
	return out;
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
	arghelp["-profile PROFILENAME"] = "Store settings, controls, and records under a separate profile.";
	
	if (argmap.find("-profiling") != argmap.end() || argmap.find("-benchmark") != argmap.end())
	{
		PROFILER.init(20);
		profilingmode = true;
	}
	arghelp["-profiling"] = "Display game performance data.";
	
	
	if (!argmap["-resolution"].empty())
	{
		string res(argmap["-resolution"]);
		vector <string> restoken = Tokenize(res, "x,");
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
			settings.SetResolution_x(xres);
			settings.SetResolution_y(yres);
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
		sound.DisableAllSound();
	arghelp["-nosound"] = "Disable all sound.";

	if (argmap.find("-benchmark") != argmap.end())
	{
		info_output << "Entering benchmark mode." << endl;
		benchmode = true;
	}
	arghelp["-benchmark"] = "Run in benchmark mode.";
	
	
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

	settings.Save(pathmanager.GetSettingsFile()); //save settings first incase later deinits cause crashes

	collision.Clear();
	track.Clear();
	trackmap.Unload();
	
	if (tracknode)
		delete tracknode;
	tracknode = NULL;

	graphics.Deinit();
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
		QUATERNION <float> camlook;
		camlook.Rotate(3.141593*0.5,1,0,0);
		camlook.Rotate(-3.141593*0.5,0,0,1);
		QUATERNION <float> camorient = -(active_camera->GetOrientation()*camlook);
		graphics.SetupScene(settings.GetFOV(), settings.GetViewDistance(), active_camera->GetPosition(), camorient);
	}
	else
		graphics.SetupScene(settings.GetFOV(), settings.GetViewDistance(), MATHVECTOR <float, 3> (), QUATERNION <float> ());

	graphics.SetContrast(settings.GetContrast());
	graphics.BeginScene(error_output);
	PROFILER.endBlock("render");

	PROFILER.beginBlock("scenegraph");
	CollapseSceneToDrawlistmap(rootnode, graphics.GetDrawlistmap(), true);
	PROFILER.endBlock("scenegraph");
	PROFILER.beginBlock("render");
	graphics.DrawScene(error_output);
	PROFILER.endBlock("render");
}

void GAME::FinishDraw()
{
	PROFILER.beginBlock("render");
	graphics.EndScene(error_output);
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
	const unsigned int maxticks = (int) (1.0f / (minfps * framerate)); //slow the game down if we can't process fast enough
	const float maxtime = 1.0/minfps; //slow the game down if we can't process fast enough
	unsigned int curticks = 0;

	//throw away wall clock time if necessary to keep the framerate above the minimum
	if (deltat > maxtime)
		deltat = maxtime;

	target_time += deltat;

	//increment game logic by however many tick periods have passed since the last GAME::Tick
	while (target_time - TickPeriod()*frame > TickPeriod() && curticks < maxticks)
	{
		frame++;

		AdvanceGameLogic();

		curticks++;
	}

	//if (curticks > 0 && frame % 100 == 0)
	//	info_output << "FPS: " << eventsystem.GetFPS() << endl;
}

void GAME::ParallelUpdate(int carindex)
{
	list <CAR>::iterator carit = cars.begin();
	for (int i = 0; i < carindex; i++)
	{
		assert(carit != cars.end());
		carit++;
	}
	assert(carit != cars.end());
	CAR & car = *carit;
	UpdateCarPhysics(car, cached_collisions_by_car[&car]);
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
			carcontrols_local.second.ProcessInput(settings.GetJoyType(), eventsystem, carcontrols_local.first->GetLastSteer(), TickPeriod(),
					settings.GetJoy200(), carcontrols_local.first->GetSpeed(), settings.GetSpeedSensitivity(), graphics.GetW(), graphics.GetH(), settings.GetButtonRamp(), settings.GetHGateShifter());
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
				ai.Visualize(rootnode);
				ai.update(TickPeriod(), &track, cars);
				PROFILER.endBlock("ai");
				
				PROFILER.beginBlock("car-update");
				for (list <CAR>::iterator i = cars.begin(); i != cars.end(); ++i)
				{
					UpdateCar(*i, TickPeriod());
				}
				PROFILER.endBlock("car-update");
				
				PROFILER.beginBlock("collision");
				if (cur_collision_frameskip == 0)
					UpdateCarChassisCollisions();
				for (list <CAR>::iterator i = cars.begin(); i != cars.end(); ++i)
				{
					if (cur_collision_frameskip == 0)
					{
						if (track.UseSurfaceTypes()) // change collision method depending on surface flag
						{
							UpdateCarWheelCollisionsWithSurfaces(*i, cached_collisions_by_car[&(*i)]);
						}
						else
						{
							UpdateCarWheelCollisions(*i, cached_collisions_by_car[&(*i)]);
						}
					}
					else
					{
						if (track.UseSurfaceTypes())
						{
							UpdateCarWheelCollisionsFromCachedWithSurfaces(*i, cached_collisions_by_car[&(*i)]);
						}
						else
						{
							UpdateCarWheelCollisionsFromCached(*i, cached_collisions_by_car[&(*i)]);
						}
					}
				}
				PROFILER.endBlock("collision");
				
				if (multithreaded)
				{
					PROFILER.beginBlock("car-physics");
					GAME * thisgame = this;
					QMP_SHARE(thisgame);
					QMP_PARALLEL_FOR(i, 0, cars.size(), quickmp::INTERLEAVED);
						QMP_USE_SHARED(thisgame, GAME*);
						thisgame->ParallelUpdate(i);
					QMP_END_PARALLEL_FOR;
					PROFILER.endBlock("car-physics");
				}
				else
				{
					for (list <CAR>::iterator i = cars.begin(); i != cars.end(); ++i)
					{
						PROFILER.beginBlock("car-physics");
						UpdateCarPhysics(*i, cached_collisions_by_car[&(*i)]);
						PROFILER.endBlock("car-physics");
					}
				}
				
				cur_collision_frameskip = (cur_collision_frameskip + 1) % collision_frameskip; //this has to be done here so it only gets incremented once per frame, instead of once per car per frame
				
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
				graphics.Screenshot(shotfile);
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
			if (!graphics.ReloadShaders(pathmanager.GetShaderPath(), info_output, error_output))
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
		    //std::cout << "Lap: " <<
			timer.Lap(carid, i->GetSector(), nextsector, (i->GetSector() >= 0)); //only count it if the car's current sector isn't -1, which is the default value when the car is loaded

			i->SetSector(nextsector);
			/*if (nextsector == 0)
			{
				if ((it->lap_number + 1) > state.GetLaps() && (state.GetGameMode() == MODE_SINGLERACE))
				{
					it->end_race = true;
				}
			}*/
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
					eventsystem.GetMousePosition()[0]/(float)graphics.GetW(), eventsystem.GetMousePosition()[1]/(float)graphics.GetH(),
							eventsystem.GetMouseButtonState(1).down, eventsystem.GetMouseButtonState(1).just_up, (float)graphics.GetH()/graphics.GetW(), error_output);
		}
		else
		{

		}
	}

	if (gui.Active())
	{
		//if the user did something that requires loading or saving options, do a sync
		bool neededsync = false;
		if (gui.OptionsNeedSync())
		{
			std::map<std::string, std::string> optionmap;
			LoadSaveOptions(LOAD, optionmap);
			//std::cout << "!!!before: " << optionmap["track"] << std::endl;
			gui.SyncOptions(false, optionmap, error_output);
			LoadSaveOptions(SAVE, optionmap);
			//std::cout << "!!!after: " << optionmap["track"] << std::endl;

			neededsync = true;
		}

		if (gui.ControlsNeedLoading())
		{
			carcontrols_local.second.Load(pathmanager.GetCarControlsFile(), info_output, error_output);
			//std::cout << "Control files are being loaded: " << gui.GetActivePageName() << ", " << gui.GetLastPageName() << std::endl;
			if (!gui.GetLastPageName().empty())
				LoadControlsIntoGUIPage(gui.GetLastPageName());
		}

		//process gui actions
		for (list <string>::iterator i = gui_actions.begin(); i != gui_actions.end(); ++i)
		{
			ProcessGUIAction(*i);
		}

		if (neededsync && gui.GetActivePageName() != "AssignControl" &&
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
		gui.ActivatePage("Main", 0.25, error_output); //uh, dunno what to do so go to the main menu
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
	std::map<std::string, std::list <std::pair <std::string, std::string> > > valuelists;
	PopulateValueLists(valuelists);
	CONFIGFILE controlfile;
	carcontrols_local.second.Save(controlfile, info_output, error_output);
	assert(gui.GetPage(pagename).Load(pathmanager.GetGUIMenuPath(settings.GetSkin())+"/"+pagename,
		    pathmanager.GetGUITexturePath(settings.GetSkin()), pathmanager.GetDataPath(),
		    controlfile, gui.GetNode(), gui.GetTextureMap(), fonts,
		    gui.GetOptionMap(), (float)graphics.GetH()/graphics.GetW(), settings.GetTextureSize(), error_output, true));
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
		if (NewGame())
		{

		}
		else
			LeaveGame();
	}
	else if (action.substr(0,14) == "controlgrabadd")
	{
		controlgrab_page = gui.GetActivePageName();
		string setting = action.substr(19);
		controlgrab_input = setting;
		if (action.substr(15,1) == "y")
			controlgrab_analog = true;
		else
			controlgrab_analog = false;
		if (action.substr(17,1) == "y")
			controlgrab_only_one = true;
		else
			controlgrab_only_one = false;
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
		gui.GetPage(gui.GetActivePageName()).UpdateOptions(false, tempoptionmap, error_output);
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
			gui.GetPage(gui.GetActivePageName()).UpdateOptions(true, tempoptionmap, error_output);

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
			gui.GetPage(gui.GetActivePageName()).UpdateOptions(true, tempoptionmap, error_output);

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
			gui.ActivatePage("Main", 0.25, error_output); //uh, dunno what to do so go to the main menu
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
			gui.ActivatePage("Main", 0.25, error_output); //uh, dunno what to do so go to the main menu
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
			gui.ActivatePage("Main", 0.25, error_output); //uh, dunno what to do so go to the main menu
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
		if (settings.GetSelectedReplay() != 0)
			if (!NewGame(true))
				gui.ActivatePage("ReplayStartError", 0.25, error_output);
		//cout << settings.GetSelectedReplay() << endl;
	}
	else if (action == "PlayerCarChange") //this means the player clicked the GUI to change their car
	{
		//re-populate gui option list for car paints
		std::list <std::pair <std::string, std::string> > carpaintlist;
		PopulateCarPaintList(settings.GetSelectedCar(), carpaintlist);
		gui.ReplaceOptionMapValues("game.car_paint", carpaintlist, error_output);
		//std::cout << "Player car changed" << endl;
	}
	else if (action == "OpponentCarChange") //this means the player clicked the GUI to change the opponent car
	{
		//re-populate gui option list for car paints
		std::list <std::pair <std::string, std::string> > carpaintlist;
		PopulateCarPaintList(settings.GetOpponentCar(), carpaintlist);
		gui.ReplaceOptionMapValues("game.opponent_car_paint", carpaintlist, error_output);
	}
	else if (action == "AddOpponent")
	{
		if (opponents.size() == 3)
			opponents.clear();
		opponents.push_back(std::make_pair(settings.GetOpponentCar(), settings.GetOpponentCarPaint()));

		std::string opponentstr = "Opponents: ";
		for (std::vector <std::pair<std::string, std::string> >::iterator i = opponents.begin(); i != opponents.end(); ++i)
		{
			if (i != opponents.begin())
				opponentstr += ", ";
			opponentstr += i->first;
		}
		gui.GetPage("SingleRace").GetLabel("OpponentsLabel").get().SetText(opponentstr);
	}
	else if (action == "RestartGame")
	{
		bool add_opponents = !opponents.empty();
		if (NewGame(false, add_opponents, race_laps))
		{

		}
		else
			LeaveGame();
	}
	else if (action == "StartRace")
	{
		//handle a single race
		if (opponents.empty())
			gui.ActivatePage("NoOpponentsError", 0.25, error_output);
		else
		{
		    int num_laps = settings.GetNumberOfLaps();

			if (NewGame(false, true, num_laps))
			{

			}
			else
				LeaveGame();
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
	//hide glass if we're inside the car
	if (carcontrols_local.first == &car)
	{
		if (active_camera == car.GetHoodCamera() || active_camera == car.GetDriverCamera())
			car.EnableGlass(false);
		else
			car.EnableGlass(true);
	}
	
	car.Update(dt);
	UpdateCarInputs(car);
	AddTireSmokeParticles(dt, car);
	UpdateDriftScore(car, dt);
}

void GAME::UpdateCarPhysics(CAR & car, std::vector <COLLISION_CONTACT> & cached_collisions) const
{
	const int extraticks = (int)((framerate+carphysics_rate*0.5)/carphysics_rate);
	const double dt = TickPeriod()/extraticks;
	
	for (int i = 0; i < extraticks; i++)
	{
		UpdateCarWheelCollisionsFromCached(car, cached_collisions);
	
		//update the car dynamics simulation
		car.TickPhysics(dt);
	}
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
                carinputs[i] = inputarray[i];
	    }
	    else
            //carinputs = carcontrols_local.second.GetInputs();
            carinputs = carcontrols_local.second.ProcessInput(settings.GetJoyType(), eventsystem, car.GetLastSteer(), TickPeriod(), settings.GetJoy200(), car.GetSpeed(), settings.GetSpeedSensitivity(), graphics.GetW(), graphics.GetH(), settings.GetButtonRamp(), settings.GetHGateShifter());
	}
	else
	{
	    carinputs = ai.GetInputs(&car);
		assert(carinputs.size() == CARINPUT::INVALID);
	}

	//force brake on and no other car inputs during staging and once the race is over
	if (timer.Staging() || ((int)timer.GetCurrentLap(cartimerids[&car]) > race_laps && race_laps > 0))
	{
	    for (int i = 0; i < CARINPUT::GAME_ONLY_INPUTS_START_HERE; i++)
	    {
	        if (i != CARINPUT::SHIFT_UP && i != CARINPUT::SHIFT_DOWN)
                carinputs[i] = 0.0f;
	    }

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
			carcontrols_local.second.ProcessInput(settings.GetJoyType(), eventsystem, car.GetLastSteer(), TickPeriod(),
					settings.GetJoy200(), car.GetSpeed(), settings.GetSpeedSensitivity(), graphics.GetW(), graphics.GetH(), settings.GetButtonRamp(), settings.GetHGateShifter());
		}

		stringstream debug_info1;
		car.DebugPrint(debug_info1, true, false, false, false);

		stringstream debug_info2;
		car.DebugPrint(debug_info2, false, true, false, false);

		stringstream debug_info3;
		car.DebugPrint(debug_info3, false, false, true, false);

		stringstream debug_info4;
		car.DebugPrint(debug_info4, false, false, false, true);

        std::pair <int, int> curplace = timer.GetPlayerPlace();
		hud.Update(fonts["lcd"], fonts["futuresans"], timer.GetPlayerTime(), timer.GetLastLap(),
			timer.GetBestLap(), timer.GetStagingTimeLeft(),
			timer.GetPlayerCurrentLap(), race_laps, curplace.first, curplace.second,
			car.GetClutch(), car.GetGear(), car.GetEngineRPM(),
			car.GetEngineRedline(), car.GetEngineRPMLimit(), car.GetSpeedometer(),
			settings.GetMPH(), debug_info1.str(), debug_info2.str(), debug_info3.str(),
			debug_info4.str(), graphics.GetW(), graphics.GetH(), car.GetABSEnabled(),
			car.GetABSActive(), car.GetTCSEnabled(), car.GetTCSActive(),
			timer.GetIsDrifting(cartimerids[&car]), timer.GetDriftScore(cartimerids[&car]),
			timer.GetThisDriftScore(cartimerids[&car]));

		//handle camera mode change inputs
		if (carcontrols_local.second.GetInput(CARINPUT::VIEW_HOOD))
		{
			active_camera = car.GetHoodCamera();
			settings.SetCameraMode("hood");
		}
		else if (carcontrols_local.second.GetInput(CARINPUT::VIEW_INCAR))
		{
			active_camera = car.GetDriverCamera();
			settings.SetCameraMode("incar");
		}
		else if (carcontrols_local.second.GetInput(CARINPUT::VIEW_FREE))
		{
			active_camera = &free_camera;
			free_camera.Reset(MATHVECTOR <float, 3> (car.GetCenterOfMassPosition()+MATHVECTOR <float, 3> (0,0,2)));
			free_camera.MoveForward(-2.0);
			free_camera.ResetRotation();
			settings.SetCameraMode("free");
		}
		else if (carcontrols_local.second.GetInput(CARINPUT::VIEW_ORBIT))
		{
			active_camera = car.GetOrbitCamera();
			settings.SetCameraMode("orbit");
		}
		else if (carcontrols_local.second.GetInput(CARINPUT::VIEW_CHASERIGID))
		{
			active_camera = car.GetRigidChaseCamera();
			settings.SetCameraMode("chaserigid");
		}
		else if (carcontrols_local.second.GetInput(CARINPUT::VIEW_CHASE))
		{
			active_camera = car.GetChaseCamera();
			settings.SetCameraMode("chase");
		}

		//handle camera inputs
		if (active_camera == &free_camera)
		{
			free_camera.RotateDown(-carcontrols_local.second.GetInput(CARINPUT::PAN_UP)*TickPeriod());
			free_camera.RotateDown(carcontrols_local.second.GetInput(CARINPUT::PAN_DOWN)*TickPeriod());
			free_camera.RotateRight(-carcontrols_local.second.GetInput(CARINPUT::PAN_LEFT)*TickPeriod());
			free_camera.RotateRight(carcontrols_local.second.GetInput(CARINPUT::PAN_RIGHT)*TickPeriod());
			free_camera.MoveForward(carcontrols_local.second.GetInput(CARINPUT::ZOOM_IN)*TickPeriod());
			free_camera.MoveForward(-carcontrols_local.second.GetInput(CARINPUT::ZOOM_OUT)*TickPeriod());
		}

		//determine whether or not we should use cockpit sounds
		bool cockpitsounds = (active_camera == car.GetDriverCamera() || active_camera == car.GetHoodCamera());
		{
			std::list <SOUNDSOURCE *> soundlist;
			car.GetEngineSoundList(soundlist);
			for (std::list <SOUNDSOURCE *>::iterator s = soundlist.begin(); s != soundlist.end(); s++)
			{
				(*s)->Set3DEffects(!cockpitsounds);
				//cout << "3d effects off" << endl;
			}
		}
		
		graphics.SetCloseShadow(cockpitsounds ? 1.0 : 5.0); //move up the close shadow distance if we're in the cockpit
	}
}

bool IsACar(const CAR * ptr, const list <CAR> & cars)
{
	bool match = false;
	for (list <CAR>::const_iterator i = cars.begin(); i != cars.end(); ++i)
		if (&(*i) == ptr)
			match = true;
	return match;
}

void GAME::UpdateCarChassisCollisions()
{
	//do car-to-car collisions
	std::map <COLLISION_OBJECT *, std::list <COLLISION_CONTACT> > cartocarcollisions;
	collision.CollideDynamicObjects(cartocarcollisions);
	//std::cout << "Dynamic colliding objects: " << cartocarcollisions.size() << endl;
	for (std::map <COLLISION_OBJECT *, std::list <COLLISION_CONTACT> >::iterator i = cartocarcollisions.begin(); i!= cartocarcollisions.end(); ++i)
	{
		std::list <COLLISION_CONTACT> & contactlist = i->second;
		//std::cout << "Number of collisions: " << contactlist.size() << endl;
		
		if (!contactlist.empty()) //don't think this should ever happen, but good to be safe
		{
			CAR * carptr = (CAR*)i->first->ObjID();
			assert(carptr);
			assert(IsACar(carptr, cars)); //make sure the car pointer is really a pointer to a car before we dereference it
			
			CAR & car = *carptr;
			
			contactlist.sort();
			COLLISION_CONTACT & contact = contactlist.back();
			
			CAR * othercarptr = (CAR*)contact.GetCollidingObject2()->ObjID();
			assert(othercarptr);
			assert(othercarptr != carptr);
			assert(IsACar(othercarptr, cars));
			CAR & othercar = *othercarptr;
			
			MATHVECTOR <float, 3> colcenter = car.GetCollisionDimensions().GetCenter();
			MATHVECTOR <float, 3> colpt ( contact.GetContactPosition() );
			MATHVECTOR <float, 3> normal ( contact.GetContactNormal() );
			float depth ( contact.GetContactDepth() );
			
			car.SetDynamicChassisContact(colpt, normal, car.GetVelocity() - othercar.GetVelocity(), depth*0.5, collision_rate);
			othercar.SetDynamicChassisContact(colpt, -normal, -car.GetVelocity() + othercar.GetVelocity(), depth*0.5, collision_rate);
		}
	}
	
	//do car-to-scenery collisions
	for (list <CAR>::iterator i = cars.begin(); i != cars.end(); ++i)
	{
		CAR & car = *i;
		
		int maxloops = 1;
		int loopcount = 0;
		bool col = true;
		while (col && loopcount < maxloops)
		{
			loopcount++;
			col = false;
	
			COLLISION_SETTINGS settings;
			settings.SetStaticCollide(true);
			list <COLLISION_CONTACT> contactlist;
			const AABB <float> & colbox = car.GetCollisionDimensions();
			MATHVECTOR <float, 3> colcenter = car.CarLocalToWorld(colbox.GetCenter());
			
			//collision.CollideBox(colcenter, car.GetOrientation(), colbox.GetSize()*0.5, contactlist, settings);
			//collision.CollideMovingBox(colcenter, car.GetVelocity(), car.GetOrientation(), colbox.GetSize()*0.5, contactlist, settings, collision_rate);
			collision.CollideObject(car.GetCollisionObject(), contactlist, settings);
			
			if (!contactlist.empty())
			{
				col = true;
				contactlist.sort();
				COLLISION_CONTACT & contact = contactlist.back();
	
				MATHVECTOR <float, 3> colpt ( contact.GetContactPosition() );
				MATHVECTOR <float, 3> normal ( contact.GetContactNormal() );
				float depth ( contact.GetContactDepth() );
	
				//normal = normal * -1.0;
	
				//cout << (colpt - colcenter).dot(normal) << endl;
	
				if ((colpt - colcenter).dot(normal) <= 0)
					car.SetChassisContact(colpt, normal, depth, collision_rate);
	
				//cout << "contact " << colpt << endl;
			}
		}
	}
}

void GAME::UpdateCarWheelCollisions(CAR & car, std::vector <COLLISION_CONTACT> & cached_collisions) const
{
	//do car wheel collisions
	for (int n = 0; n < WHEEL_POSITION_SIZE; n++)
	{
		MATHVECTOR <float, 3> wp = car.GetWheelPosition(WHEEL_POSITION(n));
			//MATHVECTOR <float, 3> wp = car.GetWheelPositionAtDisplacement(WHEEL_POSITION(n),1.0);
			//info_output << wp << "..." << cam_position << endl;
		MATHVECTOR <float, 3> dir = car.GetDownVector();
		MATHVECTOR <float, 3> raystart = wp;;
		float raylen = 10.0;
		float moveback = -car.GetWheelVelocity(WHEEL_POSITION(n))[2];
		if (moveback < 0)
			moveback = 0;
		raystart = raystart - dir*(car.GetTireRadius(WHEEL_POSITION(n))+moveback); //move back slightly

		//start by doing a road patch collision check
		MATHVECTOR <float, 3> bezierspace_raystart(raystart[1], raystart[2], raystart[0]);
		MATHVECTOR <float, 3> bezierspace_dir(dir[1], dir[2], dir[0]);
		MATHVECTOR <float, 3> colpoint, colnormal;
		const BEZIER * colpatch = NULL;
		bool col = track.CollideRoads(bezierspace_raystart, bezierspace_dir, raylen, colpoint, colpatch, colnormal);
		if (col)
		{
			COLLISION_CONTACT contact;
			contact.Set(MATHVECTOR <float, 3> (colpoint[2], colpoint[0], colpoint[1]), MATHVECTOR <float, 3> (colnormal[2], colnormal[0], colnormal[1]), (colpoint-bezierspace_raystart).Magnitude(), NULL, NULL);
			car.SetWheelContactProperties(WHEEL_POSITION(n), contact.GetContactDepth() - car.GetTireRadius(WHEEL_POSITION(n)) - moveback, contact.GetContactPosition(), contact.GetContactNormal(),1,0,1,0.9,1,0,SURFACE::ASPHALT);
			if (cached_collisions.size() != 4)
				cached_collisions.resize(4);
			cached_collisions[n] = contact;

			car.SetCurPatch(n, colpatch);
		}
		else
		{
			car.SetCurPatch(n, NULL); //this behavior is needed for the AI

			//do a track model collision check
			COLLISION_SETTINGS settings;
			settings.SetStaticCollide(true);
			list <COLLISION_CONTACT> contactlist;
			collision.CollideRay(raystart, dir, raylen, contactlist, settings);
			if (!contactlist.empty())
			{
				contactlist.sort();
				COLLISION_CONTACT & contact = contactlist.front();
				//MATHVECTOR <float, 3> cp = contact.GetContactPosition();
				//cout << "Model contact point = " << cp << endl;
				//cout << "Contact, Wheel penetration: " << cp[2] - wp[2] << " (" << cp[2] << " - " << wp[2] << "), " << contactlist.size() << "," << contact.GetContactDepth() << endl;
				const TRACK_OBJECT * const obj = reinterpret_cast <const TRACK_OBJECT * const> (contact.GetCollidingObject1()->ObjID());
				car.SetWheelContactProperties(WHEEL_POSITION(n), contact.GetContactDepth() - car.GetTireRadius(WHEEL_POSITION(n))-moveback, contact.GetContactPosition(), contact.GetContactNormal(),
						obj->GetBumpWavelength(), obj->GetBumpAmplitude(), obj->GetFrictionNoTread(), obj->GetFrictionTread(), obj->GetRollingResistanceCoefficient(), obj->GetRollingDrag(), obj->GetSurfaceType());
				if (cached_collisions.size() != 4)
					cached_collisions.resize(4);
				cached_collisions[n] = contact;
				//if (n == 0) cout << "setting cache to " << contact.GetContactDepth() << endl;
			}
			else //no collision
			{
				car.SetWheelContactProperties(WHEEL_POSITION(n), 100.0, MATHVECTOR <float, 3> (raystart+dir*100.0), MATHVECTOR <float, 3> (-dir),1,0,1,0.9,1,0,SURFACE::NONE);
				if (cached_collisions.size() != 4)
					cached_collisions.resize(4);
				cached_collisions[n].Set(MATHVECTOR <float, 3> (0), MATHVECTOR <float, 3> (0),
						-100, NULL, NULL);
			}
		}
	}
}

void GAME::UpdateCarWheelCollisionsWithSurfaces(CAR & car, std::vector <COLLISION_CONTACT> & cached_collisions) const
{
	//do car wheel collisions
	for (int n = 0; n < WHEEL_POSITION_SIZE; n++)
	{
		MATHVECTOR <float, 3> wp = car.GetWheelPosition(WHEEL_POSITION(n));
		//MATHVECTOR <float, 3> wp = car.GetWheelPositionAtDisplacement(WHEEL_POSITION(n),1.0);
		//info_output << wp << "..." << cam_position << endl;
		MATHVECTOR <float, 3> dir = car.GetDownVector();
		MATHVECTOR <float, 3> raystart = wp;;
		float raylen = 10.0;
		float moveback = -car.GetWheelVelocity(WHEEL_POSITION(n))[2];
		if (moveback < 0)
			moveback = 0;
		raystart = raystart - dir*(car.GetTireRadius(WHEEL_POSITION(n))+moveback); //move back slightly
		
		//start by doing a road patch collision check
		MATHVECTOR <float, 3> bezierspace_raystart(raystart[1], raystart[2], raystart[0]);
		MATHVECTOR <float, 3> bezierspace_dir(dir[1], dir[2], dir[0]);
		MATHVECTOR <float, 3> colpoint, colnormal;
		const BEZIER * colpatch = NULL;
		bool col = track.CollideRoads(bezierspace_raystart, bezierspace_dir, raylen, colpoint, colpatch, colnormal);
		if (col)
		{
			COLLISION_CONTACT contact;
			
			// put in a track model collision check to get the surface type
			COLLISION_SETTINGS settings;
			settings.SetStaticCollide(true);
			list <COLLISION_CONTACT> contactlist;
			collision.CollideRay(raystart, dir, raylen, contactlist, settings);
			
			if (contactlist.empty()) // prevent a crash if no object is found
			{
				error_output << "Got a Bezier contact but not a model" << endl;
				
				contact.Set(MATHVECTOR <float, 3> (colpoint[2], colpoint[0], colpoint[1]), MATHVECTOR <float, 3> (colnormal[2], colnormal[0], colnormal[1]), (colpoint-bezierspace_raystart).Magnitude(), NULL, NULL);
				car.SetWheelContactProperties(WHEEL_POSITION(n), contact.GetContactDepth() - car.GetTireRadius(WHEEL_POSITION(n)) - moveback, contact.GetContactPosition(), contact.GetContactNormal(),1,0,1,0.9,1,0,SURFACE::ASPHALT);
				if (cached_collisions.size() != 4)
					cached_collisions.resize(4);
				cached_collisions[n] = contact;
			}
			else
			{
				contactlist.sort();
				COLLISION_CONTACT & mcontact = contactlist.front();
				assert(mcontact.GetCollidingObject1() != NULL);
				const TRACK_OBJECT * const obj = reinterpret_cast <const TRACK_OBJECT * const> (mcontact.GetCollidingObject1()->ObjID()); // find the object
				// set the bezier contact to get position. Store the object for cached collisions
				contact.Set(MATHVECTOR <float, 3> (colpoint[2], colpoint[0], colpoint[1]), MATHVECTOR <float, 3> (colnormal[2], colnormal[0], colnormal[1]), (colpoint-bezierspace_raystart).Magnitude(), NULL, mcontact.GetCollidingObject1());
				
				// get the tracksurface data
				int i = obj->GetSurfaceInt();
				TRACKSURFACE tempsurf = track.GetTrackSurface(i);
				
				car.SetWheelContactProperties(WHEEL_POSITION(n), contact.GetContactDepth() - car.GetTireRadius(WHEEL_POSITION(n))-moveback, contact.GetContactPosition(), contact.GetContactNormal(),
											  tempsurf.bumpWaveLength, tempsurf.bumpAmplitude, tempsurf.frictionNonTread,
											  tempsurf.frictionTread, tempsurf.rollResistanceCoefficient, tempsurf.rollingDrag, obj->GetSurfaceType());
				
				if (cached_collisions.size() != 4)
					cached_collisions.resize(4);
				cached_collisions[n] = contact;
			}
			car.SetCurPatch(n, colpatch);
		}
		else
		{
			car.SetCurPatch(n, NULL); //this behavior is needed for the AI
			
			//do a track model collision check
			COLLISION_SETTINGS settings;
			settings.SetStaticCollide(true);
			list <COLLISION_CONTACT> contactlist;
			collision.CollideRay(raystart, dir, raylen, contactlist, settings);
			if (!contactlist.empty())
			{
				contactlist.sort();
				COLLISION_CONTACT & contact = contactlist.front();
				//MATHVECTOR <float, 3> cp = contact.GetContactPosition();
				//cout << "Model contact point = " << cp << endl;
				//cout << "Contact, Wheel penetration: " << cp[2] - wp[2] << " (" << cp[2] << " - " << wp[2] << "), " << contactlist.size() << "," << contact.GetContactDepth() << endl;
				const TRACK_OBJECT * const obj = reinterpret_cast <const TRACK_OBJECT * const> (contact.GetCollidingObject1()->ObjID());
				
				// get the tracksurface data
				int i = obj->GetSurfaceInt();
				TRACKSURFACE tempsurf = track.GetTrackSurface(i);
				
				car.SetWheelContactProperties(WHEEL_POSITION(n), contact.GetContactDepth() - car.GetTireRadius(WHEEL_POSITION(n))-moveback, contact.GetContactPosition(), contact.GetContactNormal(),
											  tempsurf.bumpWaveLength, tempsurf.bumpAmplitude, tempsurf.frictionNonTread,
											  tempsurf.frictionTread, tempsurf.rollResistanceCoefficient, tempsurf.rollingDrag, obj->GetSurfaceType());
				
				if (cached_collisions.size() != 4)
					cached_collisions.resize(4);
				cached_collisions[n] = contact;
				//if (n == 0) cout << "setting cache to " << contact.GetContactDepth() << endl;
			}
			else //no collision
			{
				car.SetWheelContactProperties(WHEEL_POSITION(n), 100.0, MATHVECTOR <float, 3> (raystart+dir*100.0), MATHVECTOR <float, 3> (-dir),1,0,1,0.9,1,0,SURFACE::NONE);
				if (cached_collisions.size() != 4)
					cached_collisions.resize(4);
				cached_collisions[n].Set(MATHVECTOR <float, 3> (0), MATHVECTOR <float, 3> (0),
										 -100, NULL, NULL);
			}
		}
	}
}

void GAME::UpdateCarWheelCollisionsFromCached(CAR & car, std::vector <COLLISION_CONTACT> & cached_collisions) const
{
	//do car wheel collisions
	for (int n = 0; n < WHEEL_POSITION_SIZE; n++)
	{
		MATHVECTOR <float, 3> wp = car.GetWheelPosition(WHEEL_POSITION(n));
		//MATHVECTOR <float, 3> wp = car.GetWheelPositionAtDisplacement(WHEEL_POSITION(n),1.0);
		//info_output << wp << "..." << cam_position << endl;
		MATHVECTOR <float, 3> dir = car.GetDownVector();
		MATHVECTOR <float, 3> raystart = wp;;
		float moveback = -car.GetWheelVelocity(WHEEL_POSITION(n))[2];
		if (moveback < 0)
			moveback = 0;
		raystart = raystart - dir*(car.GetTireRadius(WHEEL_POSITION(n))+moveback); //move back slightly

		assert(cached_collisions.size() == 4);
		if (cached_collisions[n].GetContactDepth() > 0) //if there was a contact from the last query
		{
			COLLISION_CONTACT contact;
			if (cached_collisions[n].CollideRay(raystart, dir, 10.0, contact)) //if the new ray query impacts the cached collision information
			{
				//if (n == 0) cout << "got cached value " << contact.GetContactDepth() << endl;
				if (contact.GetCollidingObject1() == NULL)
					car.SetWheelContactProperties(WHEEL_POSITION(n), contact.GetContactDepth() - car.GetTireRadius(WHEEL_POSITION(n))-moveback, contact.GetContactPosition(), contact.GetContactNormal(),1,0,1,0.9,1,0, SURFACE::ASPHALT);
				else
				{
					const TRACK_OBJECT * const obj = reinterpret_cast <const TRACK_OBJECT * const> (contact.GetCollidingObject1()->ObjID());
					car.SetWheelContactProperties(WHEEL_POSITION(n), contact.GetContactDepth() - car.GetTireRadius(WHEEL_POSITION(n))-moveback, contact.GetContactPosition(), contact.GetContactNormal(),
							obj->GetBumpWavelength(), obj->GetBumpAmplitude(), obj->GetFrictionNoTread(), obj->GetFrictionTread(), obj->GetRollingResistanceCoefficient(), obj->GetRollingDrag(), obj->GetSurfaceType());
				}
			}
			else //the new ray query doesn't impact the cached collision information
				car.SetWheelContactProperties(WHEEL_POSITION(n), 100.0, MATHVECTOR <float, 3> (raystart+dir*100.0), MATHVECTOR <float, 3> (-dir),1,0,1,0.9,1,0,SURFACE::NONE);
		}
	}
}

void GAME::UpdateCarWheelCollisionsFromCachedWithSurfaces(CAR & car, std::vector <COLLISION_CONTACT> & cached_collisions) const
{
	//do car wheel collisions
	for (int n = 0; n < WHEEL_POSITION_SIZE; n++)
	{
		MATHVECTOR <float, 3> wp = car.GetWheelPosition(WHEEL_POSITION(n));
		//MATHVECTOR <float, 3> wp = car.GetWheelPositionAtDisplacement(WHEEL_POSITION(n),1.0);
		//info_output << wp << "..." << cam_position << endl;
		MATHVECTOR <float, 3> dir = car.GetDownVector();
		MATHVECTOR <float, 3> raystart = wp;;
		float moveback = -car.GetWheelVelocity(WHEEL_POSITION(n))[2];
		if (moveback < 0)
			moveback = 0;
		raystart = raystart - dir*(car.GetTireRadius(WHEEL_POSITION(n))+moveback); //move back slightly
		
		assert(cached_collisions.size() == 4);
		if (cached_collisions[n].GetContactDepth() > 0) //if there was a contact from the last query
		{
			COLLISION_CONTACT contact;
			if (cached_collisions[n].CollideRay(raystart, dir, 10.0, contact)) //if the new ray query impacts the cached collision information
			{
				//if (n == 0) cout << "got cached value " << contact.GetContactDepth() << endl;
				if (contact.GetCollidingObject1() == NULL) // this means we hit a bezier
				{
					// check that the model is stored to save us from crashing out
					if (contact.GetCollidingObject2() == NULL)
					{
						error_output << "Colliding Object 2 is NULL" << endl;
						car.SetWheelContactProperties(WHEEL_POSITION(n), contact.GetContactDepth() - car.GetTireRadius(WHEEL_POSITION(n))-moveback, contact.GetContactPosition(), contact.GetContactNormal(),1,0,1,0.9,1,0, SURFACE::ASPHALT);
					}
					else
					{
						const TRACK_OBJECT * const obj = reinterpret_cast <const TRACK_OBJECT * const> (contact.GetCollidingObject2()->ObjID());
						
						// get the tracksurface data
						int i = obj->GetSurfaceInt();
						TRACKSURFACE tempsurf = track.GetTrackSurface(i);
						
						car.SetWheelContactProperties(WHEEL_POSITION(n), contact.GetContactDepth() - car.GetTireRadius(WHEEL_POSITION(n))-moveback, contact.GetContactPosition(), contact.GetContactNormal(),
													  tempsurf.bumpWaveLength, tempsurf.bumpAmplitude, tempsurf.frictionNonTread,
													  tempsurf.frictionTread, tempsurf.rollResistanceCoefficient, tempsurf.rollingDrag, obj->GetSurfaceType());
					}
				}
				else
				{
					const TRACK_OBJECT * const obj = reinterpret_cast <const TRACK_OBJECT * const> (contact.GetCollidingObject1()->ObjID());
					
					// get the tracksurface data
					int i = obj->GetSurfaceInt();
					TRACKSURFACE tempsurf = track.GetTrackSurface(i);
					
					car.SetWheelContactProperties(WHEEL_POSITION(n), contact.GetContactDepth() - car.GetTireRadius(WHEEL_POSITION(n))-moveback, contact.GetContactPosition(), contact.GetContactNormal(),
												  tempsurf.bumpWaveLength, tempsurf.bumpAmplitude, tempsurf.frictionNonTread,
												  tempsurf.frictionTread, tempsurf.rollResistanceCoefficient, tempsurf.rollingDrag, obj->GetSurfaceType());				}
			}
			else //the new ray query doesn't impact the cached collision information
				car.SetWheelContactProperties(WHEEL_POSITION(n), 100.0, MATHVECTOR <float, 3> (raystart+dir*100.0), MATHVECTOR <float, 3> (-dir),1,0,1,0.9,1,0,SURFACE::NONE);
		}
	}
}

///start a new game.  LeaveGame() is called first thing, which should take care of clearing out all current data.
bool GAME::NewGame(bool playreplay, bool addopponents, int num_laps)
{
	LeaveGame(); //this should clear out all data

	if (playreplay)
	{
		stringstream replayfilenamestream;

		if(benchmode)
			replayfilenamestream << pathmanager.GetReplayPath() << "/benchmark.vdr";
		else
			replayfilenamestream << pathmanager.GetReplayPath() << "/" << settings.GetSelectedReplay() << ".vdr";

		string replayfilename = replayfilenamestream.str();
		info_output << "Loading replay file " << replayfilename << endl;
		if (!replay.StartPlaying(replayfilename, error_output))
			return false;
	}

	//set the track name
	string trackname;
	if (playreplay)
	{
		trackname = replay.GetTrack();
	}
	else
		trackname = settings.GetTrack();

	if (!LoadTrack(trackname))
	{
		error_output << "Error during track loading: " << trackname << endl;
		return false;
	}

	//start out with no camera
	active_camera = NULL;

	//set the car name
	string carname;
	if (playreplay)
		carname = replay.GetCarType();
	else
		carname = settings.GetSelectedCar();

	//set the car paint
	string carpaint("00");
	if (playreplay)
		carpaint = replay.GetCarPaint();
	else
		carpaint = settings.GetCarPaint();

	//load the local player's car
	//cout << "About to load car..." << endl;
	if (playreplay)
	{
		if (!LoadCar(carname, carpaint, track.GetStart(0).first, track.GetStart(0).second, true, false, replay.GetCarFile()))
			return false;
	}
	else
	{
		//cout << "Not playing replay..." << endl;
		if (!LoadCar(carname, carpaint, track.GetStart(0).first, track.GetStart(0).second, true, false))
			return false;
		//cout << "Loaded car successfully" << endl;
	}
	//cout << "After load car: " << carcontrols_local.first << endl;

    race_laps = num_laps;

	//load AI cars
	if (addopponents)
	{
		int carcount = 1;
		for (std::vector <std::pair<std::string, std::string> >::iterator i = opponents.begin(); i != opponents.end(); ++i)
		{
			//int startplace = std::min(carcount, track.GetNumStartPositions()-1);
			int startplace = carcount;
			if (!LoadCar(i->first, i->second, track.GetStart(startplace).first, track.GetStart(startplace).second, false, true))
				return false;
			ai.add_car(&cars.back(), settings.GetAIDifficulty());
			carcount++;
		}
	}
	else
		opponents.clear();

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

	//trim out extra space between the current car positions and the ground
	if (!AlignCarsWithGround())
		return false;

	//set the camera position
	/*cam_position = track.GetStart(0).first;
	cam_position[2] += 5.0;*/
	//cam_rotation = track.GetStart(0).second;

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

		replay.StartRecording(carcontrols_local.first->GetCarType(), settings.GetCarPaint(), pathmanager.GetCarPath()+"/"+carcontrols_local.first->GetCarType()+"/"+carcontrols_local.first->GetCarType()+".car", settings.GetTrack(), error_output);
	}

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
		gui.ReplaceOptionMapValues("game.selected_replay", replaylist, error_output);
	}
	if (replay.GetPlaying())
		replay.StopPlaying();

	gui.SetInGame(false);
	track.Unload();
	if (tracknode)
	{
		tracknode->Clear();
		graphics.ClearStaticDrawlistMap();
	}
	collision.Clear();
	cached_collisions_by_car.clear();
	cur_collision_frameskip = 0;
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
	cars.clear();
	hud.Hide();
	inputgraph.Hide();
	trackmap.Unload();
	timer.Unload();
	active_camera = NULL;
	pause = false;
	tire_smoke.Clear();
}

///move the cars on the z axis until they are touching the ground; returns false if the car isn't near ground
bool GAME::AlignCarsWithGround()
{
	for (list <CAR>::iterator i = cars.begin(); i != cars.end(); ++i)
	{
		//find the precise starting position for the car (trim out the extra space)
		float lowest_point = 0;
		bool no_lowest_point_yet = true;
		for (int n = 0; n < WHEEL_POSITION_SIZE; n++)
		{
			MATHVECTOR <float, 3> wp = i->GetWheelPositionAtDisplacement(WHEEL_POSITION(n),0);
			//info_output << wp << "..." << cam_position << endl;
			MATHVECTOR <float, 3> dir;
			dir.Set(0,0,-1);
			MATHVECTOR <float, 3> raystart = wp;
			raystart = raystart - dir; //move back 1 meter
			COLLISION_SETTINGS settings;
			settings.SetStaticCollide(true);
			list <COLLISION_CONTACT> contactlist;
			collision.CollideRay(raystart, dir, 10.0, contactlist, settings); //TODO: check for road bezier collisions first
			if (!contactlist.empty())
			{
				contactlist.sort();
				COLLISION_CONTACT & contact = contactlist.front();
				//MATHVECTOR <float, 3> cp = contact.GetContactPosition();
				//cout << "Contact, Wheel penetration: " << cp[2] - wp[2] << " (" << cp[2] << " - " << wp[2] << "), " << contactlist.size() << "," << contact.GetContactDepth() << endl;
				float wheelheight = contact.GetContactDepth() - 1.0 - i->GetTireRadius(WHEEL_POSITION(n));
				//std::cout << wheelheight << std::endl;
				if (wheelheight < lowest_point || no_lowest_point_yet)
				{
					lowest_point = wheelheight;
					no_lowest_point_yet = false;
				}
			}
			else
			{
				error_output << "Car is starting too far away from the track or isn't over the track at all" << endl;
				return false;
			}
		}
		MATHVECTOR <float, 3> trimmed_position = i->GetCenterOfMassPosition();
		trimmed_position[2] -= lowest_point;
		//std::cout << "!!! trimming off " << lowest_point << std::endl;

		i->SetPosition(trimmed_position);
	}

	return true;
}

///add a car, optionally controlled by the local player
bool GAME::LoadCar(const std::string & carname, const std::string & carpaint, const MATHVECTOR <float, 3> & start_position,
		   const QUATERNION <float> & start_orientation, bool islocal, bool isai, const string & carfile)
{
	//std::cout << "Start position: " << start_position << endl;

	CONFIGFILE carconf;
	if (carfile.empty()) //if no file is passed in, then load it from disk
	{
		//cout << "Loading from disk" << endl;
		if ( !carconf.Load ( pathmanager.GetCarPath()+"/"+carname+"/"+carname+".car" ) )
			return false;
	}
	else
	{
		/*cout << "Loading from passed car" << endl;
		stringstream debugcar(carfile);
		ofstream f("debug.car");
		while (debugcar)
		{
			char tempchar[1024];
			debugcar.getline(tempchar,1024);
			f << tempchar << endl;
		}*/
		stringstream carstream(carfile);
		if ( !carconf.Load ( carstream ) )
			return false;
	}

	cars.push_back(CAR());

	if (!cars.back().Load(carconf, pathmanager.GetCarPath(), pathmanager.GetDriverPath()+"/driver2",
	     carname, carpaint, start_position,
	     start_orientation, rootnode, sound.Enabled(), sound.GetDeviceInfo(), generic_sounds,
	     settings.GetAnisotropy(), settings.GetABS() || isai, settings.GetTCS() || isai, settings.GetTextureSize(),
	     settings.GetCameraBounce(), debugmode, info_output, error_output))
	{
		error_output << "Error loading car: " << carname << endl;
		cars.pop_back();
		return false;
	}
	else
	{
		info_output << "Car loading was successful: " << carname << endl;

		if (islocal)
		{
			//load local controls
			carcontrols_local.first = &cars.back();

			//set the active camera
			active_camera = cars.back().GetChaseCamera();
			if (settings.GetCameraMode() == "hood")
			{
				//std::cout << "Hood camera mode" << std::endl;
				active_camera = cars.back().GetHoodCamera();
			}
			if (settings.GetCameraMode() == "incar")
				active_camera = cars.back().GetDriverCamera();
			if (settings.GetCameraMode() == "free")
				active_camera = &free_camera;
			if (settings.GetCameraMode() == "orbit")
				active_camera = cars.back().GetOrbitCamera();
			if (settings.GetCameraMode() == "chaserigid")
				active_camera = cars.back().GetRigidChaseCamera();
			if (settings.GetCameraMode() == "chase")
				active_camera = cars.back().GetChaseCamera();

			// setup auto clutch and auto shift
			ProcessNewSettings();
			// shift into first gear if autoshift enabled
			if (carcontrols_local.first && settings.GetAutoShift())
                carcontrols_local.first->SetGear(1);
		}
	}
	
	collision.AddPhysicsObject(cars.back().GetCollisionObject());

	return true;
}

bool GAME::LoadTrack(const std::string & trackname)
{
	//create or clear the track scenegraph node
	if (!tracknode)
		//tracknode = &rootnode.AddNode();
		tracknode = new SCENENODE();
	else
		tracknode->Clear();
	assert(tracknode);

	LoadingScreen(0.0,1.0);

	//load the track

	//info_output << "Prior to track load: ";
	//collision.DebugPrint(info_output);

	if (!track.DeferredLoad(pathmanager.GetTrackPath()+"/"+trackname, pathmanager.GetEffectsTexturePath(), *tracknode, settings.GetTrackReverse(), settings.GetAnisotropy(), settings.GetTextureSize(), graphics.GetShadows(), false, info_output, error_output))
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
			LoadingScreen(count, track.DeferredLoadTotalObjects());
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
	if (!trackmap.BuildMap(&rootnode, track.GetRoadList(), graphics.GetW(), graphics.GetH(), pathmanager.GetHUDTexturePath(), settings.GetTextureSize(), error_output))
	{
		error_output << "Error loading track map: " << trackname << endl;
		return false;
	}

	//send track collision objects to the collision subsystem
	list <COLLISION_OBJECT*> colobjlist;
	track.GetCollisionObjectsTo(colobjlist);
	for (list <COLLISION_OBJECT *>::iterator i = colobjlist.begin(); i != colobjlist.end(); ++i)
		collision.AddPhysicsObject(**i);

	//info_output << "After track load: ";
	collision.DebugPrint(info_output);
	
	assert(tracknode);
	CollapseSceneToDrawlistmap(*tracknode, graphics.GetStaticDrawlistmap(), true);
	graphics.OptimizeStaticDrawlistmap();

	return true;
}

bool GAME::LoadFonts()
{
	string fontbase = pathmanager.GetFontPath(settings.GetSkin());

	if (graphics.GetUsingShaders())
	{
		if (!fonts["freesans"].Load(fontbase+"/freesans.txt",fontbase+"/freesans.png", settings.GetTextureSize(), error_output)) return false;
		if (!fonts["lcd"].Load(fontbase+"/lcd.txt",fontbase+"/lcd.png", settings.GetTextureSize(), error_output)) return false;
		if (!fonts["futuresans"].Load(fontbase+"/futuresans.txt",fontbase+"/futuresans.png", settings.GetTextureSize(), error_output)) return false;
	}
	else
	{
		if (!fonts["freesans"].Load(fontbase+"/freesans.txt",fontbase+"/freesans_noshaders.png", settings.GetTextureSize(), error_output)) return false;
		if (!fonts["lcd"].Load(fontbase+"/lcd.txt",fontbase+"/lcd_noshaders.png", settings.GetTextureSize(), error_output)) return false;
		if (!fonts["futuresans"].Load(fontbase+"/futuresans.txt",fontbase+"/futuresans_noshaders.png", settings.GetTextureSize(), error_output)) return false;
	}

	info_output << "Loaded fonts successfully" << endl;

	return true;
}

void GAME::CalculateFPS()
{
	if (eventsystem.Get_dt() > 0)
		fps_track[fps_position] = 1.0/eventsystem.Get_dt();

	fps_position = (fps_position + 1) % 10;
	float fps_avg = 0.0;
	for (int i = 0; i < 10; i++)
		fps_avg += fps_track[i];
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

	assert(fps_draw);

	if (settings.GetShowFps())
	{
		float w = fps.GetWidth(fonts["futuresans"], "FPS: 100", 0.2);
		float screenhwratio = (float)graphics.GetH()/graphics.GetW();
		fps.Set(*fps_draw, fonts["futuresans"], fpsstr.str(), 0.5-w*0.5,1.0-0.02, screenhwratio*0.2,0.2, 1.0,1.0,1.0);
		fps_draw->SetDrawEnable(true);
	}
	else
		fps_draw->SetDrawEnable(false);
	
	if (profilingmode && frame % 10 == 0)
		//profiling_text.Revise(PROFILER.getAvgSummary(quickprof::PERCENT));
		profiling_text.Revise(PROFILER.getAvgSummary(quickprof::MICROSECONDS));

	//std::cout << cam_rotation.x() << "," << cam_rotation.y() << "," << cam_rotation.z() << "," << cam_rotation.w() << endl;
	//std::cout << cam_position[0] << "," << cam_position[1] << "," << cam_position[2] << endl;
}

bool SortStringPairBySecond (const pair<string,string> & first, const pair<string,string> & second)
{
	return first.second < second.second;
}

void GAME::PopulateReplayList(std::list <std::pair <std::string, std::string> > & replaylist)
{
	replaylist.clear();
	int numreplays = 0;
	std::list <std::string> replayfoldercontents;
	if (pathmanager.GetFolderIndex(pathmanager.GetReplayPath(),replayfoldercontents))
	{
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
		settings.SetSelectedReplay(1);
}

void GAME::PopulateCarPaintList(const std::string & carname, std::list <std::pair <std::string, std::string> > & carpaintlist)
{
	carpaintlist.clear();
	string cartexfolder = pathmanager.GetCarPath()+"/"+carname+"/textures";
	bool exists = true;
	int paintnum = 0;
	while (exists)
	{
		exists = false;

		stringstream paintstr;
		paintstr.width(2);
		paintstr.fill('0');
		paintstr << paintnum;

		std::string cartexfile = cartexfolder+"/body"+paintstr.str()+".png";
			//std::cout << cartexfile << std::endl;
		ifstream check(cartexfile.c_str());
		if (check)
		{
			exists = true;
			carpaintlist.push_back(pair<string,string>(paintstr.str(),paintstr.str()));
			paintnum++;
		}
	}
}

void GAME::PopulateValueLists(std::map<std::string, std::list <std::pair <std::string, std::string> > > & valuelists)
{
	//populate track list
	{
		list <pair<string,string> > tracklist;
		list <string> trackfolderlist;
		pathmanager.GetFolderIndex(pathmanager.GetTrackPath(),trackfolderlist);
		for (list <string>::iterator i = trackfolderlist.begin(); i != trackfolderlist.end(); ++i)
		{
			ifstream check((pathmanager.GetTrackPath() + "/" + *i + "/about.txt").c_str());
			if (check)
			{
				string displayname;
				getline(check, displayname);
				tracklist.push_back(pair<string,string>(*i,displayname));
			}
		}
		tracklist.sort(SortStringPairBySecond);
		valuelists["tracks"] = tracklist;
	}

	//populate car list
	{
		list <pair<string,string> > carlist;
		list <string> carfolderlist;
		pathmanager.GetFolderIndex(pathmanager.GetCarPath(),carfolderlist);
		for (list <string>::iterator i = carfolderlist.begin(); i != carfolderlist.end(); ++i)
		{
			ifstream check((pathmanager.GetCarPath() + "/" + *i + "/about.txt").c_str());
			if (check)
			{
				carlist.push_back(pair<string,string>(*i,*i));
			}
		}
		valuelists["cars"] = carlist;
	}

	//populate car paints
	{
		PopulateCarPaintList(settings.GetSelectedCar(), valuelists["car_paints"]);
		PopulateCarPaintList(settings.GetOpponentCar(), valuelists["opponent_car_paints"]);
	}

	//populate video mode list
	{
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
	}

	//populate anisotropy list
	int max_aniso = graphics.GetMaxAnisotropy();
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
	if (graphics.AntialiasingSupported())
	{
		valuelists["antialiasing"].push_back(pair<string,string>("2","2X"));
		valuelists["antialiasing"].push_back(pair<string,string>("4","4X"));
	}

	//populate replays list
	{
		PopulateReplayList(valuelists["replays"]);
	}

	//populate other lists
	valuelists["joy_indeces"].push_back(pair<string,string>("0","0"));
	valuelists["skins"].push_back(pair<string,string>("simple","simple"));
}

void GAME::LoadSaveOptions(OPTION_ACTION action, std::map<std::string, std::string> & options)
{
	if (action == LOAD) //load from the settings class to the options map
	{
		CONFIGFILE tempconfig;
		settings.Serialize(true, tempconfig);
		list <string> paramlistoutput;
		tempconfig.GetParamList(paramlistoutput);
		for (list <string>::iterator i = paramlistoutput.begin(); i != paramlistoutput.end(); ++i)
		{
			string val;
			tempconfig.GetParam(*i, val);
			options[*i] = val;
			//std::cout << "LOAD - PARAM: " << *i << " = " << val << endl;
		}
	}
	else //save from the options map to the settings class
	{
		CONFIGFILE tempconfig;
		for (map<string, string>::iterator i = options.begin(); i != options.end(); ++i)
		{
			tempconfig.SetParam(i->first, i->second);
			//std::cout << "SAVE - PARAM: " << i->first << " = " << i->second << endl;
		}
		settings.Serialize(false, tempconfig);

		//account for new settings
		ProcessNewSettings();
	}
}

///update the game with any new setting changes that have just been made
void GAME::ProcessNewSettings()
{
	/*std::map<std::string, std::list <std::pair <std::string, std::string> > > valuelists;
	PopulateValueLists(valuelists);
	for (std::map<std::string, std::list <std::pair <std::string, std::string> > >::iterator v = valuelists.begin(); v != valuelists.end(); v++)
	{
		gui.GetOptionMap()[v->first].ReplaceValues(valuelists[v->first]);
	}
	std::map<std::string, std::string> optionmap;
	LoadSaveOptions(LOAD, optionmap);
	gui.SyncOptions(true, optionmap, error_output);*/

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

///write the scenegraph to the output drawlist map
void GAME::CollapseSceneToDrawlistmap(SCENENODE & node, std::map < DRAWABLE_FILTER *, std::vector <SCENEDRAW> > & outputmap, bool clearfirst)
{
	if (clearfirst)
	{
		for (std::map <DRAWABLE_FILTER *, std::vector <SCENEDRAW> >::iterator mi = outputmap.begin(); mi != outputmap.end(); ++mi)
		{
			mi->second.resize(0);
		}
	}
	MATRIX4 <float> identity;
	node.GetCollapsedDrawList(outputmap, identity);

	//auto-sort a 2D list
	for (std::map <DRAWABLE_FILTER *, std::vector <SCENEDRAW> >::iterator mi = outputmap.begin(); mi != outputmap.end(); ++mi)
	{
		if (mi->first->Is2DOnlyFilter())
			std::sort(mi->second.begin(), mi->second.end());
	}
}

void GAME::LoadingScreen(float progress, float max)
{
	assert(max > 0);
	loadingscreen.Update(progress/max);

	CollapseSceneToDrawlistmap(loadingscreen_node, graphics.GetDrawlistmap(), true);

	graphics.SetupScene(45.0, 100.0, MATHVECTOR <float, 3> (), QUATERNION <float> ());

	graphics.BeginScene(error_output);
	graphics.DrawScene(error_output);
	graphics.EndScene(error_output);
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
		//float squeal = car.GetTireSquealAmount(WHEEL_POSITION(i), carphysics_rate);
		//const int extraticks = (int)((framerate+carphysics_rate*0.5)/carphysics_rate);
		float squeal = car.GetTireSquealAmount(WHEEL_POSITION(i));
		
		if (squeal > 0)
		{
			//unsigned int mininterval = 0.2/TickPeriod();
			//unsigned int maxinterval = 1.0/TickPeriod();
			unsigned int interval = 0.2/dt; //only spawn particles every so often
			if (particle_timer % interval == 0)
			{
				tire_smoke.AddParticle(car.GetWheelPosition(WHEEL_POSITION(i))-MATHVECTOR<float,3>(0,0,car.GetTireRadius(WHEEL_POSITION(i))),
					0.5, 0.5, 0.5, 0.5);
			}
		}
	}
}

void GAME::UpdateParticleSystems(float dt)
{
	if (track.Loaded())
	{
		if (active_camera)
		{
			QUATERNION <float> camlook;
			camlook.Rotate(3.141593*0.5,1,0,0);
			camlook.Rotate(-3.141593*0.5,0,0,1);
			QUATERNION <float> camorient = -(active_camera->GetOrientation()*camlook);

			tire_smoke.Update(dt, camorient);
		}
	}

	particle_timer++;
	particle_timer = particle_timer % (unsigned int)((1.0/TickPeriod()));
}

void GAME::UpdateDriftScore(CAR & car, double dt)
{
	bool is_drifting = false;
	bool spin_out = false;

	//make sure the car is not off track
	int wheel_count = 0;
	for (int i=0; i < 4; i++)
	{
		if ( car.GetCurPatch ( WHEEL_POSITION(i) ) ) wheel_count++;
	}

	bool on_track = ( wheel_count > 1 );

	//car's direction on the horizontal plane
	MATHVECTOR <float, 3> car_orientation(1,0,0);
	car.GetOrientation().RotateVector(car_orientation);
	car_orientation[2] = 0;

	//car's velocity on the horizontal plane
	MATHVECTOR <float, 3> car_velocity = car.GetVelocity();
	car_velocity[2] = 0;
	float car_speed = car_velocity.Magnitude();

	//angle between car's direction and velocity
	float car_angle = 0;
	float mag = car_orientation.Magnitude() * car_velocity.Magnitude();
	if (mag > 0.001)
	{
		float dotprod = car_orientation.dot ( car_velocity )/mag;
		if (dotprod > 1.0)
			dotprod = 1.0;
		if (dotprod < -1.0)
			dotprod = -1.0;
		car_angle = acos(dotprod);
	}
	
	assert(car_angle == car_angle); //assert that car_angle isn't NAN
	assert(cartimerids.find(&car) != cartimerids.end()); //assert that the car is registered with the timer system

	if ( on_track )
	{
		//velocity must be above 10 m/s
		if ( car_speed > 10 )
		{
			//drift starts when the angle > 0.2 (around 11.5 degrees)
			//drift ends when the angle < 0.1 (aournd 5.7 degrees)
			float angle_threshold(0.2);
			if ( timer.GetIsDrifting(cartimerids[&car]) ) angle_threshold = 0.1;

			is_drifting = ( car_angle > angle_threshold && car_angle <= M_PI/2.0 );
			spin_out = ( car_angle > M_PI/2.0 );
		}
	}

	//calculate score
	if ( is_drifting )
	{
		//base score is the drift distance
		timer.IncrementThisDriftScore(cartimerids[&car], dt * car_speed);

		//bonus score calculation is now done in TIMER
		timer.UpdateMaxDriftAngleSpeed(cartimerids[&car], car_angle, car_speed);
		
		//std::cout << timer.GetDriftScore(cartimerids[&car]) << " + " << timer.GetThisDriftScore(cartimerids[&car]) << endl;
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

