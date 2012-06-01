#ifndef _GUICONTROL_H
#define _GUICONTROL_H

#include "signalslot.h"
#include <string>
#include <map>

class CONFIG;
class SCENENODE;

class GUICONTROL
{
public:
	GUICONTROL();

	virtual ~GUICONTROL();

	/// Return true if control in focus
	bool InFocus(float x, float y) const;
	
	void OnSelect(float x, float y) const;

	void OnSelect() const;

	void OnFocus() const;

	void OnBlur() const;

	void OnMoveUp() const;

	void OnMoveDown() const;

	void OnMoveLeft() const;

	void OnMoveRight() const;

	const std::string & GetDescription() const;

	void SetDescription(const std::string & value);

	void SetRect(float xmin, float ymin, float xmax, float ymax);

	/// Register event actions
	void RegisterActions(
		const std::map<std::string, Slot1<const std::string &>*> & vactionmap,
		const std::map<std::string, Slot0*> & actionmap,
		const std::string & name,
		const CONFIG & cfg);

	/// Register event actions to signal
	static void SetActions(
		const std::map<std::string, Slot0*> & actionmap,
		const std::string & actionstr,
		Signal0 & signal);

	static void SetActions(
		const std::map<std::string, Slot1<const std::string &>*> & actionmap,
		const std::string & actionstr,
		Signal1<const std::string &> & signal);

	static const std::vector<std::string> signals;

protected:
	float m_xmin, m_ymin, m_xmax, m_ymax;
	std::string m_description;
	Signal1<const std::string &> onselectx;
	Signal1<const std::string &> onselecty;
	Signal0 onselect;
	Signal0 onfocus;
	Signal0 onblur;
	Signal0 onmoveup;
	Signal0 onmovedown;
	Signal0 onmoveleft;
	Signal0 onmoveright;
};

#endif //_GUICONTROL_H
