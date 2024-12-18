#include "DBConfig.h"

#include <cstring>
#include <filesystem>
#include <fstream>
#include <regex>

#include "Tools_L.h"

namespace fs = std::filesystem;

DBConfig *config;

// needed since strdup returns malloc-ed strings, for C++, it's better to use new/delete
char *dup_str(const std::string& string)
{
	const auto str = new char[string.length() + 1];

	strcpy(str, string.c_str());
	str[string.length()] = '\0';

	return str;
};

static void initDBConfig()
{
	if (config)
		DBFree();
	config = new DBConfig;

	config->dbpath = dup_str((fs::current_path() / "BDD Stack").string());
	config->dm_buffercount = 5;
	config->dm_policy = POLICY_LRU;
	config->pagesize = getpagesize(); // System-wide page size (used for best performance mmap)
	config->dm_maxfilesize = config->pagesize * 3;
}

void LoadDBConfig(const char* fichier_config)
{
	std::regex exp(R"(^([^=]+)\s*=\s*([^=]+)$)", std::regex::icase);
	std::ifstream file(fichier_config, std::ifstream::in);

	initDBConfig();

	size_t ln = 1;
	std::string line;
	while (std::getline(file, line))
	{
		std::smatch match;

		if (!std::regex_match(line, match, exp))
			throw std::invalid_argument("invalid input format: " + line + " (" + std::to_string(ln) + ")");

		std::string prop = match[1].str();
		std::string value = match[2].str();

		std::ranges::transform(prop, prop.begin(), ::tolower);
        std::erase_if(prop, ::isspace);
        std::erase_if(value, ::isspace);

		if (prop == "db_path")
		{
			delete[] config->dbpath;
			config->dbpath = dup_str(value);
		}
		else if (prop == "page_size")
			config->pagesize = std::stoi(value);
		else if (prop == "dm_maxfilesize")
			config->dm_maxfilesize = std::stoi(value);
		else if (prop == "dm_buffercount")
			config->dm_buffercount = std::stoi(value);
		else if (prop == "dm_policy")
		{
			std::ranges::transform(value, value.begin(), ::toupper);
			if (value == "LRU")
				config->dm_policy = POLICY_LRU;
			else if (value == "MRU")
				config->dm_policy = POLICY_MRU;
			else
				throw std::invalid_argument("invalid input format: " + line + " (" + std::to_string(ln) + ")");
		}
		else
			throw std::invalid_argument("unknown property: " + prop + " (" + std::to_string(ln) +")");

		ln++;
	}
}

void DBFree()
{
	delete[] config->dbpath;
	delete config;
}