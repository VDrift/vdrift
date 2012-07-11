#include "gui/guipage.h"
#include "gui/guiwidget.h"
#include "gui/guicontrol.h"
#include "gui/guiimage.h"
#include "gui/guilabel.h"
#include "gui/guislider.h"
#include "contentmanager.h"
#include "textureinfo.h"
#include "config.h"

#include <fstream>
#include <sstream>

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
	const FONT & font,
	const std::map <std::string, std::string> & languagemap,
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

	//error_output << "Loading " << path << std::endl;

	CONFIG pagefile;
	if (!pagefile.Load(path))
	{
		error_output << "Couldn't find GUI page file: " << path << std::endl;
		return false;
	}

	if (!pagefile.GetParam("", "name", name)) return false;

	// set page event handlers
	std::string actionstr;
	if (pagefile.GetParam("", "onfocus", actionstr))
		GUICONTROL::SetActions(actionmap, actionstr, onfocus);
	if (pagefile.GetParam("", "oncancel", actionstr))
		GUICONTROL::SetActions(actionmap, actionstr, oncancel);

	// register tooltip signal
	vsignalmap["gui.tooltip"] = &tooltip;

	// load widgets
	active_control = 0;
	std::vector<std::string> controlid;				// controls
	std::map<std::string, GUIWIDGET*> widgetmap;	// images, sliders
	for (CONFIG::const_iterator section = pagefile.begin(); section != pagefile.end(); ++section)
	{
		if (section->first.empty()) continue;

		GUIWIDGET * widget(0);

		// get widget rectangle
		float x(0.5), y(0.5), w(0), h(0);
		if (pagefile.GetParam(section, "width", w))
		{
			float l(0), r(0);
			if (pagefile.GetParam(section, "left", l))
				x = l * screenhwratio;
			else if (pagefile.GetParam(section, "right", r))
				x = 1 - (r + w) * screenhwratio;
			w = w * screenhwratio;
		}
		else
		{
			float l(0), r(0);
			pagefile.GetParam(section, "left", l);
			pagefile.GetParam(section, "right", r);
			x = l * screenhwratio;
			w = 1 - (l + r) * screenhwratio;
		}
		if (pagefile.GetParam(section, "height", h))
		{
			float t(0), b(0);
			if (pagefile.GetParam(section, "top", t))
				y = t;
			else if (pagefile.GetParam(section, "bottom", b))
				y = 1 - b - h;
		}
		else
		{
			float t(0), b(0);
			pagefile.GetParam(section, "top", t);
			pagefile.GetParam(section, "bottom", b);
			h = 1 - t - b;
			y = t;
		}
		// widgets expect rectangle center
		x = x + w * 0.5;
		y = y + h * 0.5;

		float z(0);
		pagefile.GetParam(section, "layer", z);
		// draw order offset
		z = z + 100;

		std::string text;
		if (pagefile.GetParam(section, "text", text))
		{
			// none is reserved for empty text string
			if (text == "none") text.clear();

			std::string alignstr;
			float fontsize = 0.03;
			pagefile.GetParam(section, "fontsize", fontsize);
			pagefile.GetParam(section, "align", alignstr);

			int align = 0;
			if (alignstr == "right") align = 1;
			else if (alignstr == "left") align = -1;

			float scaley = fontsize;
			float scalex = fontsize * screenhwratio;

			std::map<std::string, std::string>::const_iterator li;
			if ((li = languagemap.find(text)) != languagemap.end()) text = li->second;

			GUILABEL * new_widget = new GUILABEL();
			new_widget->SetupDrawable(
				sref, font, align, scalex, scaley,
				x, y, w, h, z);

			ConnectAction(text, vsignalmap, new_widget->set_value);
			widget = new_widget;

			std::string name;
			if (pagefile.GetParam(section, "name", name))
				labels[name] = new_widget;
		}

		std::string image;
		if (pagefile.GetParam(section, "image", image))
		{
			std::string path = texpath;
			pagefile.GetParam(section, "path", path);

			GUIIMAGE * new_widget = new GUIIMAGE();
			new_widget->SetupDrawable(sref, content, path, x, y, w, h, z);

			ConnectAction(image, vsignalmap, new_widget->set_image);
			widget = new_widget;
		}

		std::string slider;
		if (pagefile.GetParam(section, "slider", slider))
		{
			bool fill = false;
			pagefile.GetParam(section, "fill", fill);

			TEXTUREINFO texinfo;
			texinfo.mipmap = false;
			texinfo.repeatu = false;
			texinfo.repeatv = false;
			std::tr1::shared_ptr<TEXTURE> bartex;
			if (!content.load(texpath, "white.png", texinfo, bartex)) return false;

			GUISLIDER * new_widget = new GUISLIDER();
			new_widget->SetupDrawable(
				sref, bartex,
				x, y, w, h, z,
				fill, error_output);

			ConnectAction(slider, vsignalmap, new_widget->set_value);
			widget = new_widget;
		}

		if (widget)
		{
			std::string color, alpha, hue, sat, val;
			pagefile.GetParam(section, "color", color);
			pagefile.GetParam(section, "alpha", alpha);
			pagefile.GetParam(section, "hue", hue);
			pagefile.GetParam(section, "sat", sat);
			pagefile.GetParam(section, "val", val);

			ConnectAction(color, vsignalmap, widget->set_color);
			ConnectAction(alpha, vsignalmap, widget->set_alpha);
			ConnectAction(hue, vsignalmap, widget->set_hue);
			ConnectAction(sat, vsignalmap, widget->set_sat);
			ConnectAction(val, vsignalmap, widget->set_val);

			widgetmap[section->first] = widget;
			widgets.push_back(widget);
		}

		bool focus;
		if (pagefile.GetParam(section, "focus", focus))
		{
			std::string desc;
			pagefile.GetParam(section, "tip", desc);

			std::map<std::string, std::string>::const_iterator li;
			if ((li = languagemap.find(desc)) != languagemap.end()) desc = li->second;

			GUICONTROL * control = new GUICONTROL();
			control->SetRect(x - w * 0.5, y - h * 0.5, x + w * 0.5, y + h * 0.5);
			control->SetDescription(desc);

			controls.push_back(control);
			controlid.push_back(section->first);

			if (focus)
				active_control = control;
		}
	}

	// register widget focus actions, extra pass to avoid reallocations
	control_focus.reserve(controls.size());
	for (size_t i = 0; i < controls.size(); ++i)
	{
		control_focus.push_back(ControlFocusCb());
		control_focus.back().page = this;
		control_focus.back().control = controls[i];
		actionmap[controlid[i]] = &control_focus.back().action;
	}

	// register widget property actions
	typedef std::pair<std::string, Slot1<const std::string &>*> ActionValue;
	std::set<ActionValue> action_value_set;
	std::set<std::string> widget_prop_set;
	for (size_t i = 0; i < controls.size(); ++i)
	{
		CONFIG::const_iterator section;
		pagefile.GetSection(controlid[i], section);
		for (size_t j = 0; j < GUICONTROL::signals.size(); ++j)
		{
			std::string actions;
			if (!pagefile.GetParam(section, GUICONTROL::signals[j], actions))
				continue;

			std::stringstream st(actions);
			while(st.good())
			{
				std::string action;
				st >> action;

				// action with parameters?
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

				// widget property?
				size_t m = action.find('.');
				if (m == 0 || m == std::string::npos)
					continue;

				std::string wname(action.substr(0, m));
				std::map<std::string, GUIWIDGET*>::const_iterator wi = widgetmap.find(wname);
				if (wi != widgetmap.end())
				{
					widget_prop_set.insert(action);
				}
			}
		}
	}

	// actions with parameters
	action_set.reserve(action_value_set.size());
	for (std::set<ActionValue>::iterator i = action_value_set.begin(); i != action_value_set.end(); ++i)
	{
		action_set.push_back(ActionCb());
		ActionCb & ac = action_set.back();
		ac.value = i->first.substr(i->first.find(':') + 1);
		i->second->connect(ac.signal);
		actionmap[i->first] = &ac.action;
	}

	// widget properties(float)
	widget_set.reserve(widget_prop_set.size());
	for (std::set<std::string>::const_iterator i = widget_prop_set.begin(); i != widget_prop_set.end(); ++i)
	{
		size_t n = i->find('.');
		std::string name(i->substr(0, n));
		std::string prop(i->substr(n + 1));

		size_t m = prop.find(':');
		if (m == 0 || m == std::string::npos)
			continue;

		float pval = 0;
		std::string pname(prop.substr(0, m));
		std::stringstream pval_str(prop.substr(m + 1));
		pval_str >> pval;

		GUIWIDGET * widget = widgetmap.find(name)->second;

		widget_set.push_back(WidgetCb());
		WidgetCb & wc = widget_set.back();
		if (pname == "hue")
			wc.set.bind<GUIWIDGET, &GUIWIDGET::SetHue>(widget);
		else if (pname == "sat")
			wc.set.bind<GUIWIDGET, &GUIWIDGET::SetSat>(widget);
		else if (pname == "val")
			wc.set.bind<GUIWIDGET, &GUIWIDGET::SetVal>(widget);
		else if (pname == "alpha")
			wc.set.bind<GUIWIDGET, &GUIWIDGET::SetAlpha>(widget);
		else
		{
			error_output << "Failed to set action: " << *i << std::endl;
			continue;
		}
		wc.value = pval;
		actionmap[*i] = &wc.action;
	}

	// register actions to controls
	for (size_t i = 0; i < controls.size(); ++i)
	{
		controls[i]->RegisterActions(vactionmap, actionmap, controlid[i], pagefile);
	}

	// set active widget
	if (!active_control && !controls.empty())
		active_control = controls[0];

	default_control = active_control;

	if (active_control)
	{
		active_control->OnFocus();
		tooltip(active_control->GetDescription());
	}

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
			SetActiveWidget(*default_control);
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
				SetActiveWidget(**i);
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
	control_focus.clear();
	widget_set.clear();
	action_set.clear();

	if (s.valid())
	{
		SCENENODE & sref = parentnode.GetNode(s);
		sref.Clear();
	}
	s.invalidate();
}

void GUIPAGE::SetActiveWidget(GUICONTROL & widget)
{
	if (active_control != &widget)
	{
		assert(active_control);
		active_control->OnBlur();
		active_control = &widget;
		active_control->OnFocus();
		tooltip(active_control->GetDescription());
	}
}


GUIPAGE::ControlFocusCb::ControlFocusCb()
{
	action.call.bind<ControlFocusCb, &ControlFocusCb::call>(this);
	page = 0;
	control = 0;
}

GUIPAGE::ControlFocusCb::ControlFocusCb(const ControlFocusCb & other)
{
	*this = other;
}

GUIPAGE::ControlFocusCb & GUIPAGE::ControlFocusCb::operator=(const ControlFocusCb & other)
{
	action.call.bind<ControlFocusCb, &ControlFocusCb::call>(this);
	page = other.page;
	control = other.control;
	return *this;
}

void GUIPAGE::ControlFocusCb::call()
{
	assert(page && control);
	page->SetActiveWidget(*control);
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
