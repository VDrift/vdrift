#ifndef _WIDGET_STRINGWHEEL_H
#define _WIDGET_STRINGWHEEL_H

#include "widget_label.h"
#include "widget_button.h"
#include "signal.h"

class FONT;
class SCENENODE;

class WIDGET_STRINGWHEEL : public WIDGET
{
public:
	WIDGET_STRINGWHEEL();

	~WIDGET_STRINGWHEEL();

	virtual void SetAlpha(SCENENODE & scene, float newalpha);

	virtual void SetVisible(SCENENODE & scene, bool newvis);

	virtual std::string GetDescription() const;

	virtual void SetDescription(const std::string & newdesc);

	virtual bool ProcessInput(
		SCENENODE & scene,
		std::map<std::string, GUIOPTION> & optionmap,
		float cursorx, float cursory,
		bool cursordown, bool cursorjustup);

	virtual void Update(SCENENODE & scene, float dt);

	virtual std::string GetAction() const {return active_action;}

	void SetAction(const std::string & newaction) {action = newaction;}

	void SetupDrawable(
		SCENENODE & scene,
		std::map<std::string, GUIOPTION> & optionmap,
		const std::string & setting,
		const std::string & newtitle,
		std::tr1::shared_ptr<TEXTURE> left_up,
		std::tr1::shared_ptr<TEXTURE> left_down,
		std::tr1::shared_ptr<TEXTURE> right_up,
		std::tr1::shared_ptr<TEXTURE> right_down,
		const FONT & font,
		float scalex,
		float scaley,
		float centerx,
		float centery,
		float z = 0);

private:
	WIDGET_LABEL title;
	WIDGET_LABEL label;
	WIDGET_BUTTON button_left;
	WIDGET_BUTTON button_right;
	std::string description;
	std::string setting;
	std::string value;
	std::string action;
	std::string active_action;
	bool update;

	Slot1<const std::string &> set_value;
	void SetValue(const std::string & value);

	WIDGET_STRINGWHEEL(const WIDGET_STRINGWHEEL & other);
};

#endif // _WIDGET_STRINGWHEEL_H
