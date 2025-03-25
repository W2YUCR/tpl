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

class Scope {
  public:
	Scope(std::unique_ptr<Scope> parent, std::map<std::string, Value, std::less<>> values = {}) :
		_parent{std::move(parent)},
		_values{std::move(values)}
	{
	}

	Value &lookup(std::string_view name)
	{
		auto iter = _values.find(name);
		if (iter != _values.end()) {
			return iter->second;
		}
		if (_parent != nullptr) {
			return _parent->lookup(name);
		}
		return _values[std::string{name}];
	}

	void insert(std::string name, Value value) { _values.emplace(name, value); }

	std::unique_ptr<Scope> &parent() { return _parent; }

  private:
	std::unique_ptr<Scope> _parent;
	std::map<std::string, Value, std::less<>> _values;
};

class Interpreter : public ast::ExprVisitor {
  public:
	Value interpret(ast::Expr &ast)
	{
		ast.accept(*this);
		return values.top();
	}

	void visit(ast::Number &number) override { values.emplace(number.value); }
	void visit(ast::Variable &var) override { values.push(scope->lookup(var.name)); }
	void visit(ast::BinaryOp &expr) override
	{
		Value center = eval(*expr.func);

		if (FunctionValue *ptr = std::get_if<FunctionValue>(&center)) {
			FunctionValue func = std::move(*ptr);
			values.push(func(expr.lhs, expr.rhs));
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
		auto operator()(std::unique_ptr<ast::Expr> &lhs, std::unique_ptr<ast::Expr> &rhs) -> Value
		{
			return std::visit(overloaded{
				[](auto lhs, auto rhs) -> Value
					requires requires { Op{}(lhs, rhs); }
				{ return Op{}(lhs, rhs); },
				[this](Value lhs, Value rhs) -> Value {
					throw std::runtime_error{std::format("Cannot call {} with {} and {}", _name, lhs, rhs)};
				},
				},
				_interpreter->eval(*lhs),
				_interpreter->eval(*rhs));
		};

	  private:
		Interpreter *_interpreter;
		char const *_name;
	};

	std::unique_ptr<Scope> scope = std::make_unique<Scope>(
		nullptr,
		std::map<std::string, Value, std::less<>>{
			{"+", AutoOp<std::plus<>>(this, "plus")},
			{"-", AutoOp<std::minus<>>{this, "minus"}},
			{"/", AutoOp<std::divides<>>{this, "divide"}},
			{"*", AutoOp<std::multiplies<>>{this, "multiply"}},
			{"=",
			 [&](std::unique_ptr<ast::Expr> &lhs, std::unique_ptr<ast::Expr> &rhs) -> Value {
				 if (auto *var = dynamic_cast<ast::Variable *>(lhs.get())) {
					 return scope->lookup(var->name) = eval(*rhs);
				 }
				 else {
					 throw std::runtime_error{std::format("The LHS is not a variable")};
				 }
			 }},
			{"func",
			 [&](std::unique_ptr<ast::Expr> &lhs, std::unique_ptr<ast::Expr> &rhs) -> Value {
				 std::shared_ptr<ast::Expr> body = std::move(rhs);
				 return std::function(
					 [this, body](std::unique_ptr<ast::Expr> &lhs, std::unique_ptr<ast::Expr> &rhs) -> Value {
						 scope = std::make_unique<Scope>(std::move(scope));
						 scope->insert("LHS", eval(*lhs));
						 scope->insert("RHS", eval(*rhs));
						 auto value = eval(*body);
						 scope = std::move(scope->parent());
						 return value;
					 });
			 }},
		});

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
