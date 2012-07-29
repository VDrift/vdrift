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

#include "gui/guioption.h"

#include <sstream>

static const std::string null;

GUIOPTION::GUIOPTION() :
	type(type_float),
	min(0),
	max(0),
	percent(false)
{
	current_value = values.begin();
	set_valn.call.bind<GUIOPTION, &GUIOPTION::SetCurrentValueNorm>(this);
	set_val.call.bind<GUIOPTION, &GUIOPTION::SetCurrentValue>(this);
	prev_val.call.bind<GUIOPTION, &GUIOPTION::Decrement>(this);
	next_val.call.bind<GUIOPTION, &GUIOPTION::Increment>(this);
}

GUIOPTION::GUIOPTION(const GUIOPTION & other)
{
	*this = other;
	set_valn.call.bind<GUIOPTION, &GUIOPTION::SetCurrentValueNorm>(this);
	set_val.call.bind<GUIOPTION, &GUIOPTION::SetCurrentValue>(this);
	prev_val.call.bind<GUIOPTION, &GUIOPTION::Decrement>(this);
	next_val.call.bind<GUIOPTION, &GUIOPTION::Increment>(this);
}

GUIOPTION & GUIOPTION::operator=(const GUIOPTION & other)
{
	values = other.values;
	current_value = values.begin(); // fixme
	description = other.description;
	type = other.type;
	non_value_data = other.non_value_data;
	min = other.min;
	max = other.max;
	percent = other.percent;
	signal_val = other.signal_val;
	signal_str = other.signal_str;
	return *this;
}

void GUIOPTION::SetValues(const std::string & curvalue, std::list <std::pair<std::string, std::string> > & newvalues)
{
	values.clear();
	values.splice(values.begin(), newvalues);
	SetCurrentValue(curvalue);
}

void GUIOPTION::SetInfo(const std::string & newdesc, const std::string & newtype)
{
	description = newdesc;

	if (newtype == "float")
		type = type_float;
	else
		type = type_other;
}

void GUIOPTION::SetMinMaxPercentage(float newmin, float newmax, bool newpercent)
{
	min = newmin;
	max = newmax;
	percent = newpercent;
}

void GUIOPTION::SetCurrentValue(const std::string & value)
{
	if (values.empty())
	{
		non_value_data = value;
	}
	else
	{
		bool current_valid = false;
		for (std::list <std::pair<std::string, std::string> >::iterator i = values.begin(); i != values.end(); ++i)
		{
			bool match;
			if (type == type_float)
			{
				// number of trailing zeros might differ, use min match
				size_t len = std::min(i->first.length(), value.length());
				match = !i->first.compare(0, len, value, 0, len);
			}
			else
			{
				match = (i->first == value);
			}

			if (match)
			{
				current_valid = true;
				current_value = i;
				break;
			}
		}

		// if value not found set first value
		if (!current_valid)
			current_value = values.begin();
	}

	SignalValue();
}

void GUIOPTION::Increment()
{
	if (values.empty())
	{
		if (type == type_float)
		{
			std::stringstream s, v;
			float f;
			v << non_value_data;
			v >> f;
			f += (max - min) / 16;	// hardcoded for now
			if (f > max) f = max;
			s << f;
			non_value_data = s.str();
		}
		else
		{
			current_value = values.end();
			return;
		}
	}
	else
	{
		if (current_value == values.end())
		{
			current_value = values.begin();
		}
		else
		{
			++current_value;
			if (current_value == values.end())
				current_value = values.begin();
		}
	}

	SignalValue();
}

void GUIOPTION::Decrement()
{
	if (values.empty())
	{
		if (type == type_float)
		{
			std::stringstream s, v;
			float f;
			v << non_value_data;
			v >> f;
			f -= (max - min) / 16;	// hardcoded for now
			if (f < min) f = min;
			s << f;
			non_value_data = s.str();
		}
		else
		{
			current_value = values.end();
			return;
		}
	}
	else
	{
		if (current_value == values.begin())
			current_value = values.end();
		--current_value;
	}

	SignalValue();
}

void GUIOPTION::SetToFirstValue()
{
	current_value = values.begin();
	SignalValue();
}

const std::string & GUIOPTION::GetCurrentDisplayValue() const
{
	if (values.empty())
		return non_value_data;

	if (current_value == values.end())
		return null;

	return current_value->second;
}

const std::string & GUIOPTION::GetCurrentStorageValue() const
{
	if (values.empty())
		return non_value_data;

	if (current_value == values.end())
		return null;

	return current_value->first;
}

const std::list <std::pair<std::string,std::string> > & GUIOPTION::GetValueList() const
{
	return values;
}

void GUIOPTION::SignalValue()
{
	if (values.empty())
	{
		if (type != type_float)
		{
			signal_val(non_value_data);
			signal_str(non_value_data);
			return;
		}

		if (min != 0 || max != 1)
		{
			std::stringstream s, v;
			float f;
			v << non_value_data;
			v >> f;
			s << (f - min) / (max - min);
			signal_valn(s.str());
		}
		else
		{
			signal_valn(non_value_data);
		}
		signal_val(non_value_data);

		if (percent)
		{
			// format value string
			std::stringstream s, v;
			float f;
			v << non_value_data;
			v >> f;
			s << int(f * 100) << "%";
			signal_str(s.str());
		}
		else
		{
			// format value string
			std::stringstream s, v;
			float f;
			v << non_value_data;
			v >> f;
			s.setf(std::ios::fixed);
			s.precision(2);
			s << f;
			signal_str(s.str());
		}
	}
	else
	{
		signal_val(current_value->first);
		signal_str(current_value->second);
	}
}

void GUIOPTION::SetCurrentValueNorm(const std::string & value)
{
	// only valid for floats
	assert(type == type_float);

	if (min != 0 || max != 1)
	{
		std::stringstream s, v;
		float f;
		v << value;
		v >> f;
		s << f * (max - min) + min;
		SetCurrentValue(s.str());
	}
	else
	{
		SetCurrentValue(value);
	}
}
