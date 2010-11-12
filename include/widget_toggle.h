#ifndef _WIDGET_TOGGLE_H
#define _WIDGET_TOGGLE_H

#include "widget.h"
#include "widget_image.h"
#include "guioption.h"

#include <string>
#include <map>

class SCENENODE;
class TEXTURE;

class WIDGET_TOGGLE : public WIDGET
{
public:
	enum TOGGLESTATE
	{
		UP,
		DOWN,
		UPSEL,
		DOWNSEL,
		UPTRANS,
		DOWNTRANS
	};
	
	WIDGET_TOGGLE();
	
	virtual WIDGET * clone() const;
	
	virtual void SetAlpha(SCENENODE & scene, float newalpha);
	
	virtual void SetVisible(SCENENODE & scene, bool newvis);
	
	virtual std::string GetDescription() const;
	
	virtual void SetDescription(const std::string & newdesc);
	
	virtual bool ProcessInput(
		SCENENODE & scene,
		float cursorx, float cursory,
		bool cursordown, bool cursorjustup);

	virtual void UpdateOptions(
		SCENENODE & scene,
		bool save_to_options,
		std::map<std::string, GUIOPTION> & optionmap,
		std::ostream & error_output);
	
	void SetState(SCENENODE & scene, const TOGGLESTATE & newstate);
	
	void SetSetting(const std::string & newsetting);
	
	void SetupDrawable(
		SCENENODE & scene,
		std::tr1::shared_ptr<TEXTURE> teximage_up,
		std::tr1::shared_ptr<TEXTURE> teximage_down, 
		std::tr1::shared_ptr<TEXTURE> teximage_upselected,
		std::tr1::shared_ptr<TEXTURE> teximage_downselected, 
		std::tr1::shared_ptr<TEXTURE> teximage_transition,
		float centerx, float centery,
		float w,  float h);
	
private:
	WIDGET_IMAGE image_up;
	WIDGET_IMAGE image_down;
	WIDGET_IMAGE image_upsel;
	WIDGET_IMAGE image_downsel;
	WIDGET_IMAGE image_transition;
	std::string description;
	TOGGLESTATE state;
	std::string setting;
	bool wasvisible;
};

#endif
