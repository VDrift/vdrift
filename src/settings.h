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

#ifndef _SETTINGS_H
#define _SETTINGS_H

#include <map>
#include <iosfwd>
#include <string>
#include <vector>

class Config;

class Settings
{
public:
	Settings();

	void SetFailsafeSettings();

	void Serialize(bool write, Config & config);

	void Load(const std::string & settingsfile, std::ostream & error);

	void Save(const std::string & settingsfile, std::ostream & error);

	void Get(std::map<std::string, std::string> & options);

	void Set(const std::map<std::string, std::string> & options);

	unsigned int GetResolutionX() const
	{
		return resolution[0];
	}

	unsigned int GetResolutionY() const
	{
		return resolution[1];
	}

	unsigned int GetDepthBpp() const
	{
		return depth_bpp;
	}

	bool GetFullscreen() const
	{
		return fullscreen;
	}

	bool GetVsync() const
	{
		return vsync;
	}

	const std::string & GetRenderer() const
	{
		return renderer;
	}

	const std::string & GetSkin() const
	{
		return skin;
	}

	const std::string & GetLanguage() const
	{
		return language;
	}

	bool GetShowFps() const
	{
		return show_fps;
	}

	float GetSoundVolume() const
	{
		return sound_volume;
	}

	int GetMaxSoundSources() const
	{
		return sound_sources;
	}

	// get sound attenuation[4] coefficients
	const float * GetSoundAttenuation() const
	{
		return sound_attenuation;
	}

	bool GetMPH() const
	{
		return mph;
	}

	const std::string & GetTrack() const
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

	const std::string & GetHUD() const
	{
		return hud;
	}

	int GetAnisotropy() const
	{
		return anisotropic;
	}

	float GetFOV() const
	{
		return fov;
	}

	bool GetABS() const
	{
		return abs;
	}

	bool GetTCS() const
	{
		return tcs;
	}

	const std::string & GetJoyType() const
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

	const std::string & GetSelectedReplay() const
	{
		return selected_replay;
	}

	const std::string & GetTextureSize() const
	{
		return texture_size;
	}

	bool GetTextureCompress() const
	{
		return texture_compress;
	}

	float GetButtonRamp() const
	{
		return button_ramp;
	}

	const std::string & GetFFDevice() const
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

	bool GetTrackDynamic() const
	{
		return trackdynamic;
	}

	bool GetShadows() const
	{
		return shadows;
	}

	int GetReflections() const
	{
		return reflections;
	}

	bool GetDebugInfo() const
	{
		return debug_info;
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

	bool GetSkyDynamic() const
	{
		return sky_dynamic;
	}

	int GetSkyTime() const
	{
		return sky_time;
	}

	int GetSkyTimeSpeed() const
	{
		return sky_time_speed;
	}

	bool GetBloom() const
	{
		return bloom;
	}

	bool GetNormalMaps() const
	{
		return normalmaps;
	}

	const std::string & GetCar() const
	{
		return car;
	}

	const std::string & GetCarPaint() const
	{
		return car_paint;
	}

	float GetCarColorHue() const
	{
		return car_color_hue;
	}

	float GetCarColorSat() const
	{
		return car_color_sat;
	}

	float GetCarColorVal() const
	{
		return car_color_val;
	}

	float GetCameraBounce() const
	{
		return camera_bounce;
	}

	int GetNumberOfLaps() const
	{
	    return number_of_laps;
	}

	float GetContrast() const
	{
		return contrast;
	}

	float GetParticles() const
	{
		return particles;
	}

	int GetCamera() const
	{
		return camera_id;
	}

	bool GetHGateShifter() const
	{
		return hgateshifter;
	}

	float GetAILevel() const
	{
		return ai_level;
	}

	bool GetVehicleDamage() const
	{
		return vehicle_damage;
	}

	void SetResolution(unsigned w, unsigned h)
	{
		resolution[0] = w;
		resolution[1] = h;
	}

	void SetSelectedReplay ( const std::string & value )
	{
		selected_replay = value;
	}

	void SetCamera ( int value )
	{
		camera_id = value;
	}

	void SetResolutionOverride ( bool value )
	{
		res_override = value;
	}

private:
	std::vector<unsigned> resolution;
	bool res_override;
	int depth_bpp;
	bool fullscreen;
	bool vsync;
	std::string renderer;
	std::string skin;
	std::string language;
	bool show_fps;
	float music_volume;
	float sound_volume;
	int sound_sources;
	float sound_attenuation[4];
	bool mph; //if false, KPH
	std::string track;
	int antialiasing; //0 or 1 mean off
	int anisotropic; //0 or 1 mean off
	bool trackmap;
	std::string hud;
	float fov;
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
	std::string selected_replay;
	std::string texture_size;
	bool texture_compress;
	float button_ramp;
	std::string ff_device;
	float ff_gain;
	bool ff_invert;
	bool trackreverse;
	bool trackdynamic;
	bool shadows;
	int shadow_distance;
	int shadow_quality;
	int reflections;
	bool debug_info;
	bool input_graph;
	int lighting;
	bool bloom;
	bool motionblur;
	bool normalmaps;
	std::string car;
	std::string car_paint;
	std::string car_tire;
	std::string car_wheel;
	float car_color_hue;
	float car_color_sat;
	float car_color_val;
	int cars_num;
	int camera_id;
	float camera_bounce;
	int number_of_laps;
	float contrast;
	bool hgateshifter;
	float ai_level;
	bool vehicle_damage;
	int particles;
	bool sky_dynamic;
	int sky_time;
	int sky_time_speed;
};

#endif
