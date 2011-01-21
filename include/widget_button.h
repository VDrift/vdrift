#ifndef _WIDGET_BUTTON_H
#define _WIDGET_BUTTON_H

#include "widget.h"
#include "widget_label.h"
#include "widget_image.h"

#include <string>

class SCENENODE;
class TEXTURE;
class FONT;

class WIDGET_BUTTON : public WIDGET
{
public:
	WIDGET_BUTTON();
	
	~WIDGET_BUTTON() {};
	
	virtual WIDGET * clone() const;
	
	virtual void SetAlpha(SCENENODE & scene, float newalpha);
	
	virtual void SetVisible(SCENENODE & scene, bool newvis);
	
	virtual std::string GetAction() const;
	
	virtual std::string GetDescription() const;
	
	virtual void SetDescription(const std::string & newdesc);
	
	virtual bool GetCancel() const;
	
	virtual bool ProcessInput(
		SCENENODE & scene,
		float cursorx, float cursory,
		bool cursordown, bool cursorjustup);
	
	void SetCancel(bool newcancel);
	
	void SetAction(const std::string & newaction);
	
	void SetupDrawable(
		SCENENODE & scene,
		std::tr1::shared_ptr<TEXTURE> teximage_up,
		std::tr1::shared_ptr<TEXTURE> teximage_down,
		std::tr1::shared_ptr<TEXTURE> teximage_selected,
		const FONT & font,
		const std::string & text,
		float centerx, float centery,
		float scalex, float scaley,
		float r, float g, float b,
		float h = 0, float w = 0,
		float order = 0);

private:
	WIDGET_LABEL label;
	WIDGET_IMAGE image_up;
	WIDGET_IMAGE image_down;
	WIDGET_IMAGE image_selected;
	std::string action;
	std::string active_action;
	std::string description;
	enum { UP, DOWN, SELECTED } state;
	bool cancel;
};

#endif
