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
#include "guiradialslider.h"
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
	float xywh[4];
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
		else if (pagefile.get(section, "center-left", l))
			x = 0.5 + l * hwratio;
		else if (pagefile.get(section, "center-right", r))
			x = 0.5 - (r + w) * hwratio;
		w = w * hwratio;
	}
	else
	{
		float l(0), r(0);
		if (pagefile.get(section, "left", l))
			l = l * hwratio;
		else if (pagefile.get(section, "center-left", l))
			l = 0.5 + l * hwratio;
		if (pagefile.get(section, "right", r))
			r = r * hwratio;
		else if (pagefile.get(section, "center-right", r))
			r = 0.5 + r * hwratio;
		x = l;
		w = 1 - (l + r);
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
	re.xywh[0] = x;
	re.xywh[1] = y;
	re.xywh[2] = w;
	re.xywh[3] = h;
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
			posn = actionstr.find('"', n + 2) + 1;

		std::string action = actionstr.substr(pos, posn - pos);
		pos = posn;

		typename ActionMap::const_iterator it = actionmap.find(action);
		if (it != actionmap.end())
			it->second->connect(signal);
	}
}

template <class ActionMap, class WidgetMap, class WidgetListMap, class ActionValSet, class ActionValnSet>
static void ParseActions(
	const std::string & actions,
	const ActionMap & vactionmap,
	const WidgetMap & widgetmap,
	const WidgetListMap & widgetlistmap,
	ActionValSet & action_val_set,
	ActionValnSet & action_valn_set)
{
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
			posn = actions.find('"', n + 2) + 1;

		std::string action = actions.substr(pos, posn - pos);
		std::string aname = actions.substr(pos, n - pos);

		pos = posn;

		// check if action is in vactionmap
		typename ActionMap::const_iterator vai = vactionmap.find(aname);
		if (vai != vactionmap.end())
		{
			action_val_set.insert(std::make_pair(action, vai->second));
			continue;
		}

		size_t wn = aname.find('.');
		if (wn == 0 || wn == std::string::npos)
			continue;

		std::string wname = aname.substr(0, wn);
		std::string pname = aname.substr(wn + 1);

		// check if action is setting a widget property
		typename WidgetMap::const_iterator wi = widgetmap.find(wname);
		if (wi != widgetmap.end())
		{
			Slot1<const std::string &> * pslot;
			if (wi->second->GetProperty(pname, pslot))
				action_val_set.insert(std::make_pair(action, pslot));
			continue;
		}

		// check if action is setting a widget list property (only valid for control lists)
		typename WidgetListMap::const_iterator wli = widgetlistmap.find(wname);
		if (wli != widgetlistmap.end())
		{
			Slot2<int, const std::string &> * pslot;
			if (wli->second->GetProperty(pname, pslot))
				action_valn_set.insert(std::make_pair(action, pslot));
		}
	}
}

template <class ActionValSet, class SignalValVec, class ActionMap>
static void RegisterActions(
	const GuiLanguage & lang,
	const ActionValSet & actionvs,
	SignalValVec & signalvs,
	ActionMap & actionmap)
{
	signalvs.reserve(actionvs.size());
	for (typename ActionValSet::const_iterator i = actionvs.begin(); i != actionvs.end(); ++i)
	{
		signalvs.push_back(typename SignalValVec::value_type());

		const std::string & action = i->first;
		size_t n = action.find(':') + 1;
		if (action[n] == '"')
			signalvs.back().value = lang(action.substr(n + 1, action.size() - n - 2));
		else
			signalvs.back().value = action.substr(n);
		i->second->connect(signalvs.back().signal);

		actionmap[action] = &signalvs.back().action;
	}
}

