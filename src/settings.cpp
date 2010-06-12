#include "settings.h"

SETTINGS::SETTINGS() :
	res_override(false),
	resolution_x(800),resolution_y(600),bpp(16),depthbpp(24),fullscreen(false),
	shaders(true),skin("simple"),show_fps(false),mastervolume(1.0),mph(true),track("paulricard88"), antialiasing(0), anisotropic(0),
	trackmap(true), show_hud(true), FOV(45.0), abs(true), tcs(true), joytype("joystick"),
	joy200(false), speed_sensitivity(1.0), joystick_calibrated(false), view_distance(1000.0),
	autoclutch(true), autoshift(true), racingline(false), mousegrab(true),
	recordreplay(false), selected_replay(0), texturesize("large"), button_ramp(5),
	ff_device("/dev/input/event0"), ff_gain(2.0), ff_invert(false), trackreverse(false),
	shadows(false), shadow_distance(1), shadow_quality(1),
	reflections(1), input_graph(false), lighting(0), bloom(false), normalmaps(false),
	player("XS"), player_paint("00"), player_color_red(1), player_color_green(0), player_color_blue(0),
	opponent("XS"), opponent_paint("00"), opponent_color_red(0), opponent_color_green(0), opponent_color_blue(1),
	camerabounce(1.0), number_of_laps(1), contrast(1.0), camera_mode("chase"), hgateshifter(false),
	ai_difficulty(1.0)
{

}

void SETTINGS::Serialize(bool write, CONFIGFILE & config)
{
	Param(config, write, "game.ai_difficulty", ai_difficulty);
	Param(config, write, "game.track", track);
	Param(config, write, "game.antilock", abs);
	Param(config, write, "game.traction_control", tcs);
	Param(config, write, "game.record", recordreplay);
	Param(config, write, "game.selected_replay", selected_replay);
	Param(config, write, "game.player", player);
	Param(config, write, "game.player_paint", player_paint);
	Param(config, write, "game.player_color_red", player_color_red);
	Param(config, write, "game.player_color_green", player_color_green);
	Param(config, write, "game.player_color_blue", player_color_blue);
	Param(config, write, "game.opponent", opponent);
	Param(config, write, "game.opponent_paint", opponent_paint);
	Param(config, write, "game.opponent_color_red", opponent_color_red);
	Param(config, write, "game.opponent_color_green", opponent_color_green);
	Param(config, write, "game.opponent_color_blue", opponent_color_blue);
	Param(config, write, "game.reverse", trackreverse);
	Param(config, write, "game.number_of_laps", number_of_laps);
	Param(config, write, "game.camera_mode", camera_mode);

	if (!res_override)
	{
		Param(config, write, "display.width", resolution_x);
		Param(config, write, "display.height", resolution_y);
	}
	Param(config, write, "display.depth", bpp);
	Param(config, write, "display.zdepth", depthbpp);
	Param(config, write, "display.fullscreen", fullscreen);
	Param(config, write, "display.shaders", shaders);
	Param(config, write, "display.skin", skin);
	Param(config, write, "display.show_fps", show_fps);
	Param(config, write, "display.anisotropic", anisotropic);
	Param(config, write, "display.antialiasing", antialiasing);
	Param(config, write, "display.trackmap", trackmap);
	Param(config, write, "display.show_hud", show_hud);
	Param(config, write, "display.FOV", FOV);
	Param(config, write, "display.mph", mph);
	Param(config, write, "display.view_distance", view_distance);
	Param(config, write, "display.racingline", racingline);
	Param(config, write, "display.texture_size", texturesize);
	Param(config, write, "display.shadows", shadows);
	Param(config, write, "display.shadow_distance", shadow_distance);
	Param(config, write, "display.shadow_quality", shadow_quality);
	Param(config, write, "display.reflections", reflections);
	Param(config, write, "display.input_graph", input_graph);
	Param(config, write, "display.lighting", lighting);
	Param(config, write, "display.bloom", bloom);
	Param(config, write, "display.normalmaps", normalmaps);
	Param(config, write, "display.camerabounce", camerabounce);
	Param(config, write, "display.contrast", contrast);

	Param(config, write, "sound.volume", mastervolume);

	Param(config, write, "joystick.type", joytype);
	Param(config, write, "joystick.two_hundred", joy200);
	joy200 = false;
	Param(config, write, "joystick.calibrated", joystick_calibrated);
	Param(config, write, "joystick.ff_device", ff_device);
	Param(config, write, "joystick.ff_gain", ff_gain);
	Param(config, write, "joystick.ff_invert", ff_invert);
	Param(config, write, "joystick.hgateshifter", hgateshifter);

	Param(config, write, "control.speed_sens_steering", speed_sensitivity);
	Param(config, write, "control.autoclutch", autoclutch);
	Param(config, write, "control.autotrans", autoshift);
	Param(config, write, "control.mousegrab", mousegrab);
	Param(config, write, "control.button_ramp", button_ramp);
}

