#include "guioption.h"

#include <sstream>

const std::string GUIOPTION::null;

GUIOPTION::GUIOPTION() :
	current_valid(false),
	min(0),
	max(1),
	percentage(true)
{
	current_value = values.end();
}

void GUIOPTION::ReplaceValues(const std::list <std::pair<std::string, std::string> > & newvalues)
{
	values = newvalues;
	SetToFirstValue();
}

void GUIOPTION::SetInfo(const std::string & newtext, const std::string & newdesc, const std::string & newtype)
{
	text = newtext;
	description = newdesc;
	type = newtype;
}

void GUIOPTION::Insert(const std::string & stored_value, const std::string & display_value)
{
	values.push_back(std::pair<std::string, std::string>(stored_value, display_value));
}

void GUIOPTION::SetMinMaxPercentage(float newmin, float newmax, bool newpercentage)
{
	min = newmin;
	max = newmax;
	percentage = newpercentage;
}

bool GUIOPTION::SetCurrentValue(const std::string & storedvaluename)
{
	if (values.empty())
	{
		non_value_data = storedvaluename;
		current_valid = false;
		signal_val(storedvaluename);
		signal_str(storedvaluename);
		return true;
	}

	current_valid = false;
	if (type == "float")
	{
		float storedvaluefloat(0);
		{
			std::stringstream floatstr;
			floatstr.str(storedvaluename);
			floatstr >> storedvaluefloat;
		}
		for (std::list <std::pair<std::string, std::string> >::iterator i = values.begin(); i != values.end(); ++i)
		{
			float ifloat(0);
			std::stringstream floatstr;
			floatstr.str(i->first);
			floatstr >> ifloat;
			if (ifloat == storedvaluefloat)
			{
				current_valid = true;
				current_value = i;
				break;
			}
		}
	}
	else
	{
		for (std::list <std::pair<std::string, std::string> >::iterator i = values.begin(); i != values.end(); ++i)
		{
			if (i->first == storedvaluename)
			{
				current_valid = true;
				current_value = i;
				break;
			}
		}
	}

	if (!current_valid)
	{
		current_value = values.end();
	}
	else
	{
		SignalValue();
	}

	return current_valid;
}

///increment the current_value to the next value in the values list
void GUIOPTION::Increment()
{
	if (values.empty())
	{
		current_value = values.end();
		return;
	}

	if (current_value == values.end())
		current_value = values.begin();
	else
	{
		current_value++;
		if (current_value == values.end())
			current_value = values.begin();
	}

	SignalValue();
}

///decrement the current_value to the previous value in the values list
void GUIOPTION::Decrement()
{
	if (values.empty())
	{
		current_value = values.end();
		return;
	}

	if (current_value == values.begin())
	{
		current_value = values.end();
	}

	current_value--;

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
	{
		return non_value_data;
	}
	else if (current_value == values.end() || !current_valid)
	{
		return null;
	}
	else
	{
		return current_value->second;
	}
}

const std::string & GUIOPTION::GetCurrentStorageValue() const
{
	if (values.empty())
	{
		return non_value_data;
	}
	else if (current_value == values.end() || !current_valid)
	{
		return null;
	}
	else
	{
		return current_value->first;
	}
}

const std::list <std::pair<std::string,std::string> > & GUIOPTION::GetValueList() const
{
	/*std::list <std::pair<std::string,std::string> > valuelist;
	for (std::map <std::string, std::string>::const_iterator i = values.begin(); i != values.end(); i++)
	{
		valuelist.push_back(std::pair<std::string,std::string> (i->first,i->second));
	}
	return valuelist;*/
	return values;
}

void GUIOPTION::SignalValue()
{
	current_valid = !values.empty();
	if (current_valid)
	{
		signal_val(current_value->first);
		signal_str(current_value->second);
	}
}
