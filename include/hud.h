#ifndef _HUD_H
#define _HUD_H

#include <ostream>
#include <cassert>
#include <string>
#include <list>
#include <sstream>
#include <algorithm>

#include "scenegraph.h"
#include "font.h"
#include "texture.h"
#include "text_draw.h"

class HUDBAR
{
	private:
		DRAWABLE * draw;
		VERTEXARRAY verts;

	public:
		HUDBAR() : draw(NULL) {}
		void Set(SCENENODE & parent, TEXTURE_GL & bartex, float x, float y, float w, float h, float opacity, bool flip)
		{
			draw = &parent.AddDrawable();
			assert(draw);

			draw->SetDiffuseMap(&bartex);
			draw->SetVertArray(&verts);
			draw->SetLit(false);
			draw->Set2D(true);
			draw->SetCull(false, false);
			draw->SetColor(1,1,1,opacity);
			draw->SetDrawOrder(1);

			verts.SetTo2DButton(x,y,w,h,h*0.75,flip);
		}

		void SetVisible(bool newvis)
		{
			assert(draw);
			draw->SetDrawEnable(newvis);
		}
};

class HUD
{
private:
	TEXTURE_GL bartex;
	SCENENODE * hudroot;
	std::list <HUDBAR> bars;

	TEXTURE_GL progbartex;
	DRAWABLE * rpmbar;
	DRAWABLE * rpmredbar;
	VERTEXARRAY rpmbarverts;
	VERTEXARRAY rpmredbarverts;
	DRAWABLE * rpmbox;
	VERTEXARRAY rpmboxverts;

	//variables for drawing the timer
	SCENENODE * timernode;
	TEXTURE_GL timerboxtex;
	DRAWABLE * timerboxdraw;
	VERTEXARRAY timerboxverts;
	/*TEXTURE_GL timerbartex;
	DRAWABLE * timerbardraw;
	VERTEXARRAY timerbarverts;*/
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

	//variables for the abs/tcs display
	TEXT_DRAWABLE abs;
	TEXT_DRAWABLE tcs;

	TEXT_DRAW debugtext1;
	DRAWABLE * debugtextdraw1;
	TEXT_DRAW debugtext2;
	DRAWABLE * debugtextdraw2;
	TEXT_DRAW debugtext3;
	DRAWABLE * debugtextdraw3;
	TEXT_DRAW debugtext4;
	DRAWABLE * debugtextdraw4;

	TEXT_DRAW geartext;
	DRAWABLE * geartextdraw;

	TEXT_DRAW mphtext;
	DRAWABLE * mphtextdraw;

	bool debug_hud_info;

	bool racecomplete;

	void SetVisible(DRAWABLE * d,  bool newvis)
	{
		if (d) d->SetDrawEnable(newvis);
	}

	void SetVisible(bool newvis)
	{
		/*for (std::list <HUDBAR>::iterator i = bars.begin(); i != bars.end(); i++)
		{
			i->SetVisible(newvis);
		}

		SetVisible(rpmbar, newvis);
		SetVisible(rpmredbar, newvis);
		SetVisible(rpmbox, newvis);
		SetVisible(geartextdraw, newvis);
		SetVisible(mphtextdraw, newvis);
		//SetVisible(timerboxdraw, newvis);
		//SetVisible(timerbardraw, newvis);
		assert(timernode);
		timernode->SetChildVisibility(newvis);*/
		assert(hudroot);
		hudroot->SetChildVisibility(newvis);

		SetDebugVisibility(newvis && debug_hud_info);
	}

	DRAWABLE * SetupText(SCENENODE & parent, FONT & font, TEXT_DRAW & textdraw, const std::string & str, const float x, const float y, const float scalex, const float scaley, const float r, const float g, const float b)
	{
		DRAWABLE * draw = &parent.AddDrawable();
		assert(draw);
		textdraw.Set(*draw, font, str, x, y, scalex,scaley, r, g, b);
		return draw;
	}

	void GetTimeString(float time, std::string & outtime) const
	{
		int min = (int) time / 60;
		float secs = time - min*60;

		if (time != 0.0)
		{
			char tempchar[128];
			sprintf(tempchar, "%02d:%06.3f", min, secs);
			outtime = tempchar;
		}
		else
			outtime = "--:--.---";
	}

public:
	HUD() : hudroot(NULL), rpmbar(NULL), rpmredbar(NULL), timernode(NULL), timerboxdraw(NULL),
	        debugtextdraw1(NULL), debugtextdraw2(NULL),
		debugtextdraw3(NULL), debugtextdraw4(NULL), geartextdraw(NULL), mphtextdraw(NULL),
		debug_hud_info(false), racecomplete(false) {}

	bool Init(SCENENODE & parentnode, const std::string & texturepath, FONT & lcdfont, FONT & sansfont, std::ostream & error_output, float displaywidth, float displayheight, const std::string & texsize, bool debugon);

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

	void SetDebugVisibility(bool show) {debugtextdraw1->SetDrawEnable(show);debugtextdraw2->SetDrawEnable(show);
		debugtextdraw3->SetDrawEnable(show);debugtextdraw4->SetDrawEnable(show);}
};

#endif
