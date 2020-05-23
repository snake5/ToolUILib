
#pragma once
#include "pch.h"



/* SELECTOR SYNTAX

basic field offset of same struct:              field
same struct field value:                        field
field of another struct (single instance):      #another-struct.field
field offset of another struct (nth instance):  @#another-struct[#=n+4].field
field of another struct/nth/field:              #another-struct[#=n+4].field.subfield
field of another struct with type=3:            #another-struct[type=3].field
combined:                                       @#another-struct[type=3, #=n+4].field.subfield

RULES:

name = [a-zA-Z_] [a-zA-Z_0-9]+
name = "\"" [^"] "\""
field = name
struct = "#" name
locator = field | struct | locator "." field
filter = field ":" expr
filter = field "=" name
filter = filter "," filter
filtered_locator = locator "[" filter "]"
any_locator = locator | filtered_locator
selector = any_locator | "@" any_locator
expr = selector | constant | ...

*/


struct DataDesc;
struct DDStructInst;
struct DDCondition;


struct ICompileErrorOutput
{
	virtual void WriteError(const char* str, ...) = 0;
};

struct StructQueryFilter
{
	std::vector<DDCondition>& conditions;
	bool returnNth = false;
	int64_t nth = 0;
};

struct StructQueryResults : std::vector<const DDStructInst*>
{
	using vector::vector;
};

struct IVariableSource
{
	virtual bool GetVariable(const DDStructInst* inst, const std::string& field, bool offset, int64_t& outVal) = 0;
	virtual StructQueryResults GetInitialSet() = 0;
	virtual StructQueryResults Subquery(const StructQueryResults& src, const std::string& field, const StructQueryFilter& filter) = 0;
	virtual StructQueryResults RootQuery(const std::string& typeName, const StructQueryFilter& filter) = 0;
};

struct VariableSource : IVariableSource
{
	bool GetVariable(const DDStructInst* inst, const std::string& field, bool offset, int64_t& outVal) override;
	StructQueryResults GetInitialSet() override;
	StructQueryResults Subquery(const StructQueryResults& src, const std::string& field, const StructQueryFilter& filter) override;
	StructQueryResults RootQuery(const std::string& typeName, const StructQueryFilter& filter) override;

	DataDesc* desc = nullptr;
	const DDStructInst* root = nullptr;
};

struct MathExpr
{
	MathExpr() {}
	~MathExpr();
	MathExpr(const MathExpr&) = delete;

	void Compile(const char* expr);
	int64_t Evaluate(IVariableSource* vsrc);

	struct CompiledMathExpr* _impl = nullptr;
};
