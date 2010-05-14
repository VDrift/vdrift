#ifndef _WIDGET_H
#define _WIDGET_H

#include <map>
#include <string>
#include <iostream>

class SCENENODE;
class GUIOPTION;

///abstract base-class for widgets
class WIDGET
{
public:
	virtual WIDGET * clone() const = 0;
	virtual void SetAlpha(SCENENODE & scene, float newalpha) {}
	virtual void SetVisible(SCENENODE & scene, bool newvis) {}
	///returns true if the mouse is within the widget
	virtual bool ProcessInput(SCENENODE & scene, float cursorx, float cursory, bool cursordown, bool cursorjustup) {return false;}
	///returns the action associated with the widget, or an empty string if no action occurred from the input
	virtual std::string GetAction() const {return "";}
	virtual void SetName(const std::string & newname) {}
	virtual std::string GetDescription() const {return "";}
	virtual void SetDescription(const std::string & newdesc) {}
	virtual void UpdateOptions(SCENENODE & scene, bool save_to_options, std::map<std::string, GUIOPTION> & optionmap, std::ostream & error_output) {}
	virtual bool GetCancel() const {return false;}
	virtual void AddHook(WIDGET * other) {}
	virtual void HookMessage(SCENENODE & scene, const std::string & message, const std::string & from) {}
	virtual void Update(SCENENODE & scene, float dt) {}
};

#endif // _WIDGET_H
