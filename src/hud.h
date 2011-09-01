#ifndef _HUD_H
#define _HUD_H

#include "scenenode.h"
#include "font.h"
#include "texture.h"
#include "text_draw.h"

#include <ostream>
#include <cassert>
#include <string>
#include <list>
#include <sstream>
#include <algorithm>

class HUDBAR
{
	private:
		keyed_container <DRAWABLE>::handle draw;
		VERTEXARRAY verts;

	public:
		void Set(
			SCENENODE & parent,
			std::tr1::shared_ptr<TEXTURE> bartex,
			float x, float y, float w, float h,
			float opacity,
			bool flip)
		{
			draw = parent.GetDrawlist().twodim.insert(DRAWABLE());
			DRAWABLE & drawref = parent.GetDrawlist().twodim.get(draw);

			drawref.SetDiffuseMap(bartex);
			drawref.SetVertArray(&verts);
			drawref.SetCull(false, false);
			drawref.SetColor(1,1,1,opacity);
			drawref.SetDrawOrder(1);

			verts.SetTo2DButton(x, y, w, h, h*0.75, flip);
		}

		void SetVisible(SCENENODE & parent, bool newvis)
		{
			DRAWABLE & drawref = parent.GetDrawlist().twodim.get(draw);
			drawref.SetDrawEnable(newvis);
		}
};

class HUD
{
public:
	HUD() : debug_hud_info(false), racecomplete(false), lastvisible(true) {}

	bool Init(
		const std::string & texturepath,
		ContentManager & content,
		FONT & lcdfont,
		FONT & sansfont,
		float displaywidth,
		float displayheight,
		bool debugon,
		std::ostream & error_output);

	void Hide()
	{
		SetVisible(false);
	}

	void Show()
	{
		SetVisible(true);
	}

	void Update(FONT & lcdfont, FONT & sansfont, float curlap, float lastlap, float bestlap, float stagingtimeleft, int curlapnum,
		int numlaps, int curplace, int numcars, float clutch, int newgear, int newrpm, int redrpm, int maxrpm,
		float meterspersecond,
		bool mph, const std::string & debug_string1, const std::string & debug_string2,
		const std::string & debug_string3, const std::string & debug_string4, float displaywidth,
		float displayheight, bool absenabled, bool absactive, bool tcsenabled, bool tcsactive,
		bool drifting, float driftscore, float thisdriftscore);

	void SetDebugVisibility(bool show)
	{
		SCENENODE & debugnoderef = hudroot.GetNode(debugnode);
		debugnoderef.SetChildVisibility(show);
	}

	SCENENODE & GetNode() {return hudroot;}

private:
	TEXTURE bartex;
	SCENENODE hudroot;
	std::list <HUDBAR> bars;

	TEXTURE progbartex;
	keyed_container <DRAWABLE>::handle rpmbar;
	keyed_container <DRAWABLE>::handle rpmredbar;
	VERTEXARRAY rpmbarverts;
	VERTEXARRAY rpmredbarverts;
	keyed_container <DRAWABLE>::handle rpmbox;
	VERTEXARRAY rpmboxverts;

	//variables for drawing the timer
	keyed_container <SCENENODE>::handle timernode;
	TEXTURE timerboxtex;
	keyed_container <DRAWABLE>::handle timerboxdraw;
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

	// variables for abs/tcs/gas/n2o indicators
	TEXT_DRAWABLE abs;
	TEXT_DRAWABLE tcs;
	TEXT_DRAWABLE gas;
	TEXT_DRAWABLE n2o;

	//debug info
	keyed_container <SCENENODE>::handle debugnode;
	TEXT_DRAW debugtext1;
	keyed_container <DRAWABLE>::handle debugtextdraw1;
	TEXT_DRAW debugtext2;
	keyed_container <DRAWABLE>::handle debugtextdraw2;
	TEXT_DRAW debugtext3;
	keyed_container <DRAWABLE>::handle debugtextdraw3;
	TEXT_DRAW debugtext4;
	keyed_container <DRAWABLE>::handle debugtextdraw4;

	TEXT_DRAW geartext;
	keyed_container <DRAWABLE>::handle geartextdraw;

	TEXT_DRAW mphtext;
	keyed_container <DRAWABLE>::handle mphtextdraw;

	bool debug_hud_info;

	bool racecomplete;

	bool lastvisible;

	void SetVisible(bool newvis)
	{
		if (newvis != lastvisible)
		{
			hudroot.SetChildVisibility(newvis);
			SetDebugVisibility(newvis && debug_hud_info);
			lastvisible = newvis;
		}
	}

	keyed_container <DRAWABLE>::handle SetupText(SCENENODE & parent, FONT & font, TEXT_DRAW & textdraw, const std::string & str, const float x, const float y, const float scalex, const float scaley, const float r, const float g, const float b, float zorder = 0)
	{
		keyed_container <DRAWABLE>::handle draw = parent.GetDrawlist().text.insert(DRAWABLE());
		DRAWABLE & drawref = parent.GetDrawlist().text.get(draw);
		textdraw.Set(drawref, font, str, x, y, scalex,scaley, r, g, b);
		drawref.SetDrawOrder(zorder);
		return draw;
	}

	void GetTimeString(float time, std::string & outtime) const
	{
		int min = (int) time / 60;
		float secs = time - min * 60;

		if (time != 0.0)
		{
			std::stringstream s;
			s << std::setfill('0');
			s << std::setw(2) << min << ":";
			s << std::fixed << std::setprecision(3) << std::setw(6) << secs;
			outtime = s.str();
		}
		else
		{
			outtime = "--:--.---";
		}
	}
};

#endif
