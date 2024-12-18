#include "SGBD.h"

#include <cassert>
#include <cstring>
#include <iostream>
#include <ranges>
#include <set>
#include <stdexcept>

#include "BufferManager.h"
#include <string_view>
#include <filesystem>
#include <fstream>

#include "SelectCommand.h"
#include <csignal>

namespace fs = std::filesystem;

SGBD *SGBD::instance = nullptr;

static void handleSignal(int sig = -1)
{
    FlushBuffers();
    SGBD::saveDBManagerState();
    SaveState();
    clearBufferManager();
    DBFree();
    diskFREE();

    if (sig == SIGSEGV || sig == SIGABRT || sig == SIGBUS)
        std::exit(1);
    std::exit(0);
}

DBCommandBadSyntax::DBCommandBadSyntax(const std::string &cmd, const std::string &msg)
	: std::runtime_error("error syntax: " + cmd + ": " + msg)
{}

#define REGISTER_COMMAND(cmd_begin, fn) \
	{\
		std::string key = "^\\s*" cmd_begin; \
		if (command_handlers.count(key) == 1) \
			throw std::invalid_argument("Double definition for regex " + key + ", shouldn't happen...");\
		command_handlers[key] = [this](const std::string &command){fn(command);}; \
	}

fs::path SGBD::init_wd;
SGBD::SGBD(int argc, char** argv)
{
    instance = this;

    signal(SIGINT, handleSignal);
    signal(SIGABRT, handleSignal);
    signal(SIGTERM, handleSignal);
    signal(SIGBUS, handleSignal);
    signal(SIGQUIT, handleSignal);
    signal(SIGFPE, handleSignal);
    signal(SIGSEGV, handleSignal);
    signal(SIGUSR1, handleSignal);
    signal(SIGUSR2, handleSignal);

	if (argc != 2)
		throw std::invalid_argument("usage: " + std::string(argv[0]) + " <config file>");
	init_wd = fs::current_path();

	LoadDBConfig(argv[1]);

    fs::path db_path(config->dbpath);
    config->need_init = !is_regular_file(db_path / "dm.save");
    fs::create_directories(db_path / "BinData");

	diskInit();
	LoadState();
	constructBufferManager();
	dbManager.LoadState();

	REGISTER_COMMAND("CREATE DATABASE", ProcessCreateDatabaseCommand);
	REGISTER_COMMAND("SET DATABASE", ProcessSetDatabaseCommand);
	REGISTER_COMMAND("CREATE TABLE", ProcessCreateTableCommand);
	REGISTER_COMMAND("DROP TABLE", ProcessDropTableCommand);
	REGISTER_COMMAND("LIST TABLES", ProcessListTablesCommand);
	REGISTER_COMMAND("DROP TABLES", ProcessDropTablesCommand);
	REGISTER_COMMAND("DROP DATABASES", ProcessDropDatabasesCommand);
	REGISTER_COMMAND("LIST DATABASES", ProcessListDatabasesCommand);
	REGISTER_COMMAND("DROP DATABASE", ProcessDropDatabaseCommand);
	REGISTER_COMMAND("QUIT", ProcessQuitCommand);

	REGISTER_COMMAND("INSERT INTO", ProcessInsertIntoCommand);
	REGISTER_COMMAND("BULKINSERT INTO", ProcessBulkInsertIntoCommand);
	REGISTER_COMMAND("SELECT", ProcessSelectCommand);
}

SGBD::~SGBD()
{
	DBFree();
}

//Retire les espaces avant apres (Commandes)
static void prepareCommand(std::string &cmd)
{
	// std::transform(cmd.begin(), cmd.end(), cmd.begin(), ::toupper);

	auto not_space = [](unsigned char ch) {
		return !std::isspace(ch);
	};
    
	cmd.erase(cmd.begin(), std::ranges::find_if(cmd, not_space));
	cmd.erase(std::ranges::find_if(cmd | std::views::reverse, not_space).base(), cmd.end());
}

//Lire requetes
void SGBD::Run()
{
	std::string command;
	std::unordered_map<std::string, std::regex> regexps;

    //Eviter de re build regex
	for (const auto &key : command_handlers | std::views::keys) // C++ construction, should be read as pipe in bash, we have a map, and we want its keys
		regexps.emplace(key, std::regex{key});

	while (true)
	{
        std::cout << "$> ";

		if (!std::getline(std::cin, command))
			break;

		prepareCommand(command);

		bool found = false;
		for (const auto& [exp_str, handler] : command_handlers)
		{
			if (std::regex_search(command, regexps[exp_str]))
			{
				found = true;
                try
                {
				    handler(command);
                }
                catch (const DBCommandBadSyntax& err)
                {
                    std::cerr << err.what();
                }
                catch (const std::invalid_argument &ex)
                {
                    std::cerr << "error: unexpected argument: " << ex.what() << std::endl;
                }
                catch (const std::out_of_range &ex)
                {
                    std::cerr << "error: " << ex.what() << std::endl;
                }
				break;
			}
		}

		if (!found)
			std::cout << "error: couldn't parse input: " << command << std::endl;
	}
}

