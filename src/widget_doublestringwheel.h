#ifndef _WIDGET_DOUBLESTRINGWHEEL_H
#define _WIDGET_DOUBLESTRINGWHEEL_H

#include "widget.h"
#include "widget_label.h"
#include "widget_button.h"

#include <string>
#include <list>

class SCENENODE;
class TEXTURE;
class FONT;

class WIDGET_DOUBLESTRINGWHEEL : public WIDGET
{
public:
	WIDGET_DOUBLESTRINGWHEEL();

	~WIDGET_DOUBLESTRINGWHEEL() {};

	virtual void SetAlpha(SCENENODE & scene, float newalpha);

	virtual void SetVisible(SCENENODE & scene, bool newvis);

	virtual std::string GetDescription() const;

	virtual void SetDescription(const std::string & newdesc);

	virtual bool ProcessInput(
		SCENENODE & scene,
		std::map<std::string, GUIOPTION> & optionmap,
		float cursorx, float cursory,
		bool cursordown, bool cursorjustup);

	virtual void UpdateOptions(
		SCENENODE & scene,
		bool save_to_options,
		std::map<std::string, GUIOPTION> & optionmap,
		std::ostream & error_output);

	void SetupDrawable(
		SCENENODE & scene,
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

	void SetCurrent(SCENENODE & scene, const std::string & newsetting1, const std::string & newsetting2);

	void SetCurrent(SCENENODE & scene, const std::string & newsetting1, const std::string & newsetting2, std::ostream & error_output);

	void SetValueList(const std::list <std::pair<std::string,std::string> > & newvaluelist1, const std::list <std::pair<std::string,std::string> > & newvaluelist2);

	void SetSetting(const std::string & newsetting1, const std::string & newsetting2);

private:
	WIDGET_LABEL title;
	WIDGET_LABEL label;
	WIDGET_BUTTON button_left;
	WIDGET_BUTTON button_right;
	std::list <std::pair<std::string,std::string> > values1, values2;
	std::list <std::pair<std::string,std::string> >::iterator current1, current2;
	std::string description;
	std::string setting1, setting2;
};

#endif
