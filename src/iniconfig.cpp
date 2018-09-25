#include "iniconfig.h"

#include <algorithm>

std::string toLowerCase(const std::string& str)
{
	std::string lower = str;
	std::transform(lower.begin(), lower.end(), lower.begin(), ::tolower);
	return lower;
}
 
std::string trim(const std::string& str, const std::string& chars = "\t\n\v\f\r ")
{
	std::string trimmed = str;
	trimmed.erase(trimmed.find_last_not_of(chars) + 1);
	trimmed.erase(0, trimmed.find_first_not_of(chars));
	return trimmed;
}

INIConfiguration::Section::Section (const std::string& sname) : m_Name (sname), m_PropertyMap()
{
}

bool INIConfiguration::Section::getStringProperty (const std::string& name, std::string& outPropStr) const
{
	try
	{
		outPropStr = m_PropertyMap.at(toLowerCase(name)).m_Value;
		return true;
	}
	catch (std::out_of_range& oorexcept)
	{
		return false;
	}
}

bool INIConfiguration::load (std::istream& is)
{
	if (!is.good())
	{
		return false;
	}

	std::string currSectionName;

	std::string line;
	std::getline (is, line);

	while (!is.eof() && !is.bad())
	{
		if (line[0] == '[')
		{
			currSectionName = line.substr (1, line.find_last_of (']') - 1);
		}
		else if (line[0] != '#' && line.length() > 2)
		{
			int crloc = line.length() - 1;

			if (crloc >= 0  && line[crloc] == '\r') //check for Windows-style newline
				line.resize (crloc);                //and correct

			size_t equalsPos = line.find_first_of ("=");

			if (equalsPos != std::string::npos)
			{
				std::string key = line.substr (0, equalsPos);
				std::string val = line.substr (equalsPos + 1);

				addProperty (currSectionName, key , val);
			}
		}

		std::getline (is, line);
	}

	if (is.bad())
	{
		return false;
	}

	return true;
}


std::string INIConfiguration::getStringProperty(const std::string& sname, const std::string& name, const std::string& def) const
{
	auto sectionIt = m_SectionMap.find(toLowerCase(sname));

	if (sectionIt != m_SectionMap.end())
	{
		std::string prop;

		if(sectionIt->second.getStringProperty(name, prop))
		{
			return prop;
		}
	}
	
	return def;
}

void INIConfiguration::addProperty (const std::string& sname, const std::string& name, const std::string& val)
{
	if (m_SectionMap.find (toLowerCase(sname)) == m_SectionMap.end())
	{
		m_SectionMap.emplace (toLowerCase(sname), Section (sname));
	}

	Section::Property p;
	p.m_Name = trim(name);
	p.m_Value = trim(val);

	m_SectionMap.at (toLowerCase(sname)).m_PropertyMap[toLowerCase(p.m_Name)] = p;
}