void SGBD::ProcessCreateDatabaseCommand(const std::string &command)
{
	static std::regex regex("^CREATE DATABASE ([[:alnum:]]+)$");
	std::smatch match;

	if (!std::regex_match(command, match, regex))
		throw DBCommandBadSyntax("CREATE DATABASE", "couldn't parse input");
	dbManager.CreateDatabase(match[1].str());
}

void SGBD::ProcessSetDatabaseCommand(const std::string &command)
{
	static std::regex regex("^SET DATABASE ([[:alnum:]]+)$");
	std::smatch match;

	if (!std::regex_match(command, match, regex))
		throw DBCommandBadSyntax("SET DATABASE", "couldn't parse input");
	dbManager.SetCurrentDatabase(match[1].str());
}

void SGBD::ProcessCreateTableCommand(const std::string &command) const
{
	static std::regex regex(R"(^CREATE TABLE ([[:alnum:]]+) \((.+)\)$)");
	std::smatch match;

	if (!std::regex_match(command, match, regex))
		throw DBCommandBadSyntax("CREATE TABLE", "couldn't parse input");

	const std::string table_name = match[1].str();
	if (dbManager.GetTableFromCurrentDatabase(table_name) != nullptr)
		throw DBCommandBadSyntax("CREATE TABLE", "duplicated table: " + table_name);

	std::set<std::string> names;

    std::string fields_str = match[2].str();
    auto fields_view = fields_str
            | std::views::split(',')
            | std::views::transform([&names](auto field_rng) {
                std::string str;

                for (char c : field_rng)
                    str.push_back(c);
                prepareCommand(str);

                std::string name = str.substr(0, str.find(':'));
                if (names.contains(name))
                    throw DBCommandBadSyntax("CREATE TABLE", "duplicated field name: " + name);
                names.emplace(name);

                std::string type_str = str.substr(str.find(':') + 1);
                FieldType type;
                size_t len = 1;
                size_t size = 4;
                if (type_str == "INT")
                    type = INT;
                else if (type_str == "REAL")
                    type = REAL;
                else
                {
	                size = 1;
					std::string type_len = type_str.substr(type_str.find('('));
					type_str = type_str.substr(0, type_str.find('('));

					if (type_str == "CHAR")
						type = FIXED_LENGTH_STRING;
					else if (type_str == "VARCHAR")
					{
						type = VARCHAR;
						len = 0;
					}
                    else
                        throw DBCommandBadSyntax("CREATE TABLE", "couldn't parse field type " + type_str);

                    len = std::stoul(type_len.substr(1, type_len.length() - 1));
                }

                FieldMetadata meta;
                meta.type = type;
                meta.len = len;
                meta.name = strdup(name.c_str());
                meta.size = size;
                return meta;
            });

    std::vector fields(fields_view.begin(), fields_view.end());

	Relation *relation = new_relation(table_name.c_str(), static_cast<int>(fields.size()), fields.data());
	dbManager.AddTableToCurrentDatabase(relation);

	for (const auto &field : fields)
		free(const_cast<char *>(field.name));
}

void SGBD::ProcessDropTableCommand(const std::string &command) const
{
	static std::regex regex("^DROP TABLE ([[:alnum:]]+)$");
	std::smatch match;

	if (!std::regex_match(command, match, regex))
		throw DBCommandBadSyntax("DROP TABLE", "couldn't parse input");
	dbManager.RemoveTableFromCurrentDatabase(match[1].str());
}

void SGBD::ProcessListTablesCommand(const std::string & /*not needed, but still mandatory since it's in an array*/) const
{
	dbManager.ListTablesInCurrentDatabase();
}

void SGBD::ProcessDropTablesCommand(const std::string &/*not needed, but still mandatory since it's in an array*/) const
{
	dbManager.RemoveTablesFromCurrentDatabase();
}

void SGBD::ProcessDropDatabasesCommand(const std::string &/*not needed, but still mandatory since it's in an array*/)
{
	dbManager.RemoveDatabases();
}

void SGBD::ProcessListDatabasesCommand(const std::string &/*not needed, but still mandatory since it's in an array*/) const
{
	dbManager.ListDatabases();
}

void SGBD::ProcessDropDatabaseCommand(const std::string &command)
{
	static std::regex regex("^DROP DATABASE ([[:alnum:]]+)$");
	std::smatch match;

	if (!std::regex_match(command, match, regex))
		throw DBCommandBadSyntax("DROP DATABASE", "couldn't parse input");
	dbManager.RemoveDatabase(match[1].str());
}

