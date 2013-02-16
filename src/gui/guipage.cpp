/************************************************************************/
/*                                                                      */
/* This file is part of VDrift.                                         */
/*                                                                      */
/* VDrift is free software: you can redistribute it and/or modify       */
/* it under the terms of the GNU General Public License as published by */
/* the Free Software Foundation, either version 3 of the License, or    */
/* (at your option) any later version.                                  */
/*                                                                      */
/* VDrift is distributed in the hope that it will be useful,            */
/* but WITHOUT ANY WARRANTY; without even the implied warranty of       */
/* MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the        */
/* GNU General Public License for more details.                         */
/*                                                                      */
/* You should have received a copy of the GNU General Public License    */
/* along with VDrift.  If not, see <http://www.gnu.org/licenses/>.      */
/*                                                                      */
/************************************************************************/

#include "guipage.h"
#include "guilanguage.h"
#include "guiwidget.h"
#include "guicontrol.h"
#include "guiimage.h"
#include "guilabel.h"
#include "guislider.h"
#include "content/contentmanager.h"
#include "graphics/textureinfo.h"
#include "cfg/config.h"
#include <fstream>
#include <sstream>

struct Rect
{
	float x; float y;
	float w; float h;
	float z;
};

static Rect LoadRect(
	const Config & pagefile,
	const Config::const_iterator section,
	const float screenhwratio)
{
	float x(0.5), y(0.5), w(0), h(0), z(0);

	if (pagefile.get(section, "width", w))
	{
		float l(0), r(0);
		if (pagefile.get(section, "left", l))
			x = l * screenhwratio;
		else if (pagefile.get(section, "right", r))
			x = 1 - (r + w) * screenhwratio;
		w = w * screenhwratio;
	}
	else
	{
		float l(0), r(0);
		pagefile.get(section, "left", l);
		pagefile.get(section, "right", r);
		x = l * screenhwratio;
		w = 1 - (l + r) * screenhwratio;
	}
	if (pagefile.get(section, "height", h))
	{
		float t(0), b(0);
		if (pagefile.get(section, "top", t))
			y = t;
		else if (pagefile.get(section, "bottom", b))
			y = 1 - b - h;
	}
	else
	{
		float t(0), b(0);
		pagefile.get(section, "top", t);
		pagefile.get(section, "bottom", b);
		h = 1 - t - b;
		y = t;
	}
	pagefile.get(section, "layer", z);

	// widgets expect rectangle center
	x = x + w * 0.5;
	y = y + h * 0.5;

	// draw order offset
	z = z + 100;

	Rect re;
	re.x = x; re.y = y;
	re.w = w; re.h = h;
	re.z = z;
	return re;
}

template <class SlotMap, class Signal>
static void ConnectSignal(
	const std::string & valuestr,
	const SlotMap & slotmap,
	Signal & signal)
{
	typename SlotMap::const_iterator it = slotmap.find(valuestr);
	if (it != slotmap.end())
		it->second->connect(signal);
}

template <class SignalMap, class Slot>
static void ConnectAction(
	const std::string & valuestr,
	const SignalMap & signalmap,
	Slot & slot)
{
	typename SignalMap::const_iterator it = signalmap.find(valuestr);
	if (it != signalmap.end())
		slot.connect(*it->second);
	else
		slot.call(valuestr);
}

GUIPAGE::GUIPAGE() :
	default_control(0),
	active_control(0)
{
	// ctor
}

GUIPAGE::~GUIPAGE()
{
	for (std::vector <GUIWIDGET *>::iterator i = widgets.begin(); i != widgets.end(); ++i)
	{
		delete *i;
	}
	for (std::vector <GUICONTROL *>::iterator i = controls.begin(); i != controls.end(); ++i)
	{
		delete *i;
	}
}

