#pragma once

#include "DBManager.h"

#include <regex>
#include <filesystem>

namespace fs = std::filesystem;

struct DBCommandBadSyntax : std::runtime_error
{
public:
	DBCommandBadSyntax(const std::string &cmd, const std::string &msg);
};

class SGBD
{
public:
	SGBD(int argc, char **argv);
	~SGBD();

	void Run();

    static void saveDBManagerState()
    {
        if (instance)
            instance->dbManager.SaveState();
    }

private:
	static void recordInserter(const std::string& command, const std::string& fields_str, const DBManager::RelationPtr &rel);
	static fs::path init_wd;

	void ProcessCreateDatabaseCommand(const std::string &command);
	void ProcessSetDatabaseCommand(const std::string &command);
	void ProcessCreateTableCommand(const std::string &command) const;
	void ProcessDropTableCommand(const std::string &command) const;
	void ProcessListTablesCommand(const std::string &command) const;
	void ProcessDropTablesCommand(const std::string &command) const;
	void ProcessDropDatabasesCommand(const std::string &command);
	void ProcessListDatabasesCommand(const std::string &command) const;
	void ProcessDropDatabaseCommand(const std::string &command);
	void ProcessQuitCommand(const std::string &command) const;

	void ProcessInsertIntoCommand(const std::string &command) const;
	void ProcessBulkInsertIntoCommand(const std::string &command) const;
	void ProcessSelectCommand(const std::string &command) const;

	DBManager dbManager;
	std::unordered_map<std::string, std::function<void(const std::string &)>> command_handlers;

    static SGBD *instance;
};