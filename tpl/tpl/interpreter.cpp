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
	template <class Op>
	class AutoOp {
	  public:
		AutoOp(Interpreter *interpreter, char const *name) : _interpreter{interpreter}, _name{name} {}
		auto operator()(ast::Expr &lhs, ast::Expr &rhs) -> Value
		{
			return std::visit(overloaded{
				[](auto lhs, auto rhs) -> Value
					requires requires { Op{}(lhs, rhs); }
				{ return Op{}(lhs, rhs); },
				[this](Value lhs, Value rhs) -> Value {
					throw std::runtime_error{std::format("Cannot call {} with {} and {}", _name, lhs, rhs)};
				},
				},
				_interpreter->eval(lhs),
				_interpreter->eval(rhs));
		};

	  private:
		Interpreter *_interpreter;
		char const *_name;
	};

	template <class Op>
	auto auto_op(char const *name)
	{
		return AutoOp<Op>{this, name};
	}

	std::map<std::string, Value> scope{
		{"+", AutoOp<std::plus<>>(this, "plus")},
		{"-", AutoOp<std::minus<>>{this, "minus"}},
		{"/", AutoOp<std::divides<>>{this, "divide"}},
		{"*", AutoOp<std::multiplies<>>{this, "multiply"}},
		{"=",
		 [&](ast::Expr &lhs, ast::Expr &rhs) -> Value {
			 if (auto *var = dynamic_cast<ast::Variable *>(&lhs)) {
				 return scope[var->name] = eval(rhs);
			 }
			 else {
				 throw std::runtime_error{std::format("The LHS is not a variable")};
			 }
		 }},
	};

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
