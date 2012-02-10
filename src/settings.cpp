#include "settings.h"
#include "config.h"

template <typename T>
static void Param(
	CONFIG & config,
	bool write,
	CONFIG::iterator & section,
	const std::string & name,
	T & value)
{
	if (write)
	{
		config.SetParam(section, name, value);
	}
	else
	{
		config.GetParam(section, name, value);
	}
}

SETTINGS::SETTINGS() :
	res_override(false),
	resolution_x(800),
	resolution_y(600),
	bpp(16),
	depthbpp(24),
	fullscreen(false),
	shaders(true),
	skin("simple"),
	language("English"),
	show_fps(false),
	mastervolume(1.0),
	mph(true),
	track("paulricard88"),
	antialiasing(0),
	anisotropic(0),
	trackmap(true),
	show_hud(true),
	FOV(45.0),
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
	selected_replay(0),
	texturesize("large"),
	button_ramp(5),
	ff_device("/dev/input/event0"),
	ff_gain(2.0),
	ff_invert(false),
	trackreverse(false),
	trackdynamic(false),
	batch_geometry(false),
	shadows(false),
	shadow_distance(1),
	shadow_quality(1),
	reflections(1),
	input_graph(false),
	lighting(0),
	bloom(false),
	normalmaps(false),
	player("XS"),
	opponent("XS"),
	player_paint("default"),
	opponent_paint("default"),
	player_color(3, 1.0),
	opponent_color(3, 1.0),
	camerabounce(1.0),
	number_of_laps(1),
	contrast(1.0),
	camera_mode("chase"),
	hgateshifter(false),
	ai_difficulty(1.0),
	vehicle_damage(false)
{
	// ctor
}

void SETTINGS::Serialize(bool write, CONFIG & config)
{
	CONFIG::iterator section;

	config.GetSection("game", section);
	Param(config, write, section, "vehicle_damage", vehicle_damage);
	Param(config, write, section, "ai_difficulty", ai_difficulty);
	Param(config, write, section, "track", track);
	Param(config, write, section, "antilock", abs);
	Param(config, write, section, "traction_control", tcs);
	Param(config, write, section, "record", recordreplay);
	Param(config, write, section, "selected_replay", selected_replay);
	Param(config, write, section, "player", player);
	Param(config, write, section, "player_paint", player_paint);
	Param(config, write, section, "player_color", player_color);
	Param(config, write, section, "opponent", opponent);
	Param(config, write, section, "opponent_paint", opponent_paint);
	Param(config, write, section, "opponent_color", opponent_color);
	Param(config, write, section, "reverse", trackreverse);
	Param(config, write, section, "track_dynamic", trackdynamic);
	Param(config, write, section, "batch_geometry", batch_geometry);
	Param(config, write, section, "number_of_laps", number_of_laps);
	Param(config, write, section, "camera_mode", camera_mode);

	config.GetSection("display", section);
	if (!res_override)
	{
		Param(config, write, section, "width", resolution_x);
		Param(config, write, section, "height", resolution_y);
	}
	Param(config, write, section, "depth", bpp);
	Param(config, write, section, "zdepth", depthbpp);
	Param(config, write, section, "fullscreen", fullscreen);
	Param(config, write, section, "shaders", shaders);
	Param(config, write, section, "skin", skin);
	Param(config, write, section, "language", language);
	Param(config, write, section, "show_fps", show_fps);
	Param(config, write, section, "anisotropic", anisotropic);
	Param(config, write, section, "antialiasing", antialiasing);
	Param(config, write, section, "trackmap", trackmap);
	Param(config, write, section, "show_hud", show_hud);
	Param(config, write, section, "FOV", FOV);
	Param(config, write, section, "mph", mph);
	Param(config, write, section, "view_distance", view_distance);
	Param(config, write, section, "racingline", racingline);
	Param(config, write, section, "texture_size", texturesize);
	Param(config, write, section, "shadows", shadows);
	Param(config, write, section, "shadow_distance", shadow_distance);
	Param(config, write, section, "shadow_quality", shadow_quality);
	Param(config, write, section, "reflections", reflections);
	Param(config, write, section, "input_graph", input_graph);
	Param(config, write, section, "lighting", lighting);
	Param(config, write, section, "bloom", bloom);
	Param(config, write, section, "normalmaps", normalmaps);
	Param(config, write, section, "camerabounce", camerabounce);
	Param(config, write, section, "contrast", contrast);

	config.GetSection("sound", section);
	Param(config, write, section, "volume", mastervolume);

	config.GetSection("joystick", section);
	Param(config, write, section, "type", joytype);
	Param(config, write, section, "two_hundred", joy200);
	joy200 = false;
	Param(config, write, section, "calibrated", joystick_calibrated);
	Param(config, write, section, "ff_device", ff_device);
	Param(config, write, section, "ff_gain", ff_gain);
	Param(config, write, section, "ff_invert", ff_invert);
	Param(config, write, section, "hgateshifter", hgateshifter);

	config.GetSection("control", section);
	Param(config, write, section, "speed_sens_steering", speed_sensitivity);
	Param(config, write, section, "autoclutch", autoclutch);
	Param(config, write, section, "autotrans", autoshift);
	Param(config, write, section, "mousegrab", mousegrab);
	Param(config, write, section, "button_ramp", button_ramp);
}

void SETTINGS::Load(const std::string & settingsfile, std::ostream & error)
{
	CONFIG config;
	if (!config.Load(settingsfile))
	{
		error << "Failed to load " << settingsfile << std::endl;
	}
	Serialize(false, config);
}

void SETTINGS::Save(const std::string & settingsfile, std::ostream & error)
{
	CONFIG config;
	if (!config.Load(settingsfile))
	{
		error << "Failed to load " << settingsfile << std::endl;
	}
	Serialize(true, config);
	if (!config.Write())
	{
		error << "Failed to save " << settingsfile << std::endl;
	}
}

