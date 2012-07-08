#include "guicontrol.h"
#include "config.h"

static std::vector<std::string> GetSignals()
{
	std::vector<std::string> v(9);
	v.push_back("onselectx");
	v.push_back("onselecty");
	v.push_back("onselect");
	v.push_back("onfocus");
	v.push_back("onblur");
	v.push_back("onmoveup");
	v.push_back("onmovedown");
	v.push_back("onmoveleft");
	v.push_back("onmoveright");
	return v;
}
const std::vector<std::string> GUICONTROL::signals(GetSignals());

GUICONTROL::GUICONTROL() :
	m_xmin(0),
	m_ymin(0),
	m_xmax(0),
	m_ymax(0)
{
	// ctor
}

GUICONTROL::~GUICONTROL()
{
	// dtor
}

bool GUICONTROL::InFocus(float x, float y) const
{
	return x <= m_xmax && x >= m_xmin && y <= m_ymax && y >= m_ymin;
}

void GUICONTROL::OnSelect(float x, float y) const
{
	if (!InFocus(x, y))
		return;

	if (onselectx.connected())
	{
		float sx = (x - m_xmin) / (m_xmax - m_xmin);
		sx = (sx <= 1) ? (sx >= 0) ? sx : 0 : 1;
		std::stringstream s;
		s << sx;
		onselectx(s.str());
	}

	if (onselecty.connected())
	{
		float sy = (y - m_ymin) / (m_ymax - m_ymin);
		std::stringstream s;
		s << sy;
		onselectx(s.str());
	}
}

void GUICONTROL::OnSelect() const
{
	onselect();
}

void GUICONTROL::OnFocus() const
{
	onfocus();
}

void GUICONTROL::OnBlur() const
{
	onblur();
}

void GUICONTROL::OnMoveUp() const
{
	onmoveup();
}

void GUICONTROL::OnMoveDown() const
{
	onmovedown();
}

void GUICONTROL::OnMoveLeft() const
{
	onmoveleft();
}

void GUICONTROL::OnMoveRight() const
{
	onmoveright();
}

const std::string & GUICONTROL::GetDescription() const
{
	return m_description;
}

void GUICONTROL::SetDescription(const std::string & value)
{
	m_description = value;
}

void GUICONTROL::SetRect(float xmin, float ymin, float xmax, float ymax)
{
	m_xmin = xmin;
	m_ymin = ymin;
	m_xmax = xmax;
	m_ymax = ymax;
}

void GUICONTROL::RegisterActions(
	const std::map<std::string, Slot1<const std::string &>*> & vactionmap,
	const std::map<std::string, Slot0*> & actionmap,
	const std::string & name,
	const CONFIG & cfg)
{
	CONFIG::const_iterator section;
	cfg.GetSection(name, section);
	std::string actionstr;

	if (cfg.GetParam(section, "onselectx", actionstr))
		SetActions(vactionmap, actionstr, onselectx);

	if (cfg.GetParam(section, "onselecty", actionstr))
		SetActions(vactionmap, actionstr, onselecty);

	if (cfg.GetParam(section, "onselect", actionstr))
		SetActions(actionmap, actionstr, onselect);

	if (cfg.GetParam(section, "onfocus", actionstr))
		SetActions(actionmap, actionstr, onfocus);

	if (cfg.GetParam(section, "onblur", actionstr))
		SetActions(actionmap, actionstr, onblur);

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

void GUICONTROL::SetActions(
	const std::map<std::string, Slot1<const std::string &>*> & actionmap,
	const std::string & actionstr,
	Signal1<const std::string &> & signal)
{
	std::stringstream st(actionstr);
	while (st.good())
	{
		std::string action;
		st >> action;
		std::map<std::string, Slot1<const std::string &>*>::const_iterator it = actionmap.find(action);
		if (it != actionmap.end())
			it->second->connect(signal);
	}
}