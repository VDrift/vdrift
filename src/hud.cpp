#include "hud.h"
#include "contentmanager.h"
#include "texture.h"

//#define GAUGES

static keyed_container<DRAWABLE>::handle AddDrawable(SCENENODE & node)
{
	return node.GetDrawlist().twodim.insert(DRAWABLE());
}

static DRAWABLE & GetDrawable(SCENENODE & node, const keyed_container <DRAWABLE>::handle & drawhandle)
{
	return node.GetDrawlist().twodim.get(drawhandle);
}

static keyed_container <DRAWABLE>::handle SetupText(
	SCENENODE & parent, FONT & font,
	TEXT_DRAW & textdraw, const std::string & str,
	float x, float y, float scalex, float scaley,
	float r, float g , float b, float zorder = 0)
{
	keyed_container<DRAWABLE>::handle draw = parent.GetDrawlist().text.insert(DRAWABLE());
	DRAWABLE & drawref = parent.GetDrawlist().text.get(draw);
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

HUD::HUD() :
	maxrpm(9000),
	maxspeed(240),
	speedscale(3.6),
	mph(false),
	debug_hud_info(false),
	racecomplete(false),
	lastvisible(true)
{
	// ctor
}

bool HUD::Init(
	const std::string & texturepath,
	ContentManager & content,
	FONT & lcdfont,
	FONT & sansfont,
    FONT & sansfont_noshader,
	float displaywidth,
	float displayheight,
	bool debugon,
	std::ostream & error_output)
{
	const float opacity = 0.8;
	const float screenhwratio = displayheight / displaywidth;
	const float barheight = 64.0 / displayheight;
	const float barwidth = 256.0 / displaywidth;

	TEXTUREINFO texinfo;
	texinfo.mipmap = false;
	texinfo.repeatu = false;
	texinfo.repeatv = false;

	float timerbox_lowery = 0;
	{
		timernode = hudroot.AddNode();
		SCENENODE & timernoderef = hudroot.GetNode(timernode);

		float timerboxdimx = 96.0 / displaywidth;
		float timerboxdimy = 64.0 / displayheight;
		timerboxdraw = AddDrawable(timernoderef);
		DRAWABLE & timerboxdrawref = GetDrawable(timernoderef, timerboxdraw);

		TEXTUREINFO timerboxtexinfo;
		timerboxtexinfo.mipmap = false;
		timerboxtexinfo.repeatu = true;
		timerboxtexinfo.repeatv = false;
		std::tr1::shared_ptr<TEXTURE> timerboxtex;
		if (!content.load(texturepath, "timerbox.png", timerboxtexinfo, timerboxtex)) return false;

		float totalsizex = timerboxdimx * 6.05;
		float totalsizey = timerboxdimy * 2.0;
		float x = totalsizex * 0.5 - timerboxdimx * 0.65;
		float y = totalsizey * 0.5 - timerboxdimy * 0.25;
		float w = totalsizex - timerboxdimx * 2;
		float h = totalsizey - timerboxdimy * 2;
		timerboxverts.SetTo2DBox(x, y, w, h, timerboxdimx, timerboxdimy);
		timerbox_lowery = y + timerboxdimy * 0.5;

		timerboxdrawref.SetDiffuseMap(timerboxtex);
		timerboxdrawref.SetVertArray(&timerboxverts);
		timerboxdrawref.SetCull(false, false);
		timerboxdrawref.SetColor(1, 1, 1, opacity);
		timerboxdrawref.SetDrawOrder(0.1);

		float fontscaley = timerboxdimy * 0.4;
		float fontscalex = fontscaley * screenhwratio;
		float startx = timerboxdimx * 0.45 - timerboxdimx * 0.15;
		float xinc = timerboxdimx * 1.5;

		laptime_label.Init(timernoderef, sansfont, "Lap time:", startx, timerboxdimy*0.9-timerboxdimy*0.3, fontscalex, fontscaley);
		laptime_label.SetDrawOrder(timernoderef, 0.2);

		lastlaptime_label.Init(timernoderef, sansfont, "Last lap:", startx+xinc, timerboxdimy*.9-timerboxdimy*0.3, fontscalex, fontscaley);
		lastlaptime_label.SetDrawOrder(timernoderef, 0.2);

		bestlaptime_label.Init(timernoderef, sansfont, "Best lap:", startx+xinc*2.0, timerboxdimy*.9-timerboxdimy*0.3, fontscalex, fontscaley);
		bestlaptime_label.SetDrawOrder(timernoderef, 0.2);

		laptime.Init(timernoderef, lcdfont, "", startx, timerboxdimy*1.2-timerboxdimy*0.3, fontscalex, fontscaley);
		laptime.SetDrawOrder(timernoderef, 0.2);

		lastlaptime.Init(timernoderef, lcdfont, "", startx+xinc, timerboxdimy*1.2-timerboxdimy*0.3, fontscalex, fontscaley);
		lastlaptime.SetDrawOrder(timernoderef, 0.2);

		bestlaptime.Init(timernoderef, lcdfont, "", startx+xinc*2.0, timerboxdimy*1.2-timerboxdimy*0.3, fontscalex, fontscaley);
		bestlaptime.SetDrawOrder(timernoderef, 0.2);
	}

	{
		float fontscaley = barheight * 0.5;
		float fontscalex = screenhwratio * fontscaley;
		float x = fontscalex * 0.25;
		float y = timerbox_lowery + fontscaley;
		driftscoreindicator.Init(hudroot, sansfont, "", x, y, fontscalex, fontscaley);
		driftscoreindicator.SetDrawOrder(hudroot, 0.2);
	}

	{
		float fontscaley = barheight * 0.5;
		float fontscalex = screenhwratio * fontscaley;
		float x = fontscalex * 0.25;
		float y = timerbox_lowery + fontscaley * 2;
		lapindicator.Init(hudroot, sansfont, "", x, y, fontscalex, fontscaley);
		lapindicator.SetDrawOrder(hudroot, 0.2);
	}

	{
		float fontscaley = barheight * 0.5;
		float fontscalex = screenhwratio * fontscaley;
		float x = fontscalex * 0.25;
		float y = timerbox_lowery + fontscaley * 3;
		placeindicator.Init(hudroot, sansfont, "", x, y, fontscalex, fontscaley);
		placeindicator.SetDrawOrder(hudroot, 0.2);
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

		debugnode = hudroot.AddNode();
		SCENENODE & debugnoderef = hudroot.GetNode(debugnode);
		debugtextdraw1 = SetupText(debugnoderef, sansfont, debugtext1, "", 0.01, fontscaley, fontscalex, fontscaley, 1, 1, 1, 10);
		debugtextdraw2 = SetupText(debugnoderef, sansfont, debugtext2, "", 0.25, fontscaley, fontscalex, fontscaley, 1, 1, 1, 10);
		debugtextdraw3 = SetupText(debugnoderef, sansfont, debugtext3, "", 0.5, fontscaley, fontscalex, fontscaley, 1, 1, 1, 10);
		debugtextdraw4 = SetupText(debugnoderef, sansfont, debugtext4, "", 0.75, fontscaley, fontscalex, fontscaley, 1, 1, 1, 10);
	}

#ifndef GAUGES
	std::tr1::shared_ptr<TEXTURE> bartex, progbartex;
	if (!content.load(texturepath, "hudbox.png", texinfo, bartex)) return false;
	if (!content.load(texturepath, "progressbar.png", texinfo, progbartex)) return false;

	rpmbar = AddDrawable(hudroot);
	rpmredbar = AddDrawable(hudroot);
	rpmbox = AddDrawable(hudroot);

	DRAWABLE & rpmboxref = GetDrawable(hudroot, rpmbox);
	rpmboxref.SetDiffuseMap(progbartex);
	rpmboxref.SetVertArray(&rpmboxverts);
	rpmboxref.SetDrawOrder(2);
	rpmboxref.SetCull(false, false);
	rpmboxref.SetColor(0.3, 0.3, 0.3, 0.4);

	DRAWABLE & rpmbarref = GetDrawable(hudroot, rpmbar);
	rpmbarref.SetDiffuseMap(progbartex);
	rpmbarref.SetVertArray(&rpmbarverts);
	rpmbarref.SetDrawOrder(3);
	rpmbarref.SetCull(false, false);
	rpmbarref.SetColor(1.0, 1.0, 1.0, 0.7);

	DRAWABLE & rpmredbarref = GetDrawable(hudroot, rpmredbar);
	rpmredbarref.SetDiffuseMap(progbartex);
	rpmredbarref.SetVertArray(&rpmredbarverts);
	rpmredbarref.SetColor(1.0, 0.2, 0.2, 0.7);
	rpmredbarref.SetDrawOrder(3);
	rpmredbarref.SetCull(false, false);

	//lower left bar
	bars.push_back(HUDBAR());
	bars.back().Set(hudroot, bartex, 0.0 + barwidth * 0.5, 1.0-barheight*0.5, barwidth, barheight, opacity, false);

	//lower right bar
	bars.push_back(HUDBAR());
	bars.back().Set(hudroot, bartex, 1.0 - barwidth * 0.175, 1.0-barheight*0.5, barwidth, barheight, opacity, false);

	{
		float fontscaley = barheight * 0.5;
		float fontscalex = screenhwratio * fontscaley;
		float y = 1.0 - fontscaley * 0.5;
		float x0 = screenhwratio * 0.02;
		float x1 = 1.0 - screenhwratio * 0.02;

		geartextdraw = SetupText(hudroot, lcdfont, geartext, "N", x0, y, fontscalex, fontscaley, 1, 1, 1, 4);
		mphtextdraw = SetupText(hudroot, lcdfont, mphtext, "0", x1, y, fontscalex, fontscaley, 1, 1, 1, 4);
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
		std::tr1::shared_ptr<TEXTURE> texture;
		content.load("", "white1x1", tinfo, texture);

		float r = 0.12;
		float x0 = 0.15;
		float x1 = 0.85;
		float y0 = 0.85;
		float h0 = r * 0.25;
		float w0 = screenhwratio * h0;
		float angle_min = 315.0 / 180.0 * M_PI;
		float angle_max = 45.0 / 180.0 * M_PI;

		rpmgauge.Set(hudroot, texture, gaugefont, screenhwratio, x0, y0, r,
			angle_min, angle_max, 0, maxrpm * 0.001, 1);

		speedgauge.Set(hudroot, texture, gaugefont, screenhwratio, x1, y0, r,
			angle_min, angle_max, 0, maxspeed * speedscale, 10);

		float w = w0;
		float h = h0;
		float x = x0 - gaugefont.GetWidth("rpm") * w * 0.5;
		float y = y0 - r * 0.5;
		DRAWABLE & rpmd = GetDrawable(hudroot, AddDrawable(hudroot));
		rpmlabel.Set(rpmd, gaugefont, "rpm", x, y, w, h, 1, 1, 1);

		w = w0 * 0.65;
		h = h0 * 0.65;
		x = x0 - gaugefont.GetWidth("x1000") * w * 0.5;
		y = y0 - r * 0.5 + h;
		DRAWABLE & x1000d = GetDrawable(hudroot, AddDrawable(hudroot));
		rpmxlabel.Set(x1000d, gaugefont, "x1000", x, y, w, h, 1, 1, 1);

		w = w0;
		h = h0;
		x = x1 - gaugefont.GetWidth("kph") * w * 0.5;
		y = y0 - r * 0.5;
		DRAWABLE & spdd = GetDrawable(hudroot, AddDrawable(hudroot));
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

	debug_hud_info = debugon;

	return true;
}

void HUD::Update(
	FONT & lcdfont, FONT & sansfont, FONT & sansfont_noshader, float displaywidth, float displayheight,
	float curlap, float lastlap, float bestlap, float stagingtimeleft,
	int curlapnum, int numlaps, int curplace, int numcars,
	float rpm, float redrpm, float maxrpm,
	float speed, float maxspeed, bool mph, float clutch, int newgear,
	const std::string & debug_string1, const std::string & debug_string2,
	const std::string & debug_string3, const std::string & debug_string4,
	bool absenabled, bool absactive, bool tcsenabled, bool tcsactive,
	bool outofgas, bool nosactive, float nosamount,
	bool drifting, float driftscore, float thisdriftscore)
{
	if (!lastvisible)
		return;

	float screenhwratio = displayheight/displaywidth;

	if (debug_hud_info)
	{
		debugtext1.Revise(sansfont, debug_string1);
		debugtext2.Revise(sansfont, debug_string2);
		debugtext3.Revise(sansfont, debug_string3);
		debugtext4.Revise(sansfont, debug_string4);
	}
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
	std::stringstream gearstr;
	if (newgear == -1)
		gearstr << "R";
	else if (newgear == 0)
		gearstr << "N";
	else
		gearstr << newgear;
	geartext.Revise(gaugefont, gearstr.str());

	float geartext_alpha = clutch * 0.5 + 0.5;
	if (newgear == 0) geartext_alpha = 1;
	DRAWABLE & geartextdrawref = hudroot.GetDrawlist().text.get(geartextdraw);
	geartextdrawref.SetColor(1, 1, 1, geartext_alpha);

	// speed
	std::stringstream sstr;
	sstr << std::abs(int(speed * speedscale));
	//float sx = mphtext.GetScale().first;
	//float sy = mphtext.GetScale().second;
	//float w = gaugefont.GetWidth(sstr.str()) * sx;
	//float x = 1 - w;
	//float y = 1 - sy * 0.5;
	mphtext.Revise(gaugefont, sstr.str());//, x, y, fontscalex, fontscaley);
#else
	std::stringstream gearstr;
	if (newgear == -1)
		gearstr << "R";
	else if (newgear == 0)
		gearstr << "N";
	else
		gearstr << newgear;
	geartext.Revise(lcdfont, gearstr.str());

	float geartext_alpha = (newgear == 0) ? 1 : clutch * 0.5 + 0.5;
	DRAWABLE & geartextdrawref = hudroot.GetDrawlist().text.get(geartextdraw);
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

	std::stringstream speedo;
	if (mph)
		speedo << std::abs((int)(2.23693629 * speed)) << " MPH";
	else
		speedo << std::abs((int)(3.6 * speed)) << " KPH";
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
	if (outofgas)
	{
		gas.SetAlpha(hudroot, 1.0);
	}
	else
	{
		gas.SetAlpha(hudroot, 0.0);
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

	//update drift score
	if (numlaps == 0) //this is how we determine practice mode, for now
	{
		std::stringstream scorestream;
		scorestream << "Score " << (int)driftscore;
		if (drifting)
		{
			scorestream << " + " << (int)thisdriftscore;
			driftscoreindicator.SetColor(hudroot, 1,0,0);
		}
		else
		{
			driftscoreindicator.SetColor(hudroot, 1,1,1);
		}
		driftscoreindicator.Revise(scorestream.str());
	}
	else
	{
		driftscoreindicator.SetDrawEnable(hudroot, false);
	}


	if (numlaps > 0)
	{
		//update lap
		std::stringstream lapstream;
		//std::cout << curlapnum << std::endl;
		lapstream << "Lap " << std::max(1, std::min(curlapnum, numlaps)) << "/" << numlaps;
		lapindicator.Revise(lapstream.str());

		//update place
		std::stringstream stream;
		stream << "Place " << curplace << "/" << numcars;
		placeindicator.Revise(stream.str());

		//update race prompt
		std::stringstream t;
		if (stagingtimeleft > 0.5)
		{
			t << ((int)stagingtimeleft)+1;
			raceprompt.SetColor(hudroot, 1,0,0);
			racecomplete = false;
		}
		else if (stagingtimeleft > 0.0)
		{
			t << "Ready";
			raceprompt.SetColor(hudroot, 1,1,0);
		}
		else if (stagingtimeleft < 0.0f && stagingtimeleft > -1.0f) //stagingtimeleft needs to go negative to get the GO message
		{
			t << "GO";
			raceprompt.SetColor(hudroot, 0,1,0);
		}
		else if (curlapnum > numlaps && !racecomplete)
		{
			if (curplace == 1)
			{
				t << "You won!";
				raceprompt.SetColor(hudroot, 0,1,0);
			}
			else
			{
				t << "You lost";
				raceprompt.SetColor(hudroot, 1,0,0);
			}
			raceprompt.Revise(t.str());
			float width = raceprompt.GetWidth();
			raceprompt.SetPosition(0.5-width*0.5,0.5);
			racecomplete = true;
		}

		if (!racecomplete)
		{
			raceprompt.Revise(t.str());
			float width = raceprompt.GetWidth();
			raceprompt.SetPosition(0.5-width*0.5,0.5);
		}
	}
	else
	{
		lapindicator.SetDrawEnable(hudroot, false);
		placeindicator.SetDrawEnable(hudroot, false);
		raceprompt.SetDrawEnable(hudroot, false);
	}
}

SCENENODE & HUD::GetNode()
{
	return hudroot;
}

void HUD::SetVisible(bool value)
{
	if (value != lastvisible)
	{
		hudroot.SetChildVisibility(value);
		//hudroot.GetNode(timernode).SetChildVisibility(value);
		SetDebugVisible(value && debug_hud_info);
		lastvisible = value;
	}
}

void HUD::SetDebugVisible(bool value)
{
	hudroot.GetNode(debugnode).SetChildVisibility(value);
}