void SGBD::recordInserter(const std::string& command, const std::string& fields_str, const DBManager::RelationPtr &rel)
{
	std::vector<std::string> fields;
	std::string current;
	bool is_quoted = false;
	for (const char c : fields_str)
	{
		if (c == '"')
			is_quoted = !is_quoted;
		else if (c == ',' && !is_quoted)
		{
			fields.push_back(current);
			current.clear();
		}
		else
			current.push_back(c);
	}

	if (is_quoted)
		throw DBCommandBadSyntax(command, "bad input (unexpected end of input): " +fields_str);

	fields.push_back(current);

	if (static_cast<int>(fields.size()) != rel->nb_fields)
	{
		std::stringstream ss;
		ss << "bad input: unexpected number of fields, expected(" << rel->nb_fields << ") != actual(" << fields.size() << ")";
		ss << ": " << fields_str;
		throw DBCommandBadSyntax(command, ss.str());
	}

	Record *record = newRecord(rel.get());
	for (int i = 0; i < rel->nb_fields; i++)
	{
		try
		{
			switch (rel->fieldsMetadata[i].type)
			{
			case FIXED_LENGTH_STRING:
				if (fields[i].length() > rel->fieldsMetadata[i].len)
				{
					std::stringstream ss;

					ss << "bad input: field " << i << ": string too long for fixed length field (" << rel->fieldsMetadata[i].len << ")";
					throw DBCommandBadSyntax(command, ss.str());
				}

				write_field_string(record, i, fields[i].c_str(), fields[i].length());
				break;

			case VARCHAR:
				write_field_string(record, i, fields[i].c_str(), fields[i].length());
				break;

			case INT:
				write_field_i32(record, i, std::stoi(fields[i]));
				break;

			case REAL:
				write_field_f32(record, i, std::stof(fields[i]));
				break;

			default: // shouldn't happen...
				assert(0);
			}
		}
		catch (const std::out_of_range &oor)
		{
			std::stringstream ss;
			ss << "bad input: field " << i << ": out of range number: " << oor.what();
			throw DBCommandBadSyntax(command, ss.str());
		}
		catch (const std::invalid_argument &ia)
		{
			std::stringstream ss;
			ss << "bad input: field " << i << ": bad syntax: " << ia.what();
			throw DBCommandBadSyntax(command, ss.str());
		}
	}

	InsertRecord(record);
	freeRecord(record);
}

void SGBD::ProcessInsertIntoCommand(const std::string& command) const
{
	static std::regex regex(R"(^INSERT INTO ([[:alnum:]]+) VALUES \((.+)\)$)");\
	std::smatch match;

	if (!std::regex_match(command, match, regex))
		throw DBCommandBadSyntax("INSERT INTO", "couldn't parse input");

	const std::string table_name = match[1].str();
	const DBManager::RelationPtr &rel = dbManager.GetTableFromCurrentDatabase(table_name);
	if (rel == nullptr)
		throw DBCommandBadSyntax("INSERT INTO", "table not found: " + table_name);

	recordInserter("INSERT INTO", match[2].str(), rel);
}

void SGBD::ProcessBulkInsertIntoCommand(const std::string& command) const
// csv case to manage
{
	std::regex regex("^BULKINSERT INTO ([[:alnum:]]+) (.+)$");
	std::smatch match;

	if (!std::regex_match(command, match, regex))
		throw DBCommandBadSyntax("BULKINSERT INTO", "couldn't parse input");

	const std::string table_name = match[1].str();
	const DBManager::RelationPtr &rel = dbManager.GetTableFromCurrentDatabase(table_name);
	if (rel == nullptr)
		throw DBCommandBadSyntax("BULKINSERT INTO", "table not found: " + table_name);

	fs::path csv(match[2].str());
	if (csv.is_relative())
		csv = init_wd / csv;
	std::ifstream ifs(csv);
	if (!ifs.is_open())
		throw DBCommandBadSyntax("BULKINSERT INTO", "couldn't open file: " + match[2].str());

	for (std::string line; std::getline(ifs, line);)
		recordInserter("BULKINSERT INTO", line, rel);
}

void SGBD::ProcessSelectCommand(const std::string& command) const
{
	SelectCommand cmd(command);

	// Replaces c style list by C++ vector (and releases directly both the c style list and the relation pointer)
	std::vector<Record *> records;
	{
		const DBManager::RelationPtr rel = dbManager.GetTableFromCurrentDatabase(cmd.relation());
		cmd.expandProjections(rel);

		RecordList *rec_list = GetAllRecords(rel.get());
		records.assign(rec_list->records, rec_list->records + rec_list->length);
		freeRecordList(rec_list);
	}

	for (Record *rec : records)
	{
		cmd(std::cout, rec);
		freeRecord(rec);
	}
	std::cout << "Total records=" << cmd.nb_printed() << "." << std::endl;
}

void SGBD::ProcessQuitCommand(const std::string &/*not needed, but still mandatory since it's in an array*/) const
{
	handleSignal();
}
