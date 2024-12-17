#pragma once

#include <string>
#include <regex>
#include <variant>
#include <vector>

#include "DBManager.h"
#include "Record.h"

class SelectCommand
{
public:
	struct ProjElement
	{
		std::string rel;
		std::string col;
	};

	struct Condition
	{
		// Means that only SelectCommand will be able to access private elements
		friend SelectCommand;

		enum Operator
		{
			OP_EQ,
			OP_NE,
			OP_LT,
			OP_LE,
			OP_GT,
			OP_GE,
		};

		[[nodiscard]] Operator op() const;

	private:
		Condition(const std::string &el1, const std::string &op, const std::string &el2);

		// union like type, but its type safe
		using Operand = std::variant<std::monostate, ProjElement, std::string, int, float>;
		Operand e1_;
		Operand e2_;
		Operator op_;

	public:
		// Needs to be after Element Declaration
		// template functions MUST be available from header file, it is a requirement from C++
		// The 'requires' thing comes from concepts in C++20, it blocks compilation if the constraint isn't fulfilled
		// In this case, we block any access to monostate, since it's a fail-safe index in the variant.
		template <typename T>
			requires (!std::is_same_v<T, std::monostate>)
		const T& first() const
		{
			return std::get<T>(e1_);
		}

		template <typename T>
			requires (!std::is_same_v<T, std::monostate>)
		T& first()
		{
			return std::get<T>(e1_);
		}

		template <typename T>
			requires (!std::is_same_v<T, std::monostate>)
		const T& second() const
		{
			return std::get<T>(e2_);
		}

		template <typename T>
			requires (!std::is_same_v<T, std::monostate>)
		T& second()
		{
			return std::get<T>(e2_);
		}
	};

	explicit SelectCommand(const std::string &command);

	[[nodiscard]] const std::vector<Condition> &conditions() const;
	[[nodiscard]] const std::vector<ProjElement> &projections() const;

	const std::string &relation() const;
	const std::string &alias() const;

	void operator()(std::ostream &os, const Record *record);
	size_t nb_printed() const;

	void expandProjections(const DBManager::RelationPtr &relation);

private:
	using FieldData = std::variant<int, float, std::string>;
	using Fields = std::unordered_map<std::string, FieldData>;

	static std::regex global_exp;
	static std::regex proj_exp;
	static std::regex where_exp;

	void validate_aliases();

	static Fields read_record(const Record *record);
	bool check_conditions(const Fields &fields) const;

	std::vector<ProjElement> projections_;
	std::vector<Condition> conditions_;
	std::string relation_;
	std::string alias_;
	size_t nb_printed_{0};
};
