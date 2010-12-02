#ifndef _WIDGET_CONTROLGRAB_H
#define _WIDGET_CONTROLGRAB_H

#include "widget.h"
#include "widget_label.h"
#include "widget_button.h"

#include <string>
#include <list>
#include <vector>

class SCENENODE;
class TEXTURE;
class CONFIG;
class FONT;

class WIDGET_CONTROLGRAB : public WIDGET
{
public:
	WIDGET_CONTROLGRAB() {};
	
	~WIDGET_CONTROLGRAB();
	
	virtual WIDGET * clone() const;
	
	virtual void SetAlpha(SCENENODE & scene, float newalpha);
	
	virtual void SetVisible(SCENENODE & scene, bool newvis);
	
	virtual std::string GetAction() const;
	
	virtual std::string GetDescription() const;
	
	virtual void SetDescription(const std::string & newdesc);
	
	virtual bool ProcessInput(
		SCENENODE & scene,
		float cursorx, float cursory,
		bool cursordown, bool cursorjustup);
	
	void SetupDrawable(
		SCENENODE & scene,
		const CONFIG & c,
		const std::string & newsetting,
		const std::vector <std::tr1::shared_ptr<TEXTURE> > & texturevector,
		const FONT & font,
		const std::string & text,
		const float centerx,
		const float centery,
		const float scalex,
		const float scaley,
		const bool newanalog,
		const bool newonly_one);
	
	void LoadControls(SCENENODE & parent, const CONFIG & c, const FONT & font);
	
	keyed_container <SCENENODE>::handle GetNode()
	{
		return topnode;
	}
	
	enum CONTROLTEXTURE
	{
		ADD,
		ADDSEL,
		JOYAXIS,
		JOYAXISSEL,
		JOYBTN,
		JOYBTNSEL,
		KEY,
		KEYSEL,
		MOUSE,
		MOUSESEL,
		END
	};
	
private:
	struct CONTROLWIDGET
	{
		WIDGET_BUTTON widget;
		std::string type;
		std::string name;
		bool once;
		bool down;
		std::string key;
		std::string keycode;
		std::string joy_type;
		int joy_index;
		int joy_button;
		int joy_axis;
		std::string joy_axis_type;
		std::string mouse_type;
		std::string mouse_motion;
		int mouse_button;
		float deadzone;
		float exponent;
		float gain;
	};
	
	WIDGET_LABEL label;
	WIDGET_BUTTON addbutton;
	std::list <CONTROLWIDGET> controlbuttons;
	std::string setting;
	std::string description;
	std::string tempdescription;
	std::string active_action;
	keyed_container <SCENENODE>::handle topnode;
	keyed_container <SCENENODE>::handle ctrlnode;
	std::vector <std::tr1::shared_ptr<TEXTURE> > textures;
	float x, y;
	float scale_x, scale_y;
	float w, h;
	bool analog;
	bool only_one;
	
	void AddButton(
		SCENENODE & scene,
		std::tr1::shared_ptr<TEXTURE> tex_unsel,
		std::tr1::shared_ptr<TEXTURE> tex_sel,
		const FONT & font, 
		const std::string & type,
		const std::string & name,
		const float scalex,
		const float scaley,
		const float y,
		const bool once,
		const bool down,
		const std::string & key,
		const std::string & keycode,
		const std::string & joy_type,
		const int joy_index,
		const int joy_button,
		const int joy_axis,
		const std::string & joy_axis_type,
		const std::string & mouse_type,
		const std::string & mouse_motion,
		const int mouse_button,
		const float deadzone,
		const float exponent,
		const float gain);
};

#endif
