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
#include "guicontrollist.h"
#include "guiimagelist.h"
#include "guilabellist.h"
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
	const Config::const_iterator & section,
	const float hwratio)
{
	float x(0.5), y(0.5), w(0), h(0), z(0);

	if (pagefile.get(section, "width", w))
	{
		float l(0), r(0);
		if (pagefile.get(section, "left", l))
			x = l * hwratio;
		else if (pagefile.get(section, "right", r))
			x = 1 - (r + w) * hwratio;
		w = w * hwratio;
	}
	else
	{
		float l(0), r(0);
		pagefile.get(section, "left", l);
		pagefile.get(section, "right", r);
		x = l * hwratio;
		w = 1 - (l + r) * hwratio;
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

template <class LIST>
static bool LoadList(
	const Config & pagefile,
	const Config::const_iterator & section,
	const float x0, const float y0,
	const float x1, const float y1,
	const float hwratio,
	LIST *& list)
{
	unsigned rows(1), cols(1);
	if (pagefile.get(section, "rows", rows) | pagefile.get(section, "cols", cols))
	{
		// limit rows/cols to 64
		rows = rows < 64 ? rows : 64;
		cols = cols < 64 ? cols : 64;

		// padding
		float pl(0), pr(0), pt(0), pb(0);
		bool haspl(false), haspr(false);
		bool haspt(false), haspb(false);

		haspl |= pagefile.get(section, "padding", pl);
		haspb = haspt = haspr = haspl;
		pb = pt = pr = pl;

		haspl |= pagefile.get(section, "padding-lr", pl);
		haspt |= pagefile.get(section, "padding-tb", pt);
		haspr = haspl;
		haspb = haspt;
		pr = pl;
		pb = pt;

		haspl |= pagefile.get(section, "padding-left", pl);
		haspr |= pagefile.get(section, "padding-right", pr);
		haspt |= pagefile.get(section, "padding-top", pt);
		haspb |= pagefile.get(section, "padding-bottom", pb);
		pl *= hwratio;
		pr *= hwratio;

		// width (overrides padding)
		float cw(0);
		if (pagefile.get(section, "col-width", cw))
		{
			cw *= hwratio;
			float w = x1 - x0;
			float p = w / cols - cw;
			if (haspl && haspr)
			{
				float delta = (p - pl - pr) * 0.5f;
				pl += delta;
				pr += delta;
			}
			else if (haspl)
				pr = p - pl;
			else if (haspr)
				pl = p - pr;
			else
				pl = pr = p * 0.5f;
		}

		// height (overrides padding)
		float rh(0);
		if (pagefile.get(section, "row-height", rh))
		{
			float h = y1 - y0;
			float p = h / rows - rh;
			if (haspt && haspb)
			{
				float delta = (p - pt - pb) * 0.5f;
				pt += delta;
				pb += delta;
			}
			else if (haspt)
				pb = p - pt;
			else if (haspb)
				pt = p - pb;
			else
				pt = pb = p * 0.5f;
		}

		bool vert(true);
		pagefile.get(section, "vertical", vert);

		list = new LIST();
		list->SetupList(rows, cols, x0, y0, x1, y1, pl, pt, pr, pb, vert);
		return true;
	}
	return false;
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

template <class ActionMap, class Signal>
static void ConnectActions(
	const std::string & actionstr,
	const ActionMap & actionmap,
	Signal & signal)
{
	size_t len = actionstr.size();
	size_t pos = 0;
	while (pos < len)
	{
		pos = actionstr.find_first_not_of(' ', pos);
		if (pos >= len)
			break;

		size_t posn = actionstr.find(' ', pos);
		size_t n = actionstr.find(':', pos);
		if (n < posn && n + 1 < len && actionstr[n + 1] == '"')
			posn = actionstr.find('"', n + 2);
		std::string action = actionstr.substr(pos, posn - pos);
		pos = posn;

		typename ActionMap::const_iterator it = actionmap.find(action);
		if (it != actionmap.end())
			it->second->connect(signal);
	}
}

GuiPage::GuiPage() :
	default_control(0),
	active_control(0)
{
	// ctor
}

GuiPage::~GuiPage()
{
	for (std::vector <GuiWidget *>::iterator i = widgets.begin(); i != widgets.end(); ++i)
	{
		delete *i;
	}
	for (std::vector <GuiControl *>::iterator i = controls.begin(); i != controls.end(); ++i)
	{
		delete *i;
	}
}

bool GuiPage::Load(
	const std::string & path,
	const std::string & texpath,
	const float hwratio,
	const GuiLanguage & lang,
	const Font & font,
	StrSignalMap vsignalmap,
	const StrVecSlotlMap & vnactionmap,
	const StrSlotMap & vactionmap,
	IntSlotMap nactionmap,
	SlotMap actionmap,
	SceneNode & parentnode,
	ContentManager & content,
	std::ostream & error_output)
{
	assert(!s.valid());
	Clear(parentnode);
	s = parentnode.AddNode();
	SceneNode & sref = GetNode(parentnode);

	Config pagefile;
	if (!pagefile.load(path))
	{
		error_output << "Couldn't find GUI page file: " << path << std::endl;
		return false;
	}

	if (!pagefile.get("", "name", name))
		return false;

	// load widgets and controls
	active_control = 0;
	std::map<std::string, GuiWidget*> widgetmap;			// labels, images, sliders
	std::map<std::string, GuiWidgetList*> widgetlistmap;	// labels, images lists
	std::vector<Config::const_iterator> controlit;			// control iterator cache
	std::vector<Config::const_iterator> controlnit;			// control list iterator cache
	std::vector<GuiControlList*> controllists;				// control list cache
	for (Config::const_iterator section = pagefile.begin(); section != pagefile.end(); ++section)
	{
		if (section->first.empty()) continue;

		Rect r = LoadRect(pagefile, section, hwratio);
		float x0 = r.x - r.w * 0.5;
		float y0 = r.y - r.h * 0.5;
		float x1 = r.x + r.w * 0.5;
		float y1 = r.y + r.h * 0.5;

		GuiWidget * widget = 0;

		// load widget(list)
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
			float scalex = fontsize * hwratio;

			GuiLabelList * widget_list = 0;
			if (LoadList(pagefile, section, x0, y0, x1, y1, hwratio, widget_list))
			{
				// connect with the value list
				StrVecSlotlMap::const_iterator vni = vnactionmap.find(value);
				if (vni != vnactionmap.end())
				{
					StrSignalMap::const_iterator vsi = vsignalmap.find(value + ".update");
					if (vsi != vsignalmap.end())
					{
						widget_list->update_list.connect(*vsi->second);
						vni->second->connect(widget_list->get_values);
					}
				}

				// init drawable
				widget_list->SetupDrawable(sref, font, align, scalex, scaley, r.z);

				widgetlistmap[section->first] = widget_list;
				widget = widget_list;
			}
			else
			{
				// none is reserved for empty text string
				if (value == "none")
					value.clear();
				else
					value = lang(value);

				GuiLabel * new_widget = new GuiLabel();
				new_widget->SetupDrawable(
					sref, font, align, scalex, scaley,
					r.x, r.y, r.w, r.h, r.z);

				ConnectAction(value, vsignalmap, new_widget->set_value);

				std::string name;
				if (pagefile.get(section, "name", name))
					labels[name] = new_widget;

				widgetmap[section->first] = new_widget;
				widget = new_widget;
			}
		}
		else if (pagefile.get(section, "image", value))
		{
			std::string ext, path = texpath;
			pagefile.get(section, "path", path);
			pagefile.get(section, "ext", ext);

			GuiImageList * widget_list = 0;
			if (LoadList(pagefile, section, x0, y0, x1, y1, hwratio, widget_list))
			{
				// init drawable
				widget_list->SetupDrawable(sref, content, path, ext, r.z);

				// connect with the value list
				StrVecSlotlMap::const_iterator vni = vnactionmap.find(value);
				if (vni != vnactionmap.end())
				{
					StrSignalMap::const_iterator vsi = vsignalmap.find(value + ".update");
					if (vsi != vsignalmap.end())
					{
						widget_list->update_list.connect(*vsi->second);
						vni->second->connect(widget_list->get_values);
					}
				}
				else
				{
					// special case of list containing the same image?
					widget_list->SetImage(value);
				}

				widgetlistmap[section->first] = widget_list;
				widget = widget_list;
			}
			else
			{
				GuiImage * new_widget = new GuiImage();
				new_widget->SetupDrawable(
					sref, content, path, ext,
					r.x, r.y, r.w, r.h, r.z);

				ConnectAction(value, vsignalmap, new_widget->set_image);

				widgetmap[section->first] = new_widget;
				widget = new_widget;
			}
		}
		else if (pagefile.get(section, "slider", value))
		{
			bool fill = false;
			pagefile.get(section, "fill", fill);

			TextureInfo texinfo;
			texinfo.mipmap = false;
			texinfo.repeatu = false;
			texinfo.repeatv = false;
			std::tr1::shared_ptr<Texture> bartex;
			content.load(bartex, texpath, "white.png", texinfo);

			GuiSlider * new_widget = new GuiSlider();
			new_widget->SetupDrawable(
				sref, bartex,
				r.x, r.y, r.w, r.h, r.z,
				fill, error_output);

			ConnectAction(value, vsignalmap, new_widget->set_value);

			widgetmap[section->first] = new_widget;
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

			widgets.push_back(widget);
		}

		// load controls
		bool focus;
		if (pagefile.get(section, "focus", focus))
		{
			GuiControl * control = 0;
			GuiControlList * control_list = 0;
			if (LoadList(pagefile, section, x0, y0, x1, y1, hwratio, control_list))
			{
				// register control list scroll actions
				actionmap[section->first + ".scrollf"] = &control_list->scroll_fwd;
				actionmap[section->first + ".scrollr"] = &control_list->scroll_rev;

				// connect with item list
				if (pagefile.get(section, "list", value))
				{
					StrSignalMap::const_iterator vsu = vsignalmap.find(value + ".update");
					StrSignalMap::const_iterator vsn = vsignalmap.find(value + ".nth");
					if (vsu != vsignalmap.end() && vsn != vsignalmap.end())
					{
						control_list->update_list.connect(*vsu->second);
						control_list->set_nth.connect(*vsn->second);
					}
					else
					{
						error_output << value << " is not a list." << std::endl;
					}
				}

				controlnit.push_back(section);
				controllists.push_back(control_list);
				control = control_list;
			}
			else
			{
				control = new GuiControl();
			}
			control->SetRect(x0, y0, x1, y1);
			controlit.push_back(section);
			controls.push_back(control);

			std::string desc;
			if (pagefile.get(section, "tip", desc))
				control->SetDescription(lang(desc));

			if (focus)
				active_control = control;
		}
	}

	// load control actions (connect control signals)

	// first pass iterates over control signals to retrieve unique
	// actions with parameter and widget property calls (action_value_set)
	typedef std::pair<std::string, Slot1<const std::string &>*> ActionVal;
	std::set<ActionVal> action_val_set;
	for (size_t i = 0; i < controlit.size(); ++i)
	{
		const Config::const_iterator & section = controlit[i];
		for (size_t j = 0; j < GuiControl::signal_names.size(); ++j)
		{
			std::string actions;
			if (!pagefile.get(section, GuiControl::signal_names[j], actions))
				continue;

			size_t len = actions.size();
			size_t pos = 0;
			while(pos < len)
			{
				pos = actions.find_first_not_of(' ', pos);
				if (pos >= len)
					break;

				// get next action with value
				size_t posn = actions.find(' ', pos);
				size_t n = actions.find(':', pos);
				if (n > posn)
				{
					pos = posn;
					continue;
				}

				if (n + 1 < len && actions[n + 1] == '"')
					posn = actions.find('"', n + 2);

				std::string action = actions.substr(pos, posn - pos);
				std::string aname = actions.substr(pos, n - pos);
				pos = posn;

				StrSlotMap::const_iterator vai = vactionmap.find(aname);
				if (vai != vactionmap.end())
				{
					action_val_set.insert(std::make_pair(action, vai->second));
					continue;
				}

				// check if action is setting a widget property
				size_t wn = aname.find('.');
				if (wn == 0 || wn == std::string::npos)
					continue;

				std::string wname = aname.substr(0, wn);
				std::map<std::string, GuiWidget*>::const_iterator wi = widgetmap.find(wname);
				if (wi == widgetmap.end())
					continue;

				std::string pname = aname.substr(wn + 1);
				Slot1<const std::string &> * pslot;
				if (wi->second->GetProperty(pname, pslot))
					action_val_set.insert(std::make_pair(action, pslot));
			}
		}
	}

	// iterate over control list signals now
	typedef std::pair<std::string, Slot2<int, const std::string &>*> ActionValn;
	std::set<ActionValn> action_valn_set;
	for (size_t i = 0; i < controlnit.size(); ++i)
	{
		const Config::const_iterator & section = controlnit[i];
		for (size_t j = 0; j < GuiControlList::signal_names.size(); ++j)
		{
			std::string actions;
			if (!pagefile.get(section, GuiControlList::signal_names[j], actions))
				continue;

			size_t len = actions.size();
			size_t pos = 0;
			while(pos < len)
			{
				pos = actions.find_first_not_of(' ', pos);
				if (pos >= len)
					break;

				// get next action with value
				size_t posn = actions.find(' ', pos);
				size_t n = actions.find(':', pos);
				if (n > posn)
				{
					pos = posn;
					continue;
				}

				if (n + 1 < len && actions[n + 1] == '"')
					posn = actions.find('"', n + 2);

				std::string action = actions.substr(pos, posn - pos);
				std::string aname = actions.substr(pos, n - pos);
				pos = posn;

				// check if action is setting a widget property
				size_t wn = aname.find('.');
				if (wn == 0 || wn == std::string::npos)
					continue;

				std::string wname = aname.substr(0, wn);
				std::map<std::string, GuiWidgetList*>::const_iterator wi = widgetlistmap.find(wname);
				if (wi == widgetlistmap.end())
					continue;

				std::string pname = aname.substr(wn + 1);
				Slot2<int, const std::string &> * pslot;
				if (wi->second->GetProperty(pname, pslot))
					action_valn_set.insert(std::make_pair(action, pslot));
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
	action_set.reserve(action_val_set.size());
	for (std::set<ActionVal>::iterator i = action_val_set.begin(); i != action_val_set.end(); ++i)
	{
		action_set.push_back(SignalVal());
		SignalVal & cb = action_set.back();

		size_t n = i->first.find(':') + 1;
		if (i->first[n] == '"') n++;

		cb.value = i->first.substr(n);
		i->second->connect(cb.signal);

		actionmap[i->first] = &cb.action;
	}
	action_setn.reserve(action_valn_set.size());
	for (std::set<ActionValn>::iterator i = action_valn_set.begin(); i != action_valn_set.end(); ++i)
	{
		action_setn.push_back(SignalValn());
		SignalValn & cb = action_setn.back();

		size_t n = i->first.find(':') + 1;
		if (i->first[n] == '"') n++;

		cb.value = i->first.substr(n);
		i->second->connect(cb.signal);

		nactionmap[i->first] = &cb.action;
	}

	// final pass to connect control signals with their actions
	for (size_t i = 0; i < controlit.size(); ++i)
	{
		std::string actionstr;
		for (size_t j = 0; j < GuiControl::EVENTNUM; ++j)
		{
			if (pagefile.get(controlit[i], GuiControl::signal_names[j], actionstr))
				ConnectActions(actionstr, actionmap, controls[i]->m_signal[j]);
		}
		for (size_t j = 0; j < GuiControl::EVENTVNUM; ++j)
		{
			if (pagefile.get(controlit[i], GuiControl::signal_names[GuiControl::EVENTNUM + j], actionstr))
				ConnectActions(actionstr, vactionmap, controls[i]->m_signalv[j]);
		}
	}
	for (size_t i = 0; i < controlnit.size(); ++i)
	{
		std::string actionstr;
		for (size_t j = 0; j < GuiControl::EVENTNUM; ++j)
		{
			if (pagefile.get(controlnit[i], GuiControl::signal_names[j], actionstr))
				ConnectActions(actionstr, nactionmap, controllists[i]->m_signaln[j]);
		}
	}

	// connect page event handlers
	{
		std::string actionstr;
		if (pagefile.get("", "onfocus", actionstr))
			ConnectActions(actionstr, actionmap, onfocus);

		if (pagefile.get("", "oncancel", actionstr))
			ConnectActions(actionstr, actionmap, oncancel);
	}

	// set active control
	if (!active_control && !controls.empty())
		active_control = controls[0];
	if (active_control)
		active_control->Signal(GuiControl::FOCUS);

	// set default control
	default_control = active_control;

	return true;
}

void GuiPage::SetVisible(SceneNode & parent, bool value)
{
	SceneNode & sref = GetNode(parent);
	for (std::vector <GuiWidget *>::iterator i = widgets.begin(); i != widgets.end(); ++i)
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

void GuiPage::SetAlpha(SceneNode & parent, float value)
{
	SceneNode & sref = parent.GetNode(s);
	for (std::vector <GuiWidget *>::iterator i = widgets.begin(); i != widgets.end(); ++i)
	{
		(*i)->SetAlpha(sref, value);
	}
}

void GuiPage::ProcessInput(
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
		for (std::vector<GuiControl *>::iterator i = controls.begin(); i != controls.end(); ++i)
		{
			if ((**i).Focus(cursorx, cursory))
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
			active_control->Signal(GuiControl::SELECTDOWN);
		else if (select)
			active_control->Signal(GuiControl::SELECTUP);
		else if (moveleft)
			active_control->Signal(GuiControl::MOVELEFT);
		else if (moveright)
			active_control->Signal(GuiControl::MOVERIGHT);
		else if (moveup)
			active_control->Signal(GuiControl::MOVEUP);
		else if (movedown)
			active_control->Signal(GuiControl::MOVEDOWN);
	}
}

void GuiPage::Update(SceneNode & parent, float dt)
{
	SceneNode & sref = parent.GetNode(s);
	for (std::vector <GuiWidget *>::iterator i = widgets.begin(); i != widgets.end(); ++i)
	{
		(*i)->Update(sref, dt);
	}
}

void GuiPage::SetLabelText(const std::map<std::string, std::string> & label_text)
{
	for (std::map <std::string, GuiLabel*>::const_iterator i = labels.begin(); i != labels.end(); ++i)
	{
		const std::map<std::string, std::string>::const_iterator n = label_text.find(i->first);
		if (n != label_text.end())
			i->second->SetText(n->second);
	}
}

GuiLabel * GuiPage::GetLabel(const std::string & name)
{
	std::map <std::string, GuiLabel*>::const_iterator i = labels.find(name);
	if (i != labels.end())
		return i->second;
	return 0;
}

SceneNode & GuiPage::GetNode(SceneNode & parentnode)
{
	return parentnode.GetNode(s);
}

void GuiPage::Clear(SceneNode & parentnode)
{
	for (std::vector <GuiWidget *>::iterator i = widgets.begin(); i != widgets.end(); ++i)
	{
		delete *i;
	}
	for (std::vector <GuiControl *>::iterator i = controls.begin(); i != controls.end(); ++i)
	{
		delete *i;
	}

	widgets.clear();
	controls.clear();
	labels.clear();
	control_set.clear();
	action_set.clear();
	action_setn.clear();

	if (s.valid())
	{
		SceneNode & sref = parentnode.GetNode(s);
		sref.Clear();
	}
	s.invalidate();
}

void GuiPage::SetActiveControl(GuiControl & control)
{
	if (active_control != &control)
	{
		assert(active_control);
		active_control->Signal(GuiControl::BLUR);
		active_control = &control;
		active_control->Signal(GuiControl::FOCUS);
	}
}


GuiPage::ControlCb::ControlCb()
{
	action.call.bind<ControlCb, &ControlCb::call>(this);
	page = 0;
	control = 0;
}

GuiPage::ControlCb::ControlCb(const ControlCb & other)
{
	*this = other;
}

GuiPage::ControlCb & GuiPage::ControlCb::operator=(const ControlCb & other)
{
	action.call.bind<ControlCb, &ControlCb::call>(this);
	page = other.page;
	control = other.control;
	return *this;
}

void GuiPage::ControlCb::call()
{
	assert(page && control);
	page->SetActiveControl(*control);
}


GuiPage::SignalVal::SignalVal()
{
	action.call.bind<SignalVal, &SignalVal::call>(this);
}

GuiPage::SignalVal::SignalVal(const SignalVal & other)
{
	*this = other;
}

GuiPage::SignalVal & GuiPage::SignalVal::operator=(const SignalVal & other)
{
	action.call.bind<SignalVal, &SignalVal::call>(this);
	signal = other.signal;
	value = other.value;
	return *this;
}

void GuiPage::SignalVal::call()
{
	signal(value);
}


GuiPage::SignalValn::SignalValn()
{
	action.call.bind<SignalValn, &SignalValn::call>(this);
}

GuiPage::SignalValn::SignalValn(const SignalValn & other)
{
	*this = other;
}

GuiPage::SignalValn & GuiPage::SignalValn::operator=(const SignalValn & other)
{
	action.call.bind<SignalValn, &SignalValn::call>(this);
	signal = other.signal;
	value = other.value;
	return *this;
}

void GuiPage::SignalValn::call(int n)
{
	signal(n, value);
}
