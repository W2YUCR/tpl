module;

#include <format>
#include <functional>
#include <map>
#include <memory>
#include <stack>
#include <string>
#include <variant>

export module tpl.runtime;

import tpl.parser;
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
		expr.func->accept(*this);
		auto center = values.top();
		values.pop();

		if (auto *ptr = std::get_if<std::function<Value(Value, Value)>>(&center)) {
			auto func = std::move(*ptr);

			expr.lhs->accept(*this);
			auto lhs = std::move(values.top());
			values.pop();

			expr.rhs->accept(*this);
			auto rhs = std::move(values.top());
			values.pop();

			values.push(func(std::move(lhs), std::move(rhs)));
		}
		else {
			throw std::runtime_error{std::format("Attempted to call a non-function: {}", center)};
		}
	}

  private:
	std::map<std::string, Value> scope{
		{"+", [](Value lhs, Value rhs) -> Value {
			 return std::visit(
				 overloaded{
					 [](std::int64_t lhs, std::int64_t rhs) -> Value { return lhs + rhs; },
					 [](Value lhs, Value rhs) -> Value {
						 throw std::runtime_error{std::format("Cannot add {} and {}", lhs, rhs)};
					 },
				 },
				 lhs, rhs);
		 }}};
	std::stack<Value> values;
};

} // namespace tpl::runtime
