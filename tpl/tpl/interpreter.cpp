module;

#include <map>
#include <memory>
#include <stack>
#include <string>
#include <variant>

export module tpl.interpreter;

import tpl.parser;

export namespace tpl::runtime {

using Value = std::variant<std::int64_t, std::string>;

class Interpreter : public ast::ExprVisitor {
  public:
	Value interpret(ast::Expr &ast)
	{
		ast.accept(*this);
		return values.top();
	}

	void visit(ast::Number &number) override { values.emplace(number.value); }
	void visit(ast::Variable &var) override { values.push(scope[var.name]); }

  private:
	std::map<std::string, Value> scope;
	std::stack<Value> values;
};

} // namespace tpl::runtime