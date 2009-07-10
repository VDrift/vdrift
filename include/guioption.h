#ifndef _GUIOPTION_H
#define _GUIOPTION_H

#include <map>
#include <string>
#include <list>
#include <sstream>

class GUIOPTION
{
	private:
		bool current_valid;
		std::list <std::pair<std::string, std::string> >::const_iterator current_value;
		std::list <std::pair<std::string, std::string> > values; //the first element of the pair is the (sometimes numeric) stored value, while the second element is the display value.  sometimes they are the same.
		std::string non_value_data; //this is used when the values map is empty, such as for numeric settings
		std::string description;
		std::string text;
		std::string type;
		float min;
		float max;
		bool percentage;
		
	public:
		GUIOPTION() : current_valid(false),min(0),max(1),percentage(true) {current_value = values.end();}
		void ReplaceValues(const std::list <std::pair<std::string, std::string> > & newvalues)
		{
			//if (text == "Color") std::cout << "car paint: values replaced" << std::endl;
			values = newvalues;
			
			SetToFirstValue();
		}
		void SetInfo(const std::string & newtext, const std::string & newdesc, const std::string & newtype)
		{
			//std::cout << newtext << std::endl;
			text = newtext;
			description = newdesc;
			type = newtype;
		}
		void Insert(const std::string & stored_value, const std::string & display_value)
		{
			//if (text == "Color") std::cout << "car paint: values added: " << stored_value << std::endl;
			
			/*bool duplicate = false;
			for (std::list <std::pair<std::string, std::string> >::iterator i = values.begin(); i != values.end(); i++)
			{
				if (i->first == stored_value && i->second == display_value)
				{
					duplicate = true;
				}
			}
			
			if (!duplicate)*/
				values.push_back(std::pair<std::string, std::string>(stored_value,display_value));
		}
		///returns true if the storedvaluename was found
		bool SetCurrentValue(const std::string & storedvaluename)
		{
			//if (text == "Color") std::cout << "car paint: setcurrentvalue " << storedvaluename << " !! " << values.empty() << std::endl;
			
			if (values.empty())
			{
				non_value_data = storedvaluename;
				current_valid = false;
				return true;
			}
			else
			{
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
						}
					}
				}
				if (!current_valid)
					current_value = values.end();
				return current_valid;
			}
		}
		
		///increment the current_value to the next value in the values list
		void Increment()
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
			
			current_valid = !values.empty();
		}
		
		///decrement the current_value to the previous value in the values list
		void Decrement()
		{
			if (values.empty())
			{
				current_value = values.end();
				return;
			}
			
			if (current_value == values.begin())
				current_value = values.end();
		
			current_value--;
			
			current_valid = !values.empty();
		}
		
		void SetToFirstValue()
		{
			current_value = values.begin();
			current_valid = !values.empty();
		}
		
		const std::string GetCurrentDisplayValue() const
		{
			if (values.empty())
				return non_value_data;
			else if (current_value == values.end() || !current_valid)
				return "";
			else
				return current_value->second;
		}
		const std::string GetCurrentStorageValue() const
		{
			if (values.empty())
				return non_value_data;
			else if (current_value == values.end() || !current_valid)
				return "";
			else
				return current_value->first;
		}

		const std::string & GetDescription() const
		{
			return description;
		}
	
		const std::string & GetText() const
		{
			return text;
		}
	
		const std::list <std::pair<std::string,std::string> > & GetValueList() const
		{
			/*std::list <std::pair<std::string,std::string> > valuelist;
			for (std::map <std::string, std::string>::const_iterator i = values.begin(); i != values.end(); i++)
			{
				valuelist.push_back(std::pair<std::string,std::string> (i->first,i->second));
			}
			return valuelist;*/
			return values;
		}

		float GetMin() const
		{
			return min;
		}
	
		float GetMax() const
		{
			return max;
		}
	
		bool GetPercentage() const
		{
			return percentage;
		}
	
		void SetMinMaxPercentage(float newmin, float newmax, bool newpercentage)
		{
			min = newmin;
			max = newmax;
			percentage = newpercentage;
		}
};

#endif