bool GUIPAGE::Load(
	const std::string & path,
	const std::string & texpath,
	const float screenhwratio,
	const GUILANGUAGE & lang,
	const FONT & font,
	VSIGNALMAP vsignalmap,
	VACTIONMAP vactionmap,
	ACTIONMAP actionmap,
	SCENENODE & parentnode,
	ContentManager & content,
	std::ostream & error_output)
{
	assert(!s.valid());
	Clear(parentnode);
	s = parentnode.AddNode();
	SCENENODE & sref = GetNode(parentnode);

	Config pagefile;
	if (!pagefile.load(path))
	{
		error_output << "Couldn't find GUI page file: " << path << std::endl;
		return false;
	}

	if (!pagefile.get("", "name", name))
		return false;

	// set page event handlers
	std::string actionstr;
	if (pagefile.get("", "onfocus", actionstr))
		GUICONTROL::SetActions(actionmap, actionstr, onfocus);
	if (pagefile.get("", "oncancel", actionstr))
		GUICONTROL::SetActions(actionmap, actionstr, oncancel);

	// register tooltip signal
	vsignalmap["gui.tooltip"] = &tooltip;

	// load widgets and controls
	active_control = 0;
	std::map<std::string, GUIWIDGET*> widgetmap;	// labels, images, sliders
	std::vector<Config::const_iterator> controlit;	// control iterator cache
	for (Config::const_iterator section = pagefile.begin(); section != pagefile.end(); ++section)
	{
		if (section->first.empty()) continue;

		Rect r = LoadRect(pagefile, section, screenhwratio);

		GUIWIDGET * widget = 0;

		// load widget
		std::string value;
		if (pagefile.get(section, "text", value))
		{
			std::string alignstr;
			float fontsize = 0.03;
			pagefile.get(section, "fontsize", fontsize);
			pagefile.get(section, "align", alignstr);

			int align = 0;
			if (alignstr == "right") align = 1;
			else if (alignstr == "left") align = -1;

			float scaley = fontsize;
			float scalex = fontsize * screenhwratio;

			// none is reserved for empty text string
			if (value == "none")
				value.clear();
			else
				value = lang(value);

			GUILABEL * new_widget = new GUILABEL();
			new_widget->SetupDrawable(
				sref, font, align, scalex, scaley,
				r.x, r.y, r.w, r.h, r.z);

			ConnectAction(value, vsignalmap, new_widget->set_value);
			widget = new_widget;

			std::string name;
			if (pagefile.get(section, "name", name))
				labels[name] = new_widget;
		}
		else if (pagefile.get(section, "image", value))
		{
			std::string path = texpath;
			pagefile.get(section, "path", path);

			GUIIMAGE * new_widget = new GUIIMAGE();
			new_widget->SetupDrawable(
				sref, content, path,
				r.x, r.y, r.w, r.h, r.z);

			ConnectAction(value, vsignalmap, new_widget->set_image);
			widget = new_widget;
		}
		else if (pagefile.get(section, "slider", value))
		{
			bool fill = false;
			pagefile.get(section, "fill", fill);

			TEXTUREINFO texinfo;
			texinfo.mipmap = false;
			texinfo.repeatu = false;
			texinfo.repeatv = false;
			std::tr1::shared_ptr<TEXTURE> bartex;
			content.load(bartex, texpath, "white.png", texinfo);

			GUISLIDER * new_widget = new GUISLIDER();
			new_widget->SetupDrawable(
				sref, bartex,
				r.x, r.y, r.w, r.h, r.z,
				fill, error_output);

			ConnectAction(value, vsignalmap, new_widget->set_value);
			widget = new_widget;
		}

		// set widget properties (connect property slots)
		if (widget)
		{
			std::string color, opacity, hue, sat, val;
			pagefile.get(section, "color", color);
			pagefile.get(section, "opacity", opacity);
			pagefile.get(section, "hue", hue);
			pagefile.get(section, "sat", sat);
			pagefile.get(section, "val", val);

			ConnectAction(color, vsignalmap, widget->set_color);
			ConnectAction(opacity, vsignalmap, widget->set_opacity);
			ConnectAction(hue, vsignalmap, widget->set_hue);
			ConnectAction(sat, vsignalmap, widget->set_sat);
			ConnectAction(val, vsignalmap, widget->set_val);

			widgetmap[section->first] = widget;
			widgets.push_back(widget);
		}

		// load controls
		bool focus;
		if (pagefile.get(section, "focus", focus))
		{
			std::string desc;
			pagefile.get(section, "tip", desc);
			desc = lang(desc);

			float x0 = r.x - r.w * 0.5;
			float y0 = r.y - r.h * 0.5;
			float x1 = r.x + r.w * 0.5;
			float y1 = r.y + r.h * 0.5;

			GUICONTROL * control = new GUICONTROL();
			control->SetRect(x0, y0, x1, y1);
			control->SetDescription(desc);

			controls.push_back(control);
			controlit.push_back(section);

			if (focus)
				active_control = control;
		}
	}

	// load control actions (connect control signals)

	// first pass iterates over control signals to retrieve unique
	// actions with parameter(action_value) and widget property(widget_prop) calls
	// this has to happen after all widgets have been loaded
	typedef std::pair<std::string, Slot1<const std::string &>*> ActionValue;
	typedef std::pair<std::string, GUIWIDGET*> WidgetProp;
	std::set<ActionValue> action_value_set;
	std::set<WidgetProp> widget_prop_set;
	for (size_t i = 0; i < controlit.size(); ++i)
	{
		const Config::const_iterator & section = controlit[i];
		for (size_t j = 0; j < GUICONTROL::signals.size(); ++j)
		{
			std::string actions;
			if (!pagefile.get(section, GUICONTROL::signals[j], actions))
				continue;

			std::stringstream st(actions);
			while(st.good())
			{
				std::string action;
				st >> action;

				// if action has a parameter and exists in the vaction map
				// push it into action_value_set
				size_t n = action.find(':');
				if (n == 0 || n == std::string::npos)
					continue;

				std::string aname(action.substr(0, n));
				VACTIONMAP::const_iterator vai = vactionmap.find(aname);
				if (vai != vactionmap.end())
				{
					action_value_set.insert(std::make_pair(action, vai->second));
					continue;
				}

				// if action is setting a widget property for an existing widget
				// push it into widget_prop_set
				size_t m = action.find('.');
				if (m == 0 || m == std::string::npos)
					continue;

				std::string wname(action.substr(0, m));
				std::map<std::string, GUIWIDGET*>::const_iterator wi = widgetmap.find(wname);
				if (wi != widgetmap.end())
				{
					widget_prop_set.insert(std::make_pair(action, wi->second));
				}
			}
		}
	}

	// register controls, so that they can be activated/focused by control signals
	// extra pass to avoid control_set reallocations
	control_set.reserve(controls.size());
	for (size_t i = 0; i < controls.size(); ++i)
	{
		control_set.push_back(ControlCb());
		control_set.back().page = this;
		control_set.back().control = controls[i];
		actionmap[controlit[i]->first] = &control_set.back().action;
	}

	// register action calls with a parameter, so that they can be signaled by controls
	// extra pass to avoid action_set reallocations
	action_set.reserve(action_value_set.size());
	for (std::set<ActionValue>::iterator i = action_value_set.begin(); i != action_value_set.end(); ++i)
	{
		action_set.push_back(ActionCb());
		ActionCb & cb = action_set.back();

		cb.value = i->first.substr(i->first.find(':') + 1);
		i->second->connect(cb.signal);
		actionmap[i->first] = &cb.action;
	}

	// register widget float property calls, so that they can be signaled by controls
	// extra pass to avoid widget_set reallocations
	widget_set.reserve(widget_prop_set.size());
	for (std::set<WidgetProp>::const_iterator i = widget_prop_set.begin(); i != widget_prop_set.end(); ++i)
	{
		// get widget and property
		const std::string & action = i->first;
		std::string wprop(action.substr(action.find('.') + 1));
		GUIWIDGET * widget = i->second;

		// get property name and value
		size_t m = wprop.find(':');
		if (m == 0 || m == std::string::npos)
			continue;

		float pvalue = 0;
		std::stringstream pvaluestr(wprop.substr(m + 1));
		pvaluestr >> pvalue;
		std::string pname(wprop.substr(0, m));

		// set widget property callback
		widget_set.push_back(WidgetCb());
		WidgetCb & cb = widget_set.back();
		cb.value = pvalue;
		if (pname == "hue")
			cb.set.bind<GUIWIDGET, &GUIWIDGET::SetHue>(widget);
		else if (pname == "sat")
			cb.set.bind<GUIWIDGET, &GUIWIDGET::SetSat>(widget);
		else if (pname == "val")
			cb.set.bind<GUIWIDGET, &GUIWIDGET::SetVal>(widget);
		else if (pname == "opacity")
			cb.set.bind<GUIWIDGET, &GUIWIDGET::SetOpacity>(widget);
		else
		{
			error_output << "Failed to set property: " << action << std::endl;
			continue;
		}

		// register property callback to action map
		actionmap[action] = &cb.action;
	}

	// final pass to connect control signals with their actions
	for (size_t i = 0; i < controlit.size(); ++i)
	{
		controls[i]->RegisterActions(vactionmap, actionmap, controlit[i], pagefile);
	}

	// set active control
	if (!active_control && !controls.empty())
		active_control = controls[0];
	if (active_control)
	{
		active_control->OnFocus();
		tooltip(active_control->GetDescription());
	}

	// set default control(activated on page focus) to active
	default_control = active_control;

	return true;
}

