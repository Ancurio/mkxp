#ifndef INICONFIG_H
#define INICONFIG_H

#include <iostream>
#include <map>

class INIConfiguration
{
	class Section
	{
		friend class INIConfiguration;

		struct Property
		{
			std::string m_Name;
			std::string m_Value;
		};

		typedef std::map<std::string, Property> property_map;
	public:
		Section (const Section& s) = default;
		Section (Section&& s) = default;

		bool getStringProperty (const std::string& name, std::string& outPropStr) const;

	private:
		explicit Section (const std::string& name);

		std::string m_Name;
		property_map m_PropertyMap;
	};

	typedef std::map<std::string, Section> section_map;
public:
	bool load (std::istream& inStream);

	std::string getStringProperty(const std::string& sname, const std::string& name, const std::string& def = "") const;

protected:
	void addProperty (const std::string& sname, const std::string& name, const std::string& val);

private:
	section_map m_SectionMap;
};

#endif // INICONFIG_H
