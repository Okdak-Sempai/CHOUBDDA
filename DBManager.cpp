#include "DBManager.h"

#include "BufferManager.h"


#include <cassert> // assert.h but in C++ style
#include <cstring> // string.h but in C++ style
#include <filesystem>
#include <fstream>
#include <iostream>
#include <memory>
#include <ranges>
#include <string>

// Code in this file is pure C++, and can't be exposed to C code. There is however a way to do so, check the end of the file

namespace fs = std::filesystem;

// Overload of operator==
bool operator==(const Relation &r1, const Relation &r2)
{
	return strcmp(r1.name, r2.name) == 0;
}

void RelationDestructor::operator()(Relation* r) const noexcept
{
	if (destroy)
	{
		auto* lst = getDataPages(r);

		for (size_t i = 0; i < lst->length; i++)
			DeallocPage(lst->page_ids[i]);

		auto* cur_page = r->headHdrPageId;
		do
		{
			auto* page = reinterpret_cast<HeapFileHdr*>(GetPage(cur_page));

			PageId* next = nullptr;
			if (page->has_next)
				next = FindPageId(page->next);

			FreePage(cur_page, 0);
			DeallocPage(cur_page);
			cur_page = next;
		}
		while (cur_page);

		freePageIdList(lst);
	}

	free_relation(r);
}


void DBManager::CreateDatabase(const std::string& name)
{
	dbs[name] = std::make_shared<Database>();
}

void DBManager::RemoveDatabase(const std::string& name)
{
	const auto it = dbs.find(name);
	if (it == dbs.end())
		return;

	if (it == selected_db)
		selected_db = dbs.end();

	for (const RelationPtr& relation : *it->second)
		std::get_deleter<RelationDestructor>(relation)->destroy = true;
	dbs.erase(it);
}

void DBManager::RemoveDatabases()
{
	for (auto& db : dbs | std::views::values)
		for (const auto &relation : *db)
			std::get_deleter<RelationDestructor>(relation)->destroy = true;

	dbs.clear();
	selected_db = dbs.end();
}

void DBManager::SetCurrentDatabase(const std::string& name)
{
	selected_db = dbs.find(name);

	if (selected_db == dbs.end())
		throw std::out_of_range("Database not found");
}

void DBManager::ListDatabases() const
{
	// Foreach loop in C++, there we have two variables as in python and tuples
	for (const auto& name : dbs | std::views::keys)
		std::cout << "\t- " << name << std::endl;

	std::cout << "Returned " << dbs.size() << " databases" << std::endl;
}

void DBManager::ListTablesInCurrentDatabase() const
{
	auto format_field = [](const FieldMetadata& field)
	{
		if (field.type == FIXED_LENGTH_STRING)
			return std::string(field.name) + ":CHAR(" + std::to_string(field.len) + ")";

		std::string type_str;
		switch (field.type)
		{
		case INT:
			type_str = "INT";
			break;
		case REAL:
			type_str = "REAL";
			break;
		case VARCHAR:
			type_str = "VARCHAR(" + std::to_string(field.len) + ")";
			break;
		default:
			type_str = "UNKNOWN";
		}

		return std::string(field.name) + ":" + type_str;
    };

	// Foreach loop in C++, there we have two variables as in python and tuples
	for (const auto &rel : *selected_db->second)
	{
		std::string formatted = format_field(rel->fieldsMetadata[0]);
		for (size_t i = 1; i < rel->nb_fields; i++)
			formatted += "," + format_field(rel->fieldsMetadata[i]);
		std::cout << "\t- " << rel->name << " (" << formatted << ")" << std::endl;
	}

	std::cout << "Returned " << dbs.size() << " tables from db " << selected_db->first << std::endl;
}

void DBManager::AddTableToCurrentDatabase(Relation* relation) const
{
	if (selected_db == nullptr)
	{
		DeallocPage(relation->headHdrPageId);
		free_relation(relation);
		throw std::out_of_range("No database is currently selected");
	}
	selected_db->second->emplace(relation, RelationDestructor{});
}

DBManager::RelationPtr DBManager::GetTableFromCurrentDatabase(const std::string& name) const
{
    if (selected_db == dbs.end())
        return nullptr;

	const auto it = find_relation(name);

	if (it == selected_db->second->end())
		return nullptr;

	return *it;
}

void DBManager::RemoveTableFromCurrentDatabase(const std::string& name) const
{
	const auto it = find_relation(name);
	std::get_deleter<RelationDestructor>(*it)->destroy = true; // Completely erase table from disk
	selected_db->second->erase(it);
}

void DBManager::RemoveTablesFromCurrentDatabase() const
{
	// Mark all tables for complete deletion, these tables shouldn't be available even after stop/start the program
	for (const auto &rel : *selected_db->second)
		std::get_deleter<RelationDestructor>(rel)->destroy = true;

	// Clearing the hash set which resolves to a complete wipe of the database... However, the database will still exist
	selected_db->second->clear();
}


DBManager::Database::iterator DBManager::find_relation(const std::string &name)
{
	auto pred = [name](const RelationPtr &rel)
	{
		return rel->name == name;
	};

	return std::ranges::find_if(*selected_db->second, pred);
}

DBManager::Database::const_iterator DBManager::find_relation(const std::string &name) const
{
	auto pred = [name](const RelationPtr &rel)
	{
		return rel->name == name;
	};

	return std::ranges::find_if(selected_db->second->begin(), selected_db->second->end(), pred);
}

