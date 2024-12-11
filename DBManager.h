#pragma once

#include "Relation.h"

#include <functional>
#include <memory>
#include <unordered_set>
#include <unordered_map>
#include <string>



// We need to "specialize" the std::hash struct to make it work with our custom Relation struct. As it's a template
// full specialization, the template instruction MUST be empty but is still needed. The name of the struct is followed by the
// template arguments that specializes this instance, in our case, Relation is the only one needed.
template <>
struct std::hash<Relation>
{
	// In C++ we can overload any operators, even the parenthesis after a function name. So this function is named operator()
	// it takes a const pointer to Relation and return a size_t. The two modifiers after the function prototype are mandatory,
	// but not important to understand how it works.
	size_t operator()(const Relation &r) const noexcept
	{
		// This is actually 2 instructions, we call the constructor of hash<std::string> then call on it the operator()
		// with the relation's name. Yes it takes a std::string in parameter, and it works because the char * is used to
		// automatically construct a std::string.
		return hash<std::string>()(r.name);
	}
};

template <>
struct std::hash<Relation *>
{
	size_t operator()(const Relation* edge) const noexcept
	{
		return hash<Relation>()(*edge);
	}
};

struct RelationDestructor
{
	bool destroy{false};

	void operator()(Relation *r) const noexcept;
};

// No need for constructor/destructor since DBConfig is global, and all memory is automatically managed
class DBManager
{
public:
	// using is like a typedef, but only in the compilation stage, it doesn't affect the final binary
	using RelationPtr = std::shared_ptr<Relation>;

	void CreateDatabase(const std::string &name); // constant reference (const pointer equivalent)
	void RemoveDatabase(const std::string& name);
	void RemoveDatabases();
	void ListDatabases() const; // Declares a constant method, i.e. a method that doesn't change a thing in the class properties
	void SetCurrentDatabase(const std::string &name);
	void ListTablesInCurrentDatabase() const;

	void AddTableToCurrentDatabase(Relation *relation) const;
	RelationPtr GetTableFromCurrentDatabase (const std::string &name) const;
	void RemoveTableFromCurrentDatabase(const std::string &name) const;
	void RemoveTablesFromCurrentDatabase() const;

	void LoadState();
	void SaveState() const;

private:
	// Same as a hash set in java
	using Database = std::unordered_set<RelationPtr>;

	// Self-managed pointer, same behavior as a garbage collector but much more efficient
	using DatabasePtr = std::shared_ptr<Database>;
	using Databases = std::unordered_map<std::string, DatabasePtr>;

	// Equivalent to a hash map (same underlying data structure)
	Databases dbs;
	Databases::iterator selected_db;

	Database::iterator find_relation(const std::string &name);
	Database::const_iterator find_relation(const std::string &name) const;
};