#ifndef _GAME_H
#define _GAME_H

#include "window.h"
#include "graphics_interface.h"
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
#include "dynamicsworld.h"
#include "dynamicsdraw.h"
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
#include "contentmanager.h"
#include "http.h"
#include "gl3v/stringidmap.h"
#include "updatemanagement.h"
#include "game_downloader.h"

#include <ostream>
#include <string>
#include <list>
#include <map>
#include <vector>
#include <memory>

class GAME
{
friend class GAME_DOWNLOADER;
public:
	GAME(std::ostream & info_out, std::ostream & error_out);
	~GAME();
	void Start(std::list <std::string> & args);

private:
	void End();
	float TickPeriod() const {return timestep;}
	void MainLoop();
	bool ParseArguments(std::list <std::string> & args);
	void InitCoreSubsystems();
	void InitThreading();
	bool InitSound();
	bool InitGUI();
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
	void UpdateStartList(unsigned i, const std::string & value);
	void PlayerCarChange();
	void PlayerPaintChange();
	void PlayerColorChange();
	void OpponentCarChange();
	void OpponentPaintChange();
	void OpponentColorChange();
	void OpponentsChange();
	void AddControl(const std::string & controlstr);
	void EditControl(const std::string & controlstr);
	void SetButtonControl();
	void SetAnalogControl();
	bool NewGame(bool playreplay=false, bool opponents=false, int num_laps=0);
	void LeaveGame();
	bool LoadTrack(const std::string & trackname);
	///< carfile is a string containing an entire .car file (e.g. XS.car) and is used instead of reading from disk.  this is optional
	bool LoadCar(
		const std::string & carname, const std::string & carpaint, unsigned carcolor,
		const MATHVECTOR <float, 3> & start_position, const QUATERNION <float> & start_orientation,
		bool islocal, bool isai, const std::string & carfile="");
	bool LoadFonts();
	void CalculateFPS();
	void PopulateValueLists(std::map<std::string, std::list <std::pair<std::string,std::string> > > & valuelists);
	void PopulateReplayList(std::list <std::pair <std::string, std::string> > & replaylist);
	void PopulateCarPaintList(const std::string & carname, std::list <std::pair <std::string, std::string> > & carpaintlist);
	void UpdateTrackMap();
	void ShowHUD(bool value);
	void LoadingScreen(float progress, float max, bool drawGui, const std::string & optionalText, float x, float y);
	void ProcessNewSettings();
	bool AssignControls();
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
	bool Download(const std::string & file);
	bool Download(const std::vector <std::string> & files);

	std::ostream & info_output;
	std::ostream & error_output;
	unsigned int frame; ///< physics frame counter
	unsigned int displayframe; ///< display frame counter
	double clocktime; ///< elapsed wall clock time
	double target_time;
	const float timestep; ///< simulation time step

	PATHMANAGER pathmanager;
	SETTINGS settings;
	WINDOW_SDL window;
	GRAPHICS_INTERFACE * graphics_interface;
	bool enableGL3;
	bool usingGL3;
	StringIdMap stringMap;
	EVENTSYSTEM_SDL eventsystem;
	ContentManager content;
	SOUND sound;
	AUTOUPDATE autoupdate;
	UPDATE_MANAGER carupdater;
	UPDATE_MANAGER trackupdater;

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
	std::vector <std::string> cars_name;
	std::vector <std::string> cars_paint;
	std::vector <unsigned> cars_color;
	int race_laps;
	bool debugmode;
	bool profilingmode;
	std::string renderconfigfile;

	std::string controlgrab_page;
	std::string controlgrab_input;
	bool controlgrab_analog;
	bool controlgrab_only_one;
	std::pair <int,int> controlgrab_mouse_coords;
	CARCONTROLMAP_LOCAL::CONTROL controlgrab_editcontrol;
	std::vector <EVENTSYSTEM_SDL::JOYSTICK> controlgrab_joystick_state;

	std::map <std::string, FONT> fonts;
	std::list <CAR> cars;
	std::map <CAR *, int> cartimerids;
	std::pair <CAR *, CARCONTROLMAP_LOCAL> carcontrols_local;

	btDefaultCollisionConfiguration collisionconfig;
	btCollisionDispatcher collisiondispatch;
	btDbvtBroadphase collisionbroadphase;
	btSequentialImpulseConstraintSolver collisionsolver;
	DynamicsDraw dynamicsdraw;
	DynamicsWorld dynamics;
	int dynamics_drawmode;

	TRACKMAP trackmap;
	TRACK track;
	GUI gui;
	HUD hud;
	INPUTGRAPH inputgraph;
	LOADINGSCREEN loadingscreen;
	TIMER timer;
	REPLAY replay;
	PARTICLE_SYSTEM tire_smoke;
	AI ai;
	HTTP http;

#ifdef ENABLE_FORCE_FEEDBACK
	std::auto_ptr <FORCEFEEDBACK> forcefeedback;
	double ff_update_time;
#endif
};

#endif
