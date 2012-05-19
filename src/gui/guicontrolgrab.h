#ifndef _GUICONTROLGRAB_H
#define _GUICONTROLGRAB_H

#include "gui/guibutton.h"
#include <vector>
#include <list>

class SCENENODE;
class TEXTURE;
class CONFIG;
class FONT;

class GUICONTROLGRAB : public GUICONTROL
{
public:
	GUICONTROLGRAB();

	~GUICONTROLGRAB();

	virtual void SetAlpha(SCENENODE & scene, float newalpha);

	virtual void SetVisible(SCENENODE & scene, bool newvis);

	virtual bool ProcessInput(
		SCENENODE & scene,
		float cursorx, float cursory,
		bool cursordown, bool cursorjustup);

	void SetupDrawable(
		SCENENODE & scene,
		const std::string & newsetting,
		const CONFIG & c,
		const FONT & font,
		float scalex, float scaley,
		float centerx, float centery, float newz,
		bool newanalog, bool newonly_one);

	void LoadControls(SCENENODE & parent, const CONFIG & c, const FONT & font);

	keyed_container <SCENENODE>::handle GetNode();

	Signal1<const std::string &> signal_control;

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

		GUIBUTTON widget;
		std::string type;
		std::string name;
		bool once;
		bool down;
		int keycode;
		std::string key;
		std::string joy_type;
		int joy_index;
		int joy_axis;
		std::string joy_axis_type;
		std::string mouse_type;
		std::string mouse_motion;
		float deadzone;
		float exponent;
		float gain;
	};

	GUIBUTTON addbutton;
	std::list <CONTROLWIDGET> controlbuttons;
	GUICONTROL * active_widget;
	std::string setting;
	keyed_container <SCENENODE>::handle topnode;
	keyed_container <SCENENODE>::handle ctrlnode;
	float scale_x, scale_y;
	float x, y, z;
	float w, h;
	bool analog;
	bool once;

	static std::string GetDescription(CONTROLWIDGET & widget);
	static std::string GetControlString(CONTROLWIDGET & widget);
};

#endif
