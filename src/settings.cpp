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

#include "settings.h"
#include "cfg/config.h"

template <typename T>
static void Param(
	Config & config,
	bool write,
	Config::iterator & section,
	const std::string & name,
	T & value)
{
	if (write)
	{
		config.set(section, name, value);
	}
	else
	{
		config.get(section, name, value);
	}
}

Settings::Settings() :
	resolution(2),
	res_override(false),
	depth_bpp(24),
	fullscreen(false),
	vsync(false),
	renderer("gl3/deferred.conf"),
	skin("simple"),
	language("en"),
	show_fps(false),
	music_volume(0.5),
	sound_volume(0.5),
	sound_sources(64),
	mph(true),
	track("ruudskogen"),
	antialiasing(0),
	anisotropic(0),
	trackmap(true),
	hud("Hud"),
	fov(45.0),
	abs(true),
	tcs(true),
	joytype("joystick"),
	joy200(false),
	speed_sensitivity(1.0),
	joystick_calibrated(false),
	view_distance(1000.0),
	autoclutch(true),
	autoshift(true),
	racingline(false),
	mousegrab(true),
	recordreplay(false),
	selected_replay("none"),
	texture_size("large"),
	texture_compress(true),
	button_ramp(5),
	ff_device("/dev/input/event0"),
	ff_gain(1.0),
	ff_invert(false),
	trackreverse(false),
	trackdynamic(false),
	shadows(true),
	shadow_distance(1),
	shadow_quality(1),
	reflections(1),
	debug_info(false),
	input_graph(false),
	lighting(0),
	bloom(false),
	motionblur(false),
	normalmaps(false),
	car("XS/XS"),
	car_paint("default"),
	car_tire("default"),
	car_wheel("default"),
	car_color_hue(0.5),
	car_color_sat(1.0),
	car_color_val(0.5),
	cars_num(1),
	camera_id(0),
	camera_bounce(1.0),
	number_of_laps(1),
	contrast(1.0),
	hgateshifter(false),
	ai_level(1.0),
	vehicle_damage(false),
	particles(512),
	sky_dynamic(false),
	sky_time(17),
	sky_time_speed(1)
{
	resolution[0] = 800;
	resolution[1] = 600;

	sound_attenuation[0] =  0.9146065;
	sound_attenuation[1] =  0.2729276;
	sound_attenuation[2] = -0.2313740;
	sound_attenuation[3] = -0.2884304;
}

void Settings::SetFailsafeSettings()
{
	*this = Settings();
	renderer = "gl2/basic.conf";
	texture_size = "medium";
}

