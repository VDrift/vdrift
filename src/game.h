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

#ifndef _GAME_H
#define _GAME_H

#include "window.h"
#include "graphics/graphics.h"
#include "graphics/gl3v/stringidmap.h"
#include "eventsystem.h"
#include "settings.h"
#include "pathmanager.h"
#include "track.h"
#include "mathvector.h"
#include "quaternion.h"
#include "http.h"
#include "gui/gui.h"
#include "gui/text_draw.h"
#include "gui/font.h"
#include "physics/dynamicsworld.h"
#include "physics/cardynamics.h"
#include "dynamicsdraw.h"
#include "carcontrolmap.h"
#include "cargraphics.h"
#include "carsound.h"
#include "carinfo.h"
#include "sound/sound.h"
#include "camera.h"
#include "camera_free.h"
#include "trackmap.h"
#include "timer.h"
#include "replay.h"
#include "forcefeedback.h"
#include "particle.h"
#include "skidmarks.h"
#include "ai/ai.h"
#include "content/contentmanager.h"
#include "updatemanager.h"
#include "game_downloader.h"

#include "BulletCollision/CollisionDispatch/btDefaultCollisionConfiguration.h"
#include "BulletCollision/BroadphaseCollision/btDbvtBroadphase.h"
#include "BulletDynamics/ConstraintSolver/btSequentialImpulseConstraintSolver.h"

#include <iosfwd>
#include <string>
#include <list>
#include <map>
#include <vector>
#include <memory>

class Game
{
friend class GameDownloader;
public:
	Game(std::ostream & info_out, std::ostream & error_out);

	~Game();

	void Start(std::list <std::string> & args);

private:
	void End();

	/// Main game loop
	void Run();

	/// Main loop body
	void Advance();

	bool ParseArguments(std::list <std::string> & args);

	bool InitCoreSubsystems();

	void InitThreading();

	void InitPlayerCar();

	bool InitSound();

	bool InitGUI();

	void Test();

	void Tick(float dt);

	void Draw();

	void AdvanceGameLogic();

	void UpdateCars(float dt);

	void ProcessCarInputs();

	/// Updates camera, call after physics update
	void ProcessCameraInputs();

	void UpdateHUD(const size_t carid, const std::vector<float> & carinputs);

	void UpdateTimer();

	/// Check eventsystem state and update GUI
	void ProcessGUIInputs();

	void ProcessGameInputs();

	void UpdateStartList();

	void UpdateCarPosList();

	void UpdateCarInfo();

	void UpdateCarSpecs();

	void UpdateCarSpecList(GuiOption::List & speclist);

	bool NewGame(bool playreplay=false, bool opponents=false, int num_laps=0);

	bool LoadCar(
		const CarInfo & carinfo,
		const Vec3 & position,
		const Quat & orientation,
		const bool sound_enabled);

	bool LoadTrack(const std::string & trackname);

	void LoadGarage();

	void SetGarageCar();

	void SetCarColor();

	bool LoadFonts();

	void CalculateFPS();

	void PopulateValueLists(std::map<std::string, GuiOption::List> & valuelists);

	void PopulateTrackList(GuiOption::List & tracklist);

	void PopulateCarList(GuiOption::List & carlist);

	void PopulateCarVariantList(const std::string & carname, GuiOption::List & variants);

	void PopulateCarPaintList(const std::string & carname, GuiOption::List & paints);

	void PopulateCarTireList(const std::string & carname, GuiOption::List & tires);

	void PopulateCarWheelList(const std::string & carname, GuiOption::List & wheels);

	void PopulateCarSpecList(GuiOption::List & speclist);

	void PopulateDriverList(GuiOption::List & driverlist);

	void PopulateStartList(GuiOption::List & startlist);

	void PopulateReplayList(GuiOption::List & replaylist);

	void PopulateGUISkinList(GuiOption::List & skinlist);

	void PopulateGUILangList(GuiOption::List & langlist);

	void PopulateAnisoList(GuiOption::List & anisolist);

	void PopulateAntialiasList(GuiOption::List & antialiaslist);

	void UpdateTrackMap();

	void ShowLoadingScreen(float progress, float progress_max, const std::string & optional_text);

	void ProcessNewSettings();

	/// Look for keyboard, mouse, joystick input, assign local car controls.
	bool AssignControl();

	void LoadControlsIntoGUIPage(const std::string & page_name);

	void LoadControlsIntoGUI();

	void UpdateForceFeedback(float dt);

	void UpdateSkidMarks(const int carid);

	void AddTireSmokeParticles(const CarDynamics & car, float dt);

	void UpdateParticles(float dt);

	void UpdateParticleGraphics();

	void UpdateDriftScore(const int carid, const float dt);

	std::string GetReplayRecordingFilename();

	void Draw(float dt);

