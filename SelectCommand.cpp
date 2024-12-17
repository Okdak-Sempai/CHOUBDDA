#include "SelectCommand.h"
#include "SGBD.h"

#include <ranges>

std::regex SelectCommand::global_exp(R"(^SELECT ([\w*.,]*) FROM (\w+) (\w+)(?: WHERE (.+))?$)");
std::regex SelectCommand::proj_exp(R"!((\w+)\.(\w+))!");
std::regex SelectCommand::where_exp(R"!((\w+\.\w+|\d+(?:\.\d+)?|"[^"]*")(<|>|<>|<=|>=|=)(\w+\.\w+|\d+(?:\.\d+)?|"[^"]*"))!");

static std::unordered_map<std::string, SelectCommand::Condition::Operator> operators{
	{"=", SelectCommand::Condition::OP_EQ},
	{"<>", SelectCommand::Condition::OP_NE},
	{"<", SelectCommand::Condition::OP_LT},
	{"<=", SelectCommand::Condition::OP_LE},
	{">", SelectCommand::Condition::OP_GT},
	{">=", SelectCommand::Condition::OP_GE},
};

SelectCommand::Condition::Condition(const std::string& el1, const std::string& op, const std::string& el2)
	: e1_(std::monostate{}), e2_(std::monostate{}), op_{}
{
	// checks that char is one of allowed chars in rel.col construction, in this case we look for points, alphanumerics and _.
	static std::regex_traits<char> REG_TRAITS;
	static std::string word_class_name = "w";
	static auto word_class = REG_TRAITS.lookup_classname(word_class_name.begin(), word_class_name.end());


	auto parse_operand = [=, this](const std::string &operand, bool is_first)
	{
		Operand &out = is_first ? e1_ : e2_;

		if (!is_first && std::holds_alternative<ProjElement>(e1_))
		if (operand[0] == '"')
		{
			if (*operand.rbegin() != '"') // reverse begin, takes last character from string
				throw DBCommandBadSyntax("SELECT", "couldn't parse condition: " + el1 + op + el2);

			out = operand.substr(1, operand.size() - 1);
		}
		else
		{
			try
			{
				// least restrictive parsing first, if it fails, there is no chance that the second one can pass
				out = std::stof(operand);
				out = std::stoi(operand);
			}
			catch (const std::exception &)
			{
			}
		}

		if (!std::holds_alternative<std::monostate>(out))
			return;

		ProjElement p;
		bool pt_seen = false; // ensures that there is exactly one point in the expression
		for (const char c : operand)
		{
			if ((!REG_TRAITS.isctype(c, word_class) && c != '.') || (c == '.' && pt_seen))
				throw DBCommandBadSyntax("SELECT", "couldn't parse select condition: " + el1 + op + el2);
			if (c == '.')
				pt_seen = true;
			else if (pt_seen)
				p.col += c;
			else
				p.rel += c;
		}
		out = p;
	};

	parse_operand(el1, true);
	parse_operand(el2, false);

	try
	{
		op_ = operators[op];
	}
	catch (const std::out_of_range &)
	{
		throw DBCommandBadSyntax("SELECT", "couldn't parse select condition: " + el1 + op + el2);
	}
}

SelectCommand::Condition::Operator SelectCommand::Condition::op() const
{
	return op_;
}

SelectCommand::SelectCommand(const std::string& command)
{
	std::smatch full_match;

	if (!std::regex_match(command, full_match, global_exp))
		throw DBCommandBadSyntax("SELECT", "couldn't parse select command: " + command);

	const std::sregex_iterator end_it;
	std::string proj_str = full_match[1].str();
	std::string where_str = full_match[4].str();

	if (proj_str != "*")
	{
		for (std::sregex_iterator proj_it(proj_str.begin(), proj_str.end(), proj_exp);
			proj_it != end_it;
			++proj_it)
			projections_.emplace_back(proj_it->str(1), proj_it->str(2));
	}

	relation_ = full_match[2].str();
	alias_ = full_match[3].str();

	if (!where_str.empty())
	{
		for (std::sregex_iterator where_it(where_str.begin(), where_str.end(), where_exp);
			where_it != end_it;
			++where_it)
		{
			Condition cond(where_it->str(1), where_it->str(2), where_it->str(3));
			conditions_.push_back(cond);
		}
	}

	validate_aliases();
}

void SelectCommand::validate_aliases()
{
	for (auto &cond : conditions_)
	{
		try
		{
			auto& proj = cond.first<ProjElement>();
			if (proj.rel != alias_)
				throw DBCommandBadSyntax("SELECT", "unknown alias: " + proj.rel);
			proj.rel = relation_;
		}
		catch (const std::bad_variant_access &)
		{}

		try
		{
			auto& proj = cond.second<ProjElement>();
			if (proj.rel != alias_)
				throw DBCommandBadSyntax("SELECT", "unknown alias: " + proj.rel);
			proj.rel = relation_;
		}
		catch (const std::bad_variant_access &)
		{}
	}

	for (auto &proj : projections_)
	{
		if (proj.rel != alias_)
			throw DBCommandBadSyntax("SELECT", "unknown alias: " + proj.rel);
		proj.rel = relation_;
	}
}

