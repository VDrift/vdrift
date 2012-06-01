#include "gui/guioption.h"

#include <sstream>

static const std::string null;

GUIOPTION::GUIOPTION() :
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
	non_value_data = other.non_value_data;
	description = other.description;
	text = other.text;
	type = other.type;
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

void GUIOPTION::SetInfo(const std::string & newtext, const std::string & newdesc, const std::string & newtype)
{
	text = newtext;
	description = newdesc;
	type = newtype;
}

void GUIOPTION::SetMinMaxPercentage(float newmin, float newmax, bool newpercent)
{
	min = newmin;
	max = newmax;
	percent = newpercent;
}

void GUIOPTION::SetCurrentValue(const std::string & storedvaluename)
{
	if (values.empty())
	{
		non_value_data = storedvaluename;
	}
	else
	{
		bool current_valid = false;
		for (std::list <std::pair<std::string, std::string> >::iterator i = values.begin(); i != values.end(); ++i)
		{
			if (i->first == storedvaluename)
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
		if (type == "float")
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
		if (type == "float")
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
		if (type != "float")
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
			std::stringstream s, v;
			float f;
			v << non_value_data;
			v >> f;
			s << int(f * 100) << "%";
			signal_str(s.str());
		}
		else
		{
			signal_str(non_value_data);
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
	if (type != "float")
		return;
	
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