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

#ifndef _HUD_H
#define _HUD_H

#include "gui/text_draw.h"
#include "hudgauge.h"
#include "hudbar.h"
#include "memory.h"

#include <iosfwd>
#include <string>
#include <list>

class Font;
class Texture;
class GuiLanguage;
class ContentManager;

class Hud
{
public:
	Hud();

	bool Init(
		const std::string & texturepath,
		const GuiLanguage & lang,
		const Font & sansfont,
		const Font & lcdfont,
		float displaywidth,
		float displayheight,
		bool debugon,
		ContentManager & content,
		std::ostream & error_output);

	void Update(
		const Font & sansfont, const Font & lcdfont,
		float displaywidth, float displayheight,
		float curlap, float lastlap, float bestlap, float stagingtimeleft,
		int curlapnum, int numlaps, int curplace, int numcars,
		float rpm, float redrpm, float maxrpm,
		float speed, float maxspeed, bool mph, float clutch, int newgear,
		const std::string & debug_string1, const std::string & debug_string2,
		const std::string & debug_string3, const std::string & debug_string4,
		bool absenabled, bool absactive, bool tcsenabled, bool tcsactive,
		bool outofgas, bool nosactive, float nosamount,
		bool drifting, float driftscore, float thisdriftscore);

	SceneNode & GetNode();

	void SetVisible(bool value);

private:
	SceneNode hudroot;
	keyed_container<SceneNode>::handle infonode;
	std::tr1::shared_ptr<Texture> boxtex;
	std::tr1::shared_ptr<Texture> bartex;
	std::tr1::shared_ptr<Texture> progbartex;

	// timer
	keyed_container<Drawable>::handle timerboxdraw;
	VertexArray timerboxverts;
	TextDrawable laptime_label;
	TextDrawable lastlaptime_label;
	TextDrawable bestlaptime_label;
	TextDrawable laptime;
	TextDrawable lastlaptime;
	TextDrawable bestlaptime;

	// race info
	keyed_container<Drawable>::handle infoboxdraw;
	VertexArray infoboxverts;
	TextDrawable place_label;
	TextDrawable lap_label;
	TextDrawable drift_label;
	TextDrawable placeindicator;
	TextDrawable lapindicator;
	TextDrawable driftscoreindicator;
	TextDrawable raceprompt;

	// debug info
	keyed_container<SceneNode>::handle debugnode;
	keyed_container<Drawable>::handle debugtextdraw1;
	keyed_container<Drawable>::handle debugtextdraw2;
	keyed_container<Drawable>::handle debugtextdraw3;
	keyed_container<Drawable>::handle debugtextdraw4;
	TextDraw debugtext1;
	TextDraw debugtext2;
	TextDraw debugtext3;
	TextDraw debugtext4;

	// rpm/speed bar
	std::list<HudBar> bars;
	keyed_container<Drawable>::handle rpmbar;
	keyed_container<Drawable>::handle rpmredbar;
	keyed_container<Drawable>::handle rpmbox;
	VertexArray rpmbarverts;
	VertexArray rpmredbarverts;
	VertexArray rpmboxverts;

	// gear/speed values
	keyed_container<Drawable>::handle geartextdraw;
	keyed_container<Drawable>::handle mphtextdraw;
	TextDraw geartext;
	TextDraw mphtext;

	// abs/tcs/gas/nos indicators
	TextDrawable abs;
	TextDrawable tcs;
	TextDrawable gas;
	TextDrawable nos;

	// gauge labels
	TextDraw speedlabel;
	TextDraw rpmlabel;
	TextDraw rpmxlabel;

	// gauges
	HudGauge rpmgauge;
	HudGauge speedgauge;
	float maxrpm;
	float maxspeed;
	float speedscale;
	bool mph;

	// hud strings
	std::vector<std::string> str;

	bool debug_hud_info;
	bool racecomplete;
	bool lastvisible;

	void SetDebugVisible(bool value);
};

#endif
