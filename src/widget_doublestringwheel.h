#ifndef _WIDGET_DOUBLESTRINGWHEEL_H
#define _WIDGET_DOUBLESTRINGWHEEL_H

#include "widget.h"
#include "widget_label.h"
#include "widget_button.h"
#include "signal.h"

class SCENENODE;
class TEXTURE;
class FONT;

class WIDGET_DOUBLESTRINGWHEEL : public WIDGET
{
public:
	WIDGET_DOUBLESTRINGWHEEL();

	~WIDGET_DOUBLESTRINGWHEEL();

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

	void SetupDrawable(
		SCENENODE & scene,
		std::map<std::string, GUIOPTION> & optionmap,
		const std::string & newsetting1,
		const std::string & newsetting2,
		const std::string & newtitle,
		std::tr1::shared_ptr<TEXTURE> teximage_left_up,
		std::tr1::shared_ptr<TEXTURE> teximage_left_down,
		std::tr1::shared_ptr<TEXTURE> teximage_right_up,
		std::tr1::shared_ptr<TEXTURE> teximage_right_down,
		const FONT & font,
		float scalex,
		float scaley,
		float centerx,
		float centery,
		float z);

private:
	WIDGET_LABEL title;
	WIDGET_LABEL label;
	WIDGET_BUTTON button_left;
	WIDGET_BUTTON button_right;
	std::string setting1, setting2;
	std::string value1, value2;
	std::string description;
	bool update;

	Slot1<const std::string &> set_value1;
	Slot1<const std::string &> set_value2;
	void SetValue1(const std::string & value);
	void SetValue2(const std::string & value);

	WIDGET_DOUBLESTRINGWHEEL(const WIDGET_DOUBLESTRINGWHEEL & other);
};

#endif