	void BeginStartingUp();

	void DoneStartingUp();

	bool LastStartWasSuccessful() const;

	bool Download(const std::string & file);

	bool Download(const std::vector <std::string> & files);

	// game actions
	void QuitGame();
	void LeaveGame();
	void StartRace();
	void PauseGame();
	void ContinueGame();
	void RestartGame();
	void StartReplay();
	void StartCheckForUpdates();
	void StartCarManager();
	void CarManagerNext();
	void CarManagerPrev();
	void ApplyCarUpdate();
	void StartTrackManager();
	void TrackManagerNext();
	void TrackManagerPrev();
	void ApplyTrackUpdate();
	void CancelDownload();
	void ActivateEditControlPage();
	void CancelControl();
	void DeleteControl();
	void SetButtonControl();
	void SetAnalogControl();
	void LoadControls();
	void SaveControls();
	void SyncOptions();
	void SyncSettings();
	void SelectPlayerCar();

	void SetCarToEdit(const std::string & value);
	void SetCarStartPos(const std::string & value);
	void SetCarName(const std::string & value);
	void SetCarVariant(const std::string & value);
	void SetCarPaint(const std::string & value);
	void SetCarTire(const std::string & value);
	void SetCarWheel(const std::string & value);
	void SetCarColor(const std::string & value);
	void SetCarColorHue(const std::string & value);
	void SetCarColorSat(const std::string & value);
	void SetCarColorVal(const std::string & value);
	void SetCarDriver(const std::string & value);
	void SetCarAILevel(const std::string & value);
	void SetCarsNum(const std::string & value);
	void SetControl(const std::string & value);

	void BindActions();
	void InitActionMap(std::map<std::string, Delegated<>> & actionmap);
	void InitSignalMap(std::map<std::string, Signald<const std::string &>*> & signalmap);

	enum GameSignal {
		// game info signals
		LOADING,
		FPS,
		// hud info signals
		DEBUG0,
		DEBUG1,
		DEBUG2,
		DEBUG3,
		MSG,
		TIME0,
		TIME1,
		TIME2,
		LAP,
		POS,
		SCORE,
		STEER,
		ACCEL,
		BRAKE,
		GEAR,
		SHIFT,
		SPEEDO,
		SPEEDN,
		SPEED,
		TACHO,
		RPMN,
		RPMR,
		RPM,
		ABS,
		TCS,
		GAS,
		NOS,
		SIGNALNUM
	};
	Signald<const std::string &> signals[SIGNALNUM];

	std::ostream & info_output;
	std::ostream & error_output;

	unsigned int frame; ///< physics frame counter
	unsigned int displayframe; ///< display frame counter
	double clocktime; ///< elapsed wall clock time
	double target_time;
	const float timestep; ///< simulation time step

	PathManager pathmanager;
	Settings settings;
	Window window;
	Graphics * graphics;
	StringIdMap stringMap;
	EventSystem eventsystem;
	ContentManager content;
	Sound sound;
	AutoUpdate autoupdate;
	UpdateManager carupdater;
	UpdateManager trackupdater;
	std::map <std::string, Font> fonts;
	std::string renderconfigfile;

	std::vector <float> fps_track;
	int fps_position;
	float fps_min;
	float fps_max;

	bool multithreaded;
	bool profilingmode;
	bool benchmode;
	bool dumpfps;
	bool pause;

	std::vector <EventSystem::Joystick> controlgrab_joystick_state;
	std::pair <int,int> controlgrab_mouse_coords;
	CarControlMap::Control controlgrab_control;
	std::string controlgrab_page;
	std::string controlgrab_input;
	size_t controlgrab_id;
	bool controlgrab;

	CameraFree garage_camera;
	Camera * active_camera;

	CarControlMap car_controls_local;
	btAlignedObjectArray <CarDynamics> car_dynamics;
	std::vector <CarGraphics> car_graphics;
	std::vector <CarSound> car_sounds;
	std::vector <CarInfo> car_info;
	size_t player_car_id;
	size_t camera_car_id;
	size_t car_edit_id;
	int race_laps;
	bool practice;

	btDefaultCollisionConfiguration collisionconfig;
	btCollisionDispatcher collisiondispatch;
	btDbvtBroadphase collisionbroadphase;
	btSequentialImpulseConstraintSolver collisionsolver;
	DynamicsDraw dynamicsdraw;
	DynamicsWorld dynamics;
	int dynamics_drawmode;

	SkidMarks skid_marks;
	ParticleSystem tire_smoke;
	unsigned int particle_timer;

	TrackMap trackmap;
	Track track;
	Gui gui;
	Timer timer;
	Replay replay;
	Ai ai;
	Http http;

	std::unique_ptr <ForceFeedback> forcefeedback;
	float ff_update_time;
};

#endif
