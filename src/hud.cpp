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

#include "hud.h"
#include "content/contentmanager.h"
#include "graphics/texture.h"
#include "gui/guilanguage.h"

//#define GAUGES

static SceneNode::DrawableHandle AddDrawable(SceneNode & node)
{
	return node.GetDrawList().twodim.insert(Drawable());
}

static Drawable & GetDrawable(SceneNode & node, const SceneNode::DrawableHandle & drawhandle)
{
	return node.GetDrawList().twodim.get(drawhandle);
}

static SceneNode::DrawableHandle SetupText(
	SceneNode & parent, TextDraw & textdraw,
	const std::string & str, const Font & font,
	float x, float y, float scalex, float scaley,
	float r, float g , float b, float zorder = 0)
{
	SceneNode::DrawableHandle draw = parent.GetDrawList().text.insert(Drawable());
	Drawable & drawref = parent.GetDrawList().text.get(draw);
	textdraw.Set(drawref, font, str, x, y, scalex, scaley, r, g, b);
	drawref.SetDrawOrder(zorder);
	return draw;
}

static void GetTimeString(float time, std::string & outtime)
{
	int min = (int) time / 60;
	float secs = time - min * 60;

	if (time != 0.0)
	{
		std::ostringstream s;
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

enum HudStrEnum
{
	LAPTIME, LASTLAP, BESTLAP, SCORE, LAP, PLACE,
	READY, GO, YOUWON, YOULOST, MPH, KPH, STRNUM
};

Hud::Hud() :
	maxrpm(9000),
	maxspeed(240),
	speedscale(3.6),
	mph(false),
	debuginfo(false),
	racecomplete(false),
	lastvisible(true)
{
	// ctor
}

bool Hud::Init(
	const std::string & texturepath,
	const GuiLanguage & lang,
	const Font & sansfont,
	const Font & lcdfont,
	float displaywidth,
	float displayheight,
	ContentManager & content,
	std::ostream & error_output)
{
	const float opacity = 0.8;
	const float screenhwratio = displayheight / displaywidth;
	const float barheight = 64.0 / displayheight;
	const float barwidth = 256.0 / displaywidth;

	str.resize(STRNUM);
	str[LAPTIME] = lang("Lap time:");
	str[LASTLAP] = lang("Last lap:");
	str[BESTLAP] = lang("Best lap:");
	str[SCORE] = lang("Score:");
	str[LAP] = lang("Lap:");
	str[PLACE] = lang("Place:");
	str[READY] = lang("Ready");
	str[GO] = lang("GO");
	str[YOUWON] = lang("You won!");
	str[YOULOST] = lang("You lost");
	str[MPH] = lang("MPH");
	str[KPH] = lang("KPH");

	infonode = hudroot.AddNode();
	SceneNode & infonoderef = hudroot.GetNode(infonode);

	TextureInfo boxtexinfo;
	boxtexinfo.mipmap = false;
	boxtexinfo.repeatu = true;
	boxtexinfo.repeatv = false;
	content.load(boxtex, texturepath, "timerbox.png", boxtexinfo);

	{
		timerboxdraw = AddDrawable(infonoderef);
		Drawable & timerboxdrawref = GetDrawable(infonoderef, timerboxdraw);

		float timerboxdimx = 96.0 / displaywidth;
		float timerboxdimy = 60.0 / displayheight;
		float totalsizex = timerboxdimx * 6.05;
		float totalsizey = timerboxdimy * 2.0;
		float x = totalsizex * 0.5 - timerboxdimx * 0.65;
		float y = totalsizey * 0.5 - timerboxdimy * 0.25;
		float w = totalsizex - timerboxdimx * 2;
		float h = totalsizey - timerboxdimy * 2;

		float fontscaley = timerboxdimy * 0.35;
		float fontscalex = fontscaley * screenhwratio;
		float startx = timerboxdimx * 0.3;
		float xinc = timerboxdimx * 1.5;
		float x0 = startx;
		float x1 = startx + xinc;
		float x2 = startx + xinc * 2.0;
		float y0 = timerboxdimy * 0.5;
		float y1 = timerboxdimy * 0.9;

		timerboxverts.SetTo2DBox(x, y, w, h, timerboxdimx, timerboxdimy);
		timerboxdrawref.SetTextures(boxtex->GetId());
		timerboxdrawref.SetVertArray(&timerboxverts);
		timerboxdrawref.SetCull(false);
		timerboxdrawref.SetColor(1, 1, 1, opacity);
		timerboxdrawref.SetDrawOrder(0.1);

		laptime_label.Init(infonoderef, sansfont, str[LAPTIME], x0, y0, fontscalex, fontscaley);
		laptime_label.SetDrawOrder(infonoderef, 0.2);

		lastlaptime_label.Init(infonoderef, sansfont, str[LASTLAP], x1, y0, fontscalex, fontscaley);
		lastlaptime_label.SetDrawOrder(infonoderef, 0.2);

		bestlaptime_label.Init(infonoderef, sansfont, str[BESTLAP], x2, y0, fontscalex, fontscaley);
		bestlaptime_label.SetDrawOrder(infonoderef, 0.2);

		fontscaley = timerboxdimy * 0.4;
		fontscalex = fontscaley * screenhwratio;

		laptime.Init(infonoderef, lcdfont, "", x0, y1, fontscalex, fontscaley);
		laptime.SetDrawOrder(infonoderef, 0.2);

		lastlaptime.Init(infonoderef, lcdfont, "", x1, y1, fontscalex, fontscaley);
		lastlaptime.SetDrawOrder(infonoderef, 0.2);

		bestlaptime.Init(infonoderef, lcdfont, "", x2, y1, fontscalex, fontscaley);
		bestlaptime.SetDrawOrder(infonoderef, 0.2);
	}

	{
		infoboxdraw = AddDrawable(infonoderef);
		Drawable & infoboxdrawref = GetDrawable(infonoderef, infoboxdraw);

		float infoboxdimx = 60.0 / displaywidth;
		float infoboxdimy = 60.0 / displayheight;
		float totalsizex = infoboxdimx * 6.05;
		float totalsizey = infoboxdimy * 2.0;
		float x = 1.0 - (totalsizex * 0.5 - infoboxdimx * 0.65);
		float y = totalsizey * 0.5 - infoboxdimy * 0.25;
		float w = totalsizex - infoboxdimx * 2;
		float h = totalsizey - infoboxdimy * 2;

		float fontscaley = infoboxdimy * 0.35;
		float fontscalex = fontscaley * screenhwratio;
		float startx = 1.0 - totalsizex * 0.75 + infoboxdimx * 0.3;
		float xinc = infoboxdimx * 1.5;
		float x0 = startx;
		float x1 = startx + xinc;
		float x2 = startx + xinc * 2.0;
		float y0 = infoboxdimy * 0.5;
		float y1 = infoboxdimy * 0.9;

		infoboxverts.SetTo2DBox(x, y, w, h, infoboxdimx, infoboxdimy);
		infoboxdrawref.SetTextures(boxtex->GetId());
		infoboxdrawref.SetVertArray(&infoboxverts);
		infoboxdrawref.SetCull(false);
		infoboxdrawref.SetColor(1, 1, 1, opacity);
		infoboxdrawref.SetDrawOrder(0.1);

		place_label.Init(infonoderef, sansfont, str[PLACE], x0, y0, fontscalex, fontscaley);
		place_label.SetDrawOrder(infonoderef, 0.2);

		lap_label.Init(infonoderef, sansfont, str[LAP], x1, y0, fontscalex, fontscaley);
		lap_label.SetDrawOrder(infonoderef, 0.2);

		drift_label.Init(infonoderef, sansfont, str[SCORE], x2, y0, fontscalex, fontscaley);
		drift_label.SetDrawOrder(infonoderef, 0.2);

		fontscaley = infoboxdimy * 0.4;
		fontscalex = fontscaley * screenhwratio;

		placeindicator.Init(infonoderef, lcdfont, "-/-", x0, y1, fontscalex, fontscaley);
		placeindicator.SetDrawOrder(infonoderef, 0.2);

		lapindicator.Init(infonoderef, lcdfont, "-/-", x1, y1, fontscalex, fontscaley);
		lapindicator.SetDrawOrder(infonoderef, 0.2);

		driftscoreindicator.Init(infonoderef, lcdfont, "0", x2, y1, fontscalex, fontscaley);
		driftscoreindicator.SetDrawOrder(infonoderef, 0.2);
	}

	{
		float fontscaley = barheight * 0.5;
		float fontscalex = screenhwratio * fontscaley;
		float x = 0.5;
		float y = 0.5;
		raceprompt.Init(hudroot, sansfont, "", x, y, fontscalex, fontscaley);
		raceprompt.SetDrawOrder(hudroot, 1.0);
		raceprompt.SetColor(hudroot, 1, 0, 0);
	}

	{
		float fontscaley = 0.02;
		float fontscalex = screenhwratio * fontscaley;
		float y = 2 * fontscaley + 60.0 / displayheight;

		debugnode = hudroot.AddNode();
		SceneNode & debugnoderef = hudroot.GetNode(debugnode);
		debugtextdraw1 = SetupText(debugnoderef, debugtext1, "", sansfont, 0.01, y, fontscalex, fontscaley, 1, 1, 1, 10);
		debugtextdraw2 = SetupText(debugnoderef, debugtext2, "", sansfont, 0.25, y, fontscalex, fontscaley, 1, 1, 1, 10);
		debugtextdraw3 = SetupText(debugnoderef, debugtext3, "", sansfont, 0.50, y, fontscalex, fontscaley, 1, 1, 1, 10);
		debugtextdraw4 = SetupText(debugnoderef, debugtext4, "", sansfont, 0.75, y, fontscalex, fontscaley, 1, 1, 1, 10);
	}

#ifndef GAUGES
	TextureInfo texinfo;
	texinfo.mipmap = false;
	texinfo.repeatu = false;
	texinfo.repeatv = false;
	content.load(bartex, texturepath, "hudbox.png", texinfo);
	content.load(progbartex, texturepath, "progressbar.png", texinfo);

	rpmbar = AddDrawable(hudroot);
	rpmredbar = AddDrawable(hudroot);
	rpmbox = AddDrawable(hudroot);

	Drawable & rpmboxref = GetDrawable(hudroot, rpmbox);
	rpmboxref.SetTextures(progbartex->GetId());
	rpmboxref.SetVertArray(&rpmboxverts);
	rpmboxref.SetDrawOrder(2);
	rpmboxref.SetCull(false);
	rpmboxref.SetColor(0.3, 0.3, 0.3, 0.4);

	Drawable & rpmbarref = GetDrawable(hudroot, rpmbar);
	rpmbarref.SetTextures(progbartex->GetId());
	rpmbarref.SetVertArray(&rpmbarverts);
	rpmbarref.SetDrawOrder(3);
	rpmbarref.SetCull(false);
	rpmbarref.SetColor(1.0, 1.0, 1.0, 0.7);

	Drawable & rpmredbarref = GetDrawable(hudroot, rpmredbar);
	rpmredbarref.SetTextures(progbartex->GetId());
	rpmredbarref.SetVertArray(&rpmredbarverts);
	rpmredbarref.SetColor(1.0, 0.2, 0.2, 0.7);
	rpmredbarref.SetDrawOrder(3);
	rpmredbarref.SetCull(false);

	//lower left bar
	bars.push_back(HudBar());
	bars.back().Set(hudroot, bartex, 0.0 + barwidth * 0.5, 1.0-barheight*0.5, barwidth, barheight, opacity, false);

	//lower right bar
	bars.push_back(HudBar());
	bars.back().Set(hudroot, bartex, 1.0 - barwidth * 0.175, 1.0-barheight*0.5, barwidth, barheight, opacity, false);

	{
		float fontscaley = barheight * 0.5;
		float fontscalex = screenhwratio * fontscaley;
		float y = 1.0 - fontscaley * 0.5;
		float x0 = screenhwratio * 0.02;
		float x1 = 1.0 - screenhwratio * 0.02;

		geartextdraw = SetupText(hudroot, geartext, "N", lcdfont, x0, y, fontscalex, fontscaley, 1, 1, 1, 4);
		mphtextdraw = SetupText(hudroot, mphtext, "0", lcdfont, x1, y, fontscalex, fontscaley, 1, 1, 1, 4);
	}

	{
		float fontscaley = barheight * 0.25;
		float fontscalex = screenhwratio * fontscaley;
		float x0 = 1 - barwidth * 0.6;
		float x1 = 1 - barwidth * 0.7;
		float y0 = 1 - fontscaley * 1.25;
		float y1 = 1 - fontscaley * 0.5;

		abs.Init(hudroot, sansfont, "ABS", x0, y0, fontscalex, fontscaley);
		abs.SetDrawOrder(hudroot, 4);
		abs.SetColor(hudroot, 0, 1, 0);

		tcs.Init(hudroot, sansfont, "TCS", x0, y1, fontscalex, fontscaley);
		tcs.SetDrawOrder(hudroot, 4);
		tcs.SetColor(hudroot, 1, 0.77, 0.23);

		gas.Init(hudroot, sansfont, "GAS", x1, y0, fontscalex, fontscaley);
		gas.SetDrawOrder(hudroot, 4);
		gas.SetColor(hudroot, 1, 0, 0);

		nos.Init(hudroot, sansfont, "NOS", x1, y1, fontscalex, fontscaley);
		nos.SetDrawOrder(hudroot, 4);
		nos.SetColor(hudroot, 0, 1, 0);
	}
#else
	{
        FONT & gaugefont = sansfont_noshader;

		// gauge texture
		char white[] = {255, 255, 255, 255};
		TEXTUREINFO tinfo;
		tinfo.data = white;
		tinfo.width = 1;
		tinfo.height = 1;
		tinfo.bytespp = 4;
		tinfo.mipmap = false;
		content.load(bartex, "", "white1x1", tinfo);

		float r = 0.12;
		float x0 = 0.15;
		float x1 = 0.85;
		float y0 = 0.85;
		float h0 = r * 0.25;
		float w0 = screenhwratio * h0;
		float angle_min = 315.0 / 180.0 * M_PI;
		float angle_max = 45.0 / 180.0 * M_PI;

		rpmgauge.Set(hudroot, bartex, gaugefont, screenhwratio, x0, y0, r,
			angle_min, angle_max, 0, maxrpm * 0.001, 1);

		speedgauge.Set(hudroot, bartex, gaugefont, screenhwratio, x1, y0, r,
			angle_min, angle_max, 0, maxspeed * speedscale, 10);

		float w = w0;
		float h = h0;
		float x = x0 - gaugefont.GetWidth("rpm") * w * 0.5;
		float y = y0 - r * 0.5;
		Drawable & rpmd = GetDrawable(hudroot, AddDrawable(hudroot));
		rpmlabel.Set(rpmd, gaugefont, "rpm", x, y, w, h, 1, 1, 1);

		w = w0 * 0.65;
		h = h0 * 0.65;
		x = x0 - gaugefont.GetWidth("x1000") * w * 0.5;
		y = y0 - r * 0.5 + h;
		Drawable & x1000d = GetDrawable(hudroot, AddDrawable(hudroot));
		rpmxlabel.Set(x1000d, gaugefont, "x1000", x, y, w, h, 1, 1, 1);

		w = w0;
		h = h0;
		x = x1 - gaugefont.GetWidth("kph") * w * 0.5;
		y = y0 - r * 0.5;
		Drawable & spdd = GetDrawable(hudroot, AddDrawable(hudroot));
		speedlabel.Set(spdd, gaugefont, "kph", x, y, w, h, 1, 1, 1);

		w = w0 * 2;
		h = h0 * 2;
		x = x0 - w * 0.25;
		y = y0 + r * 0.64;
		geartextdraw = SetupText(hudroot, gaugefont, geartext, "N", x, y, w, h, 1, 1, 1);

		w = w0 * 1.5;
		h = h0 * 1.5;
		x = x1 - w * 0.3;
		y = y0 + r * 0.68;
		mphtextdraw = SetupText(hudroot, gaugefont, mphtext, "0", x, y, w, h, 1, 1, 1);
	}

	{
		float fontscaley = barheight * 0.25;
		float fontscalex = screenhwratio * fontscaley;
		float x0 = 1 - barwidth * 0.6;
		float x1 = 1 - barwidth * 0.7;
		float y0 = 1 - fontscaley * 1.25;
		float y1 = 1 - fontscaley * 0.5;

		abs.Init(hudroot, sansfont, "ABS", x0, y0, fontscalex, fontscaley);
		abs.SetDrawOrder(hudroot, 4);
		abs.SetColor(hudroot, 0, 1, 0);

		tcs.Init(hudroot, sansfont, "TCS", x0, y1, fontscalex, fontscaley);
		tcs.SetDrawOrder(hudroot, 4);
		tcs.SetColor(hudroot, 1, 0.77, 0.23);

		gas.Init(hudroot, sansfont, "GAS", x1, y0, fontscalex, fontscaley);
		gas.SetDrawOrder(hudroot, 4);
		gas.SetColor(hudroot, 1, 0, 0);

		nos.Init(hudroot, sansfont, "NOS", x1, y1, fontscalex, fontscaley);
		nos.SetDrawOrder(hudroot, 4);
		nos.SetColor(hudroot, 0, 1, 0);
	}
#endif

	SetVisible(false);

	return true;
}

void Hud::Update(
	const Font & sansfont, const Font & lcdfont,
	float displaywidth, float displayheight,
	float curlap, float lastlap, float bestlap, float stagingtimeleft,
	int curlapnum, int numlaps, int curplace, int numcars, bool mph,
	float rpm, float redrpm, float maxrpm,
	float speed, float maxspeed, float clutch, int newgear,
	float fuelamount, float nosamount, bool nosactive,
	bool absenabled, bool absactive, bool tcsenabled, bool tcsactive,
	bool drifting, float driftscore, float thisdriftscore,
	const std::string & debug_string1, const std::string & debug_string2,
	const std::string & debug_string3, const std::string & debug_string4)
{
	if (!lastvisible)
		return;

	bool debuginfo_new = !debug_string1.empty() || !debug_string2.empty() ||
						 !debug_string3.empty() || !debug_string4.empty();
	if (debuginfo_new != debuginfo)
	{
		debuginfo = debuginfo_new;
		hudroot.GetNode(debugnode).SetChildVisibility(debuginfo);
	}
	if (debuginfo)
	{
		debugtext1.Revise(sansfont, debug_string1);
		debugtext2.Revise(sansfont, debug_string2);
		debugtext3.Revise(sansfont, debug_string3);
		debugtext4.Revise(sansfont, debug_string4);
	}

	float screenhwratio = displayheight / displaywidth;

#ifdef GAUGES
    FONT & gaugefont = sansfont_noshader;

	if (fabs(this->maxrpm - maxrpm) > 1)
	{
		this->maxrpm = maxrpm;
		rpmgauge.Revise(hudroot, gaugefont, 0, maxrpm * 0.001, 1);
	}
	if (fabs(this->maxspeed - maxspeed) > 1 || this->mph != mph)
	{
		this->maxspeed = maxspeed;
		this->mph = mph;
		if (mph)
		{
			speedlabel.Revise(gaugefont, "mph");
			speedscale = 2.23693629;
		}
		else
		{
			speedlabel.Revise(gaugefont, "kph");
			speedscale = 3.6;
		}
		speedgauge.Revise(hudroot, gaugefont, 0, maxspeed * speedscale, 10);
	}
	rpmgauge.Update(hudroot, rpm * 0.001);
	speedgauge.Update(hudroot, fabs(speed) * speedscale);

	// gear
	std::ostringstream gearstr;
	if (newgear == -1)
		gearstr << "R";
	else if (newgear == 0)
		gearstr << "N";
	else
		gearstr << newgear;
	geartext.Revise(gaugefont, gearstr.str());

	float geartext_alpha = clutch * 0.5 + 0.5;
	if (newgear == 0) geartext_alpha = 1;
	Drawable & geartextdrawref = hudroot.GetDrawList().text.get(geartextdraw);
	geartextdrawref.SetColor(1, 1, 1, geartext_alpha);

	// speed
	std::ostringstream sstr;
	sstr << std::abs(int(speed * speedscale));
	//float sx = mphtext.GetScale().first;
	//float sy = mphtext.GetScale().second;
	//float w = gaugefont.GetWidth(sstr.str()) * sx;
	//float x = 1 - w;
	//float y = 1 - sy * 0.5;
	mphtext.Revise(gaugefont, sstr.str());//, x, y, fontscalex, fontscaley);
#else
	std::ostringstream gearstr;
	if (newgear == -1)
		gearstr << "R";
	else if (newgear == 0)
		gearstr << "N";
	else
		gearstr << newgear;
	geartext.Revise(lcdfont, gearstr.str());

	float geartext_alpha = (newgear == 0) ? 1 : clutch * 0.5 + 0.5;
	Drawable & geartextdrawref = hudroot.GetDrawList().text.get(geartextdraw);
	geartextdrawref.SetColor(1, 1, 1, geartext_alpha);

	float rpmpercent = std::min(1.0f, rpm / maxrpm);
	float rpmredpoint = redrpm / maxrpm;
	float rpmxstart = 60.0 / displaywidth;
	float rpmwidth = 200.0 / displaywidth;
	float rpmredx = rpmwidth * rpmredpoint + rpmxstart;
	float rpmy = 1.0 - 26.0 / displayheight;
	float rpmheight = 20.0 / displayheight;
	float rpmxend = rpmxstart + rpmwidth * rpmredpoint;
	float rpmrealend = rpmxstart + rpmwidth * rpmpercent;
	if (rpmxend > rpmrealend)
		rpmxend = rpmrealend;
	float rpmredxend = rpmrealend;
	if (rpmrealend < rpmredx)
		rpmredxend = rpmredx;
	rpmbarverts.SetToBillboard(rpmxstart, rpmy, rpmxend, rpmy + rpmheight);
	rpmredbarverts.SetToBillboard(rpmredx, rpmy, rpmredxend, rpmy + rpmheight);
	rpmboxverts.SetToBillboard(rpmxstart, rpmy, rpmxstart + rpmwidth, rpmy + rpmheight);

	std::ostringstream speedo;
	if (mph)
		speedo << std::abs((int)(2.23693629 * speed)) << " " << str[MPH];
	else
		speedo << std::abs((int)(3.6 * speed)) << " " << str[KPH];
	float fontscalex = mphtext.GetScale().first;
	float fontscaley = mphtext.GetScale().second;
	float speedotextwidth = lcdfont.GetWidth(speedo.str()) * fontscalex;
	float x = 1.0 - screenhwratio * 0.02 - speedotextwidth;
	float y = 1 - fontscaley * 0.5;
	mphtext.Revise(lcdfont, speedo.str(), x, y, fontscalex, fontscaley);
#endif
	//update ABS alpha value
	if (!absenabled)
	{
		abs.SetAlpha(hudroot, 0.0);
	}
	else
	{
		if (absactive)
			abs.SetAlpha(hudroot, 1.0);
		else
			abs.SetAlpha(hudroot, 0.2);
	}

	//update TCS alpha value
	if (!tcsenabled)
	{
		tcs.SetAlpha(hudroot, 0.0);
	}
	else
	{
		if (tcsactive)
			tcs.SetAlpha(hudroot, 1.0);
		else
			tcs.SetAlpha(hudroot, 0.2);
	}

	//update GAS indicator
	if (fuelamount > 0)
	{
		gas.SetAlpha(hudroot, 0.0);
	}
	else
	{
		gas.SetAlpha(hudroot, 1.0);
	}

	//update NOS indicator
	if (nosamount > 0)
	{
		if (nosactive)
			nos.SetAlpha(hudroot, 1.0);
		else
			nos.SetAlpha(hudroot, 0.2);
	}
	else
	{
		nos.SetAlpha(hudroot, 0.0);
	}

	//update timer info
	{
		std::string tempstr;
		GetTimeString(curlap, tempstr);
		laptime.Revise(tempstr);
		GetTimeString(lastlap, tempstr);
		lastlaptime.Revise(tempstr);
		GetTimeString(bestlap, tempstr);
		bestlaptime.Revise(tempstr);
	}

	std::string rps;
	if (numlaps > 0)
	{
		//update lap
		std::ostringstream lapstream;
		lapstream << std::max(1, std::min(curlapnum, numlaps)) << "/" << numlaps;
		lapindicator.Revise(lapstream.str());

		//update place
		std::ostringstream stream;
		stream << curplace << "/" << numcars;
		placeindicator.Revise(stream.str());

		//update race prompt
		if (stagingtimeleft > 0.5)
		{
			std::ostringstream s;
			s << (int)stagingtimeleft + 1;
			rps = s.str();
			raceprompt.SetColor(hudroot, 1,0,0);
			racecomplete = false;
		}
		else if (stagingtimeleft > 0.0)
		{
			rps = str[READY];
			raceprompt.SetColor(hudroot, 1,1,0);
		}
		else if (stagingtimeleft < 0.0f && stagingtimeleft > -1.0f) //stagingtimeleft needs to go negative to get the GO message
		{
			rps = str[GO];
			raceprompt.SetColor(hudroot, 0,1,0);
		}
		else if (curlapnum > numlaps)
		{
			if (curplace == 1)
			{
				rps = str[YOUWON];
				raceprompt.SetColor(hudroot, 0,1,0);
			}
			else
			{
				rps = str[YOULOST];
				raceprompt.SetColor(hudroot, 1,0,0);
			}
			raceprompt.Revise(rps);
			float width = raceprompt.GetWidth();
			raceprompt.SetPosition(0.5 - width * 0.5, 0.4);
			raceprompt.SetDrawEnable(hudroot, true);
			racecomplete = true;
		}
	}

	if (!racecomplete)
	{
		//update drift score
		std::ostringstream scorestream;
		scorestream << (int)driftscore;
		driftscoreindicator.Revise(scorestream.str());

		if (drifting && rps.empty())
		{
			std::ostringstream s;
			s << "+" << (int)thisdriftscore;
			rps = s.str();
			raceprompt.SetColor(hudroot, 1, 0, 0);
		}

		//update race prompt
		if (!rps.empty())
		{
			raceprompt.Revise(rps);
			float width = raceprompt.GetWidth();
			raceprompt.SetPosition(0.5 - width * 0.5, 0.4);
			raceprompt.SetDrawEnable(hudroot, true);
		}
		else
		{
			raceprompt.SetDrawEnable(hudroot, false);
		}
	}
}

SceneNode & Hud::GetNode()
{
	return hudroot;
}

void Hud::SetVisible(bool value)
{
	if (value != lastvisible)
	{
		hudroot.SetChildVisibility(value);
		//hudroot.GetNode(timernode).SetChildVisibility(value);
		hudroot.GetNode(debugnode).SetChildVisibility(debuginfo && value);
		lastvisible = value;
	}
}
