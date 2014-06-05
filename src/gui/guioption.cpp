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

#include "guioption.h"
#include <sstream>

static const std::string null;

GuiOption::GuiOption() :
	m_current_value(0),
	m_type(type_float),
	m_min(0),
	m_max(0),
	m_percent(false)
{
	get_val.call.bind<GuiOption, &GuiOption::GetStorageValues>(this);
	get_str.call.bind<GuiOption, &GuiOption::GetDisplayValues>(this);
	set_valn.call.bind<GuiOption, &GuiOption::SetCurrentValueNorm>(this);
	set_val.call.bind<GuiOption, &GuiOption::SetCurrentValue>(this);
	set_nth.call.bind<GuiOption, &GuiOption::SetToNthValue>(this);
	set_prev.call.bind<GuiOption, &GuiOption::Decrement>(this);
	set_next.call.bind<GuiOption, &GuiOption::Increment>(this);
}

GuiOption::GuiOption(const GuiOption & other)
{
	*this = other;
	get_val.call.bind<GuiOption, &GuiOption::GetStorageValues>(this);
	get_str.call.bind<GuiOption, &GuiOption::GetDisplayValues>(this);
	set_valn.call.bind<GuiOption, &GuiOption::SetCurrentValueNorm>(this);
	set_val.call.bind<GuiOption, &GuiOption::SetCurrentValue>(this);
	set_nth.call.bind<GuiOption, &GuiOption::SetToNthValue>(this);
	set_prev.call.bind<GuiOption, &GuiOption::Decrement>(this);
	set_next.call.bind<GuiOption, &GuiOption::Increment>(this);
}

GuiOption & GuiOption::operator=(const GuiOption & other)
{
	m_values = other.m_values;
	m_current_value = other.m_current_value;
	m_description = other.m_description;
	m_type = other.m_type;
	m_data = other.m_data;
	m_min = other.m_min;
	m_max = other.m_max;
	m_percent = other.m_percent;

	signal_update = other.signal_update;
	signal_valn = other.signal_valn;
	signal_val = other.signal_val;
	signal_str = other.signal_str;

	return *this;
}

void GuiOption::SetValues(const std::string & curvalue, const List & newvalues)
{
	m_values = newvalues;

	std::ostringstream s;
	s << m_values.size();
	signal_update(s.str());

	SetCurrentValue(curvalue);
}

void GuiOption::SetInfo(
	const std::string & newdesc,
	const std::string & newtype,
	float newmin, float newmax,
	bool newpercent)
{
	m_description = newdesc;
	m_type = (newtype == "float") ? type_float : type_other;
	m_min = newmin;
	m_max = newmax;
	m_percent = newpercent;
}

void GuiOption::SetCurrentValue(const std::string & value)
{
	if (m_values.empty())
	{
		m_data = value;
	}
	else
	{
		size_t current_value = 0;
		for (List::iterator i = m_values.begin(); i != m_values.end(); ++i)
		{
			if (IsFloat())
			{
				// number of trailing zeros might differ, use min match
				size_t len = std::min(i->first.length(), value.length());
				if (!i->first.compare(0, len, value, 0, len)) break;
			}
			else
			{
				if (i->first == value) break;
			}
			++current_value;
		}
		m_current_value = (current_value < m_values.size()) ? current_value : 0;
	}

	SignalValue();
}

void GuiOption::Increment()
{
	if (m_values.empty())
	{
		if (IsFloat())
		{
			std::stringstream s, v;
			float f;
			v << m_data;
			v >> f;
			f += (m_max - m_min) / 16;	// hardcoded for now
			if (f > m_max) f = m_max;
			s << f;
			m_data = s.str();
		}
		else
		{
			m_current_value = m_values.size();
			return;
		}
	}
	else
	{
		++m_current_value;
		if (m_current_value >= m_values.size())
			m_current_value = 0;
	}

	SignalValue();
}

void GuiOption::Decrement()
{
	if (m_values.empty())
	{
		if (IsFloat())
		{
			std::stringstream s, v;
			float f;
			v << m_data;
			v >> f;
			f -= (m_max - m_min) / 16;	// hardcoded for now
			if (f < m_min) f = m_min;
			s << f;
			m_data = s.str();
		}
		else
		{
			m_current_value = m_values.size();
			return;
		}
	}
	else
	{
		if (m_current_value == 0)
			m_current_value = m_values.size();
		--m_current_value;
	}

	SignalValue();
}

void GuiOption::SetToNthValue(int n)
{
	if (m_values.empty())
	{
		// fixme: handle non list options
	}
	else
	{
		// fixme: add bound checks
		m_current_value = n;
	}

	SignalValue();
}

void GuiOption::SetToFirstValue()
{
	m_current_value = 0;
	SignalValue();
}

const std::string & GuiOption::GetCurrentDisplayValue() const
{
	if (m_values.empty())
		return m_data;

	if (m_current_value == m_values.size())
		return null;

	return m_values[m_current_value].second;
}

const std::string & GuiOption::GetCurrentStorageValue() const
{
	if (m_values.empty())
		return m_data;

	if (m_current_value == m_values.size())
		return null;

	return m_values[m_current_value].first;
}

const GuiOption::List & GuiOption::GetValueList() const
{
	return m_values;
}

void GuiOption::SignalValue()
{
	if (m_values.empty())
	{
		if (!IsFloat())
		{
			signal_val(m_data);
			signal_str(m_data);
			return;
		}

		if (m_min != 0 || m_max != 1)
		{
			std::stringstream s, v;
			float f;
			v << m_data;
			v >> f;
			s << (f - m_min) / (m_max - m_min);
			signal_valn(s.str());
		}
		else
		{
			signal_valn(m_data);
		}
		signal_val(m_data);

		if (m_percent)
		{
			// format value string
			std::stringstream s, v;
			float f;
			v << m_data;
			v >> f;
			s << int(f * 100) << "%";
			signal_str(s.str());
		}
		else
		{
			// format value string
			std::stringstream s, v;
			float f;
			v << m_data;
			v >> f;
			s.setf(std::ios::fixed);
			s.precision(2);
			s << f;
			signal_str(s.str());
		}
	}
	else
	{
		signal_val(m_values[m_current_value].first);
		signal_str(m_values[m_current_value].second);
		if (signal_nth.connected())
		{
			std::stringstream s;
			s << m_current_value;
			signal_nth(s.str());
		}
	}
}

void GuiOption::SetCurrentValueNorm(const std::string & value)
{
	// only valid for floats
	assert(m_type == type_float);
	if (m_min != 0 || m_max != 1)
	{
		std::stringstream s, v;
		float f;
		v << value;
		v >> f;
		s << f * (m_max - m_min) + m_min;
		SetCurrentValue(s.str());
	}
	else
	{
		SetCurrentValue(value);
	}
}

void GuiOption::GetDisplayValues(int offset, std::vector<std::string> & vals)
{
	// clamp offset
	size_t noffset = (offset < 0) ? 0 : offset;

	// get values
	size_t n = 0;
	while (n < vals.size() && n + noffset < m_values.size())
	{
		vals[n] = m_values[n + noffset].second;
		++n;
	}
	vals.resize(n);
}

void GuiOption::GetStorageValues(int offset, std::vector<std::string> & vals)
{
	// clamp offset
	size_t noffset = (offset < 0) ? 0 : offset;

	// get values
	size_t n = 0;
	while (n < vals.size() && n + noffset < m_values.size())
	{
		vals[n] = m_values[n + noffset].first;
		++n;
	}
	vals.resize(n);
}
