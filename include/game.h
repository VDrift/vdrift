#ifndef _GAME_H
#define _GAME_H

#include <ostream>
#include <string>
#include <list>
#include <map>
#include <vector>
#include <memory>

#include "graphics.h"
#include "eventsystem.h"
#include "settings.h"
#include "pathmanager.h"
#include "track.h"
#include "mathvector.h"
#include "quaternion.h"
#include "font.h"
#include "text_draw.h"
#include "gui.h"
#include "car.h"
#include "collision_world.h"
#include "collision_contact.h"
#include "carcontrolmap_local.h"
#include "hud.h"
#include "inputgraph.h"
#include "sound.h"
#include "camera.h"
#include "trackmap.h"
#include "loadingscreen.h"
#include "timer.h"
#include "replay.h"
#include "forcefeedback.h"
#include "particle.h"
#include "ai.h"
#include "quickmp.h"
#include "sky.h"
#include "texturemanager.h"

class GAME
{
private:
	float TickPeriod() const {return framerate;}
	void MainLoop();
	bool ParseArguments(std::list <std::string> & args);
	void InitializeCoreSubsystems();
	void InitializeThreading();
	bool InitializeSound();
	bool InitializeGUI();
	void End();
	void Test();
	void Tick(float dt);
	void Draw();
	void AdvanceGameLogic();
	void UpdateCar(CAR & car, double dt);
	void UpdateDriftScore(CAR & car, double dt);
	void UpdateCarInputs(CAR & car);
	void UpdateTimer();
	void ProcessGUIInputs();
	void ProcessGameInputs();
	void ProcessGUIAction(const std::string & action);
	bool NewGame(bool playreplay=false, bool opponents=false, int num_laps=0);
	void LeaveGame();
	bool LoadTrack(const std::string & trackname);
	///< carfile is a string containing an entire .car file (e.g. XS.car) and is used instead of reading from disk.  this is optional
	bool LoadCar(
		const std::string & carname, const std::string & carpaint, const MATHVECTOR <float, 3> & carcolor,
		const MATHVECTOR <float, 3> & start_position, const QUATERNION <float> & start_orientation,
		bool islocal, bool isai, const std::string & carfile=""); 
	bool LoadFonts();
	void CalculateFPS();
	void PopulateValueLists(std::map<std::string, std::list <std::pair<std::string,std::string> > > & valuelists);
	void PopulateReplayList(std::list <std::pair <std::string, std::string> > & replaylist);
	void PopulateCarPaintList(const std::string & carname, std::list <std::pair <std::string, std::string> > & carpaintlist);
	enum OPTION_ACTION
	{
		SAVE,
		LOAD
	};
	void LoadSaveOptions(OPTION_ACTION action, std::map<std::string, std::string> & options);
	void UpdateTrackMap();
	void LoadingScreen(float progress, float max);
	void ProcessNewSettings();
	void RedisplayControlPage();
	void LoadControlsIntoGUIPage(const std::string & pagename);
	void UpdateForceFeedback(float dt);
	void UpdateParticleSystems(float dt);
	void AddTireSmokeParticles(float dt, CAR & car);
	std::string GetReplayRecordingFilename();
	void ParallelUpdate(int carindex);
	void BeginDraw();
	void FinishDraw();
	void BeginStartingUp();
	void DoneStartingUp();
	bool LastStartWasSuccessful() const;

	std::ostream & info_output;
	std::ostream & error_output;
	unsigned int frame; ///< physics frame counter
	unsigned int displayframe; ///< display frame counter
	double clocktime; ///< elapsed wall clock time
	double target_time;
	const float framerate;
	
	SCENENODE debugnode;
	TEXT_DRAWABLE fps_draw;
	TEXT_DRAWABLE profiling_text;
	
	std::vector <float> fps_track;
	int fps_position;
	float fps_min;
	float fps_max;
	bool multithreaded;
	bool benchmode;
	bool dumpfps;
	CAMERA * active_camera;
	bool pause;
	unsigned int particle_timer;
	std::vector <std::string> opponents;
	std::vector <std::string> opponents_paint;
	std::vector < MATHVECTOR <float, 3> > opponents_color;
	int race_laps;
	bool debugmode;
	bool profilingmode;

	std::string controlgrab_page;
	std::string controlgrab_input;
	bool controlgrab_analog;
	bool controlgrab_only_one;
	std::pair <int,int> controlgrab_mouse_coords;
	CARCONTROLMAP_LOCAL::CONTROL controlgrab_editcontrol;
	std::vector <EVENTSYSTEM_SDL::JOYSTICK> controlgrab_joystick_state;

	GRAPHICS_SDLGL graphics;
	EVENTSYSTEM_SDL eventsystem;
	SOUND sound;
	SOUNDBUFFERLIBRARY generic_sounds;
	SETTINGS settings;
	PATHMANAGER pathmanager;
	TEXTUREMANAGER textures;
	TRACKMAP trackmap;
	TRACK track;
	GUI gui;
	std::map <std::string, FONT> fonts;
	std::list <CAR> cars;
	std::map <CAR *, int> cartimerids;
	std::pair <CAR *, CARCONTROLMAP_LOCAL> carcontrols_local;
	COLLISION_WORLD collision;
	
	HUD hud;
	INPUTGRAPH inputgraph;
	LOADINGSCREEN loadingscreen;
	TIMER timer;
	REPLAY replay;
	PARTICLE_SYSTEM tire_smoke;
	AI ai;
	
	SKY sky;

#ifdef ENABLE_FORCE_FEEDBACK
	std::auto_ptr <FORCEFEEDBACK> forcefeedback;
	double ff_update_time;
#endif

public:
	GAME(std::ostream & info_out, std::ostream & err_out)
	:	info_output(info_out),
		error_output(err_out),
		frame(0),
		displayframe(0),
		clocktime(0),
		target_time(0),
		framerate(0.01),
		fps_track(10,0),
		fps_position(0),
		fps_min(0),
		fps_max(0),
		multithreaded(false),
		benchmode(false),
		dumpfps(false),
		active_camera(NULL),
		pause(false),
		particle_timer(0),
		race_laps(0),
		debugmode(false),
		profilingmode(false),
		textures(err_out),
		track(info_out, err_out),
		replay(framerate),
		sky(graphics, info_out, err_out)
	{
		carcontrols_local.first = NULL;
	}

	void Start(std::list <std::string> & args);
};

#endif