void Settings::Serialize(bool write, Config & config)
{
	Config::iterator section;

	config.get("game", section);
	Param(config, write, section, "vehicle_damage", vehicle_damage);
	Param(config, write, section, "ai_level", ai_level);
	Param(config, write, section, "track", track);
	Param(config, write, section, "antilock", abs);
	Param(config, write, section, "traction_control", tcs);
	Param(config, write, section, "record", recordreplay);
	Param(config, write, section, "selected_replay", selected_replay);
	Param(config, write, section, "car", car);
	Param(config, write, section, "car_paint", car_paint);
	Param(config, write, section, "car_tire", car_tire);
	Param(config, write, section, "car_wheel", car_wheel);
	Param(config, write, section, "car_color_hue", car_color_hue);
	Param(config, write, section, "car_color_sat", car_color_sat);
	Param(config, write, section, "car_color_val", car_color_val);
	Param(config, write, section, "cars_num", cars_num);
	Param(config, write, section, "reverse", trackreverse);
	Param(config, write, section, "track_dynamic", trackdynamic);
	Param(config, write, section, "number_of_laps", number_of_laps);
	Param(config, write, section, "camera_id", camera_id);

	config.get("display", section);
	if (!res_override)
		Param(config, write, section, "resolution", resolution);
	Param(config, write, section, "zdepth", depth_bpp);
	Param(config, write, section, "fullscreen", fullscreen);
	Param(config, write, section, "vsync", vsync);
	Param(config, write, section, "renderer", renderer);
	Param(config, write, section, "skin", skin);
	Param(config, write, section, "language", language);
	Param(config, write, section, "show_fps", show_fps);
	Param(config, write, section, "anisotropic", anisotropic);
	Param(config, write, section, "antialiasing", antialiasing);
	Param(config, write, section, "trackmap", trackmap);
	Param(config, write, section, "hud", hud);
	Param(config, write, section, "camerafov", fov);
	Param(config, write, section, "mph", mph);
	Param(config, write, section, "view_distance", view_distance);
	Param(config, write, section, "racingline", racingline);
	Param(config, write, section, "texture_size", texture_size);
	Param(config, write, section, "texture_compress", texture_compress);
	Param(config, write, section, "shadows", shadows);
	Param(config, write, section, "shadow_distance", shadow_distance);
	Param(config, write, section, "shadow_quality", shadow_quality);
	Param(config, write, section, "reflections", reflections);
	Param(config, write, section, "debug_info", debug_info);
	Param(config, write, section, "input_graph", input_graph);
	Param(config, write, section, "lighting", lighting);
	Param(config, write, section, "bloom", bloom);
	Param(config, write, section, "motionblur", motionblur);
	Param(config, write, section, "normalmaps", normalmaps);
	Param(config, write, section, "camerabounce", camera_bounce);
	Param(config, write, section, "contrast", contrast);
	Param(config, write, section, "particles", particles);
	Param(config, write, section, "sky_dynamic", sky_dynamic);
	Param(config, write, section, "sky_time", sky_time);
	Param(config, write, section, "sky_time_speed", sky_time_speed);

	config.get("sound", section);
	Param(config, write, section, "attenuation_scale", sound_attenuation[0]);
	Param(config, write, section, "attenuation_shift", sound_attenuation[1]);
	Param(config, write, section, "attenuation_exponent", sound_attenuation[2]);
	Param(config, write, section, "attenuation_offset", sound_attenuation[3]);
	Param(config, write, section, "sources", sound_sources);
	Param(config, write, section, "volume", sound_volume);
	Param(config, write, section, "music_volume", music_volume);

	config.get("joystick", section);
	Param(config, write, section, "device_type", joytype);
	Param(config, write, section, "two_hundred", joy200);
	joy200 = false;
	Param(config, write, section, "calibrated", joystick_calibrated);
	Param(config, write, section, "ff_device", ff_device);
	Param(config, write, section, "ff_gain", ff_gain);
	Param(config, write, section, "ff_invert", ff_invert);
	Param(config, write, section, "hgateshifter", hgateshifter);

	config.get("control", section);
	Param(config, write, section, "speed_sens_steering", speed_sensitivity);
	Param(config, write, section, "autoclutch", autoclutch);
	Param(config, write, section, "autotrans", autoshift);
	Param(config, write, section, "mousegrab", mousegrab);
	Param(config, write, section, "button_ramp", button_ramp);
}

void Settings::Load(const std::string & settingsfile, std::ostream & error)
{
	Config config;
	if (!config.load(settingsfile))
	{
		error << "Failed to load " << settingsfile << std::endl;
	}
	Serialize(false, config);
}

void Settings::Save(const std::string & settingsfile, std::ostream & error)
{
	Config config;
	if (!config.load(settingsfile))
	{
		error << "Failed to load " << settingsfile << std::endl;
	}
	Serialize(true, config);
	if (!config.write())
	{
		error << "Failed to save " << settingsfile << std::endl;
	}
}

void Settings::Get(std::map<std::string, std::string> & options)
{
	Config tempconfig;
	Serialize(true, tempconfig);
	for (Config::const_iterator ic = tempconfig.begin(); ic != tempconfig.end(); ++ic)
	{
		std::string section = ic->first;
		for (Config::Section::const_iterator is = ic->second.begin(); is != ic->second.end(); ++is)
		{
			if (section.length() > 0)
				options[section + "." + is->first] = is->second;
			else
				options[is->first] = is->second;
		}
	}
}

void Settings::Set(const std::map<std::string, std::string> & options)
{
	Config tempconfig;
	for (std::map<std::string, std::string>::const_iterator i = options.begin(); i != options.end(); ++i)
	{
		std::string section;
		std::string param = i->first;
		size_t n = param.find(".");
		if (n < param.length())
		{
			section = param.substr(0, n);
			param.erase(0, n + 1);
		}
		tempconfig.set(section, param, i->second);
	}
	Serialize(false, tempconfig);
}