void GUIPAGE::SetVisible(SCENENODE & parent, bool value)
{
	SCENENODE & sref = GetNode(parent);
	for (std::vector <GUIWIDGET *>::iterator i = widgets.begin(); i != widgets.end(); ++i)
	{
		(*i)->SetVisible(sref, value);
	}

	if (!value)
	{
		if (default_control)
			SetActiveControl(*default_control);
	}
	else
	{
		onfocus();
	}
}

void GUIPAGE::SetAlpha(SCENENODE & parent, float value)
{
	SCENENODE & sref = parent.GetNode(s);
	for (std::vector <GUIWIDGET *>::iterator i = widgets.begin(); i != widgets.end(); ++i)
	{
		(*i)->SetAlpha(sref, value);
	}
}

void GUIPAGE::ProcessInput(
	float cursorx, float cursory,
	bool cursormoved, bool cursordown, bool cursorjustup,
	bool moveleft, bool moveright,
	bool moveup, bool movedown,
	bool select, bool cancel)
{
	if (cancel)
	{
		oncancel();
		return;
	}

	// set active widget from cursor
	bool cursoraction = cursormoved || cursordown || cursorjustup;
	if (cursoraction)
	{
		for (std::vector<GUICONTROL *>::iterator i = controls.begin(); i != controls.end(); ++i)
		{
			if ((**i).InFocus(cursorx, cursory))
			{
				SetActiveControl(**i);
				select |= cursorjustup;	// cursor select
				break;
			}
		}
	}

	// process events
	if (active_control)
	{
		if (cursordown)
			active_control->OnSelect(cursorx, cursory);
		else if (select)
			active_control->OnSelect();
		else if (moveleft)
			active_control->OnMoveLeft();
		else if (moveright)
			active_control->OnMoveRight();
		else if (moveup)
			active_control->OnMoveUp();
		else if (movedown)
			active_control->OnMoveDown();
	}
}

