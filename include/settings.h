#ifndef _SETTINGS_H
#define _SETTINGS_H

#include <string>

#include "configfile.h"

class SETTINGS
{
private:
	bool res_override;
	
	int resolution_x;
	int resolution_y;
	int bpp;
	int depthbpp;
	bool fullscreen;
	bool shaders;
	std::string skin;
	bool show_fps;
	float mastervolume;
	bool mph; //if false, KPH
	std::string selected_car;
	std::string track;
	int antialiasing; //0 or 1 mean off
	int anisotropic; //0 or 1 mean off
	bool trackmap;
	bool show_hud;
	float FOV;
	bool abs;
	bool tcs;
	std::string joytype;
	bool joy200;
	float speed_sensitivity;
	bool joystick_calibrated;
	float view_distance;
	bool autoclutch;
	bool autoshift;
	bool racingline;
	bool mousegrab;
	bool recordreplay;
	int selected_replay;
	std::string carpaint;
	std::string texturesize;
	float button_ramp;
	std::string ff_device;
	float ff_gain;
	bool ff_invert;
	bool trackreverse;
	bool shadows;
	int shadow_distance;
	int shadow_quality;
	int reflections;
	bool input_graph;
	int lighting;
	bool bloom;
	std::string opponent_car;
	std::string opponent_car_paint;
	float camerabounce;
	int number_of_laps;
	float contrast;
	std::string camera_mode;
	bool hgateshifter;
	float ai_difficulty;

	//void LoadDefaults();

	template <typename T>
	bool Param(CONFIGFILE & conf, bool write, std::string pname, T & value)
	{
		if (write)
		{
			conf.SetParam(pname, value);
			return true;
		}
		else
			return conf.GetParam(pname, value);
	}

public:
	SETTINGS() : res_override(false),
		resolution_x(800),resolution_y(600),bpp(16),depthbpp(24),fullscreen(false),
		shaders(true),skin("simple"),show_fps(false),mastervolume(1.0),mph(true),
		selected_car("XS"), track("paulricard88"), antialiasing(0), anisotropic(0),
		trackmap(true), show_hud(true), FOV(45.0), abs(true), tcs(true), joytype("joystick"),
		joy200(false), speed_sensitivity(1.0), joystick_calibrated(false), view_distance(1000.0),
		autoclutch(true), autoshift(true), racingline(false), mousegrab(true),
		recordreplay(false), selected_replay(0), carpaint("00"), texturesize("large"),
		button_ramp(5), ff_device("/dev/input/event0"),
		ff_gain(2.0), ff_invert(false), trackreverse(false), shadows(false),
		shadow_distance(1), shadow_quality(1),
		reflections(1), input_graph(false), lighting(0), bloom(false),
		opponent_car("XS"), opponent_car_paint("00"), camerabounce(1.0),
		number_of_laps(1), contrast(1.0), camera_mode("chase"), hgateshifter(false),
		ai_difficulty(1.0)
		{}

	void Serialize(bool write, CONFIGFILE & config);
	void Load(std::string settingsfile) {CONFIGFILE config;config.Load(settingsfile);Serialize(false, config);}
	void Save(std::string settingsfile) {CONFIGFILE config;config.Load(settingsfile);Serialize(true, config);config.Write();}

	void SetResolution_x ( unsigned int theValue )
	{
		resolution_x = theValue;
	}

	unsigned int GetResolution_x() const
	{
		return resolution_x;
	}

	void SetBpp ( unsigned int theValue )
	{
		bpp = theValue;
	}

	unsigned int GetBpp() const
	{
		return bpp;
	}

	void SetDepthbpp ( unsigned int theValue )
	{
		depthbpp = theValue;
	}

	unsigned int GetDepthbpp() const
	{
		return depthbpp;
	}

	void SetFullscreen ( bool theValue )
	{
		fullscreen = theValue;
	}

	bool GetFullscreen() const
	{
		return fullscreen;
	}

	void SetResolution_y ( unsigned int value )
	{
		resolution_y = value;
	}

	unsigned int GetResolution_y() const
	{
		return resolution_y;
	}

	void SetShaders ( bool value )
	{
		shaders = value;
	}

	bool GetShaders() const
	{
		return shaders;
	}