template <class ControlSecVec, class ControlVec, class ControlCbVec, class ActionMap>
static void RegisterControls(
	const ControlSecVec & controlsecs,
	const ControlVec & controls,
	GuiPage * page,
	ControlCbVec & controlcbs,
	ActionMap & actionmap)
{
	for (size_t i = 0; i < controlsecs.size(); ++i)
	{
		controlcbs.push_back(typename ControlCbVec::value_type());
		controlcbs.back().page = page;
		controlcbs.back().control = controls[i];
		actionmap[controlsecs[i]->first] = &controlcbs.back().action;
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
	Clear();
}

bool GuiPage::Load(
	const std::string & path,
	const std::string & texpath,
	const float hwratio,
	const GuiLanguage & lang,
	const Font & font,
	const StrSignalMap & vsignalmap,
	const StrVecSlotMap & vnactionmap,
	const StrSlotMap & vactionmap,
	IntSlotMap nactionmap,
	SlotMap actionmap,
	ContentManager & content,
	std::ostream & error_output)
{
	Clear();

	Config pagefile;
	if (!pagefile.load(path))
	{
		error_output << "Couldn't find GUI page file: " << path << std::endl;
		return false;
	}

	if (!pagefile.get("", "name", name, error_output))
		return false;

	//error_output << "Loading " << path << std::endl;

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
		float x0 = r.xywh[0] - r.xywh[2] * 0.5f;
		float y0 = r.xywh[1] - r.xywh[3] * 0.5f;
		float x1 = r.xywh[0] + r.xywh[2] * 0.5f;
		float y1 = r.xywh[1] + r.xywh[3] * 0.5f;

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
				StrVecSlotMap::const_iterator vni = vnactionmap.find(value);
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
				widget_list->SetupDrawable(node, font, align, scalex, scaley, r.z);

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
					node, font, align, scalex, scaley, r.xywh, r.z);

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
			std::string slider, ext, path = texpath;
			pagefile.get(section, "path", path);
			pagefile.get(section, "ext", ext);

			float uv[4] = {0, 0, 1, 1};
			Slice<float*> slice(uv, uv + 4);
			pagefile.get(section, "image-rect", slice);

			GuiImageList * widget_list = 0;
			if (LoadList(pagefile, section, x0, y0, x1, y1, hwratio, widget_list))
			{
				// init drawable
				widget_list->SetupDrawable(node, content, path, ext, uv, r.z);

				// connect with the value list
				StrVecSlotMap::const_iterator vni = vnactionmap.find(value);
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
			else if (pagefile.get(section, "slider", slider))
			{
				int fill = 0;
				std::string fillstr;
				pagefile.get(section, "fill", fillstr);
				if (fillstr == "lower")
					fill = 1;
				else if (fillstr == "upper")
					fill = -1;

				TextureInfo texinfo;
				texinfo.mipmap = false;
				texinfo.repeatu = false;
				texinfo.repeatv = false;
				std::shared_ptr<Texture> tex;
				content.load(tex, texpath, value, texinfo);

				float radius = 0.0;
				if (pagefile.get(section, "radius", radius))
				{
					float start_angle(0), end_angle(2 * M_PI);
					pagefile.get(section, "start-angle", start_angle);
					pagefile.get(section, "end-angle", end_angle);

					GuiRadialSlider * new_widget = new GuiRadialSlider();
					new_widget->SetupDrawable(
						node, tex, r.xywh, r.z, start_angle, end_angle, radius,
						hwratio, fill, error_output);

					ConnectAction(slider, vsignalmap, new_widget->set_value);
					widget = new_widget;
				}
				else
				{
					GuiSlider * new_widget = new GuiSlider();
					new_widget->SetupDrawable(
						node, tex, r.xywh, r.z, fill, error_output);

					ConnectAction(slider, vsignalmap, new_widget->set_value);
					widget = new_widget;
				}

				widgetmap[section->first] = widget;
			}
			else
			{
				GuiImage * new_widget = new GuiImage();
				new_widget->SetupDrawable(
					node, content, path, ext,
					r.xywh, uv, r.z);

				ConnectAction(value, vsignalmap, new_widget->set_image);

				widgetmap[section->first] = new_widget;
				widget = new_widget;
			}
		}

		// set widget properties (connect property slots)
		if (widget)
		{
			std::string val;
			if (pagefile.get(section, "visible", val))
				ConnectAction(val, vsignalmap, widget->set_visible);
			if (pagefile.get(section, "opacity", val))
				ConnectAction(val, vsignalmap, widget->set_opacity);
			if (pagefile.get(section, "color", val))
				ConnectAction(val, vsignalmap, widget->set_color);
			if (pagefile.get(section, "hue", val))
				ConnectAction(val, vsignalmap, widget->set_hue);
			if (pagefile.get(section, "sat", val))
				ConnectAction(val, vsignalmap, widget->set_sat);
			if (pagefile.get(section, "val", val))
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

				control = control_list;
				controlnit.push_back(section);
				controllists.push_back(control_list);
			}
			else
			{
				control = new GuiControl();
				controlit.push_back(section);
				controls.push_back(control);
			}
			control->SetRect(x0, y0, x1, y1);

			if (focus)
				active_control = control;
		}
	}

	// load control actions (connect control signals)

	// parse control event actions with values(arguments)
	typedef std::pair<std::string, Slot2<int, const std::string &>*> ActionValn;
	typedef std::pair<std::string, Slot1<const std::string &>*> ActionVal;
	std::set<ActionValn> action_valn_set;
	std::set<ActionVal> action_val_set;
	std::string actionstr;
	for (size_t i = 0; i < controlit.size(); ++i)
	{
		for (size_t j = 0; j < GuiControl::signal_names.size(); ++j)
		{
			if (pagefile.get(controlit[i], GuiControl::signal_names[j], actionstr))
				ParseActions(actionstr, vactionmap, widgetmap, widgetlistmap,
					action_val_set, action_valn_set);
		}
	}
	for (size_t i = 0; i < controlnit.size(); ++i)
	{
		for (size_t j = 0; j < GuiControl::signal_names.size(); ++j)
		{
			if (pagefile.get(controlnit[i], GuiControl::signal_names[j], actionstr))
				ParseActions(actionstr, vactionmap, widgetmap, widgetlistmap,
					action_val_set, action_valn_set);
		}
	}

	// parse page event actions with values
	if (pagefile.get("", "onfocus", actionstr))
		ParseActions(actionstr, vactionmap, widgetmap, widgetlistmap,
			action_val_set, action_valn_set);

	if (pagefile.get("", "oncancel", actionstr))
		ParseActions(actionstr, vactionmap, widgetmap, widgetlistmap,
			action_val_set, action_valn_set);

	// register controls, so that they can be activated by control events
	control_set.reserve(controlit.size() + controlnit.size());
	RegisterControls(controlit, controls, this, control_set, actionmap);
	RegisterControls(controlnit, controllists, this, control_set, actionmap);

	// register action calls with a parameter, so that they can be signaled by controls
	RegisterActions(lang, action_val_set, action_set, actionmap);
	RegisterActions(lang, action_valn_set, action_setn, nactionmap);

	// connect control signals with their actions
	for (size_t i = 0; i < controlit.size(); ++i)
	{
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
		for (size_t j = 0; j < GuiControl::EVENTNUM; ++j)
		{
			if (pagefile.get(controlnit[i], GuiControl::signal_names[j], actionstr))
			{
				ConnectActions(actionstr, actionmap, controllists[i]->m_signal[j]);
				ConnectActions(actionstr, nactionmap, controllists[i]->m_signaln[j]);
			}
		}
	}

	// connect page event signals with their actions
	if (pagefile.get("", "onfocus", actionstr))
		ConnectActions(actionstr, actionmap, onfocus);

	if (pagefile.get("", "oncancel", actionstr))
		ConnectActions(actionstr, actionmap, oncancel);

	controls.insert(controls.end(), controllists.begin(), controllists.end());

	// set active control
	if (!active_control && !controls.empty())
		active_control = controls[0];

	// enable active control
	if (active_control)
		active_control->Signal(GuiControl::FOCUS);

	// set default control
	default_control = active_control;

	return true;
}

void GuiPage::SetVisible(bool value)
{
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

void GuiPage::SetAlpha(float value)
{
	for (std::vector <GuiWidget *>::iterator i = widgets.begin(); i != widgets.end(); ++i)
		(*i)->SetAlpha(node, value);
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

void GuiPage::Update(float dt)
{
	for (std::vector <GuiWidget *>::iterator i = widgets.begin(); i != widgets.end(); ++i)
		(*i)->Update(node, dt);
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

SceneNode & GuiPage::GetNode()
{
	return node;
}

void GuiPage::Clear()
{
	for (std::vector <GuiWidget *>::iterator i = widgets.begin(); i != widgets.end(); ++i)
		delete *i;

	for (std::vector <GuiControl *>::iterator i = controls.begin(); i != controls.end(); ++i)
		delete *i;

	node.Clear();
	labels.clear();
	controls.clear();
	widgets.clear();
	control_set.clear();
	action_set.clear();
	action_setn.clear();
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
