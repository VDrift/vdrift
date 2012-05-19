#ifndef _GUICONTROL_H
#define _GUICONTROL_H

#include "gui/guiwidget.h"
#include <map>

class CONFIG;
class SCENENODE;

class GUICONTROL : public GUIWIDGET
{
public:
	/// event handlers, true if event signaled
	//bool OnCancel() const;

	bool OnSelect() const;

	bool OnMoveUp() const;

	bool OnMoveDown() const;

	bool OnMoveLeft() const;

	bool OnMoveRight() const;

	const std::string & GetDescription() const;

	void SetDescription(const std::string & value);

	/// True if control active
	virtual bool ProcessInput(
		SCENENODE & scene,
		float cursorx, float cursory,
		bool cursordown, bool cursorjustup);

	/// Register event actions
	void RegisterActions(
		const std::map<std::string, Slot0*> & actionmap,
		const std::string & name,
		const CONFIG & cfg);

	/// Register event actions to signal
	static void SetActions(
		const std::map<std::string, Slot0*> & actionmap,
		const std::string & actionstr,
		Signal0 & signal);

	virtual ~GUICONTROL() {};

private:
	std::string m_description;
	//Signal0 oncancel;
	Signal0 onselect;
	Signal0 onmoveup;
	Signal0 onmovedown;
	Signal0 onmoveleft;
	Signal0 onmoveright;
};

#endif //_GUICONTROL_H
