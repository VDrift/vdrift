#include "gui.h"

#include "configfile.h"

#include <map>
using std::map;

#include <string>
using std::string;

#include <iostream>
using std::endl;

#include <list>
using std::list;

#include <sstream>
using std::stringstream;

string GUI::LoadOptions(const std::string & optionfile, const std::map<std::string, std::list <std::pair <std::string, std::string> > > & valuelists, std::ostream & error_output)
{
	CONFIGFILE o;
	
	if (!o.Load(optionfile))
	{
		error_output << "Can't find options file: " << optionfile << endl;
		return "File loading";
	}
	
	std::list <std::string> sectionlist;
	o.GetSectionList(sectionlist);
	for (std::list <std::string>::iterator i = sectionlist.begin(); i != sectionlist.end(); ++i)
	{
		if (!i->empty())
		{
			string cat, name, defaultval, values, text, desc, type;
			if (!o.GetParam(*i+".cat", cat)) return *i+".cat";
			if (!o.GetParam(*i+".name", name)) return *i+".name";
			if (!o.GetParam(*i+".default", defaultval)) return *i+".default";
			if (!o.GetParam(*i+".values", values)) return *i+".values";
			if (!o.GetParam(*i+".title", text)) return *i+".title";
			if (!o.GetParam(*i+".desc", desc)) return *i+".desc";
			if (!o.GetParam(*i+".type", type)) return *i+".type";
			
			float min(0),max(1);
			bool percentage(true);
			o.GetParam(*i+".min",min);
			o.GetParam(*i+".max",max);
			o.GetParam(*i+".percentage",percentage);
			
			string optionname = cat+"."+name;
			
			optionmap[optionname].SetInfo(text, desc, type);
			optionmap[optionname].SetMinMaxPercentage(min, max, percentage);
			
			//different ways to populate the options
			if (values == "list")
			{
				int valuenum;
				if (!o.GetParam(*i+".num_vals", valuenum)) return *i+".num_vals";
				
				for (int n = 0; n < valuenum; n++)
				{
					stringstream tstr;
					tstr.width(2);
					tstr.fill('0');
					tstr << n;
					
					string displaystr, storestr;
					if (!o.GetParam(*i+".opt"+tstr.str(), displaystr)) return *i+".opt"+tstr.str();
					if (!o.GetParam(*i+".val"+tstr.str(), storestr)) return *i+".val"+tstr.str();
					
					optionmap[optionname].Insert(storestr, displaystr);
				}
			}
			else if (values == "bool")
			{
				string truestr, falsestr;
				if (!o.GetParam(*i+".true", truestr)) return *i+".true";
				if (!o.GetParam(*i+".false", falsestr)) return *i+".false";
					
				optionmap[optionname].Insert("true", truestr);
				optionmap[optionname].Insert("false", falsestr);
			}
			else if (values == "ip_valid")
			{
				
			}
			else if (values == "port_valid")
			{
				
			}
			else if (values == "float")
			{
				
			}
			else if (values == "string")
			{
				
			}
			else //assume it's "values", meaning the GAME populates the values
			{
				std::map<std::string, std::list <std::pair<std::string,std::string> > >::const_iterator vlist = valuelists.find(values);
				if (vlist == valuelists.end())
				{
					error_output << "Can't find value type \"" << values << "\" in list of GAME values" << endl;
					return "GAME valuelist";
				}
				else
				{
					//std::cout << "Populating values:" << endl;
					for (std::list <std::pair<std::string,std::string> >::const_iterator n = vlist->second.begin(); n != vlist->second.end(); n++)
					{
						//std::cout << "\t" << n->second << endl;
						optionmap[optionname].Insert(n->first, n->second);
					}
				}
			}
			
			optionmap[optionname].SetCurrentValue(defaultval);
		}
	}
	
	UpdateOptions(error_output);
	
	return "";
}
