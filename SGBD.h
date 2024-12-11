#pragma once

#include "DBManager.h"

#include <regex>

class SGBD
{
public:
	SGBD(int argc, char **argv);
	~SGBD();

	void Run();

private:
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

	void ProcessInsertIntoCommand(const std::string &command);
	void ProcessBulkInsertIntoCommand(const std::string &command);
	void ProcessSelectCommand(const std::string &command);

	DBManager dbManager;
	std::unordered_map<std::string, std::function<void(const std::string &)>> command_handlers;
};