	void SetSkin ( const std::string& value )
	{
		skin = value;
	}

	std::string GetSkin() const
	{
		return skin;
	}

	void SetShowFps ( bool value )
	{
		show_fps = value;
	}

	bool GetShowFps() const
	{
		return show_fps;
	}

	void SetMasterVolume ( float value )
	{
		mastervolume = value;
	}

	float GetMasterVolume() const
	{
		return mastervolume;
	}

	void SetMPH ( bool value )
	{
		mph = value;
	}

	bool GetMPH() const
	{
		return mph;
	}

	std::string GetSelectedCar() const
	{
		return selected_car;
	}

	std::string GetTrack() const
	{
		return track;
	}

	///returns 0 or 1 for off
	int GetAntialiasing() const
	{
		return antialiasing;
	}

	///return 0 or 1 for off
	int GetAnisotropic() const
	{
		return anisotropic;
	}

	bool GetTrackmap() const
	{
		return trackmap;
	}

	bool GetShowHUD() const
	{
		return show_hud;
	}

	int GetAnisotropy() const
	{
		return anisotropic;
	}

	float GetFOV() const
	{
		return FOV;
	}

	bool GetABS() const
	{
		return abs;
	}

	bool GetTCS() const
	{
		return tcs;
	}

	std::string GetJoyType() const
	{
		return joytype;
	}

	bool GetJoy200() const
	{
		return joy200;
	}

	float GetSpeedSensitivity() const
	{
		return speed_sensitivity;
	}

	void SetJoystickCalibrated ( bool value )
	{
		joystick_calibrated = value;
	}

	bool GetJoystickCalibrated() const
	{
		return joystick_calibrated;
	}

	float GetViewDistance() const
	{
		return view_distance;
	}

	bool GetAutoClutch() const
	{
		return autoclutch;
	}

	bool GetAutoShift() const
	{
		return autoshift;
	}

	bool GetRacingline() const
	{
		return racingline;
	}

	bool GetMouseGrab() const
	{
		return mousegrab;
	}

	bool GetRecordReplay() const
	{
		return recordreplay;
	}

	int GetSelectedReplay() const
	{
		return selected_replay;
	}

	std::string GetCarPaint() const
	{
		return carpaint;
	}

	void SetSelectedReplay ( int value )
	{
		selected_replay = value;
	}

	std::string GetTextureSize() const
	{
		return texturesize;
	}

	float GetButtonRamp() const
	{
		return button_ramp;
	}

	std::string GetFFDevice() const
	{
		return ff_device;
	}

	float GetFFGain() const
	{
		return ff_gain;
	}

	bool GetFFInvert() const
	{
		return ff_invert;
	}

	bool GetTrackReverse() const
	{
		return trackreverse;
	}

	bool GetShadows() const
	{
		return shadows;
	}

	int GetReflections() const
	{
		return reflections;
	}

	bool GetInputGraph() const
	{
		return input_graph;
	}

	int GetShadowDistance() const
	{
		return shadow_distance;
	}

	int GetShadowQuality() const
	{
		return shadow_quality;
	}

	int GetLighting() const
	{
		return lighting;
	}

	bool GetBloom() const
	{
		return bloom;
	}

	std::string GetOpponentCar() const
	{
		return opponent_car;
	}

	std::string GetOpponentCarPaint() const
	{
		return opponent_car_paint;
	}

	float GetCameraBounce() const
	{
		return camerabounce;
	}

	int GetNumberOfLaps() const
	{
	    return number_of_laps;
	}

	void SetContrast ( float value )
	{
		contrast = value;
	}

	float GetContrast() const
	{
		return contrast;
	}

	void SetCameraMode ( const std::string& value )
	{
		//std::cout << "Camera mode set to " << value << std::endl;
		camera_mode = value;
	}

	std::string GetCameraMode() const
	{
		return camera_mode;
	}

	void SetResolutionOverride ( bool value )
	{
		res_override = value;
	}

	bool GetHGateShifter() const
	{
		return hgateshifter;
	}
	
	void SetFailsafeSettings()
	{
		*this = SETTINGS();
		depthbpp = 16;
		shaders = false;
		texturesize = "medium";
	}

	float GetAIDifficulty() const
	{
		return ai_difficulty;
	}
	
};

#endif
