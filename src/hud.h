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

#include "scenenode.h"
#include "text_draw.h"
#include "hudgauge.h"
#include "hudbar.h"

#include <ostream>
#include <string>
#include <list>

class FONT;
class ContentManager;

class HUD
{
public:
	HUD();

	bool Init(
		const std::string & texturepath,
		ContentManager & content,
		FONT & lcdfont,
		FONT & sansfont,
		FONT & sansfont_noshader,
		float displaywidth,
		float displayheight,
		bool debugon,
		std::ostream & error_output);

	void Update(
		FONT & lcdfont, FONT & sansfont, FONT & sansfont_noshader, float displaywidth, float displayheight,
		float curlap, float lastlap, float bestlap, float stagingtimeleft,
		int curlapnum, int numlaps, int curplace, int numcars,
		float rpm, float redrpm, float maxrpm,
		float speed, float maxspeed, bool mph, float clutch, int newgear,
		const std::string & debug_string1, const std::string & debug_string2,
		const std::string & debug_string3, const std::string & debug_string4,
		bool absenabled, bool absactive, bool tcsenabled, bool tcsactive,
		bool outofgas, bool nosactive, float nosamount,
		bool drifting, float driftscore, float thisdriftscore);

	SCENENODE & GetNode();

	void SetVisible(bool value);

private:
	SCENENODE hudroot;

	// timer
	keyed_container<SCENENODE>::handle timernode;
	keyed_container<DRAWABLE>::handle timerboxdraw;
	VERTEXARRAY timerboxverts;
	TEXT_DRAWABLE laptime_label;
	TEXT_DRAWABLE laptime;
	TEXT_DRAWABLE lastlaptime_label;
	TEXT_DRAWABLE lastlaptime;
	TEXT_DRAWABLE bestlaptime_label;
	TEXT_DRAWABLE bestlaptime;
	TEXT_DRAWABLE lapindicator;
	TEXT_DRAWABLE driftscoreindicator;
	TEXT_DRAWABLE placeindicator;
	TEXT_DRAWABLE raceprompt;

	// debug info
	keyed_container<SCENENODE>::handle debugnode;
	keyed_container<DRAWABLE>::handle debugtextdraw1;
	keyed_container<DRAWABLE>::handle debugtextdraw2;
	keyed_container<DRAWABLE>::handle debugtextdraw3;
	keyed_container<DRAWABLE>::handle debugtextdraw4;
	TEXT_DRAW debugtext1;
	TEXT_DRAW debugtext2;
	TEXT_DRAW debugtext3;
	TEXT_DRAW debugtext4;

	// rpm/speed bar
	std::list<HUDBAR> bars;
	keyed_container<DRAWABLE>::handle rpmbar;
	keyed_container<DRAWABLE>::handle rpmredbar;
	keyed_container<DRAWABLE>::handle rpmbox;
	VERTEXARRAY rpmbarverts;
	VERTEXARRAY rpmredbarverts;
	VERTEXARRAY rpmboxverts;

	// gear/speed values
	keyed_container<DRAWABLE>::handle geartextdraw;
	keyed_container<DRAWABLE>::handle mphtextdraw;
	TEXT_DRAW geartext;
	TEXT_DRAW mphtext;

	// abs/tcs/gas/nos indicators
	TEXT_DRAWABLE abs;
	TEXT_DRAWABLE tcs;
	TEXT_DRAWABLE gas;
	TEXT_DRAWABLE nos;

	// gauge labels
	TEXT_DRAW speedlabel;
	TEXT_DRAW rpmlabel;
	TEXT_DRAW rpmxlabel;

	// gauges
	HUDGAUGE rpmgauge;
	HUDGAUGE speedgauge;
	float maxrpm;
	float maxspeed;
	float speedscale;
	bool mph;

	bool debug_hud_info;
	bool racecomplete;
	bool lastvisible;

	void SetDebugVisible(bool value);
};

#endif
