#include "hud.h"

keyed_container <DRAWABLE>::handle AddDrawable(SCENENODE & node)
{
	return node.GetDrawlist().twodim.insert(DRAWABLE());
}

DRAWABLE & GetDrawable(SCENENODE & node, keyed_container <DRAWABLE>::handle & drawhandle)
{
	return node.GetDrawlist().twodim.get(drawhandle);
}

bool HUD::Init(
	const std::string & texturepath,
	const std::string & texsize,
	MANAGER<TEXTURE, TEXTUREINFO> & textures, 
	FONT & lcdfont,
	FONT & sansfont,
	float displaywidth,
	float displayheight,
	bool debugon,
	std::ostream & error_output)
{
    float opacity = 0.8;

    TEXTUREINFO bartexinfo(texturepath+"/hudbox.png");
    bartexinfo.SetMipMap(false);
    bartexinfo.SetRepeat(false, false);
    bartexinfo.SetSize(texsize);
    std::tr1::shared_ptr<TEXTURE> bartex = textures.Get(bartexinfo);
    if (!bartex->Loaded()) return false;

    TEXTUREINFO progbartexinfo(texturepath+"/progressbar.png");
    progbartexinfo.SetMipMap(false);
    progbartexinfo.SetRepeat(false, false);
    progbartexinfo.SetSize(texsize);
    std::tr1::shared_ptr<TEXTURE> progbartex = textures.Get(progbartexinfo);
    if (!progbartex->Loaded()) return false;

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

    float barheight = 64.0/displayheight;
    float barwidth = 256.0/displaywidth;

    //lower left bar
    bars.push_back(HUDBAR());
    bars.back().Set(hudroot, bartex, 0.0+barwidth*0.5, 1.0-barheight*0.5, barwidth, barheight, opacity, false);

    //lower right bar
    bars.push_back(HUDBAR());
    bars.back().Set(hudroot, bartex, 1.0-barwidth*0.14, 1.0-barheight*0.5, barwidth, barheight, opacity, false);

    float screenhwratio = displayheight/displaywidth;

    float timerbox_lowery = 0;
    //load the timer
    {
        timernode = hudroot.AddNode();
		SCENENODE & timernoderef = hudroot.GetNode(timernode);

        float timerboxdimx = 96.0/displaywidth;
        float timerboxdimy = 64.0/displayheight;
        timerboxdraw = AddDrawable(timernoderef);
		DRAWABLE & timerboxdrawref = GetDrawable(timernoderef, timerboxdraw);

        TEXTUREINFO timerboxtexinfo(texturepath+"/timerbox.png");
        timerboxtexinfo.SetMipMap(false);
        timerboxtexinfo.SetRepeat(true, false);
        timerboxtexinfo.SetSize(texsize);
        std::tr1::shared_ptr<TEXTURE> timerboxtex = textures.Get(timerboxtexinfo);
        if (!timerboxtex->Loaded()) return false;

        float totalsizex = timerboxdimx*6.05;
        float totalsizey = timerboxdimy*2.0;
        float x = -timerboxdimx*0.65+totalsizex*0.5;
        float y = totalsizey*0.5-timerboxdimy*0.3;
        timerboxverts.SetTo2DBox(x,y,totalsizex-timerboxdimx*2,totalsizey-timerboxdimy*2,
                     timerboxdimx,timerboxdimy);
        timerbox_lowery = y+timerboxdimy*0.5;

        timerboxdrawref.SetDiffuseMap(timerboxtex);
        timerboxdrawref.SetVertArray(&timerboxverts);
        timerboxdrawref.SetCull(false, false);
        timerboxdrawref.SetColor(1,1,1,opacity);
        timerboxdrawref.SetDrawOrder(0.1);

        float fontscaley = timerboxdimy*2.5;
        float fontscalex = fontscaley*screenhwratio;

        float startx = timerboxdimx*0.45-timerboxdimx*0.15;
        float xinc = timerboxdimx*1.5;

        laptime_label.Init(timernoderef, sansfont, "Lap time:", startx, timerboxdimy*.9-timerboxdimy*0.3, fontscalex, fontscaley);
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
        float fontscaley = 0.12;
        float fontscalex = screenhwratio*fontscaley;

		debugnode = hudroot.AddNode();
		SCENENODE & debugnoderef = hudroot.GetNode(debugnode);
        debugtextdraw1 = SetupText(debugnoderef, sansfont, debugtext1, "", 0.01,0.05, fontscalex,fontscaley, 1,1,1, 10);
        debugtextdraw2 = SetupText(debugnoderef, sansfont, debugtext2, "", 0.25,0.05, fontscalex,fontscaley, 1,1,1, 10);
        debugtextdraw3 = SetupText(debugnoderef, sansfont, debugtext3, "", 0.5,0.05, fontscalex,fontscaley, 1,1,1, 10);
        debugtextdraw4 = SetupText(debugnoderef, sansfont, debugtext4, "", 0.75,0.05, fontscalex,fontscaley, 1,1,1, 10);
    }

    {
        float fontscaley = barheight*3.0;
        float fontscalex = screenhwratio*fontscaley;

        geartextdraw = SetupText(hudroot, lcdfont, geartext, "N", screenhwratio*0.02,1.0-0.015, fontscalex,fontscaley, 1,1,1, 4);    
        mphtextdraw = SetupText(hudroot, lcdfont, mphtext, "0", 1.0-screenhwratio*0.02,1.0-0.01, fontscalex,fontscaley, 1,1,1, 4);
    }

    //load ABS and TCS indicators
    {
        float fontscaley = barheight*3.0*0.5;
        float fontscalex = screenhwratio*fontscaley;
		float xpos = barwidth * 0.63;
        abs.Init(hudroot, sansfont, "ABS", 1.0-xpos,1.0-0.025, fontscalex,fontscaley);
        abs.SetDrawOrder(hudroot, 4);
        abs.SetColor(hudroot, 0,1,0);

        tcs.Init(hudroot, sansfont, "TCS", 1.0-xpos,1.0-0.005, fontscalex,fontscaley);
        tcs.SetDrawOrder(hudroot, 4);
        tcs.SetColor(hudroot, 1,0.77,0.23);
    }

    {
        float fontscaley = barheight*3.0;
        float fontscalex = screenhwratio*fontscaley;
        lapindicator.Init(hudroot, sansfont, "", fontscalex*0.1, timerbox_lowery + fontscaley*0.2, fontscalex, fontscaley);
        lapindicator.SetDrawOrder(hudroot, 0.2);
    }
	
	{
		float fontscaley = barheight*3.0;
		float fontscalex = screenhwratio*fontscaley;
		driftscoreindicator.Init(hudroot, sansfont, "", fontscalex*0.1, timerbox_lowery + fontscaley*0.2, fontscalex, fontscaley);
		driftscoreindicator.SetDrawOrder(hudroot, 0.2);
	}

    {
        float fontscaley = barheight*3.0;
        float fontscalex = screenhwratio*fontscaley;
        placeindicator.Init(hudroot, sansfont, "", fontscalex*0.1, timerbox_lowery + fontscaley*0.4, fontscalex, fontscaley);
        placeindicator.SetDrawOrder(hudroot, 0.2);
    }

    {
        float fontscaley = barheight*7.0;
        float fontscalex = screenhwratio*fontscaley;
        raceprompt.Init(hudroot, sansfont, "", 0.5, 0.5, fontscalex, fontscaley);
        raceprompt.SetDrawOrder(hudroot, 1.0);
        raceprompt.SetColor(hudroot, 1,0,0);
    }

    Hide();
	
	debug_hud_info = debugon;

    return true;
}