void SelectCommand::expandProjections(const DBManager::RelationPtr& relation)
{
	if (!projections_.empty())
		return;

	for (size_t i = 0; i < relation->nb_fields; i++)
		projections_.push_back(ProjElement{
			.rel = relation->name,
			.col = relation->fieldsMetadata[i].name
		});
	projections_.shrink_to_fit();
}

const std::vector<SelectCommand::Condition> &SelectCommand::conditions() const
{
	return conditions_;
}

const std::vector<SelectCommand::ProjElement> &SelectCommand::projections() const
{
	return projections_;
}

const std::string & SelectCommand::relation() const
{
	return relation_;
}

const std::string & SelectCommand::alias() const
{
	return alias_;
}

bool SelectCommand::check_conditions(const Fields& fields) const
{
	static std::unordered_map<Condition::Operator, std::function<bool(const FieldData&, const FieldData&)>> operators{
		{Condition::OP_EQ, [](const FieldData &a, const FieldData &b) -> bool { return a == b; } },
		{Condition::OP_NE, [](const FieldData &a, const FieldData &b) -> bool { return a != b; } },
		{Condition::OP_LT, [](const FieldData &a, const FieldData &b) -> bool { return a < b; } },
		{Condition::OP_LE, [](const FieldData &a, const FieldData &b) -> bool { return a <= b; } },
		{Condition::OP_GT, [](const FieldData &a, const FieldData &b) -> bool { return a > b; } },
		{Condition::OP_GE, [](const FieldData &a, const FieldData &b) -> bool { return a >= b; } },
	};

	for (auto &cond : conditions_)
	{
		FieldData op1;

		if (std::holds_alternative<int>(cond.e1_))
			op1 = std::get<int>(cond.e1_);
		else if (std::holds_alternative<float>(cond.e1_))
			op1 = std::get<float>(cond.e1_);
		else if (std::holds_alternative<std::string>(cond.e1_))
			op1 = std::get<std::string>(cond.e1_);
		else if (std::holds_alternative<ProjElement>(cond.e1_))
			op1 = fields.at(std::get<ProjElement>(cond.e1_).col);
		else
			throw std::logic_error("unknown operand type");

		FieldData op2;
		if (std::holds_alternative<int>(cond.e2_))
			op2 = std::get<int>(cond.e2_);
		else if (std::holds_alternative<float>(cond.e2_))
			op2 = std::get<float>(cond.e2_);
		else if (std::holds_alternative<std::string>(cond.e2_))
			op2 = std::get<std::string>(cond.e2_);
		else if (std::holds_alternative<ProjElement>(cond.e2_))
			op2 = fields.at(std::get<ProjElement>(cond.e2_).col);
		else
			throw std::logic_error("unknown operand type");

		if (!operators[cond.op_](op1, op2))
			return false;
	}
	return true;
}

SelectCommand::Fields SelectCommand::read_record(const Record* record)
{
	const Relation *rel = record->rel;
	std::vector metaDatas(rel->fieldsMetadata, rel->fieldsMetadata + rel->nb_fields);

	Fields fields;
	fields.reserve(record->rel->nb_fields);


	for (size_t i = 0; i < metaDatas.size(); ++i)
	{
		FieldData data;

		switch (metaDatas[i].type)
		{
		case FieldType::INT:
			data = read_field_i32(record, i);
			break;
		case FieldType::REAL:
			data = read_field_f32(record, i);
			break;
		case FieldType::FIXED_LENGTH_STRING:
		case FieldType::VARCHAR:
			{
				char *str;
				const size_t len = read_field_string(record, i, &str, nullptr);
				data = std::string{str, str + len};
				free(str);
				break;
			}
		}

		fields[metaDatas[i].name] = data;
	}

	return fields;
}


void SelectCommand::operator()(std::ostream& os, const Record* record)
{
	Fields fields = read_record(record);
	if (!check_conditions(fields))
		return;
	nb_printed_++;

	bool first = true;
	for (const std::string &col : projections_ | std::views::transform([](const ProjElement &proj) {return proj.col;}))
	{
		if (!first)
			os << " ; ";

		if (std::holds_alternative<int>(fields[col]))
			os << std::get<int>(fields[col]);
		else if (std::holds_alternative<float>(fields[col]))
			os << std::get<float>(fields[col]);
		else if (std::holds_alternative<std::string>(fields[col]))
			os << std::get<std::string>(fields[col]);

		first = false;
	}
	os << std::endl;
}

size_t SelectCommand::nb_printed() const
{
	return nb_printed_;
}
