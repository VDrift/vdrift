#include "settings.h"

void SETTINGS::Serialize(bool write, CONFIGFILE & config)
{
	Param(config, write, "game.selected_car", selected_car);
	Param(config, write, "game.opponent", opponent_car);
	Param(config, write, "game.ai_difficulty", ai_difficulty);
	Param(config, write, "game.track", track);
	Param(config, write, "game.antilock", abs);
	Param(config, write, "game.traction_control", tcs);
	Param(config, write, "game.record", recordreplay);
	Param(config, write, "game.selected_replay", selected_replay);
	Param(config, write, "game.car_paint", carpaint);
	Param(config, write, "game.opponent_car_paint", opponent_car_paint);
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