void DBManager::LoadState()
{
	fs::path base_folder = config->dbpath;
	if (!is_directory(base_folder))
		throw std::runtime_error("cannot save database: " + base_folder.string() + " does not exist");

	fs::path save_path = base_folder / "databases.save";

	std::ifstream ifs(save_path, std::ios::in | std::ios::binary);
	if (!ifs)
		return;

	size_t nb_db;
	ifs.read(reinterpret_cast<char*>(&nb_db), sizeof(nb_db));

	dbs.reserve(nb_db);

	for (size_t i = 0; i < nb_db; i++)
	{
		ssize_t len;
		std::string name;
		ifs.read(reinterpret_cast<char *>(&len), sizeof(len));
		name.resize(len);
		ifs.read(name.data(), len);

		auto dbs_emplace_res = dbs.emplace(name, std::make_shared<Database>());
		assert(dbs_emplace_res.second); // means that the record has actually been inserted
		Database &cur_db = *dbs_emplace_res.first->second;

		size_t nb_relations;
		ifs.read(reinterpret_cast<char*>(&nb_relations), sizeof(nb_relations));

		cur_db.reserve(nb_relations);
		for (size_t j = 0; j < nb_relations; j++)
		{
			// The following line is needed to avoid the need to migrate the allocation mechanisms to C++ for relations
			std::shared_ptr<Relation> rel(static_cast<Relation *>(malloc(sizeof(Relation))), RelationDestructor{});

			ifs.read(reinterpret_cast<char *>(&rel->oid), sizeof(rel->oid));
			PageId page_id;
			ifs.read(reinterpret_cast<char *>(&page_id), sizeof(page_id));
			rel->headHdrPageId = FindPageId(page_id);
			ifs.read(reinterpret_cast<char *>(&page_id), sizeof(page_id));
			rel->tailHdrPageId = FindPageId(page_id);

			ifs.read(reinterpret_cast<char *>(&len), sizeof(len));
			name.assign(len, '\0');
			ifs.read(name.data(), len);
			rel->name = strdup(name.c_str());

			ifs.read(reinterpret_cast<char *>(&rel->is_dynamic), sizeof(rel->is_dynamic));
			ifs.read(reinterpret_cast<char *>(&rel->nb_fields), sizeof(rel->nb_fields));

			rel->fieldsMetadata = static_cast<FieldMetadata*>(calloc(rel->nb_fields, sizeof(FieldMetadata)));

			for (size_t k = 0; k < rel->nb_fields; k++)
			{
				FieldMetadata *meta = rel->fieldsMetadata + k;
				ifs.read(reinterpret_cast<char *>(&len), sizeof(len));
				name.assign(len, '\0');
				ifs.read(name.data(), len);
				meta->name = strdup(name.c_str());

				ifs.read(reinterpret_cast<char *>(&meta->type), sizeof(meta->type));
				ifs.read(reinterpret_cast<char *>(&meta->size), sizeof(meta->size));
				ifs.read(reinterpret_cast<char *>(&meta->len), sizeof(meta->len));
			}

			assert(cur_db.emplace(rel).second);
		}
	}
}

void DBManager::SaveState() const
{
	fs::path base_folder = config->dbpath;
	if (!exists(base_folder) || !is_directory(base_folder))
		throw std::runtime_error("cannot save database: " +  base_folder.string() + " does not exist");

	fs::path save_path = base_folder / "databases.save";

	std::ofstream ofs(save_path, std::ios::out | std::ios::binary | std::ios::trunc);

	size_t len = dbs.size();
	ofs.write(reinterpret_cast<const char *>(&len), sizeof(len));

	for (const auto &[name, db] : dbs)
	{
		len = name.length();
		ofs.write(reinterpret_cast<const char *>(&len), sizeof(len));
		ofs.write(name.c_str(), static_cast<ssize_t>(len));

		len = db->size();
		ofs.write(reinterpret_cast<const char *>(&len), sizeof(len));

		for (const auto &relPtr : *db)
		{
			const auto &relation = *relPtr;
			ofs.write(reinterpret_cast<const char *>(&relation.oid), sizeof(relation.oid));
			ofs.write(reinterpret_cast<const char *>(relation.headHdrPageId), sizeof(*relation.headHdrPageId));
			ofs.write(reinterpret_cast<const char *>(relation.tailHdrPageId), sizeof(*relation.tailHdrPageId));

			len = strlen(relation.name);
			ofs.write(reinterpret_cast<const char *>(&len), sizeof(len));
			ofs.write(relation.name, static_cast<ssize_t>(len));

			ofs.write(reinterpret_cast<const char *>(&relation.is_dynamic), sizeof(relation.is_dynamic));
			ofs.write(reinterpret_cast<const char *>(&relation.nb_fields), sizeof(relation.nb_fields));

			for (size_t i = 0; i < relation.nb_fields; i++)
			{
				const FieldMetadata *meta = relation.fieldsMetadata + i;
				len = strlen(meta->name);
				ofs.write(reinterpret_cast<const char *>(&len), sizeof(len));
				ofs.write(meta->name, static_cast<ssize_t>(len));

				ofs.write(reinterpret_cast<const char *>(&meta->type), sizeof(meta->type));
				ofs.write(reinterpret_cast<const char *>(&meta->size), sizeof(meta->size));
				ofs.write(reinterpret_cast<const char *>(&meta->len), sizeof(meta->len));
			}
		}
	}

	ofs.flush();
	ofs.close();
}
