module;

#include <format>
#include <functional>
#include <map>
#include <memory>
#include <stack>
#include <string>
#include <variant>

export module tpl.runtime;

import tpl.ast;
import tpl.util;

export import :value;

export namespace tpl::runtime {

class Interpreter : public ast::ExprVisitor {
  public:
	Value interpret(ast::Expr &ast)
	{
		ast.accept(*this);
		return values.top();
	}

	void visit(ast::Number &number) override { values.emplace(number.value); }
	void visit(ast::Variable &var) override { values.push(scope[var.name]); }
	void visit(ast::BinaryOp &expr) override
	{
		Value center = eval(*expr.func);

		if (FunctionValue *ptr = std::get_if<FunctionValue>(&center)) {
			FunctionValue func = std::move(*ptr);
			values.push(func(*expr.lhs, *expr.rhs));
		}
		else {
			throw std::runtime_error{std::format("Attempted to call a non-function: {}", center)};
		}
	}

  private:
	std::map<std::string, Value> scope{
		{"+",
		 [&](ast::Expr &lhs, ast::Expr &rhs) -> Value {
			 return std::visit(
				 overloaded{
					 [](std::int64_t lhs, std::int64_t rhs) -> Value { return lhs + rhs; },
					 [](Value lhs, Value rhs) -> Value {
						 throw std::runtime_error{std::format("Cannot add {} and {}", lhs, rhs)};
					 },
				 },
				 eval(lhs), eval(rhs));
		 }},
		{
			"=",
			[&](ast::Expr &lhs, ast::Expr &rhs) -> Value {
				if (auto *var = dynamic_cast<ast::Variable *>(&lhs)) {
					return scope[var->name] = eval(rhs);
				}
				else {
					throw std::runtime_error{std::format("The LHS is not a variable")};
				}
			},
		}};

	Value eval(ast::Expr &expr)
	{
		expr.accept(*this);
		Value value = values.top();
		values.pop();
		return value;
	}

	std::stack<Value> values;
};

} // namespace tpl::runtime