void HUD::Update(FONT & lcdfont, FONT & sansfont, float curlap, float lastlap, float bestlap, float stagingtimeleft, int curlapnum,
		int numlaps, int curplace, int numcars, float clutch, int newgear, int newrpm, int redrpm, int maxrpm,
		float meterspersecond,
		bool mph, const std::string & debug_string1, const std::string & debug_string2,
		const std::string & debug_string3, const std::string & debug_string4, float displaywidth,
		float displayheight, bool absenabled, bool absactive, bool tcsenabled, bool tcsactive,
		bool drifting, float driftscore, float thisdriftscore)
{
    float screenhwratio = displayheight/displaywidth;

    if (debug_hud_info)
    {
		debugtext1.Revise(sansfont, debug_string1);
        debugtext2.Revise(sansfont, debug_string2);
        debugtext3.Revise(sansfont, debug_string3);
        debugtext4.Revise(sansfont, debug_string4);
    }

    //geartextdraw->SetDrawEnable(true);
    std::stringstream gearstr;
    if (newgear == -1)
        gearstr << "R";
    else if (newgear == 0)
        gearstr << "N";
    else
        gearstr << newgear;
	DRAWABLE & geartextdrawref = hudroot.GetDrawlist().text.get(geartextdraw);
    geartext.Revise(lcdfont, gearstr.str());
    if (newgear == 0)
        geartextdrawref.SetColor(1,1,1,1);
    else
        geartextdrawref.SetColor(1,1,1,clutch*0.5+0.5);

    float rpmpercent = newrpm / (float) maxrpm;
    if (rpmpercent > 1.0)
        rpmpercent = 1.0;
    float rpmredpoint = redrpm / (float) maxrpm;
    float rpmxstart = 60.0/displaywidth;
    float rpmwidth = 200.0/displaywidth;
    float rpmredx = rpmwidth*rpmredpoint + rpmxstart;
    float rpmy = 1.0-26.0/displayheight;
    float rpmheight = 20.0/displayheight;
    float rpmxend = rpmxstart+rpmwidth*rpmredpoint;
    float rpmrealend = rpmxstart+rpmwidth*rpmpercent;
    if (rpmxend > rpmrealend)
        rpmxend = rpmrealend;
    float rpmredxend = rpmrealend;
    if (rpmrealend < rpmredx)
        rpmredxend = rpmredx;
    rpmbarverts.SetToBillboard(rpmxstart, rpmy, rpmxend, rpmy+rpmheight);
    rpmredbarverts.SetToBillboard(rpmredx, rpmy, rpmredxend, rpmy+rpmheight);
    rpmboxverts.SetToBillboard(rpmxstart, rpmy, rpmxstart+rpmwidth, rpmy+rpmheight);

    //mphtextdraw->SetDrawEnable(true);
    std::stringstream speedo;
    if (mph)
        speedo << std::abs((int)(2.23693629 * meterspersecond)) << " MPH";
    else
        speedo << std::abs((int)(3.6 * meterspersecond)) << " KPH";
    float speedotextwidth = mphtext.GetWidth(lcdfont, speedo.str(), mphtext.GetCurrentScale().first);
    mphtext.Revise(lcdfont, speedo.str(), 1.0-screenhwratio*0.02-speedotextwidth, 1.0-0.015, mphtext.GetCurrentScale().first, mphtext.GetCurrentScale().second);

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

    //update ABS alpha value
    if (!absenabled)
        abs.SetAlpha(hudroot, 0.0);
    else
    {
        if (absactive)
            abs.SetAlpha(hudroot, 1.0);
        else
            abs.SetAlpha(hudroot, 0.2);
    }

    //update TCS alpha value
    if (!tcsenabled)
        tcs.SetAlpha(hudroot, 0.0);
    else
    {
        if (tcsactive)
            tcs.SetAlpha(hudroot, 1.0);
        else
            tcs.SetAlpha(hudroot, 0.2);
    }

    //update lap
    if (numlaps > 0)
    {
        std::stringstream lapstream;
        //std::cout << curlapnum << std::endl;
        lapstream << "Lap " << std::max(1,std::min(curlapnum, numlaps)) << "/" << numlaps;
        lapindicator.Revise(lapstream.str());
    }
    else
        placeindicator.SetDrawEnable(hudroot, false);
	
	//update drift score
	if (numlaps == 0) //this is how we determine practice mode, for now
	{
		std::stringstream scorestream;
		scorestream << "Score: " << (int)driftscore;
		if (drifting)
		{
			scorestream << " + " << (int)thisdriftscore;
			driftscoreindicator.SetColor(hudroot, 1,0,0);
		}
		else
			driftscoreindicator.SetColor(hudroot, 1,1,1);
		driftscoreindicator.Revise(scorestream.str());
	}
	else
		driftscoreindicator.SetDrawEnable(hudroot, false);

    //update place
    if (numlaps > 0)
    {
        std::stringstream stream;
        stream << "Place " << curplace << "/" << numcars;
        placeindicator.Revise(stream.str());
    }
    else
        placeindicator.SetDrawEnable(hudroot, false);

    //update race prompt
    if (numlaps > 0)
    {
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
        raceprompt.SetDrawEnable(hudroot, false);
}
