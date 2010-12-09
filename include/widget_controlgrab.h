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
		const std::vector <std::tr1::shared_ptr<TEXTURE> > & textures,
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
	
	// awfull widget description hack
	enum STRING
	{
		ADDNEW_STR,
		EDIT_STR,
		PRESS_STR,
		RELEASE_STR,
		ONCE_STR,
		HELD_STR,
		KEY_STR,
		JOY_STR,
		MOUSE_STR,
		BUTTON_STR,
		AXIS_STR,
		MOTION_STR,
		END_STR
	};
	static std::string Str[END_STR];
	
private:
	struct CONTROLWIDGET
	{
		CONTROLWIDGET();
		
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
	float scale_x, scale_y;
	float x, y;
	float w, h;
	bool analog;
	bool only_one;
};

#endif
