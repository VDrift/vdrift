#include "guicontrol.h"
#include "config.h"
/*
bool GUICONTROL::OnCancel() const
{
	onselect();
	return onselect.connected();
}
*/
bool GUICONTROL::OnSelect() const
{
	onselect();
	return onselect.connected();
}

bool GUICONTROL::OnMoveUp() const
{
	onmoveup();
	return onmoveup.connected();
}

bool GUICONTROL::OnMoveDown() const
{
	onmovedown();
	return onmovedown.connected();
}

bool GUICONTROL::OnMoveLeft() const
{
	onmoveleft();
	return onmoveleft.connected();
}

bool GUICONTROL::OnMoveRight() const
{
	onmoveright();
	return onmoveright.connected();
}

const std::string & GUICONTROL::GetDescription() const
{
	return m_description;
}

void GUICONTROL::SetDescription(const std::string & value)
{
	m_description = value;
}

bool GUICONTROL::ProcessInput(
	SCENENODE & scene,
	float cursorx, float cursory,
	bool cursordown, bool cursorjustup)
{
	return false;
}

void GUICONTROL::RegisterActions(
	const std::map<std::string, Slot0*> & actionmap,
	const std::string & name,
	const CONFIG & cfg)
{
	CONFIG::const_iterator section;
	cfg.GetSection(name, section);
	std::string actionstr;

	//if (cfg.GetParam(section, "oncancel", actionstr))
	//	SetActions(actionmap, actionstr, oncancel);

	if (cfg.GetParam(section, "onselect", actionstr))
		SetActions(actionmap, actionstr, onselect);

	if (cfg.GetParam(section, "onmoveup", actionstr))
		SetActions(actionmap, actionstr, onmoveup);

	if (cfg.GetParam(section, "onmovedown", actionstr))
		SetActions(actionmap, actionstr, onmovedown);

	if (cfg.GetParam(section, "onmoveleft", actionstr))
		SetActions(actionmap, actionstr, onmoveleft);

	if (cfg.GetParam(section, "onmoveright", actionstr))
		SetActions(actionmap, actionstr, onmoveright);
}

void GUICONTROL::SetActions(
	const std::map<std::string, Slot0*> & actionmap,
	const std::string & actionstr,
	Signal0 & signal)
{
	std::stringstream st(actionstr);
	while (st.good())
	{
		std::string action;
		st >> action;
		std::map<std::string, Slot0*>::const_iterator it = actionmap.find(action);
		if (it != actionmap.end())
			it->second->connect(signal);
	}
}