void GUIPAGE::Update(SCENENODE & parent, float dt)
{
	SCENENODE & sref = parent.GetNode(s);
	for (std::vector <GUIWIDGET *>::iterator i = widgets.begin(); i != widgets.end(); ++i)
	{
		(*i)->Update(sref, dt);
	}
}

void GUIPAGE::SetLabelText(const std::map<std::string, std::string> & label_text)
{
	for (std::map <std::string, GUILABEL*>::const_iterator i = labels.begin(); i != labels.end(); ++i)
	{
		const std::map<std::string, std::string>::const_iterator n = label_text.find(i->first);
		if (n != label_text.end())
			i->second->SetText(n->second);
	}
}

GUILABEL * GUIPAGE::GetLabel(const std::string & name)
{
	std::map <std::string, GUILABEL*>::const_iterator i = labels.find(name);
	if (i != labels.end())
		return i->second;
	return 0;
}

SCENENODE & GUIPAGE::GetNode(SCENENODE & parentnode)
{
	return parentnode.GetNode(s);
}

void GUIPAGE::Clear(SCENENODE & parentnode)
{
	for (std::vector <GUIWIDGET *>::iterator i = widgets.begin(); i != widgets.end(); ++i)
	{
		delete *i;
	}
	for (std::vector <GUICONTROL *>::iterator i = controls.begin(); i != controls.end(); ++i)
	{
		delete *i;
	}

	widgets.clear();
	controls.clear();
	labels.clear();
	control_set.clear();
	widget_set.clear();
	action_set.clear();

	if (s.valid())
	{
		SCENENODE & sref = parentnode.GetNode(s);
		sref.Clear();
	}
	s.invalidate();
}

void GUIPAGE::SetActiveControl(GUICONTROL & control)
{
	if (active_control != &control)
	{
		assert(active_control);
		active_control->OnBlur();
		active_control = &control;
		active_control->OnFocus();
		tooltip(active_control->GetDescription());
	}
}


GUIPAGE::ControlCb::ControlCb()
{
	action.call.bind<ControlCb, &ControlCb::call>(this);
	page = 0;
	control = 0;
}

GUIPAGE::ControlCb::ControlCb(const ControlCb & other)
{
	*this = other;
}

GUIPAGE::ControlCb & GUIPAGE::ControlCb::operator=(const ControlCb & other)
{
	action.call.bind<ControlCb, &ControlCb::call>(this);
	page = other.page;
	control = other.control;
	return *this;
}

void GUIPAGE::ControlCb::call()
{
	assert(page && control);
	page->SetActiveControl(*control);
}


GUIPAGE::WidgetCb::WidgetCb()
{
	value = 0;
	action.call.bind<WidgetCb, &WidgetCb::call>(this);
}

GUIPAGE::WidgetCb::WidgetCb(const WidgetCb & other)
{
	*this = other;
}

GUIPAGE::WidgetCb & GUIPAGE::WidgetCb::operator=(const WidgetCb & other)
{
	value = other.value;
	set = other.set;
	action.call.bind<WidgetCb, &WidgetCb::call>(this);
	return *this;
}

void GUIPAGE::WidgetCb::call()
{
	set(value);
}

GUIPAGE::ActionCb::ActionCb()
{
	action.call.bind<ActionCb, &ActionCb::call>(this);
}

GUIPAGE::ActionCb::ActionCb(const ActionCb & other)
{
	*this = other;
}

GUIPAGE::ActionCb & GUIPAGE::ActionCb::operator=(const ActionCb & other)
{
	action.call.bind<ActionCb, &ActionCb::call>(this);
	value = other.value;
	return *this;
}

void GUIPAGE::ActionCb::call()
{
	signal(value);
}
