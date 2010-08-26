#ifndef _SETTINGS_H
#define _SETTINGS_H

#include "configfile.h"

#include <string>

class SETTINGS
{
public:
	SETTINGS();

	void Serialize(bool write, CONFIGFILE & config);
	
	void Load(std::string settingsfile)
	{
		CONFIGFILE config;
		config.Load(settingsfile);
		Serialize(false, config);
	}
	
	void Save(std::string settingsfile)
	{
		CONFIGFILE config;
		config.Load(settingsfile);
		Serialize(true, config);config.Write();
	}

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
	
	bool GetNormalMaps() const
	{
		return normalmaps;
	}
	
	std::string GetSelectedCar() const
	{
		return player;
	}
	
	std::string GetCarPaint() const
	{
		return player_paint;
	}
	
	void GetCarColor(float & r, float & g, float & b) const
	{
		r = player_color_red;
		g = player_color_green;
		b = player_color_blue;
	}

	std::string GetOpponentCar() const
	{
		return opponent;
	}

	std::string GetOpponentCarPaint() const
	{
		return opponent_paint;
	}
	
	void GetOpponentColor(float & r, float & g, float & b) const
	{
		r = opponent_color_red;
		g = opponent_color_green;
		b = opponent_color_blue;
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
	bool normalmaps;
	std::string player;
	std::string player_paint;
	float player_color_red;
	float player_color_green;
	float player_color_blue;
	std::string opponent;
	std::string opponent_paint;
	float opponent_color_red;
	float opponent_color_green;
	float opponent_color_blue;
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
	
};

#endif
